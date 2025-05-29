#include <windows.h>
#include <tchar.h>

#include "resource.h"

#define TAM 100
#define TAM_USERNAME 50
#define MAX_CONCURRENT_USERS 20
#define MINIMUM_GAME_PLAYERS 2
#define MAX_LETRAS 12
#define MEMORY_NAME _T("SHARED_MEMORY")

typedef struct {
    TCHAR name[TAM_USERNAME];
    float points;
}PLAYER_MP;
typedef struct {
    DWORD num_letras;
    TCHAR letras[MAX_LETRAS];
    PLAYER_MP players[MAX_CONCURRENT_USERS];
    TCHAR ultima_palavra[MAX_LETRAS];
}MEMORIA_PARTILHADA;

typedef struct {
    BOOL ativo; // se o jogo est� a decorrer
    DWORD max_jogadores;
    BOOL continua;
    HANDLE hMutex;
    HANDLE hThread;
    RECT dim;
    HINSTANCE hInst;
    HWND hWnd;
    MEMORIA_PARTILHADA* memoria;
} TDATA;


LRESULT CALLBACK trataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base");

int comparaPontuacoes(const void* a, const void* b) {
    const PLAYER_MP* pa = (const PLAYER_MP*)a;
    const PLAYER_MP* pb = (const PLAYER_MP*)b;

    if (pb->points > pa->points) return 1;
    if (pb->points < pa->points) return -1;
    return 0;
}

