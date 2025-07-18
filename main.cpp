#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <mmsystem.h>
#include <wininet.h>
#include "libs/bass.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wininet.lib")

#define ID_ABOUT 1001
#define ID_EXIT 1002

// Radio control IDs
#define ID_TUNING_DIAL 2001
#define ID_VOLUME_KNOB 2002
#define ID_POWER_BUTTON 2003

// Radio station data
typedef struct {
	float frequency;
	char name[64];
	char description[128];
	char streamUrl[256];
} RadioStation;

RadioStation g_stations[] = {
	{14.230f, "SomaFM Groove", "Downtempo and chillout", "http://ice1.somafm.com/groovesalad-128-mp3"},
	{15.770f, "Radio Paradise", "Eclectic music mix", "http://stream.radioparadise.com/mp3-128"},
	{17.895f, "Jazz Radio", "Smooth jazz", "http://jazz-wr04.ice.infomaniak.ch/jazz-wr04-128.mp3"},
	{21.500f, "Classical Music", "Classical radio", "http://stream.wqxr.org/wqxr"},
	{31.100f, "Chillout Lounge", "Relaxing music", "http://air.radiorecord.ru:805/chil_320"}
};

#define NUM_STATIONS (sizeof(g_stations) / sizeof(RadioStation))
#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define CHANNELS 1
#define BUFFER_SIZE 4410  // 0.1 seconds of audio
#define NUM_BUFFERS 4

// Audio state
typedef struct {
	// BASS handles
	HSTREAM currentStream;
	HSTREAM staticStream;
	int isPlaying;
	float staticVolume;
	float radioVolume;
	
	// Station tracking
	RadioStation* currentStation;
} AudioState;

// Radio state
typedef struct {
	float frequency;
	float volume;
	int power;
	int signalStrength;
	int isDraggingDial;
	int isDraggingVolume;
} RadioState;

RadioState g_radio = {14.230f, 0.8f, 0, 0, 0, 0};  // Increase default volume to 0.8
AudioState g_audio = {0};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawRadioInterface(HDC hdc, RECT* rect);
void DrawTuningDial(HDC hdc, int x, int y, int radius, float frequency);
void DrawFrequencyDisplay(HDC hdc, int x, int y, float frequency);
void DrawSignalMeter(HDC hdc, int x, int y, int strength);
void DrawVolumeKnob(HDC hdc, int x, int y, int radius, float volume);
void DrawPowerButton(HDC hdc, int x, int y, int radius, int power);
int IsPointInCircle(int px, int py, int cx, int cy, int radius);
float GetAngleFromPoint(int px, int py, int cx, int cy);
void UpdateFrequencyFromMouse(int mouseX, int mouseY);
void UpdateVolumeFromMouse(int mouseX, int mouseY);

// Audio functions
int InitializeAudio();
void CleanupAudio();
void StartAudio();
void StopAudio();
RadioStation* FindNearestStation(float frequency);
float GetStationSignalStrength(RadioStation* station, float currentFreq);

