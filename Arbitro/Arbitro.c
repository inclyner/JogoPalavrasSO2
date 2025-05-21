// Arbitro.c : This file contains the 'main' function. Program execution begins and ends there.
//
#include "header.h"

// prototipos funções
DWORD WINAPI processArbitroComands(LPVOID param);
TCHAR* getRandomLetter(TCHAR* abecedario, int max_letras);


MENSAGEM consola_arbitro(MENSAGEM msg,TDATA *ptd) {
	TDATA td;
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
	MENSAGEM resposta;
	resposta.tipo = ERRO;
	DWORD i;

	WaitForSingleObject(ptd->hMutex, INFINITE);
	td = *ptd;
	ReleaseMutex(ptd->hMutex);


	if (token == NULL) {
		_tprintf(_T("Comando inválido.\n"));
		// envia isto 
		return;
	}

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
			// envia isto 
			return;
		}
		_tprintf(_T("excluir %s \n"), token);

	}
	else if (_tcscmp(token, _T("iniciarbot")) == 0) {
		token = _tcstok_s(comando_token, _T(" ,\n"), &comando_token);
		if (token == NULL) {
			_tprintf(_T("Comando inválido.\n"));
			// envia isto 
			return;
		}
		_tprintf(_T("inciarbot %s \n"), token);
	}
	else if (_tcscmp(token, _T("acelerar")) == 0) {
		_tprintf(_T("acelerar\n"));
	}
	else if (_tcscmp(token, _T("travar")) == 0) {
		_tprintf(_T("travar\n"));
	}
	else {
		_tprintf(_T("Comando desconhecido: %s\n"), token);
	}

	return resposta;
}

MENSAGEM consola_jogoui(MENSAGEM msg, TDATA* ptd) {
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
	MENSAGEM resposta;
	TDATA td;
	DWORD user_id = -1;
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
			_stprintf_s(resposta.comando, TAM, _T("%d"), ptd->next_id); // STR ---> 
			_tcscpy_s(ptd->players[ptd->next_id].name, TAM_USERNAME, msg.comando);
			_tprintf_s(_T("n_users %d, name = %s\n"), ptd->n_users, ptd->players[ptd->next_id].name);
			ptd->n_users++;
			if (ptd->next_id + 1 == ptd->n_users) {
				ptd->next_id++;
			}
			else {
				ptd->next_id = ptd->n_users;
			}
			ReleaseMutex(ptd->hMutex);
		}
		else {
			_stprintf_s(resposta.comando, TAM, _T("%d"), -1);
		}
		
		break;
	case PALAVRA:
		resposta.tipo = PALAVRA;
		_tprintf_s(_T(" palavra %s\n"), msg.comando);
		break;
	case COMANDO:
		_tprintf_s(_T(" comando %s\n"), msg.comando);
		resposta.tipo = COMANDO;
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
		// TODO todo o processamento vai ser feito aqui
		//ANTES DA OPERACAO 
		
		

		resposta = consola_jogoui(msg,ptd);
		
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


		if (FALSE) {
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

		if (_tcsicmp(msg.comando, _T(":sair")) == 0) {
			
			DisconnectNamedPipe(ptd->players[myPos].hPipe);
			CloseHandle(ptd->players[myPos].hPipe);
			WaitForSingleObject(ptd->hMutex, INFINITE);
			_tprintf_s(_T("[ARBITRO] O jogador (id=%d) %s vai sair.\n"),myPos , ptd->players[myPos].name);
			ptd->players[myPos].hPipe = NULL;
			_tcscpy_s(ptd->players[myPos].name, TAM, _T(""));
			ptd->n_users--;
			ptd->next_id = myPos;

			ReleaseMutex(ptd->hMutex);
			ativo = FALSE;
			continue;
		}
	} while (ativo);
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

		if (resposta.tipo < ERRO) {
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
				_tprintf_s(_T("[ESCRITOR] Enviei %d bytes ao leitor...  (i = %d) (WriteFile)\n"), n, i);
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

int _tmain(int argc, TCHAR* argv[]) {

	DWORD i;
	HANDLE hPipe;
	TCHAR buf[256];
	TDATA td;
	BOOL isGameOn;

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif


	int max_letras, ritmo;
	getRegistryValues(&max_letras, &ritmo);

	TCHAR* letras_jogo = (TCHAR*)malloc(max_letras * sizeof(TCHAR));

	TCHAR abecedario[26] = {
		_T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'),
		_T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'), _T('O'), _T('P'),
		_T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'), _T('W'), _T('X'),
		_T('Y'), _T('Z')
	};

	if (letras_jogo == NULL) {
		_tprintf(_T("Erro a alocar memória!\n"));
		return 1;
	}
	for (int i = 0; i < max_letras; i++) {
		letras_jogo[i] = _T('_');
	}

	/*
	DWORD adminThreadId;
	//lança thread que ouve comandos do admin
	
	HANDLE hThreadArbitro;
	HANDLE hThreadAdmitUsers;
	
	hThreadArbitro = CreateThread(
		NULL,
		0,
		processArbitroComands,
		NULL,
		0,
		&adminThreadId
	);

	if (hThreadArbitro == NULL) {
		_tprintf("Erro ao criar thread(arbitro): %lu\n", GetLastError());
		return 1;
	}
	
	hThreadAdmitUsers = CreateThread(
		NULL,
		0,
		admitUsers,
		NULL,
		0,
		&adminThreadId
	);
	if (hThreadAdmitUsers == NULL) {
		_tprintf("Erro ao criar thread(AdmitUsers): %lu\n", GetLastError());
		return 1;
	}
	*/
	// Preparar dados da thread 
	td.next_id = 0;
	td.n_users = 0;
	td.isGameOn = FALSE;
	td.continua = TRUE;
	td.hMutex = NULL;
	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		td.players[i].hPipe = NULL;
		_tcscpy_s(td.players[i].name, TAM_USERNAME, _T("NO_USER"));
		td.players[i].points = 0;
	}
	td.hMutex = CreateMutex(NULL, FALSE, NULL);
	HANDLE hThreadDistribui = CreateThread(NULL, 0, distribui, (LPVOID)&td, 0, NULL);

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

		if (!td.isGameOn && td.n_users == 2) {
			_tprintf_s(_T("\nStart Game\n"));
			td.isGameOn == TRUE;
		}

		if (td.isGameOn && td.n_users == 1) {
			_tprintf_s(_T("\nEnd GAME\n"));
			td.isGameOn == FALSE;
		}


		ReleaseMutex(td.hMutex);
		//Criar theread cliente (hpipe
		
		HANDLE hThreadAtende = CreateThread(NULL, 0, atende_cliente, (LPVOID)&td, 0, NULL);

	} while (td.continua);


	//TODO main loop do jogo
	//while (currentUsers >= MINIMUM_GAME_PLAYERS)
	//{}
	

	WaitForSingleObject(hThreadDistribui, INFINITE);
	CloseHandle(hThreadDistribui);
	CloseHandle(td.hMutex);



	/*
	WaitForSingleObject(hThreadArbitro, INFINITE);
	// Libertar memória
	free(letras_jogo);
	CloseHandle(hThreadArbitro);
	CloseHandle(hThreadAdmitUsers);
	*/

	return 0;

}



