// Bot.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>


#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")


//protótipos funções
HANDLE esperarPipeServidor(int maxTentativas, int intervaloMs);


int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif

    //verifica a existencia do pipe (verifica se o arbitro está a correr)
    HANDLE hPipe = esperarPipeServidor(10, 1000); // tenta 10 vezes, com 1s entre cada

    if (hPipe == NULL) {
        _tprintf(_T("[ERRO] Não foi possível ligar ao árbitro.\n"));
        exit(EXIT_FAILURE);
    }

}




HANDLE esperarPipeServidor(int maxTentativas, int intervaloMs) {
    HANDLE hPipe;
    int tentativas = 0;

    do {
        hPipe = CreateFile(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hPipe != INVALID_HANDLE_VALUE) {
            return hPipe; // Pipe encontrado
        }

        _tprintf(_T("A aguardar o árbitro... (%d/%d)\n"), tentativas + 1, maxTentativas);
        Sleep(intervaloMs);
        tentativas++;

    } while (tentativas < maxTentativas);

    return NULL; // Pipe não encontrado
}