// BASS streaming functions
int StartBassStreaming(RadioStation* station);
void StopBassStreaming();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	// Allocate console for debugging
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	printf("Shortwave Radio Debug Console\n");
	printf("=============================\n");

	const char* CLASS_NAME = "ShortwaveRadio";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hbrBackground = CreateSolidBrush(RGB(101, 67, 33)); // Wood grain brown
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	RegisterClass(&wc);

	HWND hwnd = CreateWindow(
		CLASS_NAME,
		"Shortwave Radio Tuner",
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, // Fixed size
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 450,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hwnd == NULL) {
		return 0;
	}

	// Initialize audio system
	if (InitializeAudio() != 0) {
		MessageBox(hwnd, "Failed to initialize audio system", "Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	// Audio starts when power button is pressed

	// Create menu
	HMENU hMenu = CreateMenu();

	// Radio menu
	HMENU hRadioMenu = CreatePopupMenu();
	AppendMenu(hRadioMenu, MF_STRING, ID_ABOUT, "&About");
	AppendMenu(hRadioMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hRadioMenu, MF_STRING, ID_EXIT, "E&xit");
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hRadioMenu, "&Radio");

	SetMenu(hwnd, hMenu);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Cleanup audio
	StopAudio();
	CleanupAudio();

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_SIZE:
			InvalidateRect(hwnd, NULL, TRUE);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			RECT rect;
			GetClientRect(hwnd, &rect);

			DrawRadioInterface(hdc, &rect);

			EndPaint(hwnd, &ps);
			return 0;
		}

		case WM_LBUTTONDOWN: {
			int mouseX = LOWORD(lParam);
			int mouseY = HIWORD(lParam);

			// Check if clicking on tuning dial
			if (IsPointInCircle(mouseX, mouseY, 150, 200, 60)) {
				g_radio.isDraggingDial = 1;
				SetCapture(hwnd);
				UpdateFrequencyFromMouse(mouseX, mouseY);
				InvalidateRect(hwnd, NULL, TRUE);
			}
			// Check if clicking on volume knob
			else if (IsPointInCircle(mouseX, mouseY, 350, 200, 30)) {
				g_radio.isDraggingVolume = 1;
				SetCapture(hwnd);
				UpdateVolumeFromMouse(mouseX, mouseY);
				InvalidateRect(hwnd, NULL, TRUE);
			}
			// Check if clicking on power button
			else if (IsPointInCircle(mouseX, mouseY, 500, 120, 25)) {
				g_radio.power = !g_radio.power;
				if (g_radio.power) {
					StartAudio();
				} else {
					StopAudio();
				}
				InvalidateRect(hwnd, NULL, TRUE);
			}
			return 0;
		}

		case WM_LBUTTONUP: {
			if (g_radio.isDraggingDial || g_radio.isDraggingVolume) {
				g_radio.isDraggingDial = 0;
				g_radio.isDraggingVolume = 0;
				ReleaseCapture();
			}
			return 0;
		}

		case WM_MOUSEMOVE: {
			if (g_radio.isDraggingDial) {
				int mouseX = LOWORD(lParam);
				int mouseY = HIWORD(lParam);
				UpdateFrequencyFromMouse(mouseX, mouseY);
				InvalidateRect(hwnd, NULL, TRUE);
			}
			else if (g_radio.isDraggingVolume) {
				int mouseX = LOWORD(lParam);
				int mouseY = HIWORD(lParam);
				UpdateVolumeFromMouse(mouseX, mouseY);
				InvalidateRect(hwnd, NULL, TRUE);
			}
			return 0;
		}

		case WM_KEYDOWN: {
			switch (wParam) {
				case VK_UP: {
					// Increase frequency by 0.1 MHz (fine tuning)
					g_radio.frequency += 0.1f;
					if (g_radio.frequency > 34.0f) g_radio.frequency = 34.0f;

					// Update signal strength for new frequency
					RadioStation* station = FindNearestStation(g_radio.frequency);
					if (station) {
						g_radio.signalStrength = (int)(GetStationSignalStrength(station, g_radio.frequency) * 100.0f);
						if (g_radio.signalStrength > 50 && station != g_audio.currentStation) {
							StopBassStreaming();
							StartBassStreaming(station);
						}
					} else {
						g_radio.signalStrength = 5 + (int)(15.0f * sin(g_radio.frequency));
						StopBassStreaming();
					}

					if (g_radio.signalStrength < 0) g_radio.signalStrength = 0;
					if (g_radio.signalStrength > 100) g_radio.signalStrength = 100;

					InvalidateRect(hwnd, NULL, TRUE);
					break;
				}

				case VK_DOWN: {
					// Decrease frequency by 0.1 MHz (fine tuning)
					g_radio.frequency -= 0.1f;
					if (g_radio.frequency < 10.0f) g_radio.frequency = 10.0f;

					// Update signal strength for new frequency
					RadioStation* station = FindNearestStation(g_radio.frequency);
					if (station) {
						g_radio.signalStrength = (int)(GetStationSignalStrength(station, g_radio.frequency) * 100.0f);
						if (g_radio.signalStrength > 50 && station != g_audio.currentStation) {
							StopBassStreaming();
							StartBassStreaming(station);
						}
					} else {
						g_radio.signalStrength = 5 + (int)(15.0f * sin(g_radio.frequency));
						StopBassStreaming();
					}

					if (g_radio.signalStrength < 0) g_radio.signalStrength = 0;
					if (g_radio.signalStrength > 100) g_radio.signalStrength = 100;

					InvalidateRect(hwnd, NULL, TRUE);
					break;
				}

				case VK_RIGHT: {
					// Increase frequency by 1.0 MHz (coarse tuning)
					g_radio.frequency += 1.0f;
					if (g_radio.frequency > 34.0f) g_radio.frequency = 34.0f;

					// Update signal strength for new frequency
					RadioStation* station = FindNearestStation(g_radio.frequency);
					if (station) {
						g_radio.signalStrength = (int)(GetStationSignalStrength(station, g_radio.frequency) * 100.0f);
						if (g_radio.signalStrength > 50 && station != g_audio.currentStation) {
							StopBassStreaming();
							StartBassStreaming(station);
						}
					} else {
						g_radio.signalStrength = 5 + (int)(15.0f * sin(g_radio.frequency));
						StopBassStreaming();
					}

					if (g_radio.signalStrength < 0) g_radio.signalStrength = 0;
					if (g_radio.signalStrength > 100) g_radio.signalStrength = 100;

					InvalidateRect(hwnd, NULL, TRUE);
					break;
				}

				case VK_LEFT: {
					// Decrease frequency by 1.0 MHz (coarse tuning)
					g_radio.frequency -= 1.0f;
					if (g_radio.frequency < 10.0f) g_radio.frequency = 10.0f;

					// Update signal strength for new frequency
					RadioStation* station = FindNearestStation(g_radio.frequency);
					if (station) {
						g_radio.signalStrength = (int)(GetStationSignalStrength(station, g_radio.frequency) * 100.0f);
						if (g_radio.signalStrength > 50 && station != g_audio.currentStation) {
							StopBassStreaming();
							StartBassStreaming(station);
						}
					} else {
						g_radio.signalStrength = 5 + (int)(15.0f * sin(g_radio.frequency));
						StopBassStreaming();
					}

					if (g_radio.signalStrength < 0) g_radio.signalStrength = 0;
					if (g_radio.signalStrength > 100) g_radio.signalStrength = 100;

					InvalidateRect(hwnd, NULL, TRUE);
					break;
				}
			}
			return 0;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case ID_ABOUT: {
					const char* aboutText = "Shortwave Radio Tuner\n\n"
										  "Version: 1.0.0\n"
										  "Built for Rewind V2 Hackathon\n\n"
										  "A vintage shortwave radio simulator\n"
										  "compatible with Windows XP\n\n"
										  "Features:\n"
										  "- Realistic tuning interface\n"
										  "- Internet radio streaming\n"
										  "- Authentic static noise\n\n"
										  "Controls:\n"
										  "- Drag tuning dial to change frequency\n"
										  "- UP/DOWN arrows: Fine tuning (0.1 MHz)\n"
										  "- LEFT/RIGHT arrows: Coarse tuning (1.0 MHz)\n"
										  "- Click power button to turn on/off\n"
										  "- Drag volume knob to adjust volume";
					MessageBox(hwnd, aboutText, "About Shortwave Radio",
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

void DrawRadioInterface(HDC hdc, RECT* rect) {
	// Create vintage radio background
	HBRUSH woodBrush = CreateSolidBrush(RGB(101, 67, 33));
	FillRect(hdc, rect, woodBrush);
	DeleteObject(woodBrush);

	// Draw radio panel (darker inset)
	RECT panel = {50, 50, rect->right - 50, rect->bottom - 50};
	HBRUSH panelBrush = CreateSolidBrush(RGB(80, 50, 25));
	FillRect(hdc, &panel, panelBrush);
	DeleteObject(panelBrush);

	// Draw panel border (raised effect)
	HPEN lightPen = CreatePen(PS_SOLID, 2, RGB(140, 100, 60));
	HPEN darkPen = CreatePen(PS_SOLID, 2, RGB(40, 25, 15));

	SelectObject(hdc, lightPen);
	MoveToEx(hdc, panel.left, panel.bottom, NULL);
	LineTo(hdc, panel.left, panel.top);
	LineTo(hdc, panel.right, panel.top);

	SelectObject(hdc, darkPen);
	LineTo(hdc, panel.right, panel.bottom);
	LineTo(hdc, panel.left, panel.bottom);

	DeleteObject(lightPen);
	DeleteObject(darkPen);

	// Draw frequency display
	DrawFrequencyDisplay(hdc, 200, 80, g_radio.frequency);

	// Draw tuning dial
	DrawTuningDial(hdc, 150, 200, 60, g_radio.frequency);

	// Draw volume knob
	DrawVolumeKnob(hdc, 350, 200, 30, g_radio.volume);

	// Draw signal meter
	DrawSignalMeter(hdc, 450, 150, g_radio.signalStrength);

	// Draw power button
	DrawPowerButton(hdc, 500, 120, 25, g_radio.power);

	// Draw station info if tuned to a station
	RadioStation* currentStation = FindNearestStation(g_radio.frequency);
	if (currentStation && g_radio.signalStrength > 30) {
		RECT stationRect = {80, 320, 520, 360};
		HBRUSH stationBrush = CreateSolidBrush(RGB(0, 0, 0));
		FillRect(hdc, &stationRect, stationBrush);
		DeleteObject(stationBrush);

		HPEN stationPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
		SelectObject(hdc, stationPen);
		Rectangle(hdc, stationRect.left, stationRect.top, stationRect.right, stationRect.bottom);
		DeleteObject(stationPen);

		SetTextColor(hdc, RGB(0, 255, 0));
		HFONT stationFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
									  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
									  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
									  DEFAULT_PITCH | FF_SWISS, "Arial");
		SelectObject(hdc, stationFont);

		char stationText[256];
		sprintf(stationText, "%.3f MHz - %s: %s",
				currentStation->frequency, currentStation->name, currentStation->description);

		SetTextAlign(hdc, TA_LEFT);
		TextOut(hdc, stationRect.left + 5, stationRect.top + 5, stationText, strlen(stationText));

		DeleteObject(stationFont);
	}

	// Draw labels
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(255, 255, 255));
	HFONT font = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
						   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
						   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
						   DEFAULT_PITCH | FF_SWISS, "Arial");
	SelectObject(hdc, font);

	TextOut(hdc, 180, 300, "TUNING", 6);
	TextOut(hdc, 330, 260, "VOLUME", 6);
	TextOut(hdc, 430, 200, "SIGNAL", 6);
	TextOut(hdc, 485, 160, "POWER", 5);

	DeleteObject(font);
}

