#include <windows.h>
#include <tchar.h>

LRESULT CALLBACK trataEventos(HWND, UINT, WPARAM, LPARAM);

TCHAR szProgName[] = TEXT("Base");

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
    wcApp.lpszMenuName = NULL;  
    wcApp.cbClsExtra = 0; 
    wcApp.cbWndExtra = 0; 
    wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  
  


    if (!RegisterClassEx(&wcApp))
        return(0);

    hWnd = CreateWindow(
        szProgName,  
        TEXT("Exemplo de Janela Principal em C"),  
        WS_OVERLAPPEDWINDOW,	
        CW_USEDEFAULT,  
        CW_USEDEFAULT,  
        CW_USEDEFAULT,  
        CW_USEDEFAULT,  
        (HWND)HWND_DESKTOP,	
      
        (HMENU)NULL,  
        (HINSTANCE)hInst, 
     
        0);  

    ShowWindow(hWnd, nCmdShow); 

    UpdateWindow(hWnd);  
 
    while (GetMessage(&lpMsg, NULL, 0, 0) > 0) {
        TranslateMessage(&lpMsg); 
        DispatchMessage(&lpMsg);  

    }

    return (int)lpMsg.wParam;  
}


LRESULT CALLBACK trataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
    switch (messg) {
    case WM_DESTROY:	    
        PostQuitMessage(0);  	
        break;
    default:
        return DefWindowProc(hWnd, messg, wParam, lParam);
    }
    return 0;
}
