
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <winreg.h>

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
	*letra = abecedario[randomIndex];
	return letra;
}


