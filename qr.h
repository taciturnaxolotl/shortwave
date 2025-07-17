#pragma once
#include <windows.h>

#define QR_SIZE 21
#define MAX_TEXT_LEN 256

// Pure C struct for QR code - no C++ classes
typedef struct {
    BOOL modules[QR_SIZE][QR_SIZE];
    char text[MAX_TEXT_LEN];
} QRCode;

// Function prototypes
void QRCode_Init(QRCode* qr, const char* inputText);
void QRCode_GeneratePattern(QRCode* qr);
void QRCode_AddFinderPattern(QRCode* qr, int x, int y);
BOOL QRCode_IsReserved(int x, int y);
void QRCode_DrawToHDC(QRCode* qr, HDC hdc, int startX, int startY, int moduleSize);
int QRCode_GetSize(void);
const char* QRCode_GetText(QRCode* qr);

// Implementation
void QRCode_Init(QRCode* qr, const char* inputText) {
    int x, y, i;
    
    // Initialize modules array
    for (y = 0; y < QR_SIZE; y++) {
        for (x = 0; x < QR_SIZE; x++) {
            qr->modules[y][x] = FALSE;
        }
    }
    
    // Copy text (safe copy)
    i = 0;
    while (inputText[i] && i < MAX_TEXT_LEN - 1) {
        qr->text[i] = inputText[i];
        i++;
    }
    qr->text[i] = '\0';
    
    // Generate pattern
    QRCode_GeneratePattern(qr);
}

void QRCode_GeneratePattern(QRCode* qr) {
    int i, x, y;
    unsigned int hash = 0;
    unsigned char textBytes[MAX_TEXT_LEN];
    int textLen = 0;
    
    // Add finder patterns (corners)
    QRCode_AddFinderPattern(qr, 0, 0);
    QRCode_AddFinderPattern(qr, QR_SIZE - 7, 0);
    QRCode_AddFinderPattern(qr, 0, QR_SIZE - 7);
    
    // Add timing patterns
    for (i = 8; i < QR_SIZE - 8; i++) {
        qr->modules[6][i] = (i % 2 == 0) ? TRUE : FALSE;
        qr->modules[i][6] = (i % 2 == 0) ? TRUE : FALSE;
    }
    
    // Convert text to bytes and calculate length
    while (qr->text[textLen] && textLen < MAX_TEXT_LEN - 1) {
        textBytes[textLen] = (unsigned char)qr->text[textLen];
        textLen++;
    }
    
    // Add format information (fake but realistic looking)
    // These would normally encode error correction level and mask pattern
    qr->modules[8][0] = TRUE;
    qr->modules[8][1] = FALSE;
    qr->modules[8][2] = TRUE;
    qr->modules[8][3] = TRUE;
    qr->modules[8][4] = FALSE;
    qr->modules[8][5] = TRUE;
    
    // Add data in a more realistic zigzag pattern
    int bitIndex = 0;
    BOOL upward = TRUE;
    
    for (x = QR_SIZE - 1; x > 0; x -= 2) {
        if (x == 6) x--; // Skip timing column
        
        for (i = 0; i < QR_SIZE; i++) {
            y = upward ? (QR_SIZE - 1 - i) : i;
            
            // Fill two columns (right to left)
            for (int col = 0; col < 2; col++) {
                int currentX = x - col;
                if (currentX >= 0 && !QRCode_IsReserved(currentX, y)) {
                    // Use text data in a more structured way
                    BOOL bit = FALSE;
                    if (bitIndex < textLen * 8) {
                        int byteIndex = bitIndex / 8;
                        int bitPos = 7 - (bitIndex % 8);
                        bit = (textBytes[byteIndex] >> bitPos) & 1;
                        bitIndex++;
                    } else {
                        // Padding pattern
                        bit = ((currentX + y) % 3 == 0) ? TRUE : FALSE;
                    }
                    qr->modules[y][currentX] = bit;
                }
            }
        }
        upward = !upward;
    }
}

void QRCode_AddFinderPattern(QRCode* qr, int x, int y) {
    int dx, dy;
    BOOL dark;
    
    for (dy = 0; dy < 7; dy++) {
        for (dx = 0; dx < 7; dx++) {
            if (x + dx < QR_SIZE && y + dy < QR_SIZE) {
                dark = (dx == 0 || dx == 6 || dy == 0 || dy == 6 ||
                       (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4)) ? TRUE : FALSE;
                qr->modules[y + dy][x + dx] = dark;
            }
        }
    }
}

BOOL QRCode_IsReserved(int x, int y) {
    // Check if position is part of finder patterns
    if ((x < 9 && y < 9) || 
        (x >= QR_SIZE - 8 && y < 9) || 
        (x < 9 && y >= QR_SIZE - 8)) {
        return TRUE;
    }
    
    // Check timing patterns
    if (x == 6 || y == 6) {
        return TRUE;
    }
    
    // Check format information areas
    if ((x < 9 && y == 8) || (x == 8 && y < 9)) {
        return TRUE;
    }
    if ((x >= QR_SIZE - 8 && y == 8) || (x == 8 && y >= QR_SIZE - 7)) {
        return TRUE;
    }
    
    return FALSE;
}

void QRCode_DrawToHDC(QRCode* qr, HDC hdc, int startX, int startY, int moduleSize) {
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    int x, y;
    RECT rect;
    
    for (y = 0; y < QR_SIZE; y++) {
        for (x = 0; x < QR_SIZE; x++) {
            rect.left = startX + x * moduleSize;
            rect.top = startY + y * moduleSize;
            rect.right = startX + (x + 1) * moduleSize;
            rect.bottom = startY + (y + 1) * moduleSize;
            
            FillRect(hdc, &rect, qr->modules[y][x] ? blackBrush : whiteBrush);
        }
    }
    
    DeleteObject(blackBrush);
    DeleteObject(whiteBrush);
}

int QRCode_GetSize(void) {
    return QR_SIZE;
}

const char* QRCode_GetText(QRCode* qr) {
    return qr->text;
}