void DrawFrequencyDisplay(HDC hdc, int x, int y, float frequency) {
	// Draw display background (black LCD style)
	RECT display = {x - 80, y - 20, x + 80, y + 20};
	HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
	FillRect(hdc, &display, blackBrush);
	DeleteObject(blackBrush);

	// Draw display border
	HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
	SelectObject(hdc, borderPen);
	Rectangle(hdc, display.left, display.top, display.right, display.bottom);
	DeleteObject(borderPen);

	// Draw frequency text
	char freqText[32];
	sprintf(freqText, "%.3f MHz", frequency);

	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, RGB(0, 255, 0)); // Green LCD color
	HFONT lcdFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
							  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							  FIXED_PITCH | FF_MODERN, "Courier New");
	SelectObject(hdc, lcdFont);

	SetTextAlign(hdc, TA_CENTER);
	TextOut(hdc, x, y - 8, freqText, strlen(freqText));

	DeleteObject(lcdFont);
}

void DrawTuningDial(HDC hdc, int x, int y, int radius, float frequency) {
	// Draw dial background
	HBRUSH dialBrush = CreateSolidBrush(RGB(160, 120, 80));
	SelectObject(hdc, dialBrush);
	Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
	DeleteObject(dialBrush);

	// Draw dial border
	HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(60, 40, 20));
	SelectObject(hdc, borderPen);
	Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
	DeleteObject(borderPen);

	// Draw frequency markings
	SetTextColor(hdc, RGB(0, 0, 0));
	HFONT smallFont = CreateFont(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
								DEFAULT_PITCH | FF_SWISS, "Arial");
	SelectObject(hdc, smallFont);
	SetTextAlign(hdc, TA_CENTER);

	for (int i = 0; i < 12; i++) {
		float angle = (float)i * 3.14159f / 6.0f;
		int markX = x + (int)((radius - 15) * cos(angle));
		int markY = y + (int)((radius - 15) * sin(angle));

		char mark[8];
		sprintf(mark, "%d", 10 + i * 2);
		TextOut(hdc, markX, markY - 5, mark, strlen(mark));
	}

	// Draw pointer based on frequency
	float angle = (frequency - 10.0f) / 24.0f * 3.14159f;
	int pointerX = x + (int)((radius - 10) * cos(angle));
	int pointerY = y + (int)((radius - 10) * sin(angle));

	HPEN pointerPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
	SelectObject(hdc, pointerPen);
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, pointerX, pointerY);
	DeleteObject(pointerPen);

	DeleteObject(smallFont);
}

