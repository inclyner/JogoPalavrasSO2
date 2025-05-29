// Arbitro.c : This file contains the 'main' function. Program execution begins and ends there.
//
#include "header.h"
#include "Arbitro.h"


const TCHAR* DICIONARIO[] = {
	_T("GATO"), _T("CÃO"), _T("RATO"), _T("SAPO"), _T("LOBO"),
	_T("LIVRO"), _T("BOLA"), _T("MESA"), _T("PORTA"), _T("CASA"),
	_T("CARRO"), _T("FACA"), _T("PATO"), _T("PEIXE"), _T("CHAVE"),
	_T("BANCO"), _T("CARTA"), _T("FAROL"), _T("LATA"), _T("FITA"),
	_T("VIDRO"), _T("TECLA"), _T("JANELA"), _T("CADEIRA"), _T("CAMA"),
	_T("TINHA"), _T("BICHO"), _T("MANTO"), _T("FUMO"), _T("TROCO"),_T("A"),_T("B"),_T("C"),_T("D"),_T("E"),_T("F"),_T("G"),_T("H"),_T("I"),_T("J"),_T("K"),_T("L"),_T("M"),_T("N"),_T("O"),_T("P"),_T("Q"),_T("R"),_T("S"),_T("T"),_T("U"),_T("V"),_T("W"),_T("X"),_T("Y"),_T("Z")
};
const int NUM_PALAVRAS = sizeof(DICIONARIO) / sizeof(DICIONARIO[0]);

TCHAR letraRandom() {
	return _T('A') + (rand() % 26);
}

void acelerarRitmo(TDATA* ptd) {
	WaitForSingleObject(ptd->hMutex, INFINITE);
	ptd->ritmo--;
	ReleaseMutex(ptd->hMutex);
	_tprintf_s(_T("[ARBITRO] Novo ritmo: %d.\n"), ptd->ritmo);

}

void travarRitmo(TDATA* ptd) {
	WaitForSingleObject(ptd->hMutex, INFINITE);
	ptd->ritmo++;
	ReleaseMutex(ptd->hMutex);
	_tprintf_s(_T("[ARBITRO] Novo ritmo: %d.\n"), ptd->ritmo);
}

DWORD getPlayerByName(TDATA td, TCHAR token[]) {
	DWORD i;

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (_tcsicmp(td.players[i].name, token) == 0) {
			return i;
		}
	}

	return -1;
}

void EliminarPlayer(TDATA* ptd, DWORD id)
{
	MENSAGEM sair = { .tipo = COMANDO };
	_tcscpy_s(sair.comando, TAM, _T(":sair"));
	DWORD n;

	WriteFile(ptd->players[id].hPipe, &sair, sizeof(MENSAGEM), &n, NULL);

	DisconnectNamedPipe(ptd->players[id].hPipe);
	CloseHandle(ptd->players[id].hPipe);
	WaitForSingleObject(ptd->hMutex, INFINITE);
	_tprintf_s(_T("O jogador %s vai sair.\n"), ptd->players[id].name);
	ptd->players[id].hPipe = NULL;
	_tcscpy_s(ptd->players[id].name, TAM_USERNAME, _T(""));
	ptd->n_users--;
	ptd->next_id = id;

	for (int i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (_tcscmp(ptd->memoria_partilhada->players[i].name, ptd->players[id].name) == 0) {
			_tcscpy_s(ptd->memoria_partilhada->players[i].name, TAM_USERNAME, _T(""));
			ptd->memoria_partilhada->players[i].points = 0.0f;
			break;
		}
	}

	//pausar jogo se restar apenas um jogador
	if (ptd->n_users < 2 && ptd->isGameOn) {
		_tprintf_s(_T("[ARBITRO] Só resta 1 jogador. A pausar jogo.\n"));
		ptd->isGameOn = FALSE;
		SuspendThread(ptd->hThreadLetras);
	}


	ReleaseMutex(ptd->hMutex);


}


