// Arbitro.c : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <winreg.h>
#include "arbitro.h"
#include "utils.c"

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")
#define TAM 200
#define MAX_CONCURRENT_USERS 20
#define MINIMUM_GAME_PLAYERS 2





int _tmain(int argc, TCHAR* argv[]) {

	DWORD i;
	HANDLE hPipe;
	TCHAR buf[256];

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

	// Preparar dados da thread 
	TDATA td = { {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},NULL,TRUE };
	td.hMutex = CreateMutex(NULL, FALSE, NULL);
	HANDLE hThreadDistribui = CreateThread(NULL, 0, distribui, (LPVOID)&td, 0, NULL);

	// Criar mutex
	// CRiar thread
	do {
		_tprintf_s(_T("[ESCRITOR] Criar uma cópia do pipe '%s' ... (CreateNamedPipe)\n"),
			PIPE_NAME);
		hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT
			| PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, MAX_CONCURRENT_USERS, sizeof(buf), sizeof(buf),
			3000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf_s(_T("[ERRO] Criar Named Pipe! (CreateNamedPipe)"));
			exit(-1);
		}

		_tprintf_s(_T("[ESCRITOR] Esperar ligação de um leitor... (ConnectNamedPipe)\n"));
		if (!ConnectNamedPipe(hPipe, NULL)) {
			_tprintf_s(_T("[ERRO] Ligação ao leitor! (ConnectNamedPipe\n"));
			exit(-1);
		}


		WaitForSingleObject(td.hMutex, INFINITE);
		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.hPipe[i] == NULL) {
				td.hPipe[i] = hPipe;
				break;
			}
		}
		ReleaseMutex(td.hMutex);
		//Criar theread cliente (hpipe)
		
		HANDLE hThreadAtende = CreateThread(NULL, 0, atende_cliente, (LPVOID)hPipe, 0, NULL);

	} while (td.continua);


	

	WaitForSingleObject(hThreadDistribui, INFINITE);
	CloseHandle(hThreadDistribui);
	CloseHandle(td.hMutex);
	free(letras_jogo);


	return 0;

}


void consola_arbitro(TCHAR comando[]) {
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(comando, _T(" ,\n"), &comando_token);

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
		_tprintf(_T("listar\n"));
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
}

void consola_jogoui(TCHAR comando[]) {
	TCHAR* comando_token;
	TCHAR* token = _tcstok_s(comando, _T(" ,\n"), &comando_token);

	if (token == NULL) {
		_tprintf(_T("Comando inválido.\n"));
		// envia isto 
		return;
	}

	if (_tcscmp(token, _T(":sair")) == 0) {
		// termina a theard 
		_tprintf(_T("sair\n"));
		return;
	}
	else if (_tcscmp(token, _T(":pont")) == 0) {
		_tprintf(_T(":pont\n"));
	}
	else if (_tcscmp(token, _T(":jogs")) == 0) {
		_tprintf(_T(":jogs\n"));
	}
	else {
		token = _tcstok_s(comando_token, _T(" ,\n"), &comando_token);
		if (token == NULL) {
			// quer dizer que é uma palavra
			// envia isto 
			// faz o quer for necessario
			return;
		}
		_tprintf(_T("Demasiados argumentos \n"), token);
	}
}

// nesta thread o arbitro vai receber os pedido neste caso as palavras ou os comandos 
// e vai fazer fazer o que tem a fazer sobre os pontos e tudo mais 
// e depois envia por exemplo os pontos ao user ou que ele pediu nos comandos
DWORD WINAPI atende_cliente(LPVOID data) {
	HANDLE hPipe = (HANDLE)data;
	TCHAR buf[256];
	DWORD n;
	BOOL ret;

	//FORA CILCO...
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		ZeroMemory(&ov, sizeof(OVERLAPPED));
		ov.hEvent = hEv;
		//ANTEs da operaçao 
		ret = ReadFile(hPipe, buf, sizeof(buf), &n, &ov);
		if (!ret && GetLastError() != ERROR_IO_PENDING) {
			_tprintf_s(_T("[ERROR READ] %d (%d bytes)... (ReadFile)\n"), ret, n);
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
			GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
		}
		buf[n / sizeof(TCHAR)] = _T('\0');

		_tprintf_s(_T("[ESCRITOR] Recebi '%s'(%d bytes) ...... (ReadFile)\n"), buf, n);
			// PROCESSAMENTO
		// TODO todo o processamento vai ser feito aqui
		//ANTES DA OPERACAO 
		consola_jogoui(buf);

		// envia a resposta
		ret = WriteFile(hPipe, buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, NULL);
		if (!ret && GetLastError() != ERROR_IO_PENDING) { // || n != _tcslen(buf) * sizeof(TCHAR)
			_tprintf_s(_T("[ERROR] Write failed code(%d)\n"), GetLastError());
			break;
		}
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
			GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
		}
		_tprintf_s(_T("[ARBITRO] Resposta '%s'(%d bytes) ... (WriteFile)\n"), buf, n);
	} while (1);
	CloseHandle(hEv);

	ExitThread(0);
}

// Nesta thread tratamos os comandos do arbitro e enviamos o que seja necessario para todos os utilizadores
DWORD WINAPI distribui(LPVOID data) {
	TDATA* ptd = (TDATA*)data;
	TCHAR buf[256];
	DWORD n, i;
	TDATA td;
	BOOL ret;
	// FOra do cliclo 
	OVERLAPPED ov;
	HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
	do {
		_tprintf_s(_T("[ESCRITOR] Frase: "));
		_fgetts(buf, 256, stdin);
		buf[_tcslen(buf) - 1] = '\0';

		WaitForSingleObject(ptd->hMutex, INFINITE);
		td = *ptd;
		ReleaseMutex(ptd->hMutex);

		for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
			if (td.hPipe[i] != NULL) {
				//ANTES Da operacao 
				ZeroMemory(&ov, sizeof(OVERLAPPED));
				ov.hEvent = hEv;
				ret = WriteFile(td.hPipe[i], buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, &ov);
				if (!ret && GetLastError() != ERROR_IO_PENDING) {
					_tprintf_s(_T("[ERRO] Escrever no pipe! (WriteFile)\n"));
					break;
				}
				if (GetLastError() == ERROR_IO_PENDING) {
					WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
					GetOverlappedResult(td.hPipe[i], &ov, &n, FALSE); // obter resultado da operacao
				}
			}
			_tprintf_s(_T("[ESCRITOR] Enviei %d bytes ao leitor...  (i = %d) (WriteFile)\n"), n, i);
		}
	} while (_tcsicmp(buf, _T("fim")));

	for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
		if (td.hPipe[i] != NULL) {
			FlushFileBuffers(td.hPipe[i]);
			_tprintf_s(_T("[ESCRITOR] Desligar o pipe (DisconnectNamedPipe)\n"));
			if (!DisconnectNamedPipe(td.hPipe[i])) {
				_tprintf_s(_T("[ERRO] Desligar o pipe! (DisconnectNamedPipe)"));
				exit(-1);
			}
			CloseHandle(td.hPipe[i]);
		}
	}
	CloseHandle(hEv);
	ExitThread(0);
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



