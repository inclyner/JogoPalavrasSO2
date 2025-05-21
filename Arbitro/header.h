#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <winreg.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")
#define TAM 256
#define TAM_USERNAME 50
#define MAX_CONCURRENT_USERS 20
#define MINIMUM_GAME_PLAYERS 2



typedef struct {
	HANDLE hPipe[MAX_CONCURRENT_USERS];
	TCHAR users[MAX_CONCURRENT_USERS][TAM_USERNAME];
	DWORD points[MAX_CONCURRENT_USERS];
	HANDLE hMutex;
	BOOL continua, isGameOn;
	DWORD n_users;
}TDATA;

typedef enum {
	PALAVRA,
	COMANDO,
	USERNAME
}MENSAGEM_TYPE;

typedef struct {
	TCHAR comando[TAM];
	MENSAGEM_TYPE tipo;
	DWORD user;
} MENSAGEM;