void enviar_todos(TDATA* ptd, MENSAGEM msg, DWORD id) {
	BOOL ret;
	DWORD i, n;
	TDATA td;
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);


	WaitForSingleObject(ptd->hMutex, INFINITE);
	td = *ptd;
	ReleaseMutex(ptd->hMutex);

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (id == i) {
			break;
		}
		if (ptd->players[i].hPipe != NULL) {
			//ANTES Da operacao 
			ZeroMemory(&ov, sizeof(OVERLAPPED));
			ov.hEvent = hEv;
			ret = WriteFile(ptd->players[i].hPipe, &msg, sizeof(MENSAGEM), &n, &ov);
			if (!ret && GetLastError() != ERROR_IO_PENDING) {
				_tprintf_s(_T("[ERRO] Escrever no pipe! (WriteFile)\n"));
				break;
			}
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
				GetOverlappedResult(ptd->players[i].hPipe, &ov, &n, FALSE); // obter resultado da operacao
			}
			_tprintf_s(_T("[ENVIAR TODOS] Enviei %s bytes ao jogoui...  (i = %d) (WriteFile)\n"), msg.comando, i);
		}
	}
	CloseHandle(hEv);
}

MENSAGEM consola_arbitro(MENSAGEM msg, TDATA* ptd) {
	TDATA td;
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
	MENSAGEM resposta;
	FLOAT maior,maior2;
	DWORD id_maior;
	TCHAR aux[TAM];
	TCHAR nome[TAM_USERNAME];
	resposta.tipo = ERRO;
	_stprintf_s(resposta.comando, TAM, _T(""));
	DWORD i, id;

	WaitForSingleObject(ptd->hMutex, INFINITE);
	td = *ptd;
	ReleaseMutex(ptd->hMutex);


	if (token == NULL) {
		_tprintf(_T("Comando inválido.\n"));
		// envia isto 
		return resposta;
	}
	resposta.tipo = COMANDO;

	if (_tcscmp(token, _T("encerrar")) == 0) {
		WaitForSingleObject(ptd->hMutex, INFINITE);
		if (!ptd->isGameOn) {
			ResumeThread(ptd->hThreadLetras);
		}
		else {
			ptd->isGameOn = FALSE;
		}
		ptd->continua = FALSE;
		ReleaseMutex(ptd->hMutex);
		// termina a theard 
		resposta.tipo = COMANDO;
		_tcscpy_s(resposta.comando, TAM, _T("\nJogo a encerrar ...\n"));
		_stprintf_s(nome, TAM_USERNAME, _T("%s"), _T(""));
		maior = -1.0;
		maior2 = -1.0;
		id_maior = -1;
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.players[i].hPipe != NULL) {
				if (td.players[i].points >= maior) {
					 maior = td.players[i].points;
					_stprintf_s(nome, TAM_USERNAME, _T("%s"), td.players[i].name);
					id_maior = i;
				}
				
			}
		}
		_stprintf_s(aux, TAM, _T("O vencedor foi %s que ganhou por "), nome);
		_tcscat_s(resposta.comando, TAM, aux);
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.players[i].hPipe != NULL && i != id_maior) {
				if (td.players[i].points > maior2) {
					maior2 = td.players[i].points;
				}
			}
		}

		_stprintf_s(aux, TAM, _T("%.1f\n"), maior - maior2);
		_tcscat_s(resposta.comando, TAM, aux);

		_tprintf_s(_T("%s"), resposta.comando);
	}
	else if (_tcscmp(token, _T("listar")) == 0) {
		_tprintf(_T("Lista de jogadores \n"));
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.players[i].hPipe != NULL) {
				_tprintf_s(_T("Nome: %s \t | Pontos: %.1f\n"), td.players[i].name, td.players[i].points);
			}
		}
	}
	else if (_tcscmp(token, _T("excluir")) == 0) {
		token = _tcstok_s(comando_token, _T(" ,\n"), &comando_token);
		if (token == NULL) {
			_tprintf(_T("Comando inválido.\n"));
		}
		else {
			id = getPlayerByName(td, token);
			if (id != -1) {
				WaitForSingleObject(ptd->hMutex, INFINITE);
				EliminarPlayer(ptd, id);
				ReleaseMutex(ptd->hMutex);
				//_tcscpy_s(resposta.comando, TAM, _T(":sair"));
			}
			else {
				_tprintf(_T("Username não existe.\n"));
			}
		}
	}
	else if (_tcscmp(token, _T("iniciarbot")) == 0) {
		token = _tcstok_s(comando_token, _T(" ,\n"), &comando_token);
		if (token == NULL) {
			_tprintf(_T("Comando inválido.\n"));
		}
		else {
			_tprintf(_T("Um jogador %s juntou-se ao jogo através do bot. \n"), token);

			TCHAR cmdLine[100];
			_stprintf_s(cmdLine, 100, _T("Bot.exe %s"), token);

			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi;

			if (CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
				_tprintf(_T("[ARBITRO] Bot '%s' iniciado.\n"), token);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
			}
			else {
				_tprintf(_T("[ERRO] Falha ao iniciar Bot: %d\n"), GetLastError());
			}

		}
	}
	else if (_tcscmp(token, _T("acelerar")) == 0) {
		if (td.ritmo > 1) {
			acelerarRitmo(ptd);
			_stprintf_s(resposta.comando, TAM, _T("[ARBITRO] Ritmo foi aumentado para %d segundos."), ptd->ritmo);
		}
		else {
			_tprintf(_T("O Ritmo já se encontra no minimo (1).\n"));
		}
	}
	else if (_tcscmp(token, _T("travar")) == 0) {
		travarRitmo(ptd);
		_stprintf_s(resposta.comando, TAM, _T("[ARBITRO] Ritmo foi reduzido para %d segundos."), ptd->ritmo);
	}
	else {
		_tprintf(_T("Comando desconhecido: %s\n"), token);
		resposta.tipo = ERRO;
	}

	return resposta;
}


