

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
        _tprintf(_T("[DEBUG] Tentativa %d de ligar ao pipe '%s'\n"), tentativas + 1, PIPE_NAME);
        hPipe = CreateFile(
            PIPE_NAME,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hPipe != INVALID_HANDLE_VALUE) {
            _tprintf(_T("[DEBUG] Conectado ao pipe com sucesso.\n"));
            return hPipe; 
        }

        _tprintf(_T("[DEBUG] Pipe não disponível. A aguardar o árbitro... (%d/%d)\n"), tentativas + 1, maxTentativas);
        Sleep(intervaloMs);
        tentativas++;

    } while (tentativas < maxTentativas);

    _tprintf(_T("[DEBUG] Excedeu número máximo de tentativas para ligar ao pipe.\n"));
    return NULL;
}

BOOL podeJogarPalavra(const TCHAR* palavra, MEMORIA_PARTILHADA* memoria) {
    TCHAR letras[MAX_LETRAS];
    BOOL usadas[MAX_LETRAS] = { FALSE };
    _tcsncpy_s(letras, MAX_LETRAS, memoria->letras, min(memoria->num_letras, MAX_LETRAS - 1));
    letras[MAX_LETRAS - 1] = '\0';
    int len = _tcslen(palavra);

    _tprintf(_T("[DEBUG] Verificando se posso jogar a palavra '%s'...\n"), palavra);

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
        if (!encontrada) {
            _tprintf(_T("[DEBUG] A letra '%c' não está disponível para a palavra '%s'.\n"), c, palavra);
            return FALSE;
        }
    }
    _tprintf(_T("[DEBUG] Palavra '%s' é válida para jogar.\n"), palavra);
    return TRUE;
}

