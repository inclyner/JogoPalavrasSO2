// Jogoui.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")
#define TAM 256

typedef enum {
    ERRO = -1,
    PALAVRA,
    COMANDO,
    USERNAME,
    PONTOS,
    SAIR
}MENSAGEM_TYPE;

typedef struct {
    TCHAR comando[TAM];
    MENSAGEM_TYPE tipo;
} MENSAGEM;

typedef struct {
    HANDLE hPipe;
    BOOL* running;
} THREAD_DATA;


DWORD WINAPI recebMsg(LPVOID data) {
    THREAD_DATA* tData = (THREAD_DATA*)data;
    HANDLE hPipe = tData->hPipe;
    BOOL* running = tData->running;
    MENSAGEM msg = {NULL,NULL};
    BOOL ret, isGameOn = TRUE;
    DWORD n, id = -1;
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

    do {
        ZeroMemory(&ov, sizeof(OVERLAPPED));
        ov.hEvent = hEv;
        ret = ReadFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov);

        if (!ret && GetLastError() != ERROR_IO_PENDING) {
            _tprintf_s(_T("[ERROR]  %d (%d bytes)... (ReadFile)\n"), ret, n);
            break;
        }

        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(hEv, INFINITE);
            GetOverlappedResult(hPipe, &ov, &n, FALSE); 
        }

        switch (msg.tipo){
        case USERNAME:
            _stscanf_s(msg.comando, _T("%d"), &id);
            if (id >= 0) {
                _tprintf_s(_T("Seja bem vindo ao Jogo\n"));
            }
            else {
                _tprintf_s(_T("[ERROR] User invalido"));
                isGameOn = FALSE;
				running = FALSE;

            }
            break;
         
        default:
            _tprintf_s(_T("[ARBITRO] %s\n"),msg.comando);
            break;
        }

        ResetEvent(hEv);
       
    } while (_tcsicmp(msg.comando, _T(":sair")) && _tcsicmp(msg.comando, _T("-1")));
    _tprintf(_T("Thread recebeMsg a TERMINAR.\n"));
    _tprintf(_T("[ARBITRO] Foste excluído. A terminar...\n"));
	running = FALSE;
    // todo melhorar mensagem de saida
    CloseHandle(hEv);
    ExitThread(0);
}

MENSAGEM consola_jogoui(MENSAGEM msg) {
    TCHAR* comando_token;
    TCHAR* token = _tcstok_s(msg.comando, _T(" ,\n"), &comando_token);
    MENSAGEM resposta;
    resposta.tipo = ERRO;
     _tcscpy_s(resposta.comando, TAM, msg.comando);

    if (token == NULL) {
        _tprintf(_T("Comando inválido.\n"));
        return resposta;
    }
    if (_tcscmp(token, _T(":sair")) == 0) {
        resposta.tipo = COMANDO;
    }
    else if (_tcscmp(token, _T(":pont")) == 0) {
        resposta.tipo = COMANDO;
    }
    else if (_tcscmp(token, _T(":jogs")) == 0) {
        resposta.tipo = COMANDO;
    }
    else {
        token = _tcstok_s(comando_token, _T(" ,\n"), &comando_token);
        if (token == NULL) {
            resposta.tipo = PALAVRA;
        }
        else {
            _tprintf(_T("Demasiados argumentos \n"), token);
        }
    }

    return resposta;
}




int _tmain(int argc, TCHAR* argv[]) {
    BOOL running = TRUE;
    HANDLE hPipe;
    BOOL ret;
    DWORD n = 0;
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
    MENSAGEM msg, resposta;
    system("cls");//limpa o ecra
    _tprintf(_T("===========================================\n"));
    _tprintf(_T("        JOGO DE PALAVRAS - SO2 (ISEC)      \n"));
    _tprintf(_T("===========================================\n\n"));

    #ifdef UNICODE
	    _setmode(_fileno(stdin), _O_WTEXT);
	    _setmode(_fileno(stdout), _O_WTEXT);
	    _setmode(_fileno(stderr), _O_WTEXT);

    #endif
        
  

    if (argc != 2) {
        _tprintf(_T("[ERROR] Syntax: jogoui [username] \n"));
        exit(-1);
    }

    if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(_T("[ERROR] Arbitro.exe não está a correr\n"));
        exit(-1);
    }
    _tprintf_s(_T("[LEITOR] Ligação ao pipe do escritor... (CreateFile)\n"));
    hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, NULL);
    if (hPipe == NULL) {
        _tprintf_s(_T("[ERROR] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }

    ZeroMemory(&ov, sizeof(OVERLAPPED));
    ov.hEvent = hEv;

    _tcscpy_s(msg.comando, TAM, argv[1]);
    msg.tipo = USERNAME;

    ret = WriteFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov); 
    if (!ret && GetLastError() != ERROR_IO_PENDING) { 
        _tprintf_s(_T("[Leitor] %s (%d bytes)... (WriteFile)\n"), msg.comando, n);
        exit(-1);
    }
    if (GetLastError() == ERROR_IO_PENDING) {
        WaitForSingleObject(hEv, INFINITE); 
        GetOverlappedResult(hPipe, &ov, &n, FALSE); 
    }

    // TIRa uma mensagem de cada vez 
    DWORD modo = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &modo, NULL, NULL);

    //CRIAR THREAD MSG
    THREAD_DATA* tData = malloc(sizeof(THREAD_DATA));
    tData->hPipe = hPipe;
    tData->running = &running;
    HANDLE hThread = CreateThread(NULL, 0, recebMsg, (LPVOID)tData, 0, NULL);


    if (hThread == NULL) {
        _tprintf_s(_T("Erro ao criar thread: %d\n"), GetLastError());
        return 1;
    }
        
    while (running) {
        _tprintf_s(_T("[Leitor] Pedido: "));
        if (_fgetts(msg.comando, TAM, stdin) == NULL) break;

        if (_tcslen(msg.comando) > 0)
            msg.comando[_tcslen(msg.comando) - 1] = _T('\0');

        resposta = consola_jogoui(msg);

        if (resposta.tipo > ERRO) {
            ZeroMemory(&ov, sizeof(OVERLAPPED));
            ov.hEvent = hEv;
            _tprintf_s(_T("[Leitor] buf %s  bytes %d \n"), resposta.comando, n);

            ret = WriteFile(hPipe, &resposta, sizeof(MENSAGEM), &n, &ov);
            if (!ret && GetLastError() != ERROR_IO_PENDING) {
                _tprintf_s(_T("[Leitor] %s (%d bytes)... (WriteFile)\n"), resposta.comando, n);
                break;
            }
            if (GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(hEv, INFINITE);
                GetOverlappedResult(hPipe, &ov, &n, FALSE);
            }

            _tprintf_s(_T("[LEITOR] Enviei %d bytes: '%s'... (WriteFile)\n"), n, resposta.comando);

            if (_tcsicmp(resposta.comando, _T(":sair")) == 0)
                running = FALSE;
        }
    }

    WaitForSingleObject(hThread, INFINITE);
    _tprintf(_T("Thread TERMINOU.\n"));
    CloseHandle(hEv);
    CloseHandle(hPipe);
    CloseHandle(hThread);
    free(tData);
    // close thread
    return 0;

   

}
