// Painel.c : This file contains the 'main' function. Program execution begins and ends there.
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

}
