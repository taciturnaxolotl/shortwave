#include <windows.h>
#include "qr.h"

#define ID_ABOUT 1001
#define ID_EXIT 1002
#define ID_GENERATE_QR 1003

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global QR code instance - static allocation, no new/delete
QRCode g_qrCode;
BOOL g_hasQrCode = FALSE;
char g_qrText[256] = "Hello World!";

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
    
    // Tools menu
    HMENU hToolsMenu = CreatePopupMenu();
    AppendMenu(hToolsMenu, MF_STRING, ID_GENERATE_QR, "&Generate QR Pattern");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hToolsMenu, "&Tools");
    
    // Help menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenu(hHelpMenu, MF_STRING, ID_ABOUT, "&About");
    AppendMenu(hHelpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hHelpMenu, MF_STRING, ID_EXIT, "E&xit");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hHelpMenu, "&Help");
    
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
        case WM_SIZE:
            // Trigger repaint when window is resized
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
            
        case WM_DESTROY:
            // No cleanup needed for static allocation
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            if (g_hasQrCode) {
                // Draw QR code - calculate size based on window
                int qrSize = QRCode_GetSize();
                int moduleSize = 8;
                int qrPixelSize = qrSize * moduleSize;
                
                // Center horizontally and vertically with some padding
                int startX = (rect.right - qrPixelSize) / 2;
                int startY = (rect.bottom - qrPixelSize - 80) / 2; // Leave space for text
                
                QRCode_DrawToHDC(&g_qrCode, hdc, startX, startY, moduleSize);
                
                // Draw text below QR code
                SetTextAlign(hdc, TA_CENTER);
                SetBkMode(hdc, TRANSPARENT);
                int textLen = 0;
                while (g_qrText[textLen]) textLen++; // Calculate length
                TextOut(hdc, rect.right / 2, startY + qrPixelSize + 20, 
                       g_qrText, textLen);
                       
                // Add disclaimer
                const char* disclaimer = "(Visual demo - not scannable)";
                int disclaimerLen = 0;
                while (disclaimer[disclaimerLen]) disclaimerLen++;
                TextOut(hdc, rect.right / 2, startY + qrPixelSize + 40, 
                       disclaimer, disclaimerLen);
            } else {
                // Default view - center in current window size
                SetTextAlign(hdc, TA_CENTER);
                SetBkMode(hdc, TRANSPARENT);
                int centerY = rect.bottom / 2;
                TextOut(hdc, rect.right / 2, centerY - 10, "Hello World!", 12);
                TextOut(hdc, rect.right / 2, centerY + 10, 
                       "Use Tools > Generate QR Pattern", 32);
            }
            
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
                                          "compatible with Windows XP\n\n"
                                          "Features:\n"
                                          "- QR Pattern Generation (visual demo)\n"
                                          "- XP Compatible Design\n"
                                          "- Pure Win32 API";
                    MessageBox(hwnd, aboutText, "About Hello World App", 
                              MB_OK | MB_ICONINFORMATION);
                    break;
                }
                case ID_GENERATE_QR: {
                    // Simple input dialog using InputBox simulation
                    if (MessageBox(hwnd, "Generate QR code pattern for current text?\n\n(Note: This creates a visual QR-like pattern for demo purposes,\nnot a scannable QR code)\n\nClick OK to use default text,\nor Cancel to cycle through presets.", 
                                  "Generate QR Pattern", MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL) {
                        
                        // For now, use a simple preset - in a real app you'd want a proper input dialog
                        const char* presets[] = {
                            "Hello World!",
                            "https://github.com/taciturnaxolotl/shortwave",
                            "Made with love by Kieran Klukas",
                            "Windows XP Forever!",
                            "QR codes are cool!"
                        };
                        
                        static int presetIndex = 0;
                        const char* selectedText = presets[presetIndex % 5];
                        presetIndex++;
                        
                        // Copy selected text to global buffer
                        int i = 0;
                        while (selectedText[i] && i < 255) {
                            g_qrText[i] = selectedText[i];
                            i++;
                        }
                        g_qrText[i] = '\0';
                    }
                    
                    // Generate new QR code - no new/delete, just reinitialize
                    QRCode_Init(&g_qrCode, g_qrText);
                    g_hasQrCode = TRUE;
                    
                    // Refresh the window
                    InvalidateRect(hwnd, NULL, TRUE);
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