DWORD WINAPI escutaArbitro(LPVOID data) {
    HANDLE hPipe = (HANDLE)data;
    MENSAGEM msg;
    DWORD n;
    BOOL ret;

    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL); 

    _tprintf(_T("[DEBUG] Thread escutaArbitro iniciada.\n"));

    while (TRUE) {
        ZeroMemory(&ov, sizeof(OVERLAPPED));
        ov.hEvent = hEv;
        _tprintf(_T("[DEBUG] A aguardar mensagem do árbitro...\n"));
        ret = ReadFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov);

        if (!ret && GetLastError() != ERROR_IO_PENDING) {
            _tprintf_s(_T("[ERROR] ReadFile falhou: %d (%d bytes)...\n"), GetLastError(), n);
            break;
        }

        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(hEv, INFINITE);
            GetOverlappedResult(hPipe, &ov, &n, FALSE);
        }

        _tprintf(_T("[DEBUG] Mensagem recebida do árbitro: tipo=%d comando='%s'\n"), msg.tipo, msg.comando);

        if (_tcscmp(msg.comando, _T(":sair")) == 0) {
            _tprintf(_T("[BOT] Recebi :sair. A sair...\n"));
            ExitProcess(0);
        }
    }

    _tprintf(_T("[DEBUG] Thread escutaArbitro terminou.\n"));
    ExitProcess(0);
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
    _T("TINHA"), _T("BICHO"), _T("MANTO"), _T("FUMO"), _T("TROCO"),_T("A"),_T("B"),_T("C"),_T("D"),_T("E"),_T("F"),_T("G"),_T("H"),_T("I"),_T("J"),_T("K"),_T("L"),_T("M"),_T("N"),_T("O"),_T("P"),_T("Q"),_T("R"),_T("S"),_T("T"),_T("U"),_T("V"),_T("W"),_T("X"),_T("Y"),_T("Z")
    };
    const int NUM_PALAVRAS = sizeof(DICIONARIO) / sizeof(DICIONARIO[0]);

    _tprintf(_T("[DEBUG] Iniciando bot...\n"));

    HANDLE hPipe = esperarPipeServidor(10, 1000);
    if (hPipe == NULL) {
        _tprintf(_T("[ERRO] Não foi possível ligar ao árbitro.\n"));
        exit(EXIT_FAILURE);
    }

    _tprintf(_T("[DEBUG] Pipe do árbitro aberto com handle %p\n"), hPipe);

    OVERLAPPED ov;
    HANDLE hEv = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEv == NULL) {
        _tprintf(_T("[ERRO] Falha ao criar evento para overlapped (%d)\n"), GetLastError());
        return 1;
    }

    // Envia username
    MENSAGEM msg;
    DWORD n;
    BOOL ret;
    msg.tipo = USERNAME;
    _tcscpy_s(msg.comando, TAM, argc > 1 ? argv[1] : _T("BOT1"));

    _tprintf(_T("[DEBUG] Enviando USERNAME: %s\n"), msg.comando);

    ZeroMemory(&ov, sizeof(OVERLAPPED));
    ov.hEvent = hEv;
    ret = WriteFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov);

    if (!ret && GetLastError() != ERROR_IO_PENDING) {
        _tprintf_s(_T("[ERROR] WriteFile falhou: %d (%d bytes)...\n"), GetLastError(), n);
        return 0;
    }

    if (GetLastError() == ERROR_IO_PENDING) {
        WaitForSingleObject(hEv, INFINITE);
        GetOverlappedResult(hPipe, &ov, &n, FALSE);
    }

    _tprintf(_T("[DEBUG] USERNAME enviado com sucesso (%d bytes).\n"), n);

    // Thread para escutar encerramento
    HANDLE hThread = CreateThread(NULL, 0, escutaArbitro, (LPVOID)hPipe, 0, NULL);

    if (hThread == NULL) {
        _tprintf(_T("[ERRO] Erro ao criar a thread escutaArbitro (%d).\n"), GetLastError());
        return 1;
    }
    _tprintf(_T("[DEBUG] Thread escutaArbitro criada com handle %p\n"), hThread);

    HANDLE hMemoPart = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, MEMORY_NAME);
    if (hMemoPart == NULL) {
        _tprintf(_T("[ERRO] Erro ao ligar à memória partilhada (%d).\n"), GetLastError());
        return 1;
    }
    _tprintf(_T("[DEBUG] Memória partilhada aberta com handle %p\n"), hMemoPart);

    MEMORIA_PARTILHADA* memoria = (MEMORIA_PARTILHADA*)MapViewOfFile(hMemoPart, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MEMORIA_PARTILHADA));
    if (memoria == NULL) {
        _tprintf(_T("[ERRO] Erro ao mapear a memória (%d).\n"), GetLastError());
        return 1;
    }
    _tprintf(_T("[DEBUG] Memória partilhada mapeada em %p\n"), memoria);

    srand((unsigned int)time(NULL));
    while (TRUE) {
        int waitTime = (rand() % 26 + 5) * 1000;
        _tprintf(_T("[DEBUG] A dormir %d ms antes de tentar jogar...\n"), waitTime);
        Sleep(waitTime);

        for (int i = 0; i < NUM_PALAVRAS; i++) {
            if (podeJogarPalavra(DICIONARIO[i], memoria)) {
                msg.tipo = PALAVRA;
                _tcscpy_s(msg.comando, TAM, DICIONARIO[i]);

                _tprintf(_T("[DEBUG] A tentar enviar palavra: %s\n"), msg.comando);

                ZeroMemory(&ov, sizeof(OVERLAPPED));
                ov.hEvent = hEv;
                ret = WriteFile(hPipe, &msg, sizeof(MENSAGEM), &n, &ov);

                if (!ret && GetLastError() != ERROR_IO_PENDING) {
                    _tprintf_s(_T("[ERROR] WriteFile falhou: %d (%d bytes)...\n"), GetLastError(), n);
                    return 0;
                }

                if (GetLastError() == ERROR_IO_PENDING) {
                    WaitForSingleObject(hEv, INFINITE);
                    GetOverlappedResult(hPipe, &ov, &n, FALSE);
                }

                _tprintf(_T("[BOT] Joguei: %s\n"), DICIONARIO[i]);
                break;
            }
        }
    }

    CloseHandle(hPipe);
    UnmapViewOfFile(memoria);
    CloseHandle(hMemoPart);

    return 0;
}
