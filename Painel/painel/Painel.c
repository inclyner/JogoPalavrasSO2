#include <windows.h>
#include <tchar.h>

#include "resource.h"

#define TAM 100
#define PIPE_NAME _T("\\\\.\\pipe\\JogoPalavrasSO2")

typedef struct {
    DWORD max_jogadores;
    BOOL continua;
    HANDLE hMutex;
    HINSTANCE hInst;
} TDATA;

LRESULT CALLBACK trataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base");

LRESULT CALLBACK trataDlg(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) {
    TDATA* ptd;
    HANDLE hWnd;
    TCHAR str[TAM];

    hWnd = GetParent(hDlg);

    ptd = (TDATA*)GetWindowLongPtr(hWnd, 0);

    switch (messg) {
    case WM_INITDIALOG:
        WaitForSingleObject(ptd->hMutex, INFINITE);
        _stprintf_s(str, TAM, _T("%d"), ptd->max_jogadores);
        ReleaseMutex(ptd->hMutex);
        SetDlgItemText(hDlg, IDC_JOGADORES, str);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            GetDlgItemText(hDlg, IDC_EDIT1, str, TAM);
            WaitForSingleObject(ptd->hMutex, INFINITE);
            _stscanf_s(str, _T("%d"), &ptd->max_jogadores);
            ReleaseMutex(ptd->hMutex);
            EndDialog(hDlg, 0);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        return TRUE;
    }
    return FALSE;
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

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nCmdShow) {
    HWND hWnd;
    MSG lpMsg;
    WNDCLASSEX wcApp;

    wcApp.cbSize = sizeof(WNDCLASSEX);
    wcApp.hInstance = hInst;
    wcApp.lpszClassName = szProgName;
    wcApp.lpfnWndProc = trataEventos;
    wcApp.style = CS_HREDRAW | CS_VREDRAW;
    wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcApp.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
    wcApp.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);;
    wcApp.cbClsExtra = 0;
    wcApp.cbWndExtra = sizeof(TDATA *);
    wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

    //verifica a existencia do pipe (verifica se o arbitro está a correr)
    HANDLE hPipe = esperarPipeServidor(10, 1000); // tenta 10 vezes, com 1s entre cada

    if (hPipe == NULL) {
        _tprintf(_T("[ERRO] Não foi possível ligar ao árbitro.\n"));
        exit(EXIT_FAILURE);
    }

    if (!RegisterClassEx(&wcApp))
        return(0);

    hWnd = CreateWindow(
        szProgName,
        TEXT("Painel"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (HWND)HWND_DESKTOP,
        (HMENU)NULL,
        (HINSTANCE)hInst,
        0);

    TDATA* ptd = GetWindowLongPtr(hWnd, 0);
    ptd->hInst = hInst;

    ShowWindow(hWnd, nCmdShow);

    UpdateWindow(hWnd);

    while (GetMessage(&lpMsg, NULL, 0, 0) > 0) {
        TranslateMessage(&lpMsg);
        DispatchMessage(&lpMsg);

    }
    return (int)lpMsg.wParam;
}


LRESULT CALLBACK trataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
    TDATA* ptd;
    switch (messg) {
    case WM_CREATE:
        ptd = (TDATA*)malloc(sizeof(TDATA));
        SetWindowLongPtr(hWnd, 0, (LONG_PTR)ptd);
        ptd->hMutex = CreateMutex(NULL, FALSE, NULL);
        ptd->max_jogadores = 5;
        ptd->continua = TRUE;

        break;
    case WM_COMMAND:
        ptd = (TDATA*)GetWindowLongPtr(hWnd, 0);
        if (HIWORD(wParam) == 0) {
            switch (LOWORD(wParam)) {
            case ID_MENU_EDITAR:
                DialogBox(ptd->hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, trataDlg);
                break;
            case ID_MENU_SAIR:
                PostMessage(hWnd, WM_CLOSE, 0, 0);
                break;
            case ID_INFO:
                MessageBox(hWnd,
                    _T("Trabalho realizado por: \n\tMarco Pereira 202013341\n"), 
                    _T("AUTORES:"),
                    MB_OK);
                break;
            default:
                break;
            }
        }
        break;
    case WM_DESTROY:
        ptd = (TDATA*)GetWindowLongPtr(hWnd, 0);
        CloseHandle(ptd->hMutex);
        DestroyWindow(hWnd);
        free(ptd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return 0;
}