void DrawVolumeKnob(HDC hdc, int x, int y, int radius, float volume) {
	// Draw knob background
	HBRUSH knobBrush = CreateSolidBrush(RGB(140, 100, 60));
	SelectObject(hdc, knobBrush);
	Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
	DeleteObject(knobBrush);

	// Draw knob border
	HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(60, 40, 20));
	SelectObject(hdc, borderPen);
	Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
	DeleteObject(borderPen);

	// Draw volume indicator
	float angle = volume * 3.14159f * 1.5f - 3.14159f * 0.75f;
	int indicatorX = x + (int)((radius - 5) * cos(angle));
	int indicatorY = y + (int)((radius - 5) * sin(angle));

	HPEN indicatorPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
	SelectObject(hdc, indicatorPen);
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, indicatorX, indicatorY);
	DeleteObject(indicatorPen);
}

void DrawSignalMeter(HDC hdc, int x, int y, int strength) {
	// Draw meter background
	RECT meter = {x, y, x + 80, y + 20};
	HBRUSH meterBrush = CreateSolidBrush(RGB(0, 0, 0));
	FillRect(hdc, &meter, meterBrush);
	DeleteObject(meterBrush);

	// Draw meter border
	HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
	SelectObject(hdc, borderPen);
	Rectangle(hdc, meter.left, meter.top, meter.right, meter.bottom);
	DeleteObject(borderPen);

	// Draw signal bars
	int barWidth = 8;
	int numBars = strength / 10;
	for (int i = 0; i < numBars && i < 10; i++) {
		RECT bar = {x + 2 + i * barWidth, y + 2,
				   x + 2 + (i + 1) * barWidth - 1, y + 18};

		COLORREF barColor;
		if (i < 3) barColor = RGB(0, 255, 0);      // Green
		else if (i < 7) barColor = RGB(255, 255, 0); // Yellow
		else barColor = RGB(255, 0, 0);              // Red

		HBRUSH barBrush = CreateSolidBrush(barColor);
		FillRect(hdc, &bar, barBrush);
		DeleteObject(barBrush);
	}
}