MENSAGEM consola_jogoui(MENSAGEM msg, TDATA* ptd, DWORD myPos, DWORD* ativo) {
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
	MENSAGEM resposta = { .tipo = ERRO };
	MENSAGEM broadcast;
	DWORD i;

	WaitForSingleObject(ptd->hMutex, INFINITE);
	TDATA td = *ptd;
	ReleaseMutex(ptd->hMutex);

	_tcscpy_s(resposta.comando, TAM, msg.comando);

	switch (msg.tipo) {
	case USERNAME:
		_tprintf_s(_T("O jogador %s está a tentar entrar no jogo.\n"), msg.comando);

		if (isUserValid(msg, td)) {
			WaitForSingleObject(ptd->hMutex, INFINITE);

			resposta.tipo = USERNAME;
			_stprintf_s(resposta.comando, TAM, _T("%d"), ptd->next_id);
			_tcscpy_s(ptd->players[ptd->next_id].name, TAM_USERNAME, msg.comando);
			ptd->n_users++;

			_tprintf_s(_T("n_users %d, name = %s\n"), ptd->n_users, ptd->players[ptd->next_id].name);

			if (ptd->next_id + 1 == ptd->n_users)
				ptd->next_id++;
			else
				ptd->next_id = ptd->n_users;

			if (!ptd->isGameOn && ptd->n_users == 2) {
				ptd->isGameOn = TRUE;
				ResumeThread(ptd->hThreadLetras);
			}

			ReleaseMutex(ptd->hMutex);

			broadcast.tipo = COMANDO;
			_stprintf_s(broadcast.comando, TAM, _T("Um jogador %s juntou-se ao jogo através do jogoui."),
				msg.comando);
			enviar_todos(ptd, broadcast, myPos);
		}
		else {
			_stprintf_s(resposta.comando, TAM, _T("%d"), -1);
			_tprintf_s(_T("O jogador %s já existe.\n"), msg.comando);
			*ativo = FALSE;
		}
		break;

	case PALAVRA: {
		_tprintf_s(_T("O jogador %s indicou a palavra: %s\n"), td.players[myPos].name, msg.comando);
		resposta.tipo = PALAVRA;

		int letras_usadas[MAX_LETRAS];
		BOOL palavra_valida = validarPalavra(msg.comando, ptd->memoria_partilhada, letras_usadas);

		if (palavra_valida) {
			float pontos_adicionados = (float)_tcslen(msg.comando);

			WaitForSingleObject(ptd->hMutex, INFINITE);

			// Atualizar pontos do jogador
			ptd->players[myPos].points += pontos_adicionados;

			// Atualizar memória partilhada
			_tcscpy_s(ptd->memoria_partilhada->players[myPos].name, TAM_USERNAME, ptd->players[myPos].name);
			ptd->memoria_partilhada->players[myPos].points = ptd->players[myPos].points;

			// Teste de debug:
			float debug_val = ptd->memoria_partilhada->players[myPos].points;
			_tprintf(_T("[DEBUG] Escrita points = %.1f\n"), debug_val);

			// Atualizar última palavra
			_tcscpy_s(ptd->memoria_partilhada->ultima_palavra, MAX_LETRAS, msg.comando);

			// Remover letras usadas
			for (int i = 0; i < _tcslen(msg.comando); i++) {
				ptd->memoria_partilhada->letras[letras_usadas[i]] = _T('_');
			}

			ReleaseMutex(ptd->hMutex);

			_stprintf_s(resposta.comando, TAM, _T("Palavra válida! +%.1f pontos."), (float)_tcslen(msg.comando));

			broadcast.tipo = COMANDO;
			_stprintf_s(broadcast.comando, TAM, _T("O jogador %s adivinhou a palavra %s."),
				td.players[myPos].name, msg.comando);
			enviar_todos(ptd, broadcast, myPos);

			DWORD novo_lider = getIdLider(ptd);
			if (novo_lider != ptd->id_lider_atual) {
				MENSAGEM m_lider = { .tipo = COMANDO };
				_stprintf_s(m_lider.comando, TAM, _T("O jogador %s passou para a frente com %.1f pontos!"),
					ptd->players[novo_lider].name, ptd->players[novo_lider].points);
				enviar_todos(ptd, m_lider, -1);
				ptd->id_lider_atual = novo_lider;
			}
		}
		else {

			float penalizacao = _tcslen(msg.comando) / 2.0f;

			WaitForSingleObject(ptd->hMutex, INFINITE);
			ptd->players[myPos].points -= penalizacao;
			ptd->memoria_partilhada->players[myPos].points = ptd->players[myPos].points;
			ReleaseMutex(ptd->hMutex);
			_stprintf_s(resposta.comando, TAM, _T("Palavra inválida! -%.1f pontos."), (_tcslen(msg.comando) / 2.0f));
		}
		break;
	}

	case COMANDO:
		resposta.tipo = COMANDO;

		if (_tcsicmp(msg.comando, _T(":pont")) == 0) {
			_stprintf_s(resposta.comando, TAM, _T("Pontos: %.1f\n"), td.players[myPos].points);
		}
		else if (_tcsicmp(msg.comando, _T(":jogs")) == 0) {
			_tcscpy_s(resposta.comando, TAM, _T("\nLista de Jogadores: \n"));
			for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
				if (td.players[i].hPipe != NULL) {
					TCHAR aux[TAM_USERNAME + 5];
					_stprintf_s(aux, TAM_USERNAME + 5, _T("%s\n"), td.players[i].name);
					_tcscat_s(resposta.comando, TAM, aux);
				}
			}
		}
		else if (_tcsicmp(msg.comando, _T(":sair")) == 0) {
			*ativo = FALSE;

		}
		break;

	default:
		_tprintf_s(_T("DEFAULT\n"));
		break;
	}

	return resposta;
}


