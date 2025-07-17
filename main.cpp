#include <windows.h>

#define ID_ABOUT 1001
#define ID_EXIT 1002

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char* CLASS_NAME = "HelloWorldWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindow(
        CLASS_NAME,
        "Hello World App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        return 0;
    }
    
    // Create menu
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    
    AppendMenu(hSubMenu, MF_STRING, ID_ABOUT, "&About");
    AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenu, MF_STRING, ID_EXIT, "E&xit");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "&Help");
    
    SetMenu(hwnd, hMenu);
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // Center the text
            SetTextAlign(hdc, TA_CENTER);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, rect.right / 2, rect.bottom / 2 - 10, "Hello World!", 12);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_ABOUT: {
                    const char* aboutText = "Hello World App\n\n"
                                          "Version: 1.0.0\n"
                                          "Built by: Kieran Klukas\n\n"
                                          "A simple Win32 application\n"
                                          "compatible with Windows XP";
                    MessageBox(hwnd, aboutText, "About Hello World App", 
                              MB_OK | MB_ICONINFORMATION);
                    break;
                }
                case ID_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}