void DrawPowerButton(HDC hdc, int x, int y, int radius, int power) {
	// Draw button background
	COLORREF buttonColor = power ? RGB(255, 0, 0) : RGB(100, 100, 100);
	HBRUSH buttonBrush = CreateSolidBrush(buttonColor);
	SelectObject(hdc, buttonBrush);
	Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
	DeleteObject(buttonBrush);

	// Draw button border
	HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(60, 60, 60));
	SelectObject(hdc, borderPen);
	Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
	DeleteObject(borderPen);

	// Draw power symbol
	if (power) {
		HPEN symbolPen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
		SelectObject(hdc, symbolPen);

		// Draw power symbol (circle with line)
		Arc(hdc, x - 8, y - 8, x + 8, y + 8, x + 6, y - 6, x - 6, y - 6);
		MoveToEx(hdc, x, y - 10, NULL);
		LineTo(hdc, x, y - 2);

		DeleteObject(symbolPen);
	}
}

int IsPointInCircle(int px, int py, int cx, int cy, int radius) {
	int dx = px - cx;
	int dy = py - cy;
	return (dx * dx + dy * dy) <= (radius * radius);
}

float GetAngleFromPoint(int px, int py, int cx, int cy) {
	return atan2((float)(py - cy), (float)(px - cx));
}