BOOL isUserValid(MENSAGEM msg, TDATA td) {
	DWORD i;
	if (td.n_users == 0) return TRUE;

	for (i = 0; i < td.n_users; i++) {
		if (_tcsicmp(td.players[i].name, msg.comando) == 0) {
			return FALSE;
		}
	}
	return TRUE;
}


DWORD WINAPI letras(LPVOID data) {
	_tprintf_s(_T("[LETRAS] Thread iniciada.\n"));

	TDATA* ptd = (TDATA*)data;
	MEMORIA_PARTILHADA* memoria;
	DWORD i, pos;
	TCHAR letra;
	MENSAGEM msg;

	HANDLE hMemoPart = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MEMORY_NAME);
	if (!hMemoPart) {
		_tprintf(TEXT("Erro ao abrir memória partilhada (%d).\n"), GetLastError());
		return 1;
	}

	memoria = (MEMORIA_PARTILHADA*)MapViewOfFile(hMemoPart, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MEMORIA_PARTILHADA));
	if (!memoria) {
		_tprintf(TEXT("Erro ao mapear memória (%d).\n"), GetLastError());
		CloseHandle(hMemoPart);
		return 1;
	}

	while (ptd->isGameOn) {
		WaitForSingleObject(ptd->hMutex, INFINITE);
		DWORD ritmo = ptd->ritmo;
		DWORD id_letra = ptd->id_letra;
		ReleaseMutex(ptd->hMutex);

		pos = -1;
		for (i = 0; i < memoria->num_letras; i++) {
			if (memoria->letras[i] == _T('_')) {
				pos = i;
				break;
			}
		}

		letra = letraRandom();

		WaitForSingleObject(ptd->hMutex, INFINITE);
		if (pos != -1) {
			memoria->letras[pos] = letra;
			ptd->id_letra = pos;
		}
		else {
			memoria->letras[ptd->id_letra] = letra;
			ptd->id_letra = (ptd->id_letra + 1) % memoria->num_letras;
		}
		ReleaseMutex(ptd->hMutex);

		msg.tipo = LETRAS;
		_stprintf_s(msg.comando, TAM, _T("%s"), memoria->letras);
		enviar_todos(ptd, msg, -1);

		Sleep(ritmo * 1000);
	}

	UnmapViewOfFile(memoria);
	CloseHandle(hMemoPart);

	ExitThread(0);
}


