#pragma once
#define MAX_CONCURRENT_USERS 20

// prototipos funções
DWORD WINAPI processArbitroComands(LPVOID param);
TCHAR* getRandomLetter(TCHAR* abecedario, int max_letras);
DWORD WINAPI distribui(LPVOID data);
DWORD WINAPI atende_cliente(LPVOID data);
void consola_jogoui(TCHAR comando[]);
void consola_arbitro(TCHAR comando[]);

typedef struct {
	HANDLE hPipe[MAX_CONCURRENT_USERS];
	HANDLE hMutex;
	BOOL continua;
}TDATA;