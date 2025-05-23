#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <winreg.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")
#define MEMORY_NAME _T("SHARED_MEMORY")
#define TAM 256
#define TAM_USERNAME 50
#define MAX_CONCURRENT_USERS 20
#define MINIMUM_GAME_PLAYERS 2
#define MAX_LETRAS 12
#define DEFAULT_LETRAS 6
#define DEFAULT_RITMO 3


typedef struct {
	HANDLE hPipe;
	TCHAR name[TAM_USERNAME];
	DWORD points;
} PLAYER;

typedef struct {
	TCHAR name[TAM_USERNAME];
	DWORD points;
}PLAYER_MP;

typedef struct {
	DWORD num_letras;
	TCHAR letras[MAX_LETRAS];
	PLAYER_MP players[MAX_CONCURRENT_USERS];
	TCHAR ultima_palavra[MAX_LETRAS];
}MEMORIA_PARTILHADA;


typedef struct {
	PLAYER players[MAX_CONCURRENT_USERS];
	HANDLE hMutex;
	BOOL continua, isGameOn;
	DWORD n_users;
	DWORD next_id;
	DWORD ritmo;
	DWORD max_letras;
	DWORD id_letra;
	HANDLE hThreadLetras;
	MEMORIA_PARTILHADA *memoria_partilhada;
	DWORD id_lider_atual;
}TDATA;







typedef enum {
	ERRO = -1,
	PALAVRA,
	COMANDO,
	USERNAME
}MENSAGEM_TYPE;

typedef struct {
	TCHAR comando[TAM];
	MENSAGEM_TYPE tipo;
} MENSAGEM;