void UpdateFrequencyFromMouse(int mouseX, int mouseY) {
	float angle = GetAngleFromPoint(mouseX, mouseY, 150, 200);

	// Convert angle to frequency (10-34 MHz range)
	// Normalize angle from -PI to PI to 0-1 range
	float normalizedAngle = (angle + 3.14159f) / (2.0f * 3.14159f);

	// Map to frequency range
	g_radio.frequency = 10.0f + normalizedAngle * 24.0f;

	// Clamp frequency
	if (g_radio.frequency < 10.0f) g_radio.frequency = 10.0f;
	if (g_radio.frequency > 34.0f) g_radio.frequency = 34.0f;

	// Calculate signal strength based on nearest station
	RadioStation* nearestStation = FindNearestStation(g_radio.frequency);
	if (nearestStation) {
		g_radio.signalStrength = (int)(GetStationSignalStrength(nearestStation, g_radio.frequency) * 100.0f);

		// Start streaming if signal is strong enough and station changed
		if (g_radio.signalStrength > 50 && nearestStation != g_audio.currentStation) {
			StopBassStreaming();
			StartBassStreaming(nearestStation);
		}
	} else {
		g_radio.signalStrength = 5 + (int)(15.0f * sin(g_radio.frequency));
		StopBassStreaming();
	}

	if (g_radio.signalStrength < 0) g_radio.signalStrength = 0;
	if (g_radio.signalStrength > 100) g_radio.signalStrength = 100;
}

void UpdateVolumeFromMouse(int mouseX, int mouseY) {
	float angle = GetAngleFromPoint(mouseX, mouseY, 350, 200);

	// Convert angle to volume (0-1 range)
	// Map from -135 degrees to +135 degrees
	float normalizedAngle = (angle + 3.14159f * 0.75f) / (3.14159f * 1.5f);

	g_radio.volume = normalizedAngle;

	// Clamp volume
	if (g_radio.volume < 0.0f) g_radio.volume = 0.0f;
	if (g_radio.volume > 1.0f) g_radio.volume = 1.0f;
}

int InitializeAudio() {
	// Initialize BASS with more detailed error reporting
	printf("Initializing BASS audio system...\n");
	
	if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
		DWORD error = BASS_ErrorGetCode();
		printf("BASS initialization failed (Error: %lu)\n", error);
		
		// Try alternative initialization methods
		printf("Trying alternative audio device...\n");
		if (!BASS_Init(0, 44100, 0, 0, NULL)) {
			error = BASS_ErrorGetCode();
			printf("Alternative BASS init also failed (Error: %lu)\n", error);
			printf("BASS Error meanings:\n");
			printf("  1 = BASS_ERROR_MEM (memory error)\n");
			printf("  2 = BASS_ERROR_FILEOPEN (file/URL error)\n");
			printf("  3 = BASS_ERROR_DRIVER (no audio driver)\n");
			printf("  8 = BASS_ERROR_ALREADY (already initialized)\n");
			printf("  14 = BASS_ERROR_DEVICE (invalid device)\n");
			return -1;
		}
	}
	
	printf("BASS initialized successfully\n");
	
	// Get BASS version info
	DWORD version = BASS_GetVersion();
	printf("BASS version: %d.%d.%d.%d\n", 
		   HIBYTE(HIWORD(version)), LOBYTE(HIWORD(version)),
		   HIBYTE(LOWORD(version)), LOBYTE(LOWORD(version)));
	
	g_audio.currentStream = 0;
	g_audio.staticStream = 0;
	g_audio.isPlaying = 0;
	g_audio.staticVolume = 0.8f;
	g_audio.radioVolume = 0.0f;
	g_audio.currentStation = NULL;

	return 0;
}

void CleanupAudio() {
	StopBassStreaming();
	
	// Free BASS
	BASS_Free();
	printf("BASS cleaned up\n");
}

void StartAudio() {
	if (!g_audio.isPlaying) {
		g_audio.isPlaying = 1;
		printf("Audio started\n");
	}
}

