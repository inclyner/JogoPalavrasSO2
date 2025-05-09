// Jogoui.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")


int _tmain(int argc, TCHAR* argv[]) {
    #ifdef UNICODE
	    _setmode(_fileno(stdin), _O_WTEXT);
	    _setmode(_fileno(stdout), _O_WTEXT);
	    _setmode(_fileno(stderr), _O_WTEXT);
    #endif
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
    
    //TODO verificar se o username está disponivel, se não estiver encerra o programa
	TCHAR username[50];
    _tcscpy(username, argv[1]);
    /*
    //escrever no pipe e esperar resposta

    */



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


}