// nesta thread o arbitro vai receber os pedido neste caso as palavras ou os comandos 
// e vai fazer fazer o que tem a fazer sobre os pontos e tudo mais 
// e depois envia por exemplo os pontos ao user ou que ele pediu nos comandos

DWORD WINAPI atende_cliente(LPVOID data) {
	TDATA* ptd = (TDATA*)data;
	MENSAGEM msg, resposta;
	DWORD n, i, myPos;
	BOOL ret, ativo = TRUE;
	TDATA td;
	MENSAGEM broadcast;
	TCHAR nome_removido[TAM_USERNAME];

	// Determinar a posição do jogador
	WaitForSingleObject(ptd->hMutex, INFINITE);
	myPos = ptd->next_id;
	ReleaseMutex(ptd->hMutex);

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL); // Evento para I/O assíncrono

	do {
		//==============================//
		// Ler mensagem do jogador      //
		//==============================//
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;

		ret = ReadFile(ptd->players[myPos].hPipe, &msg, sizeof(MENSAGEM), &n, &ov);
		if (!ret && GetLastError() != ERROR_IO_PENDING) {
			_tprintf_s(_T("[ERROR READ] %d (%d bytes)... (ReadFile)\n"), ret, n);
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(ptd->players[myPos].hPipe, &ov, &n, FALSE);
		}

		_tprintf_s(_T("[ESCRITOR] Recebi '%s' (%d bytes)... (ReadFile)\n"), msg.comando, n);

		//==============================//
		// Processar e responder        //
		//==============================//
		resposta = consola_jogoui(msg, ptd, myPos, &ativo);

		ret = WriteFile(ptd->players[myPos].hPipe, &resposta, sizeof(MENSAGEM), &n, &ov);
		if (!ret && GetLastError() != ERROR_IO_PENDING) {
			_tprintf_s(_T("[ERROR] Write failed code(%d)\n"), GetLastError());
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE);
			GetOverlappedResult(ptd->players[myPos].hPipe, &ov, &n, FALSE);
		}

		_tprintf_s(_T("[ARBITRO] Resposta '%s' (%d bytes)... (WriteFile)\n"), resposta.comando, n);


		nome_removido[TAM_USERNAME];
		_tcscpy_s(nome_removido, TAM_USERNAME, ptd->players[myPos].name);  // <- garantir acesso direto à estrutura viva

	} while (ativo);

	EliminarPlayer(ptd, myPos);
	broadcast.tipo = COMANDO;
	_stprintf_s(broadcast.comando, TAM, _T("Um jogador %s saiu."), nome_removido);
	enviar_todos(ptd, broadcast, myPos);
	CloseHandle(hEv);
	_tprintf_s(_T("Thread atende a sair\n"));
	ExitThread(0);
}