int getRegistryValues(int* maxletras, int* ritmo) {
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
		_tprintf(_T("MAXLETRAS não existe, a criar com valor 6.\n"));
		valMaxLetras = 6;
		RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
	}
	else
	{
		_tprintf(_T("MAXLETRAS lido! => %d\n"), valMaxLetras);
		if (valMaxLetras > 12) {
			_tprintf(_T("MAXLETRAS superior a 12. A corrigir para 12.\n"));
			valMaxLetras = 12;
			RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
		}
	}
	res = RegQueryValueEx(hKey, _T("RITMO"), NULL, &tipo, (LPBYTE)&valRitmo, &tam);
	if (res != ERROR_SUCCESS) {
		_tprintf(_T("RITMO não existe, a criar com valor 3.\n"));
		valRitmo = 3;
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


TCHAR* getRandomLetter(TCHAR* abecedario, int max_letras) {
	int randomIndex = rand() % 26;
	TCHAR* letra = (TCHAR*)malloc(sizeof(TCHAR));
	if (letra == NULL) {
		_tprintf(_T("Erro a alocar memória!\n"));
		return NULL;
	}
	letra = abecedario[randomIndex];
	return letra;
}


DWORD WINAPI processArbitroComands(LPVOID param) {
	_tprintf(_T("Thread processArbitroComands a correr!\n"));
	int isLeaving = 0;

	while (isLeaving) {


	}

	return 0;
}



DWORD WINAPI admitUsers(LPVOID param) {
	_tprintf(_T("Thread admitUsers a correr!\n"));
	int nCurrentUsers = 0;
	while (nCurrentUsers <= MAX_CONCURRENT_USERS)
	{
		//TODO lógica para admitir utilizadores
		//ler pipe PIPE_NAME até haver algo


	}


	return 0;
}



