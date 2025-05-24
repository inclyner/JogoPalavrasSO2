// Arbitro.c : This file contains the 'main' function. Program execution begins and ends there.
//
#include "header.h"
#include "Arbitro.h"

// prototipos funções
DWORD WINAPI processArbitroComands(LPVOID param);


const TCHAR* DICIONARIO[] = {
	_T("GATO"), _T("CÃO"), _T("RATO"), _T("SAPO"), _T("LOBO"),
	_T("LIVRO"), _T("BOLA"), _T("MESA"), _T("PORTA"), _T("CASA"),
	_T("CARRO"), _T("FACA"), _T("PATO"), _T("PEIXE"), _T("CHAVE"),
	_T("BANCO"), _T("CARTA"), _T("FAROL"), _T("LATA"), _T("FITA"),
	_T("VIDRO"), _T("TECLA"), _T("JANELA"), _T("CADEIRA"), _T("CAMA"),
	_T("TINHA"), _T("BICHO"), _T("MANTO"), _T("FUMO"), _T("TROCO")
};
const int NUM_PALAVRAS = sizeof(DICIONARIO) / sizeof(DICIONARIO[0]);

TCHAR letraRandom() {
	return _T('A') + (rand() % 26);
}

void acelerarRitmo(TDATA* ptd) {
	WaitForSingleObject(ptd->hMutex, INFINITE);
	ptd->ritmo--;
	ReleaseMutex(ptd->hMutex);
	_tprintf_s(_T("[ARBITRO] Novo ritmo: %d.\n"),ptd->ritmo);

}

void travarRitmo(TDATA* ptd) {
	WaitForSingleObject(ptd->hMutex, INFINITE);
	ptd->ritmo++;
	ReleaseMutex(ptd->hMutex);
	_tprintf_s(_T("[ARBITRO] Novo ritmo: %d.\n"), ptd->ritmo);
}

DWORD getPlayerByName(TDATA td,TCHAR token[]) {
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
	DisconnectNamedPipe(ptd->players[id].hPipe);
	CloseHandle(ptd->players[id].hPipe);
	WaitForSingleObject(ptd->hMutex, INFINITE);
	_tprintf_s(_T("[ARBITRO] O jogador (id=%d) %s vai sair.\n"), id, ptd->players[id].name);
	ptd->players[id].hPipe = NULL;
	_tcscpy_s(ptd->players[id].name, TAM_USERNAME, _T(""));
	ptd->n_users--;
	_tprintf_s(_T("[ELIMINAR PLAYER] O jogador (id=%d) %s vai sair. n_users=%d\n"), id, ptd->players[id].name, ptd->n_users);
	ptd->next_id = id;

	//pausar jogo se restar apenas um jogador
	if (ptd->n_users < 2 && ptd->isGameOn) {
		_tprintf_s(_T("[ARBITRO] Só resta 1 jogador. A pausar jogo.\n"));
		ptd->isGameOn = FALSE;
		SuspendThread(ptd->hThreadLetras);
	}

	ReleaseMutex(ptd->hMutex);
}


void enviar_todos(TDATA* ptd, MENSAGEM msg) {
	BOOL ret;
	DWORD i, n;
	TDATA td;
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);


	WaitForSingleObject(ptd->hMutex, INFINITE);
	td = *ptd;
	ReleaseMutex(ptd->hMutex);

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
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