DWORD WINAPI distribui(LPVOID data) {
	TDATA* ptd = (TDATA*)data;
	TCHAR buf[256];
	DWORD n, i;
	BOOL ret;
	MENSAGEM msg, resposta;
	TDATA td;

	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL); // Evento para I/O assíncrono

	do {
		//==============================//
		// Ler comando do árbitro       //
		//==============================//
		_tprintf_s(_T("[ESCRITOR] Frase: "));
		_fgetts(msg.comando, 256, stdin);
		msg.comando[_tcslen(msg.comando) - 1] = '\0';

		WaitForSingleObject(ptd->hMutex, INFINITE);
		td = *ptd;
		ReleaseMutex(ptd->hMutex);

		// Processar comando
		resposta = consola_arbitro(msg, ptd);

		//==============================//
		// Enviar a todos os jogadores  //
		//==============================//
		if (resposta.tipo > ERRO) {
			if (_tcscmp(msg.comando, _T("listar")) != 0) {
				for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
					if (td.players[i].hPipe != NULL) {
						ZeroMemory(&ov, sizeof(OVERLAPPED));
						ov.hEvent = hEv;

						ret = WriteFile(td.players[i].hPipe, &resposta, sizeof(MENSAGEM), &n, &ov);
						if (!ret && GetLastError() != ERROR_IO_PENDING) {
							_tprintf_s(_T("[ERRO] Escrever no pipe! (WriteFile)\n"));
							break;
						}
						if (GetLastError() == ERROR_IO_PENDING) {
							WaitForSingleObject(hEv, INFINITE);
							GetOverlappedResult(td.players[i].hPipe, &ov, &n, FALSE);
						}

						_tprintf_s(_T("[ESCRITOR] Enviei '%s' ao jogoui (i = %d)\n"), resposta.comando, i);
					}
				}
			}
		}
	} while (_tcsicmp(msg.comando, _T("encerrar")));

	//==============================//
	// Desligar e fechar pipes     //
	//==============================//
	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (ptd->players[i].hPipe != NULL) {
			FlushFileBuffers(ptd->players[i].hPipe);
			_tprintf_s(_T("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
			if (!DisconnectNamedPipe(ptd->players[i].hPipe)) {
				_tprintf_s(_T("[ERRO] Desligar o pipe! (DisconnectNamedPipe)\n"));
				exit(-1);
			}
			CloseHandle(ptd->players[i].hPipe);
		}
	}

	_tprintf_s(_T("Thread distribui a sair\n"));

	CloseHandle(hEv);
	ExitThread(0);
}


void getRegistryValues(int* maxletras, int* ritmo) {
	HKEY hKey;
	DWORD tipo = REG_DWORD;
	DWORD tam = sizeof(DWORD);
	DWORD valMaxLetras = 6;
	DWORD valRitmo = 3;

	TCHAR chave[] = REGISTRY_NAME;


	LSTATUS res = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		chave,
		0,
		KEY_ALL_ACCESS,
		&hKey
	);

	if (res != ERROR_SUCCESS) {
		_tprintf(_T("Chave não encontrada. A criar...\n"));

		res = RegCreateKeyEx(
			HKEY_CURRENT_USER,
			_T("Software\\TrabSO2"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey,
			NULL
		);

		if (res != ERROR_SUCCESS) {
			_tprintf(_T("Erro ao criar a chave!\n"));
			return 1;
		}


	}

	tam = sizeof(DWORD);
	res = RegQueryValueEx(hKey, _T("MAXLETRAS"), NULL, &tipo, (LPBYTE)&valMaxLetras, &tam);
	if (res != ERROR_SUCCESS) {
		_tprintf(_T("MAXLETRAS não existe, a criar com valor %d.\n"), DEFAULT_LETRAS);
		valMaxLetras = DEFAULT_LETRAS;
		RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
	}
	else
	{
		_tprintf(_T("MAXLETRAS lido! => %d\n"), valMaxLetras);
		if (valMaxLetras > MAX_LETRAS) {
			_tprintf(_T("MAXLETRAS superior a %d. A corrigir para %d.\n"), MAX_LETRAS, MAX_LETRAS);
			valMaxLetras = MAX_LETRAS;
			RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
		}
	}
	res = RegQueryValueEx(hKey, _T("RITMO"), NULL, &tipo, (LPBYTE)&valRitmo, &tam);
	if (res != ERROR_SUCCESS) {
		_tprintf(_T("RITMO não existe, a criar com valor %d.\n"), DEFAULT_RITMO);
		valRitmo = DEFAULT_RITMO;
		RegSetValueEx(hKey, _T("RITMO"), 0, REG_DWORD, (const BYTE*)&valRitmo, sizeof(DWORD));
	}
	else
	{
		_tprintf(_T("RITMO lido! => %d\n"), valRitmo);
	}


	RegCloseKey(hKey);
	*maxletras = valMaxLetras;
	*ritmo = valRitmo;

	return 0;
}


