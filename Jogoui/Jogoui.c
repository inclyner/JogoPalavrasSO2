// Jogoui.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")




DWORD WINAPI recebMsg(LPVOID data) {
    TCHAR buf[256];
    HANDLE hPipe = (HANDLE)data;
    BOOL ret;
    DWORD n;


    //FORA CILCO...
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

    do {

        //ANTEs da operaçao 
        ZeroMemory(&ov, sizeof(OVERLAPPED));
        ov.hEvent = hEv;
        //leitor assincrona com o &ov
        ret = ReadFile(hPipe, buf, sizeof(buf), &n, &ov);
        if (!ret && GetLastError() != ERROR_IO_PENDING) {
            _tprintf_s(_T("[ERROR] %d (%d bytes)... (ReadFile)\n"), ret, n);
            break;
        }
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
            GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
        }
        buf[n / sizeof(TCHAR)] = _T('\0');

        _tprintf_s(_T("[LEITOR] Recebi %d bytes: '%s'... (ReadFile)\n"), n, buf);
    } while (1);
    CloseHandle(hEv);
    ExitThread(0);
}



int _tmain(int argc, TCHAR* argv[]) {
    TCHAR buf[256];
    HANDLE hPipe;
    BOOL ret;
    DWORD n = 0;
    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);

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

        ret = WriteFile(hPipe, argv[1], (DWORD)_tcslen(argv[1]) * sizeof(TCHAR), &n, &ov); // muder ov
        if (!ret && GetLastError() != ERROR_IO_PENDING) { // || n != _tcslen(buf) * sizeof(TCHAR)
            _tprintf_s(_T("[Leitor] %s (%d bytes)... (WriteFile)\n"), buf, n);
            exit(-1);
        }
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
            GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
        }

        _tprintf_s(_T("[LEITOR] Enviei %d bytes: '%s'... (WriteFile)\n"), n, argv[1]);



        // TIRa uma mensagem de cada vez 
        DWORD modo = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(hPipe, &modo, NULL, NULL);

        _tprintf_s(_T("[LEITOR] Liguei-me...\n"));

        //CRIAR THREAD MSG

        HANDLE hThread = CreateThread(NULL, 0, recebMsg, (LPVOID)hPipe, 0, NULL);
        //FORA CILCO...
        
        while (1) {
            //scanf
            // 
            _tprintf_s(_T("[Leitor] Pedido: "));
            _fgetts(buf, 256, stdin);
            buf[_tcslen(buf) - 1] = _T('\0');
            
            //writefile
            // ANTES da operacao
            ZeroMemory(&ov, sizeof(OVERLAPPED));
            ov.hEvent = hEv;
            _tprintf_s(_T("[Leitor] buf %s  bytes %d \n"), buf, n);

            ret = WriteFile(hPipe, buf, (DWORD)_tcslen(buf) * sizeof(TCHAR), &n, &ov); // muder ov
            if (!ret && GetLastError() != ERROR_IO_PENDING) { // || n != _tcslen(buf) * sizeof(TCHAR)
                _tprintf_s(_T("[Leitor] %s (%d bytes)... (WriteFile)\n"), buf, n);
                break;
            }
            if (GetLastError() == ERROR_IO_PENDING) {
                WaitForSingleObject(hEv, INFINITE); // esperar pelo fim da operacao
                GetOverlappedResult(hPipe, &ov, &n, FALSE); // obter resultado da operacao
            }

            _tprintf_s(_T("[LEITOR] Enviei %d bytes: '%s'... (WriteFile)\n"), n, buf);


        }
        CloseHandle(hEv);
        CloseHandle(hPipe);
        CloseHandle(hThread);
        // close thread
        return 0;

    /*
    HANDLE hPipe;

    int IsLeaving = 0; //serve para o quit
	
	//TODO verificar a existencia do named pipe, se não existir fechar programa
    /*
    do {

        hPipe = CreateFile(PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,  // Acesso de Leitura e Escrita (read/write)
            0,                             // Sem “sharing”
            NULL,                          // Atributos de Segurança
            OPEN_EXISTING,                 // O Pipe já tem de estar criado
            0,                             // Atributos e Flags
            NULL);                         // Ficheiro Template

        if (hPipe != INVALID_HANDLE_VALUE) { //quando existe pipe saimos do loop
            break;
        }

       
		_tprintf(_T("A aguardar o servidor...\n"));
        Sleep(1000);
    } while (1);
    */
    /*
    //TODO verificar se o username está disponivel, se não estiver encerra o programa
	TCHAR username[50];
    _tcscpy(username, argv[1]);
    
    //escrever no pipe e esperar resposta

    */


    /*
    system("cls"); //! apenas funciona para windows
    _tprintf(_T("====================================\n\n"));
    _tprintf(_T("Bem Vindo ao Jogo das palavras!"));
    _tprintf(_T("\n\n====================================\n\n"));

    TCHAR command[250];
    TCHAR* cmd[102];

   

    while (IsLeaving == 0) {
        fflush(stdout);
        _tprintf(_T("\nComando: "));
        _fgetts(command, sizeof(command) / sizeof(command[0]), stdin);
        _tprintf(_T("\n"));

		// Remove o newline character 
        size_t len = _tcslen(command);
        if (len > 0 && command[len - 1] == _T('\n')) {
            command[len - 1] = _T('\0');
        }
        //TODO resolver problema de extra caracter no inicio do input do comando
        _tprintf(_T("comando: %s \n", command));

        // escrever no pipe o comando




		
   

    
    }
    */

}