MENSAGEM consola_arbitro(MENSAGEM msg,TDATA *ptd) {
	TDATA td;
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
	MENSAGEM resposta;
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
		// termina a theard 
		return;
	}
	else if (_tcscmp(token, _T("listar")) == 0) {
		_tprintf(_T("Lista de jogadores \n"));
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.players[i].hPipe != NULL) {
				_tprintf(_T("Nome: %s \t | Pontos: %d\n"), td.players[i].name, td.players[i].points);
			}
		}
	}
	else if (_tcscmp(token, _T("excluir")) == 0) {
		token = _tcstok_s(comando_token, _T(" ,\n"), &comando_token);
		if (token == NULL) {
			_tprintf(_T("Comando inválido.\n"));
		}
		else {
			_tprintf(_T("excluir %s \n"), token);
			id = getPlayerByName(td, token);
			if (id != -1) {
				_tprintf(_T("TODO excluir username\n"));
				WaitForSingleObject(ptd->hMutex, INFINITE);
				EliminarPlayer(ptd, id);
				ReleaseMutex(ptd->hMutex);
				//_tcscpy_s(resposta.comando, TAM, _T(":sair"));
			} else {
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
			_tprintf(_T("inciarbot %s \n"), token);

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


MENSAGEM consola_jogoui(MENSAGEM msg, TDATA* ptd, DWORD myPos, DWORD *ativo) {
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
	MENSAGEM resposta;
	TDATA td;
	DWORD user_id = -1;
	TCHAR aux[TAM_USERNAME + 5];
	DWORD i;
	WaitForSingleObject(ptd->hMutex, INFINITE);
	td = *ptd;
	ReleaseMutex(ptd->hMutex);
	_tcscpy_s(resposta.comando, TAM, msg.comando);
	resposta.tipo = ERRO;

	switch (msg.tipo) {
	case USERNAME:
		_tprintf_s(_T("O jogador %s está a tentar entrar no jogo.\n"), msg.comando);

		if (isUserValid(msg, td)) {
			resposta.tipo = USERNAME;
			WaitForSingleObject(ptd->hMutex, INFINITE);
			_tprintf_s(_T("next_id %d\n"), ptd->next_id);
			_stprintf_s(resposta.comando, TAM, _T("%d"), ptd->next_id); 
			_tcscpy_s(ptd->players[ptd->next_id].name, TAM_USERNAME, msg.comando);
			ptd->n_users++;
			_tprintf_s(_T("n_users %d, name = %s\n"), ptd->n_users, ptd->players[ptd->next_id].name);
			if (ptd->next_id + 1 == ptd->n_users) {
				ptd->next_id++;
			}
			else {
				ptd->next_id = ptd->n_users;
			}
			if (!ptd->isGameOn && ptd->n_users == 2) {
				ptd->isGameOn = TRUE;
				ResumeThread(ptd->hThreadLetras);
			}

			ReleaseMutex(ptd->hMutex);
		}
		else {
			_stprintf_s(resposta.comando, TAM, _T("%d"), -1);
			_tprintf_s(_T("O jogador %s já existe.\n"), msg.comando);
			// fecha pipe do jogador
			// quando o jogoui sabe que deve fechar a conexão não fecha
			/*WaitForSingleObject(ptd->hMutex, INFINITE);
			ptd->players[myPos].hPipe = NULL;  // desassocia o pipe
			_tcscpy_s(ptd->players[myPos].name, TAM_USERNAME, _T(""));  // limpa o nome
			ReleaseMutex(ptd->hMutex);
			*/
		
		}
		
		break;
	case PALAVRA:
		resposta.tipo = PALAVRA;
		int letras_usadas[MAX_LETRAS];
		BOOL palavra_valida = validarPalavra(msg.comando, ptd->memoria_partilhada, letras_usadas);

		_tprintf_s(_T(" palavra %s\n"), msg.comando);

		if (palavra_valida) {
			_tprintf(_T("[ARBITRO] Palavra válida: %s\n"), msg.comando);

			WaitForSingleObject(ptd->hMutex, INFINITE);
			td.players[myPos].points += _tcslen(msg.comando);
			_tcscpy_s(ptd->memoria_partilhada->ultima_palavra, MAX_LETRAS, msg.comando);

			// Remover letras usadas
			for (int i = 0; i < _tcslen(msg.comando); i++) {
				ptd->memoria_partilhada->letras[letras_usadas[i]] = _T('_');
			}
			ReleaseMutex(ptd->hMutex);

			_stprintf_s(resposta.comando, TAM, _T("Palavra válida! +%d pontos."), _tcslen(msg.comando));
			//anunciar que jogador x acertou palavra y e ganhou z pontos
			MENSAGEM broadcast;
			broadcast.tipo = COMANDO; 
			_stprintf_s(broadcast.comando, TAM, _T("%s acertou: %s (+%d pontos)"),
				td.players[myPos].name,
				msg.comando,
				_tcslen(msg.comando)
			);
			enviar_todos(ptd, broadcast);

			//verificar se há novo lider
			DWORD novo_lider = getIdLider(ptd);

			if (novo_lider != ptd->id_lider_atual) {
				MENSAGEM m_lider;
				m_lider.tipo = COMANDO;
				_stprintf_s(m_lider.comando, TAM, _T("O jogador %s passou para a frente com %d pontos!"),
					ptd->players[novo_lider].name,
					ptd->players[novo_lider].points
				);
				enviar_todos(ptd, m_lider);

				ptd->id_lider_atual = novo_lider;
			}
		}
		else {
			_tprintf(_T("[ARBITRO] Palavra inválida: %s\n"), msg.comando);

			WaitForSingleObject(ptd->hMutex, INFINITE);
			td.players[myPos].points -= (_tcslen(msg.comando) / 2.0);
			ReleaseMutex(ptd->hMutex);

			_stprintf_s(resposta.comando, TAM, _T("Palavra inválida! -%.1f pontos."), (_tcslen(msg.comando) / 2.0));
		}
		break;
		// ver se existe no dicionario
		// ver se as letras todas fazem parte das letras da shared memory
		// ver se as letras da palavra são menores do que 
		_tprintf_s(_T(" palavra %s\n"), msg.comando);
		break;
	case COMANDO:
		resposta.tipo = COMANDO;
		_tprintf_s(_T(" comando %s\n"), msg.comando);
		if (_tcsicmp(msg.comando, _T(":pont")) == 0) {
			_stprintf_s(resposta.comando, TAM, _T("Pontos: %d\n"),td.players[myPos].points); 
			_tprintf_s(_T("pontos %d\n"), td.players[myPos].points);
		}
		else if (_tcsicmp(resposta.comando, _T(":jogs")) == 0) {
			_stprintf_s(resposta.comando, TAM, _T("\nLista de Jogadores: \n"));
			for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
				if (td.players[i].hPipe != NULL && i != myPos) {
					_stprintf_s(aux, TAM_USERNAME + 5, _T("%s\n"), td.players[i].name);
					_tcscat_s(resposta.comando, TAM, aux);
					_tprintf_s(_T("nomes %s\n"), td.players[i].name);
				}
			}
			_tprintf_s(_T("nomes %s\n"), resposta.comando);
		}
		else {
			ativo = FALSE;
		}
		// se for :sair ele faz a logica fora desta funcao por causa de eliminar o player
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
	_tprintf_s(_T("[LETRAS] LETRAS\n"));

	TDATA* ptd = (TDATA*)data;
	TDATA td;
	DWORD i;
	HANDLE hMemoPart;
	TCHAR letra;
	DWORD pos;
	BOOL livre;
	MENSAGEM msg;
	MEMORIA_PARTILHADA* memoria;

	hMemoPart = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		MEMORY_NAME
	);

	if (hMemoPart == NULL) {
		_tprintf(TEXT("Erro ao abrir memória partilhada (%d).\n"), GetLastError());
		return 1;
	}

	memoria = (LPTSTR)MapViewOfFile(
		hMemoPart,
		FILE_MAP_ALL_ACCESS,
		0, 0,
		sizeof(MEMORIA_PARTILHADA)
	);

	if (memoria == NULL) {
		_tprintf(TEXT("Erro ao mapear memória (%d).\n"), GetLastError());
		CloseHandle(hMemoPart);
		return 1;
	}

	do{
		WaitForSingleObject(ptd->hMutex, INFINITE);
		td = *ptd;
		ReleaseMutex(ptd->hMutex);

		pos = -1;
		livre = TRUE;
		_tprintf_s(_T("letras:\t"));
		for (i = 0; i < memoria->num_letras; i++) {
			_tprintf_s(_T("  %c\t"), memoria->letras[i]);
			if (livre) {
				if (memoria->letras[i] == _T('_')) {
					pos = i;
					livre = FALSE;
				}
			}
		}
		_tprintf_s(_T("\n"));
		Sleep(td.ritmo * 1000);
		letra = letraRandom();
		
		_tprintf(_T(" pos(%d) td.id(%d) max(%d).\n"), pos, td.id_letra, td.max_letras);
		if (pos != -1) {
			memoria->letras[pos] = letra;
			td.id_letra = pos;
		}
		else {
			memoria->letras[td.id_letra] = letra;
			WaitForSingleObject(ptd->hMutex, INFINITE);
			if (td.id_letra >= memoria->num_letras - 1) {
				ptd->id_letra = 0;
			}else {
				ptd->id_letra++;
			}
			ReleaseMutex(ptd->hMutex);
		}
		_stprintf_s(msg.comando, TAM, _T("%s"), memoria->letras);

		enviar_todos(ptd,msg);
		
	} while (ptd->isGameOn);


}


// nesta thread o arbitro vai receber os pedido neste caso as palavras ou os comandos 
// e vai fazer fazer o que tem a fazer sobre os pontos e tudo mais 
// e depois envia por exemplo os pontos ao user ou que ele pediu nos comandos
DWORD WINAPI atende_cliente(LPVOID data) {
	TDATA* ptd = (TDATA*)data;
	MENSAGEM msg, resposta;
	DWORD n;
	BOOL ret, ativo = TRUE;
	DWORD i, myPos;
	TDATA td;

	WaitForSingleObject(ptd->hMutex, INFINITE);
	myPos = ptd->next_id;
	ReleaseMutex(ptd->hMutex);


	//FORA CILCO...
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		//ANTEs da operaçao 
		ret = ReadFile(ptd->players[myPos].hPipe, &msg, sizeof(MENSAGEM), &n, &ov);
		if (!ret && GetLastError() != ERROR_IO_PENDING) {
			_tprintf_s(_T("[ERROR READ] %d (%d bytes)... (ReadFile)\n"), ret, n);
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
			GetOverlappedResult(ptd->players[myPos].hPipe, &ov, &n, FALSE); // obter resultado da operacao
		}
		// buf[n / sizeof(TCHAR)] = _T('\0');

		_tprintf_s(_T("[ESCRITOR] Recebi '%s'(%d bytes) ...... (ReadFile)\n"), msg.comando, n);
		// PROCESSAMENTO
		


		
		//ANTES DA OPERACAO 
		
		

		resposta = consola_jogoui(msg,ptd,myPos,&ativo);
		
		// envia a resposta
		ret = WriteFile(ptd->players[myPos].hPipe, &resposta, sizeof(MENSAGEM), &n, &ov);
		if (!ret && GetLastError() != ERROR_IO_PENDING) { // || n != _tcslen(buf) * sizeof(TCHAR)
			_tprintf_s(_T("[ERROR] Write failed code(%d)\n"), GetLastError());
			
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
			GetOverlappedResult(ptd->players[myPos].hPipe, &ov, &n, FALSE); // obter resultado da operacao
		}


		_tprintf_s(_T("[ARBITRO] Resposta '%s'(%d bytes) ... (WriteFile)\n"), resposta.comando, n);


		if (FALSE) { // vai servir para enviar coisas a todos os users 
			WaitForSingleObject(ptd->hMutex, INFINITE);
			td = *ptd;
			ReleaseMutex(ptd->hMutex);

			for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
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
				}
				_tprintf_s(_T("[ARBITRO ALL] Enviei %d bytes ao leitor...  (i = %d) (WriteFile)\n"), n, i);
			}
		}

	} while (ativo);
	EliminarPlayer(ptd, myPos);
	CloseHandle(hEv);
	_tprintf_s(_T("Theread atende a sair\n"));
	ExitThread(0);
}