BOOL validarPalavra(const TCHAR* palavra, MEMORIA_PARTILHADA* memoria_partilhada, int letras_usadas[MAX_LETRAS]) {
	TCHAR letras_visiveis[MAX_LETRAS];
	BOOL usada[MAX_LETRAS] = { FALSE };
	int i, j, len_palavra = _tcslen(palavra);
	TCHAR palavra_upper[MAX_LETRAS];

	// Copiar letras visíveis (para manipular localmente)
	for (i = 0; i < memoria_partilhada->num_letras; i++) {
		letras_visiveis[i] = memoria_partilhada->letras[i];
	}

	// Verificar se todas as letras da palavra estão nas letras visíveis (e marcar posições)
	for (i = 0; i < len_palavra; i++) {
		TCHAR c = _totupper(palavra[i]);
		palavra_upper[i] = c;
		BOOL encontrada = FALSE;

		for (j = 0; j < memoria_partilhada->num_letras; j++) {
			if (!usada[j] && letras_visiveis[j] == c) {
				usada[j] = TRUE;
				letras_usadas[i] = j; // regista a posição usada
				encontrada = TRUE;
				break;
			}
		}

		if (!encontrada) {
			return FALSE; // letra não encontrada suficientes vezes
		}

	}
	palavra_upper[len_palavra] = _T('\0');

	// Verificar se palavra está no dicionário
	BOOL in_dicionario = FALSE;
	for (i = 0; i < NUM_PALAVRAS; i++) {
		if (_tcscmp(DICIONARIO[i], palavra_upper) == 0) {
			in_dicionario = TRUE;
			break;
		}
	}

	return in_dicionario;
}


DWORD getIdLider(TDATA* ptd) {
	DWORD i, id_lider = -1;
	int max_pontos = -9999;

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (ptd->players[i].hPipe != NULL && _tcscmp(ptd->players[i].name, _T("")) != 0) {
			if ((float)ptd->players[i].points > max_pontos) {
				max_pontos = (float)ptd->players[i].points;
				id_lider = i;
			}
		}
	}
	return id_lider;
}