void StopAudio() {
	if (g_audio.isPlaying) {
		g_audio.isPlaying = 0;
		StopBassStreaming();
		printf("Audio stopped\n");
	}
}

RadioStation* FindNearestStation(float frequency) {
	RadioStation* nearest = NULL;
	float minDistance = 999.0f;

	for (int i = 0; i < NUM_STATIONS; i++) {
		float distance = fabs(g_stations[i].frequency - frequency);
		if (distance < minDistance) {
			minDistance = distance;
			nearest = &g_stations[i];
		}
	}

	// Only return station if we're close enough (within 0.5 MHz)
	if (minDistance <= 0.5f) {
		return nearest;
	}

	return NULL;
}

float GetStationSignalStrength(RadioStation* station, float currentFreq) {
	if (!station) return 0.0f;

	float distance = fabs(station->frequency - currentFreq);

	// Signal strength drops off with distance from exact frequency
	if (distance < 0.05f) {
		return 0.9f; // Very strong signal
	} else if (distance < 0.1f) {
		return 0.7f; // Strong signal
	} else if (distance < 0.2f) {
		return 0.5f; // Medium signal
	} else if (distance < 0.5f) {
		return 0.2f; // Weak signal
	}

	return 0.0f; // No signal
}

int StartBassStreaming(RadioStation* station) {
	if (!station) {
		printf("StartBassStreaming failed: no station\n");
		return 0;
	}

	StopBassStreaming();

	printf("Attempting to stream: %s at %s\n", station->name, station->streamUrl);

	// Check if BASS is initialized
	if (!BASS_GetVersion()) {
		printf("BASS not initialized - cannot stream\n");
		return 0;
	}

	// Create BASS stream from URL with more options
	printf("Creating BASS stream...\n");
	g_audio.currentStream = BASS_StreamCreateURL(station->streamUrl, 0, 
		BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, NULL, 0);

	if (g_audio.currentStream) {
		printf("Successfully connected to stream: %s\n", station->name);
		
		// Get stream info
		BASS_CHANNELINFO info;
		if (BASS_ChannelGetInfo(g_audio.currentStream, &info)) {
			printf("Stream info: %lu Hz, %lu channels, type=%lu\n", 
				   info.freq, info.chans, info.ctype);
		}
		
		// Set volume based on signal strength and radio volume
		float volume = g_radio.volume * (g_radio.signalStrength / 100.0f);
		BASS_ChannelSetAttribute(g_audio.currentStream, BASS_ATTRIB_VOL, volume);
		printf("Set volume to: %.2f\n", volume);
		
		// Start playing
		if (BASS_ChannelPlay(g_audio.currentStream, FALSE)) {
			printf("Stream playback started\n");
		} else {
			DWORD error = BASS_ErrorGetCode();
			printf("Failed to start playback (BASS Error: %lu)\n", error);
		}
		
		g_audio.currentStation = station;
		return 1;
	} else {
		DWORD error = BASS_ErrorGetCode();
		printf("Failed to connect to stream: %s (BASS Error: %lu)\n", station->name, error);
		printf("BASS Error meanings:\n");
		printf("  1 = BASS_ERROR_MEM (out of memory)\n");
		printf("  2 = BASS_ERROR_FILEOPEN (file/URL cannot be opened)\n");
		printf("  3 = BASS_ERROR_DRIVER (no audio driver available)\n");
		printf("  6 = BASS_ERROR_FORMAT (unsupported format)\n");
		printf("  7 = BASS_ERROR_POSITION (invalid position)\n");
		printf("  14 = BASS_ERROR_DEVICE (invalid device)\n");
		printf("  21 = BASS_ERROR_TIMEOUT (connection timeout)\n");
		printf("  41 = BASS_ERROR_SSL (SSL/HTTPS not supported)\n");
	}

	return 0;
}

void StopBassStreaming() {
	if (g_audio.currentStream) {
		BASS_StreamFree(g_audio.currentStream);
		g_audio.currentStream = 0;
		printf("Stopped streaming\n");
	}

	g_audio.currentStation = NULL;
}