// Nesta thread tratamos os comandos do arbitro e enviamos o que seja necessario para todos os utilizadores
DWORD WINAPI distribui(LPVOID data) {
	TDATA* ptd = (TDATA*)data;
	TCHAR buf[256];
	DWORD n, i;
	TDATA td;
	BOOL ret;
	MENSAGEM msg, resposta;
	// FOra do cliclo 
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		_tprintf_s(_T("[ESCRITOR] Frase: "));
		_fgetts(msg.comando, 256, stdin);
		msg.comando[_tcslen(msg.comando) - 1] = '\0';

		WaitForSingleObject(ptd->hMutex, INFINITE);
		td = *ptd;
		ReleaseMutex(ptd->hMutex);

		resposta = consola_arbitro(msg, ptd);

		if (resposta.tipo > ERRO) {
			for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
				if (ptd->players[i].hPipe != NULL) {
					//ANTES Da operacao 
					ZeroMemory(&ov, sizeof(OVERLAPPED));
					ov.hEvent = hEv;
					ret = WriteFile(ptd->players[i].hPipe, &resposta, sizeof(MENSAGEM), &n, &ov);
					if (!ret && GetLastError() != ERROR_IO_PENDING) {
						_tprintf_s(_T("[ERRO] Escrever no pipe! (WriteFile)\n"));
						break;
					}
					if (GetLastError() == ERROR_IO_PENDING) {
						WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
						GetOverlappedResult(ptd->players[i].hPipe, &ov, &n, FALSE); // obter resultado da operacao
					}
					_tprintf_s(_T("[ESCRITOR] Enviei %s bytes ao jogoui...  (i = %d) (WriteFile)\n"), resposta.comando, i);

				}
			}
		}

	} while (_tcsicmp(buf, _T("fim")));

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (ptd->players[i].hPipe != NULL) {
			FlushFileBuffers(ptd->players[i].hPipe);
			_tprintf_s(_T("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
			if (!DisconnectNamedPipe(ptd->players[i].hPipe)) {
				_tprintf_s(_T("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
				exit(-1);
			}
			CloseHandle(ptd->players[i].hPipe);
		}
	}
	CloseHandle(hEv);
	ExitThread(0);
}

void getRegistryValues(int* maxletras, int* ritmo) {
	HKEY hKey;
	DWORD tipo = REG_DWORD;
	DWORD tam = sizeof(DWORD);
	DWORD valMaxLetras = 6;
	DWORD valRitmo = 3;

	TCHAR chave[] = _T("Software\\TrabSO2");


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

		//RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
		//RegSetValueEx(hKey, _T("RITMO"), 0, REG_DWORD, (const BYTE*)&valRitmo, sizeof(DWORD));
	}

	tam = sizeof(DWORD);
	res = RegQueryValueEx(hKey, _T("MAXLETRAS"), NULL, &tipo, (LPBYTE)&valMaxLetras, &tam);
	if (res != ERROR_SUCCESS) {
		_tprintf(_T("MAXLETRAS não existe, a criar com valor %D.\n"), DEFAULT_LETRAS);
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

	// Copiar letras visíveis (para manipular localmente)
	for (i = 0; i < memoria_partilhada->num_letras; i++) {
		letras_visiveis[i] = memoria_partilhada->letras[i];
	}

	// Verificar se todas as letras da palavra estão nas letras visíveis (e marcar posições)
	for (i = 0; i < len_palavra; i++) {
		TCHAR c = _totupper(palavra[i]);
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

	// Verificar se palavra está no dicionário
	BOOL in_dicionario = FALSE;
	for (i = 0; i < NUM_PALAVRAS; i++) {
		if (_tcsicmp(DICIONARIO[i], palavra) == 0) {
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
			if ((int)ptd->players[i].points > max_pontos) {
				max_pontos = (int)ptd->players[i].points;
				id_lider = i;
			}
		}
	}
	return id_lider;
}

int _tmain(int argc, TCHAR* argv[]) {

	DWORD i;
	HANDLE hPipe;
	TCHAR buf[256];
	TDATA td;
	BOOL isGameOn;
	HANDLE hMemoPart;
	HANDLE hThreadLetras, hThreadDistribui;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	srand((unsigned int)time(NULL));

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

	hMemoPart = CreateFileMapping(
		INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,       
		0, sizeof(MEMORIA_PARTILHADA), MEMORY_NAME
	);

	if (hMemoPart == NULL) {
		_tprintf(_T("Erro ao criar a memória partilhada (%d).\n"), GetLastError());
		return 1;
	}

	MEMORIA_PARTILHADA* memoria = MapViewOfFile(
		hMemoPart,            
		FILE_MAP_ALL_ACCESS,0, 0,              
		sizeof(MEMORIA_PARTILHADA)
	);

	if (memoria == NULL) {
		_tprintf(_T("Erro ao mapear (%d).\n"), GetLastError());
		CloseHandle(memoria);
		return 1;
	}

	for (int i = 0; i < max_letras; ++i) {
		memoria->letras[i] = _T('_');
	}
	memoria->num_letras = max_letras;
	_stprintf_s(memoria->ultima_palavra, MAX_LETRAS, _T("------"));


	td.ritmo = ritmo;
	td.next_id = 0;
	td.n_users = 0;
	td.isGameOn = FALSE;
	td.continua = TRUE;
	td.hMutex = NULL;
	td.max_letras = max_letras;
	td.id_letra = 0;
	td.memoria_partilhada = memoria;
	td.id_lider_atual = -1;
	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		td.players[i].hPipe = NULL;
		_tcscpy_s(td.players[i].name, TAM_USERNAME, _T("NO_USER"));
		td.players[i].points = 0;
	}
	td.hMutex = CreateMutex(NULL, FALSE, NULL);
	td.hThreadLetras = NULL;
	hThreadDistribui = CreateThread(NULL, 0, distribui, (LPVOID)&td, 0, NULL);

	// Criar mutex
	// CRiar thread
	do {

		_tprintf_s(_T("[ESCRITOR] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"),
			PIPE_NAME);
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT
			| PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, MAX_CONCURRENT_USERS, sizeof(MENSAGEM), sizeof(MENSAGEM),
			3000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf_s(_T("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
			exit(-1);
		}

		_tprintf_s(_T("[ARBITRO] Esperar ligação de um jogador... (ConnectNamedPipe)\n"));
		if (!ConnectNamedPipe(hPipe, NULL)) {
			_tprintf_s(_T("[ERRO] Ligação ao leitor! (ConnectNamedPipe\n"));
			exit(-1);
		}

		WaitForSingleObject(td.hMutex, INFINITE);
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.players[i].hPipe == NULL) {
				td.players[i].hPipe = hPipe;
				break;
			}
		}
		ReleaseMutex(td.hMutex);

		

		HANDLE hThreadAtende = CreateThread(NULL, 0, atende_cliente, (LPVOID)&td, 0, NULL);
		if (td.hThreadLetras == NULL) {
			td.hThreadLetras = CreateThread(NULL, 0, letras, (LPVOID)&td, 0, NULL);
			SuspendThread(td.hThreadLetras);
		}

	} while (td.continua);


	WaitForSingleObject(hThreadDistribui, INFINITE);
	CloseHandle(hThreadDistribui);
	CloseHandle(td.hMutex);
	UnmapViewOfFile(memoria);
	CloseHandle(hMemoPart);

	return 0;

}