DWORD WINAPI pintor(LPVOID data) {
    TDATA* ptd = (TDATA*)data;
    TDATA td;
    MEMORIA_PARTILHADA* memoria;

    WaitForSingleObject(ptd->hMutex, INFINITE);
    
    td = *ptd;
    ReleaseMutex(ptd->hMutex);

    do {
        Sleep(1000);
        InvalidateRect(ptd->hWnd, NULL, TRUE);
    } while (ptd->continua);
}

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
            InvalidateRect(ptd->hWnd, NULL, TRUE);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        return TRUE;
    }
    return FALSE;
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
    wcApp.hbrBackground = CreateSolidBrush(RGB(190, 200, 210));

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
    TDATA td;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT dim;
    POINT pt = { 0 };
    DWORD i,j;
    SIZE textSize;
    DWORD altura, largura;
    MEMORIA_PARTILHADA* memoria;
    HANDLE hMemoPart = NULL;
    TCHAR ch[2];

    switch (messg) {
    case WM_CREATE:
        ptd = (TDATA*)malloc(sizeof(TDATA));
        SetWindowLongPtr(hWnd, 0, (LONG_PTR)ptd);
        GetClientRect(hWnd, &ptd->dim);
        ptd->hWnd = hWnd;
        ptd->hMutex = CreateMutex(NULL, FALSE, NULL);
        if (ptd->hMutex == NULL) {
            MessageBox(NULL, _T("Erro ao criar Mutex"), _T("Erro"), MB_OK);
            return -1;
        }
        ptd->max_jogadores = 5;
        ptd->continua = TRUE;
        ptd->memoria = NULL;
        ptd->hThread = CreateThread(NULL, 0, pintor, (LPVOID)ptd, 0, NULL);
        hMemoPart = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            MEMORY_NAME
        );

        if (hMemoPart == NULL) {
            _tprintf(_T("Erro ao abrir mem�ria partilhada (%d).\n"), GetLastError());
            return 1;
        }

        memoria = (MEMORIA_PARTILHADA* )MapViewOfFile(
            hMemoPart,
            FILE_MAP_ALL_ACCESS,
            0, 0,
            sizeof(MEMORIA_PARTILHADA)
        );

        if (memoria == NULL) {
            _tprintf(_T("Erro ao mapear mem�ria (%d).\n"), GetLastError());
            CloseHandle(hMemoPart);
            return 1;
        }

        WaitForSingleObject(ptd->hMutex, INFINITE);
        ptd->memoria = memoria;
        ReleaseMutex(ptd->hMutex);

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
                    _T("Trabalho realizado por: \n\tMarco Pereira 2020133341\n\tFrancisco Carvalho 2019129635\n"),
                    _T("AUTORES:"),
                    MB_OK);
                break;
            default:
                break;
            }
        }
        break;
    case WM_PAINT:
        ptd = (TDATA*)GetWindowLongPtr(hWnd, 0);
        hdc = BeginPaint(hWnd, &ps);
        GetTextExtentPoint32(hdc, _T("_"), 1, &textSize);
        WaitForSingleObject(ptd->hMutex, INFINITE);
        dim = ptd->dim;
        td = *ptd;
        ReleaseMutex(ptd->hMutex);

		SetBkMode(hdc, TRANSPARENT); // retira os quadrados brancos de tr�s dos textos

        // __________TITULO-------------
        TextOut(hdc, 20, 20, _T("Jogo das Palavras"), sizeof(TCHAR) * 9);
        // __________TITULO-------------
        // 
        // __________LETRAS-------------
        pt.x = (dim.right / 2) - (50 * td.memoria->num_letras / 2) - 30;
        pt.y = 130;
        for (i = 0; i < td.memoria->num_letras; i++) {
            pt.x += 50;
            Rectangle(hdc, pt.x - 15, pt.y - 15, pt.x + 15, pt.y + 15);
            ch[0] = td.memoria->letras[i];
            ch[1] = '\0';  // or 0
            TextOut(hdc, pt.x - (textSize.cx / 2), pt.y - (textSize.cy / 2), ch, 1);
        }
        // __________LETRAS-------------
        // 
        // __________PALAVRA-------------
        pt.x = (dim.right / 2);
        pt.y = 70;
        Rectangle(hdc, pt.x - 80, pt.y - 15, pt.x + 80, pt.y + 15);
        TextOut(hdc, pt.x - (textSize.cx / 2), pt.y - (textSize.cy / 2), td.memoria->ultima_palavra, lstrlen(td.memoria->ultima_palavra));

        // __________PALAVRA-------------

        // __________JOGADORES-------------
        pt.x = (dim.right / 2) - 150;
        pt.y = 200;
        altura = 18 * td.max_jogadores; // 20 -> jogadores 
        largura = 120;
        TextOut(hdc, pt.x - largura, pt.y - 20, _T("Jogadores"), 9);
        Rectangle(hdc, pt.x - largura, pt.y, pt.x + largura, pt.y + altura);
        pt.y += 5;
        TCHAR linha[100];
        // Jogadores
        PLAYER_MP jogadoresOrdenados[MAX_CONCURRENT_USERS];
        DWORD count = 0;

        for (i = 0; i < MAX_CONCURRENT_USERS; i++) {
            if (_tcscmp(td.memoria->players[i].name, _T("")) != 0) {
                jogadoresOrdenados[count++] = td.memoria->players[i];
            }
        }

        // Ordenar por pontua��o decrescente
        qsort(jogadoresOrdenados, count, sizeof(PLAYER_MP), comparaPontuacoes);

        // Mostrar nomes
        for (j = 0; j < count && j < td.max_jogadores; j++) {
            _stprintf_s(linha, 100, _T("%s"), jogadoresOrdenados[j].name);
            TextOut(hdc, (dim.right / 2) - 150 - largura + 10, pt.y + j * 17, linha, _tcslen(linha));
        }

        // Mostrar pontua��es
        for (j = 0; j < count && j < td.max_jogadores; j++) {
            _stprintf_s(linha, 100, _T("%.1f"), jogadoresOrdenados[j].points);
            TextOut(hdc, (dim.right / 2) + 150 - largura + 10, pt.y + j * 17, linha, _tcslen(linha));
        }


        // __________JOGADORES-------------



        // aqui vai ser o Jogo  
        //Rectangle(hdc, 24, 25, 120, 55);
        //TextOut(hdc, 30, 30,_T("TITULO"),20);


        break;
    case WM_DESTROY:
        ptd = (TDATA*)GetWindowLongPtr(hWnd, 0);
        ptd->continua = FALSE; 
        WaitForSingleObject(ptd->hThread, INFINITE); 
        CloseHandle(ptd->hThread);
        CloseHandle(ptd->hMutex);
        UnmapViewOfFile(ptd->memoria);
        CloseHandle(hMemoPart);
        free(ptd);
        PostQuitMessage(0);
        break;
    default:
      
        return DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return 0;
}
