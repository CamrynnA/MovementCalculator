#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <iomanip>

// Includes your custom game movement logic module calculation parameters
#include "./movement.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

// Global arrays allocated safely with padding spaces to bypass web markup corruption
HWND hLabels[6];
HWND hRegular, hAllied, hNonAllied, hForest, hSea, hBark, hButton, hClearButton, hOutput;
HWND hUpDowns[6];

HFONT hBoldFont = NULL;

// DEEP EMERALD THEME: Crisp jade grid lines on a velvet emerald backdrop
HBRUSH hTextureBrush = CreateHatchBrush(HS_DIAGCROSS, RGB(20, 95, 45));
COLORREF textureBgColor = RGB(10, 60, 30);

int GetWindowInt(HWND hWnd) {
    wchar_t buf[32];
    GetWindowTextW(hWnd, buf, 32);
    std::wstring ws(buf);
    if (ws.empty()) return 0;
    try { return std::stoi(ws); }
    catch (...) { return 0; }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        DeleteObject(hTextureBrush);
        if (hBoldFont) DeleteObject(hBoldFont);
        PostQuitMessage(0);
        return 0;
    }

    // Explicitly paints the emerald backdrop and handles layout redraw optimization passes
    if (uMsg == WM_ERASEBKGND) {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(hwnd, &rect);
        SetBkColor(hdc, textureBgColor);
        FillRect(hdc, &rect, hTextureBrush);
        return 1;
    }

    if (uMsg == WM_SIZE) {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        int leftMargin = 20;
        int spacingY = 32;
        int labelWidth = 160;
        int inputX = leftMargin + labelWidth + 5;

        int spinWidth = 18;
        int editWidth = 47;
        int columnOneRightEdge = inputX + editWidth + spinWidth;

        HWND inputs[6] = { hRegular, hAllied, hNonAllied, hForest, hSea, hBark };
        int currentY = 20;
        for (int i = 0; i < 6; i++) {
            if (hLabels[i]) {
                SetWindowPos(hLabels[i], NULL, leftMargin, currentY, labelWidth, 20, SWP_NOZORDER);
            }
            if (inputs[i]) {
                SetWindowPos(inputs[i], NULL, inputX, currentY, editWidth, 20, SWP_NOZORDER);
            }
            if (hUpDowns[i]) {
                SetWindowPos(hUpDowns[i], NULL, inputX + editWidth, currentY, spinWidth, 20, SWP_NOZORDER);
            }
            currentY += spacingY;
        }

        int btnWidth = 110;
        int buttonsGap = 10;
        if (hButton) SetWindowPos(hButton, NULL, leftMargin, currentY, btnWidth, 30, SWP_NOZORDER);
        if (hClearButton) SetWindowPos(hClearButton, NULL, leftMargin + btnWidth + buttonsGap, currentY, btnWidth, 30, SWP_NOZORDER);

        if (hOutput) {
            int outputX = columnOneRightEdge + 25;
            int outputY = 20;
            int outputWidth = width - outputX - leftMargin;
            int outputHeight = height - (outputY * 2);

            if (outputWidth < 100) outputWidth = 100;
            if (outputHeight < 100) outputHeight = 100;

            SetWindowPos(hOutput, NULL, outputX, outputY, outputWidth, outputHeight, SWP_NOZORDER);
        }

        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    if (uMsg == WM_CTLCOLORSTATIC) {
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == hOutput) {
            HDC hdcOutput = (HDC)wParam;
            SetTextColor(hdcOutput, RGB(0, 0, 0));
            SetBkColor(hdcOutput, RGB(255, 255, 255));
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }
        HDC hdcStatic = (HDC)wParam;

        // DEEP EMERALD THEME: Set clean text coloring to map perfectly onto the green backing canvas
        SetTextColor(hdcStatic, RGB(245, 245, 245));
        SetBkColor(hdcStatic, textureBgColor);
        SetBkMode(hdcStatic, OPAQUE);
        return (INT_PTR)hTextureBrush;
    }

    if (uMsg == WM_COMMAND) {
        if (LOWORD(wParam) == 1) { // Calculate
            int regular = GetWindowInt(hRegular);
            int allied = GetWindowInt(hAllied);
            int nonAllied = GetWindowInt(hNonAllied);
            int forest = GetWindowInt(hForest);
            int sea = GetWindowInt(hSea);
            int bark = GetWindowInt(hBark);

            if (regular < 0 || allied < 0 || nonAllied < 0 || forest < 0 || sea < 0 || bark < 0) {
                MessageBoxW(hwnd, L"Values cannot be negative!", L"Error", MB_ICONERROR);
                return 0;
            }

            Movement move(regular, allied, nonAllied, forest, sea, bark);
            move.calculateMovement();
            float totalCost = move.getCost();

            if (totalCost > 10000.0f) {
                MessageBoxW(hwnd, L"Calculated cost exceeds the limit of 10,000 stamina. Please check your tile counts.", L"Movement Range Limit", MB_ICONWARNING);
                SetWindowTextW(hOutput, L"ERROR: Movement route exceeds the stamina limit (10,000 stamina).");
                return 0;
            }

            std::wstringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << L"TOTAL COST: " << totalCost << L" stamina.\r\n\r\n";

            if (totalCost <= 12.0f) {
                ss << L"Stamina remaining for this movement: " << (12.0f - totalCost) << L".\r\n";
            }
            else {
                ss << L"Movement is too expensive for a single turn.\r\n\r\n";
                float remainingCost = totalCost;
                int movementCount = 1;
                while (remainingCost > 12.0f) {
                    ss << L"Movement " << movementCount << L" uses 12.00 stamina.\r\nArmy exhausted.\r\n\r\n";
                    remainingCost -= 12.0f;
                    movementCount++;
                }
                ss << L"Movement " << movementCount << L" will require the final " << remainingCost << L" stamina.\r\n";
                ss << L"\r\nStamina remaining at final destination: " << (12.0f - remainingCost) << L".\r\n";
            }
            SetWindowTextW(hOutput, ss.str().c_str());
        }
        else if (LOWORD(wParam) == 2) { // Clear
            SetWindowTextW(hRegular, L"0");
            SetWindowTextW(hAllied, L"0");
            SetWindowTextW(hNonAllied, L"0");
            SetWindowTextW(hForest, L"0");
            SetWindowTextW(hSea, L"0");
            SetWindowTextW(hBark, L"0");
            SetWindowTextW(hOutput, L"Available stamina per turn: 12.00");

            for (int i = 0; i < 6; i++) {
                if (hUpDowns[i]) {
                    SendMessageW(hUpDowns[i], UDM_SETPOS32, 0, 0);
                }
            }
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"StaminaCalcWindowClass";
    WNDCLASSW wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hTextureBrush;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Movement Stamina Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 320, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));

    hBoldFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"MS Shell Dlg");

    const wchar_t* labels[] = { L"Regular Tile:", L"Allied Road Tile:", L"Non-Allied Road Tile:", L"Forest Tile:", L"Sea Tile:", L"Disembarks/Embarks:" };
    HWND* inputs[] = { &hRegular, &hAllied, &hNonAllied, &hForest, &hSea, &hBark };

    for (int i = 0; i < 6; i++) {
        hLabels[i] = CreateWindowW(L"STATIC", labels[i], WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, NULL, hInstance, NULL);
        if (hBoldFont) SendMessageW(hLabels[i], WM_SETFONT, (WPARAM)hBoldFont, TRUE);

        *inputs[i] = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0", WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_RIGHT, 0, 0, 0, 0, hwnd, NULL, hInstance, NULL);
        SendMessageW(*inputs[i], EM_SETLIMITTEXT, 3, 0);

        hUpDowns[i] = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
            WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
            0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

        SendMessageW(hUpDowns[i], UDM_SETBUDDY, (WPARAM)*inputs[i], 0);
        SendMessageW(hUpDowns[i], UDM_SETRANGE32, 0, 999);
    }

    hButton = CreateWindowW(L"BUTTON", L"Calculate", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)1, hInstance, NULL);
    hClearButton = CreateWindowW(L"BUTTON", L"Clear", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)2, hInstance, NULL);
    hOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Available stamina per turn: 12.00", WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    if (hBoldFont) {
        SendMessageW(hButton, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
        SendMessageW(hClearButton, WM_SETFONT, (WPARAM)hBoldFont, TRUE);
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    SendMessageW(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
