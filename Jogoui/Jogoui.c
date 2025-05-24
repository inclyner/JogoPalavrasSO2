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


DWORD WINAPI recebMsg(LPVOID data) {
    MENSAGEM msg;
    HANDLE hPipe = (HANDLE)data;
    BOOL ret, isGameOn = TRUE;
    DWORD n, id = -1;


    //FORA CILCO...
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

    do {

        //ANTEs da operaçao 
        ZeroMemory(&ov, sizeof(OVERLAPPED));
        ov.hEvent = hEv;
        //leitor assincrona com o &ov
        ret = ReadFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov);
        if (!ret && GetLastError() != ERROR_IO_PENDING) {
            _tprintf_s(_T("[ERROR] %d (%d bytes)... (ReadFile)\n"), ret, n);
            break;
        }
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
            GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
        }

        _tprintf_s(_T("[recebMsg] Recebi %s (%d bytes)\n"), msg.comando, n);
        
        switch (msg.tipo){
        case USERNAME:
            _stscanf_s(msg.comando, _T("%d"), &id);
            if (id >= 0) {
                _tprintf_s(_T("Username valido (%d)\n"),id);
            }
            else {
                _tprintf_s(_T("[ERROR] User invalido"));
                isGameOn = FALSE;
            }
            break;
        default:
            break;
        }

        ResetEvent(hEv);
        
    } while (_tcsicmp(msg.comando, _T(":sair")) || id < 0 || isGameOn);
    _tprintf(_T("Thread recebeMsg a TERMINAR.\n"));
    _tprintf(_T("[ARBITRO] Foste excluído. A terminar...\n"));
    
    CloseHandle(hEv);
    ExitThread(0);
    ExitProcess(0); // termina tudo imediatamente
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
    HANDLE hPipe;
    BOOL ret;
    DWORD n = 0;
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
    MENSAGEM msg, resposta;

    #ifdef UNICODE
	    _setmode(_fileno(stdin), _O_WTEXT);
	    _setmode(_fileno(stdout), _O_WTEXT);
	    _setmode(_fileno(stderr), _O_WTEXT);

    #endif
        
  

    if (argc != 2) {
        _tprintf(_T("[ERROR] Syntax: jogoui [username] \n"));
        exit(-1);
    }

    _tprintf_s(_T("[ARBITRO] Esperar pelo pipe '%s' (WaitNamedPipe)\n"),
        PIPE_NAME);
    if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(_T("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf_s(_T("[LEITOR] Ligação ao pipe do escritor... (CreateFile)\n"));
    hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED, NULL);
    if (hPipe == NULL) {
        _tprintf_s(_T("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }

    // envia o user name para saber se é valido
    ZeroMemory(&ov, sizeof(OVERLAPPED));
    ov.hEvent = hEv;

    _tcscpy_s(msg.comando, TAM, argv[1]);
    msg.tipo = USERNAME;
       


    ret = WriteFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov); // muder ov
    if (!ret && GetLastError() != ERROR_IO_PENDING) { // || n != _tcslen(buf) * sizeof(TCHAR)
        _tprintf_s(_T("[Leitor] %s (%d bytes)... (WriteFile)\n"), msg.comando, n);
        exit(-1);
    }
    if (GetLastError() == ERROR_IO_PENDING) {
        WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
        GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
    }

    _tprintf_s(_T("[JOGOUI] Tentar login (bytes %d) (%s)\n"), n, argv[1]);



    // TIRa uma mensagem de cada vez 
    DWORD modo = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &modo, NULL, NULL);

    _tprintf_s(_T("[LEITOR] Liguei-me...\n"));

    //CRIAR THREAD MSG

    HANDLE hThread = CreateThread(NULL, 0, recebMsg, (LPVOID)hPipe, 0, NULL);
    //FORA CILCO...

    if (hThread == NULL) {
        _tprintf_s(_T("Erro ao criar thread: %d\n"), GetLastError());
        return 1;
    }
        
    do {
        //scanf
        // 
        _tprintf_s(_T("[Leitor] Pedido: "));
        _fgetts(msg.comando, TAM, stdin);
        if(_tcslen(msg.comando) > 0)
            msg.comando[_tcslen(msg.comando) - 1] = _T('\0');

           
        resposta = consola_jogoui(msg);
        //writefile
        // ANTES da operacao
        if (resposta.tipo > ERRO) {
            ZeroMemory(&ov, sizeof(OVERLAPPED));
            ov.hEvent = hEv;
            _tprintf_s(_T("[Leitor] buf %s  bytes %d \n"), resposta.comando, n);

            ret = WriteFile(hPipe, &resposta, sizeof(MENSAGEM), &n, &ov); // muder ov
            if (!ret && GetLastError() != ERROR_IO_PENDING) { // || n != _tcslen(buf) * sizeof(TCHAR)
                _tprintf_s(_T("[Leitor] %s (%d bytes)... (WriteFile)\n"), resposta.comando, n);
                break;
            }
            if (GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
                GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
                    
            }

            _tprintf_s(_T("[LEITOR] Enviei %d bytes: '%s'... (WriteFile)\n"), n, resposta.comando);

        }
    } while (_tcsicmp(resposta.comando, _T(":sair")));

    WaitForSingleObject(hThread, INFINITE);
    _tprintf(_T("Thread TERMINOU.\n"));
    CloseHandle(hEv);
    CloseHandle(hPipe);
    CloseHandle(hThread);
    // close thread
    return 0;

   

}