int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	system("cls");//limpa o ecra
	_tprintf(_T("===========================================\n"));
	_tprintf(_T("        JOGO DE PALAVRAS - SO2 (ISEC)      \n"));
	_tprintf(_T("===========================================\n\n"));
	//============================//
	// Declaração de variáveis    //
	//============================//
	DWORD i;
	HANDLE hPipe = NULL;
	HANDLE hMemoPart = NULL;
	HANDLE hMutexGlobal = NULL;
	HANDLE hThreadLetras = NULL, hThreadDistribui = NULL;
	TCHAR buf[256];
	TDATA td;
	BOOL isGameOn;
	DWORD erro;
	BOOL ligado;

	//=======================================//
	// Mutex para evitar múltiplos árbitros  //
	//=======================================//
	hMutexGlobal = CreateMutex(NULL, TRUE, _T("Global\\MutexUnico_ArbitroSO2"));
	if (hMutexGlobal == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		_tprintf(_T("[ERRO] Já existe uma instância do árbitro em execução.\n"));
		return 1;
	}

	srand((unsigned int)time(NULL));

	//===============================//
	// Ler valores do registo        //
	//===============================//
	DWORD max_letras, ritmo;
	getRegistryValues(&max_letras, &ritmo);

	TCHAR* letras_jogo = (TCHAR*)malloc(max_letras * sizeof(TCHAR));
	if (letras_jogo == NULL) {
		_tprintf(_T("Erro a alocar memória!\n"));
		return 1;
	}

	for (int i = 0; i < max_letras; i++) {
		letras_jogo[i] = _T('_');
	}

	//===============================//
	// Criar memória partilhada      //
	//===============================//
	hMemoPart = CreateFileMapping(
		INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
		0, sizeof(MEMORIA_PARTILHADA), MEMORY_NAME
	);

	if (hMemoPart == NULL) {
		_tprintf(_T("Erro ao criar a memória partilhada (%d).\n"), GetLastError());
		free(letras_jogo);
		CloseHandle(hMutexGlobal);
		return 1;
	}

	MEMORIA_PARTILHADA* memoria = MapViewOfFile(
		hMemoPart,
		FILE_MAP_ALL_ACCESS, 0, 0,
		sizeof(MEMORIA_PARTILHADA)
	);

	if (memoria == NULL) {
		_tprintf(_T("Erro ao mapear (%d).\n"), GetLastError());
		CloseHandle(hMemoPart);
		CloseHandle(hMutexGlobal);
		free(letras_jogo);
		return 1;
	}

	// Inicializar memória partilhada
	for (int i = 0; i < max_letras; ++i) {
		memoria->letras[i] = _T('_');
	}
	memoria->num_letras = max_letras;
	_stprintf_s(memoria->ultima_palavra, MAX_LETRAS, _T("------"));

	//===============================//
	// Inicializar estrutura TDATA   //
	//===============================//
	td.ritmo = ritmo;
	td.next_id = 0;
	td.n_users = 0;
	td.isGameOn = FALSE;
	td.continua = TRUE;
	td.hMutex = CreateMutex(NULL, FALSE, NULL);
	td.max_letras = max_letras;
	td.id_letra = 0;
	td.memoria_partilhada = memoria;
	td.id_lider_atual = -1;
	td.hThreadLetras = NULL;

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		td.players[i].hPipe = NULL;
		_tcscpy_s(td.players[i].name, TAM_USERNAME, _T("NO_USER"));
		td.players[i].points = 0;
	}

	//===============================//
	// Criar thread de distribuição  //
	//===============================//
	hThreadDistribui = CreateThread(NULL, 0, distribui, (LPVOID)&td, 0, NULL);

	//===============================//
	// Loop principal do árbitro     //
	//===============================//
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (hEv == NULL) {
		_tprintf(_T("[ERRO] Falha ao criar evento para overlapped (%d)\n"), GetLastError());
		return 1;
	}

	do {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;

		_tprintf_s(_T("[ESCRITOR] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"), PIPE_NAME);

		hPipe = CreateNamedPipe(
			PIPE_NAME,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			MAX_CONCURRENT_USERS,
			sizeof(MENSAGEM),
			sizeof(MENSAGEM),
			3000,
			NULL
		);

		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf_s(_T("[ERRO] Criar Named Pipe! (CreateNamedPipe)\n"));
			break;
		}

		_tprintf_s(_T("[ARBITRO] Esperar ligação de um jogador... (ConnectNamedPipe)\n"));

		ligado = FALSE;
		if (!ConnectNamedPipe(hPipe, &ov)) {
			erro = GetLastError();
			if (erro == ERROR_IO_PENDING) {
				while (td.continua) {
					DWORD waitResult = WaitForSingleObject(hEv, 1000);
					if (waitResult == WAIT_OBJECT_0) {
						ligado = TRUE;
						break;
					}
				}

				if (!td.continua) {
					CancelIo(hPipe); 
					CloseHandle(hPipe);
					break;
				}
			}
			else if (erro == ERROR_PIPE_CONNECTED) {
				ligado = TRUE;
			}
			else {
				_tprintf_s(_T("[ERRO] Erro a ligar ao pipe (ConnectNamedPipe) (%d)\n"), erro);
				CloseHandle(hPipe);
				break;
			}
		}
		else {
			ligado = TRUE;
		}


		if (!ligado) {
			_tprintf_s(_T("[ERROR] Ligação falhou foi cancelada\n"));
			CloseHandle(hPipe);
			break;
		}

		//===============================//
		// Atribuir jogador à estrutura //
		//===============================//
		WaitForSingleObject(td.hMutex, INFINITE);
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.players[i].hPipe == NULL) {
				td.players[i].hPipe = hPipe;
				break;
			}
		}
		ReleaseMutex(td.hMutex);

		//===============================//
		// Criar thread para o jogador  //
		//===============================//
		HANDLE hThreadAtende = CreateThread(NULL, 0, atende_cliente, (LPVOID)&td, 0, NULL);

		//===============================//
		// Criar thread de letras (uma vez) //
		//===============================//
		if (td.hThreadLetras == NULL) {
			td.hThreadLetras = CreateThread(NULL, 0, letras, (LPVOID)&td, 0, NULL);
			SuspendThread(td.hThreadLetras);
		}

		ResetEvent(hEv);  // prepara para próxima iteração

	} while (td.continua);

	_tprintf_s(_T("Thread main a sair\n"));
	CloseHandle(hEv);
	WaitForSingleObject(hThreadDistribui, INFINITE);
	CloseHandle(hThreadDistribui);
	CloseHandle(td.hMutex);
	UnmapViewOfFile(memoria);
	CloseHandle(hMemoPart);
	CloseHandle(hMutexGlobal);
	free(letras_jogo);

	return 0;
}


