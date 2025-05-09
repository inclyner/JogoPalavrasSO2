// Arbitro.c : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <winreg.h>

#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")
#define TAM 200
#define MAX_CONCURRENT_USERS 20
#define MINIMUM_GAME_PLAYERS 2



// prototipos funções
DWORD WINAPI processArbitroComands(LPVOID param);
TCHAR* getRandomLetter(TCHAR* abecedario, int max_letras);



int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	int max_letras, ritmo;
	getRegistryValues(&max_letras, &ritmo);

	TCHAR* letras_jogo = (TCHAR*)malloc(max_letras * sizeof(TCHAR));

	TCHAR abecedario[26] = {
		_T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'),
		_T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'), _T('O'), _T('P'),
		_T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'), _T('W'), _T('X'),
		_T('Y'), _T('Z')
	};

	if (letras_jogo == NULL) {
		_tprintf(_T("Erro a alocar memória!\n"));
		return 1;
	}
	for (int i = 0; i < max_letras; i++) {
		letras_jogo[i] = _T('_');
	}

	DWORD adminThreadId;
	//lança thread que ouve comandos do admin
	/**/
	HANDLE hThreadArbitro;
	HANDLE hThreadAdmitUsers;
	
	hThreadArbitro = CreateThread(
		NULL,
		0,
		processArbitroComands,
		NULL,
		0,
		&adminThreadId
	);

	if (hThreadArbitro == NULL) {
		_tprintf("Erro ao criar thread(arbitro): %lu\n", GetLastError());
		return 1;
	}
	
	hThreadAdmitUsers = CreateThread(
		NULL,
		0,
		admitUsers,
		NULL,
		0,
		&adminThreadId
	);
	if (hThreadAdmitUsers == NULL) {
		_tprintf("Erro ao criar thread(AdmitUsers): %lu\n", GetLastError());
		return 1;
	}
	

	//TODO main loop do jogo
	//while (currentUsers >= MINIMUM_GAME_PLAYERS)
	//{}
	








	WaitForSingleObject(hThreadArbitro, INFINITE);
	// Libertar memória
	free(letras_jogo);
	CloseHandle(hThreadArbitro);
	CloseHandle(hThreadAdmitUsers);


	return 0;

}



int getRegistryValues(int* maxletras, int* ritmo) {
	HKEY hKey;
	DWORD tipo = REG_DWORD;
	DWORD tam = sizeof(DWORD);
	DWORD valMaxLetras = 6; 
	DWORD valRitmo = 3;     
	
	TCHAR chave[] = _T("Software\\TrabSO2");

	
	LSTATUS res = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		chave,
		0,
		KEY_ALL_ACCESS,
		&hKey
	);

	if (res != ERROR_SUCCESS) {
		_tprintf(_T("Chave não encontrada. A criar...\n"));

		res = RegCreateKeyEx(
			HKEY_CURRENT_USER,
			_T("Software\\TrabSO2"),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey,
			NULL
		);

		if (res != ERROR_SUCCESS) {
			_tprintf(_T("Erro ao criar a chave!\n"));
			return 1;
		}

		//RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
		//RegSetValueEx(hKey, _T("RITMO"), 0, REG_DWORD, (const BYTE*)&valRitmo, sizeof(DWORD));
	}
	
	tam = sizeof(DWORD);
	res = RegQueryValueEx(hKey, _T("MAXLETRAS"), NULL, &tipo, (LPBYTE)&valMaxLetras, &tam);
	if (res != ERROR_SUCCESS) {
		_tprintf(_T("MAXLETRAS não existe, a criar com valor 6.\n"));
		valMaxLetras = 6;
		RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
	}
	else
	{
		_tprintf(_T("MAXLETRAS lido! => %d\n"), valMaxLetras);
		if (valMaxLetras > 12) {
			_tprintf(_T("MAXLETRAS superior a 12. A corrigir para 12.\n"));
			valMaxLetras = 12;
			RegSetValueEx(hKey, _T("MAXLETRAS"), 0, REG_DWORD, (const BYTE*)&valMaxLetras, sizeof(DWORD));
		}
	}
	res = RegQueryValueEx(hKey, _T("RITMO"), NULL, &tipo, (LPBYTE)&valRitmo, &tam);
	if (res != ERROR_SUCCESS) {
		_tprintf(_T("RITMO não existe, a criar com valor 3.\n"));
		valRitmo = 3;
		RegSetValueEx(hKey, _T("RITMO"), 0, REG_DWORD, (const BYTE*)&valRitmo, sizeof(DWORD));
	}
	else
	{
		_tprintf(_T("RITMO lido! => %d\n"), valRitmo);
	}


	



	RegCloseKey(hKey);
	*maxletras = valMaxLetras;
	*ritmo = valRitmo;

	return 0;
}


TCHAR* getRandomLetter(TCHAR* abecedario, int max_letras) {
	int randomIndex = rand() % 26;
	TCHAR* letra = (TCHAR*)malloc(sizeof(TCHAR));
	if (letra == NULL) {
		_tprintf(_T("Erro a alocar memória!\n"));
		return NULL;
	}
	letra = abecedario[randomIndex];
	return letra;
}


DWORD WINAPI processArbitroComands(LPVOID param) {
	_tprintf(_T("Thread processArbitroComands a correr!\n"));
	int isLeaving = 0;

	while (isLeaving) {


	}

	return 0;
}



DWORD WINAPI admitUsers(LPVOID param) {
	_tprintf(_T("Thread admitUsers a correr!\n"));
	int nCurrentUsers = 0;
	while (nCurrentUsers <= MAX_CONCURRENT_USERS)
	{
		//TODO lógica para admitir utilizadores
		//ler pipe PIPE_NAME até haver algo


	}


	return 0;
}


DWORD processUserComands(LPVOID param) {
	_tprintf(_T("Thread processUserComands a correr!\n"));
	int isLeaving = 0;
	TCHAR command[250];

	while (isLeaving) {
	
		//TODO ler comando do pipe

		// comando sair
		if (_tcsstr(command, _T("sair")) != NULL) {
			isLeaving = 1;
			_tprintf(_T("A sair do jogo...\n"));
			break;
		}

		// comando pont => obter pontuação do próprio jogador
		if (_tcsstr(command, _T("pont")) != NULL) {
			//TODO lógica para obter a própria pontuação
		}

		// comando jogs => obter lista de jogadores
		if (_tcsstr(command, _T("jogs")) != NULL) {
			//TODO lógica para obter a lista dos jogadores
		}
	}
}
