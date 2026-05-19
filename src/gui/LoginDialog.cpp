#include "LoginDialog.h"
#include "../core/Database.h"
#include "../core/CryptoManager.h"
#include "../core/Logger.h"

#define COLOR_BG       RGB(18, 18, 24)
#define COLOR_CARD     RGB(30, 30, 40)
#define COLOR_ACCENT   RGB(88, 101, 242)
#define COLOR_ACCENT_H RGB(114, 126, 255)
#define COLOR_TEXT     RGB(229, 229, 234)
#define COLOR_TEXT_SEC RGB(148, 148, 160)

LoginDialog::LoginDialog() : hwnd(nullptr), hwndUsername(nullptr), hwndPassword(nullptr),
    hwndServerHost(nullptr), hFontTitle(nullptr), hFontNormal(nullptr), hBrushBg(nullptr), hBrushCard(nullptr) {}

LoginDialog::~LoginDialog() {
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontNormal) DeleteObject(hFontNormal);
    if (hBrushBg) DeleteObject(hBrushBg);
    if (hBrushCard) DeleteObject(hBrushCard);
}

void LoginDialog::applyTheme() {
    hBrushBg = CreateSolidBrush(COLOR_BG);
    hBrushCard = CreateSolidBrush(COLOR_CARD);
    hFontTitle = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontNormal = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

bool LoginDialog::show(HINSTANCE hInstance) {
    applyTheme();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = LoginDialog::windowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = hBrushBg;
    wc.lpszClassName = L"FuturaLinkLogin";

    RegisterClassExW(&wc);

    hwnd = CreateWindowExW(WS_EX_LAYERED, L"FuturaLinkLogin", L"FuturaLink",
        WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 520,
        nullptr, nullptr, hInstance, this);
    
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    RECT rc;
    GetClientRect(hwnd, &rc);
    
    HWND card = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD,
        20, 20, 420, 440, hwnd, nullptr, hInstance, nullptr);
    SetWindowLongPtrW(card, GWLP_USERDATA, (LONG_PTR)hBrushCard);
    
    HWND title = CreateWindowExW(0, L"STATIC", L"FuturaLink", WS_VISIBLE | WS_CHILD,
        40, 40, 200, 40, card, nullptr, hInstance, nullptr);
    SendMessageW(title, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    SetWindowLongPtrW(title, GWLP_USERDATA, COLOR_ACCENT);
    
    HWND subtitle = CreateWindowExW(0, L"STATIC", L"Modern Messenger", WS_VISIBLE | WS_CHILD,
        40, 85, 200, 25, card, nullptr, hInstance, nullptr);
    SendMessageW(subtitle, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    SetWindowLongPtrW(subtitle, GWLP_USERDATA, COLOR_TEXT_SEC);

    auto createLabel = [&](const wchar_t* text, int y) {
        HWND lbl = CreateWindowExW(0, L"STATIC", text, WS_VISIBLE | WS_CHILD,
            40, y, 150, 25, card, nullptr, hInstance, nullptr);
        SendMessageW(lbl, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
        SetWindowLongPtrW(lbl, GWLP_USERDATA, COLOR_TEXT_SEC);
        return lbl;
    };

    createLabel(L"Username", 140);
    hwndUsername = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        40, 165, 380, 38, card, (HMENU)1001, hInstance, nullptr);
    SendMessageW(hwndUsername, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    createLabel(L"Password", 220);
    hwndPassword = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
        40, 245, 380, 38, card, (HMENU)1002, hInstance, nullptr);
    SendMessageW(hwndPassword, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    createLabel(L"Server Address", 300);
    hwndServerHost = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"127.0.0.1",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        40, 325, 380, 38, card, (HMENU)1003, hInstance, nullptr);
    SendMessageW(hwndServerHost, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    HWND btnLogin = CreateWindowExW(0, L"BUTTON", L"Sign In",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        40, 395, 180, 44, card, (HMENU)1, hInstance, nullptr);
    SendMessageW(btnLogin, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    HWND btnReg = CreateWindowExW(0, L"BUTTON", L"Create Account",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        240, 395, 180, 44, card, (HMENU)2, hInstance, nullptr);
    SendMessageW(btnReg, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return !loggedInUser.empty();
}

LRESULT CALLBACK LoginDialog::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LoginDialog* pThis = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
        pThis = (LoginDialog*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    pThis = (LoginDialog*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (pThis) return pThis->handleMessage(msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT LoginDialog::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;
            LONG_PTR color = GetWindowLongPtrW(hCtrl, GWLP_USERDATA);
            if (color == COLOR_ACCENT) {
                SetTextColor(hdc, COLOR_ACCENT);
            } else if (color == COLOR_TEXT_SEC) {
                SetTextColor(hdc, COLOR_TEXT_SEC);
            } else {
                SetTextColor(hdc, COLOR_TEXT);
            }
            SetBkColor(hdc, COLOR_CARD);
            return (LRESULT)hBrushCard;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, COLOR_TEXT);
            SetBkColor(hdc, COLOR_CARD);
            return (LRESULT)hBrushCard;
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, COLOR_ACCENT);
            return (LRESULT)CreateSolidBrush(COLOR_ACCENT);
        }
        case WM_COMMAND: {
            wchar_t username[256] = {}, password[256] = {}, host[256] = {};
            GetWindowTextW(hwndUsername, username, 255);
            GetWindowTextW(hwndPassword, password, 255);
            GetWindowTextW(hwndServerHost, host, 255);

            std::string usernamestr(username, username + wcslen(username));
            std::string passwordstr(password, password + wcslen(password));
            serverHost = std::string(host, host + wcslen(host));
            if (serverHost.empty()) serverHost = "127.0.0.1";

            if (LOWORD(wParam) == 1) {
                if (login(usernamestr, passwordstr)) {
                    DestroyWindow(hwnd);
                    PostQuitMessage(0);
                } else {
                    MessageBoxW(hwnd, L"Invalid username or password", L"FuturaLink", MB_OK | MB_ICONERROR);
                }
            } else if (LOWORD(wParam) == 2) {
                if (registerUser(usernamestr, passwordstr)) {
                    MessageBoxW(hwnd, L"Account created successfully! Please sign in.", L"FuturaLink", MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxW(hwnd, L"Registration failed (username may be taken)", L"FuturaLink", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool LoginDialog::login(const std::string& username, const std::string& password) {
    auto& db = Database::getInstance();
    if (!db.init("testkey")) db.init("testkey");

    User* user = db.getUser(username);
    if (user && user->passwordHash == CryptoManager::hashPassword(password)) {
        loggedInUser = username;
        db.setCurrentUser(username);
        Logger::getInstance().log("Login successful for user: " + username);
        return true;
    }
    return false;
}

bool LoginDialog::registerUser(const std::string& username, const std::string& password) {
    auto& db = Database::getInstance();
    db.init("testkey");

    if (username.empty() || password.empty()) return false;
    if (db.getUser(username) != nullptr) return false;

    User newUser(username, CryptoManager::hashPassword(password));
    if (db.saveUser(newUser)) {
        Logger::getInstance().log("User registered: " + username);
        return true;
    }
    return false;
}
