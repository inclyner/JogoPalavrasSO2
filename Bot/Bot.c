// Bot.c : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")
#define MEMORY_NAME _T("SHARED_MEMORY")
#define TAM 256
#define TAM_USERNAME 50
#define MAX_CONCURRENT_USERS 20
#define MAX_LETRAS 12

typedef enum {
    ERRO = -1,
    PALAVRA,
    COMANDO,
    USERNAME,
    PONTOS,
    SAIR
} MENSAGEM_TYPE;

typedef struct {
    TCHAR comando[TAM];
    MENSAGEM_TYPE tipo;
} MENSAGEM;

typedef struct {
    TCHAR name[TAM_USERNAME];
    float points;
} PLAYER_MP;

typedef struct {
    DWORD num_letras;
    TCHAR letras[MAX_LETRAS];
    PLAYER_MP players[MAX_CONCURRENT_USERS];
    TCHAR ultima_palavra[MAX_LETRAS];
} MEMORIA_PARTILHADA;

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

    return NULL;
}

BOOL podeJogarPalavra(const TCHAR* palavra, MEMORIA_PARTILHADA* memoria) {
    TCHAR letras[MAX_LETRAS];
    BOOL usadas[MAX_LETRAS] = { FALSE };
    _tcsncpy_s(letras, MAX_LETRAS, memoria->letras, memoria->num_letras);
    int len = _tcslen(palavra);
    for (int i = 0; i < len; i++) {
        TCHAR c = _totupper(palavra[i]);
        BOOL encontrada = FALSE;
        for (int j = 0; j < memoria->num_letras; j++) {
            if (!usadas[j] && letras[j] == c) {
                usadas[j] = TRUE;
                encontrada = TRUE;
                break;
            }
        }
        if (!encontrada) return FALSE;
    }
    return TRUE;
}

DWORD WINAPI escutaArbitro(LPVOID data) {
    HANDLE hPipe = (HANDLE)data;
    MENSAGEM msg;
    DWORD n;
    while (ReadFile(hPipe, &msg, sizeof(MENSAGEM), &n, NULL)) {
        if (_tcscmp(msg.comando, _T(":sair")) == 0) {
            _tprintf(_T("[BOT] Recebi :sair. A sair...\n"));
            ExitProcess(0);
        }
    }
    return 0;
}

int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif

    const TCHAR* DICIONARIO[] = {
        _T("GATO"), _T("CÃO"), _T("RATO"), _T("SAPO"), _T("LOBO"),
        _T("LIVRO"), _T("BOLA"), _T("MESA"), _T("PORTA"), _T("CASA"),
        _T("CARRO"), _T("FACA"), _T("PATO"), _T("PEIXE"), _T("CHAVE"),
        _T("BANCO"), _T("CARTA"), _T("FAROL"), _T("LATA"), _T("FITA"),
        _T("VIDRO"), _T("TECLA"), _T("JANELA"), _T("CADEIRA"), _T("CAMA"),
        _T("TINHA"), _T("BICHO"), _T("MANTO"), _T("FUMO"), _T("TROCO")
    };
    const int NUM_PALAVRAS = sizeof(DICIONARIO) / sizeof(DICIONARIO[0]);

    HANDLE hPipe = esperarPipeServidor(10, 1000);
    if (hPipe == NULL) {
        _tprintf(_T("[ERRO] Não foi possível ligar ao árbitro.\n"));
        exit(EXIT_FAILURE);
    }

    // Envia username
    MENSAGEM msg;
    DWORD n;
    msg.tipo = USERNAME;
    _tcscpy_s(msg.comando, TAM, argc > 1 ? argv[1] : _T("BOT1"));
    WriteFile(hPipe, &msg, sizeof(MENSAGEM), &n, NULL);

    // Thread para escutar encerramento
    CreateThread(NULL, 0, escutaArbitro, (LPVOID)hPipe, 0, NULL);

    HANDLE hMemoPart = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MEMORY_NAME);
    if (hMemoPart == NULL) {
        _tprintf(_T("Erro ao ligar à memória partilhada (%d).\n"), GetLastError());
        return 1;
    }
    MEMORIA_PARTILHADA* memoria = (MEMORIA_PARTILHADA*)MapViewOfFile(hMemoPart, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MEMORIA_PARTILHADA));

    srand((unsigned int)time(NULL));
    while (TRUE) {
        Sleep((rand() % 26 + 5) * 1000); // espera entre 5 e 30 segundos

        for (int i = 0; i < NUM_PALAVRAS; i++) {
            if (podeJogarPalavra(DICIONARIO[i], memoria)) {
                msg.tipo = PALAVRA;
                _tcscpy_s(msg.comando, TAM, DICIONARIO[i]);
                WriteFile(hPipe, &msg, sizeof(MENSAGEM), &n, NULL);
                _tprintf(_T("[BOT] Joguei: %s\n"), DICIONARIO[i]);
                break;
            }
        }
    }

    return 0;
}
