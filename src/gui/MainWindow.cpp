#include "MainWindow.h"
#include "../core/Database.h"
#include "../core/Logger.h"
#include <commdlg.h>
#include <cstring>
#include <ctime>
#include <sstream>

#define IDC_MESSAGE_LIST    1001
#define IDC_MESSAGE_INPUT   1002
#define IDC_CONTACTS_LIST   1003
#define IDC_SEND_BUTTON     1004
#define IDC_SEND_FILE       1005
#define IDC_SEARCH_INPUT    1006
#define IDC_SEARCH_BUTTON   1007
#define IDC_SEARCH_RESULTS  1008
#define IDC_ADD_CONTACT     1009
#define IDC_STATUS_BAR      1010

#define COLOR_BG         RGB(18, 18, 24)
#define COLOR_SIDEBAR    RGB(25, 25, 33)
#define COLOR_CHAT       RGB(30, 30, 40)
#define COLOR_INPUT      RGB(35, 35, 45)
#define COLOR_ACCENT     RGB(88, 101, 242)
#define COLOR_ACCENT_H   RGB(114, 126, 255)
#define COLOR_TEXT       RGB(229, 229, 234)
#define COLOR_TEXT_MUTED RGB(148, 148, 160)
#define COLOR_MY_MSG     RGB(88, 101, 242)
#define COLOR_THEIR_MSG  RGB(45, 45, 55)

MainWindow::MainWindow() : hwnd(nullptr), hwndMessageList(nullptr),
    hwndMessageInput(nullptr), hwndContactsList(nullptr),
    hwndSendButton(nullptr), hwndSendFileButton(nullptr),
    hwndStatusBar(nullptr), hwndHeader(nullptr), hFontTitle(nullptr), 
    hFontNormal(nullptr), hBrushBg(nullptr), hBrushSidebar(nullptr),
    hBrushChat(nullptr), hBrushInput(nullptr) {}

MainWindow::~MainWindow() {
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontNormal) DeleteObject(hFontNormal);
    if (hBrushBg) DeleteObject(hBrushBg);
    if (hBrushSidebar) DeleteObject(hBrushSidebar);
    if (hBrushChat) DeleteObject(hBrushChat);
    if (hBrushInput) DeleteObject(hBrushInput);
}

void MainWindow::applyTheme() {
    hBrushBg = CreateSolidBrush(COLOR_BG);
    hBrushSidebar = CreateSolidBrush(COLOR_SIDEBAR);
    hBrushChat = CreateSolidBrush(COLOR_CHAT);
    hBrushInput = CreateSolidBrush(COLOR_INPUT);
    hFontTitle = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

bool MainWindow::create(HINSTANCE hInstance, const std::string& username, const std::string& serverHost) {
    currentUser = username;
    Logger::getInstance().log("Creating main window for user: " + username);

    applyTheme();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = hBrushBg;
    wc.lpszClassName = L"FuturaLinkMainWindow";

    RegisterClassExW(&wc);

    std::wstring windowTitle = L"FuturaLink — " + std::wstring(username.begin(), username.end());

    hwnd = CreateWindowExW(
        WS_EX_LAYERED, L"FuturaLinkMainWindow", windowTitle.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1300, 800,
        nullptr, nullptr, hInstance, this
    );
    
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    if (!hwnd) return false;

    auto createLabel = [&](const wchar_t* text, int x, int y, int w, int h, bool title = false) {
        HWND lbl = CreateWindowExW(0, L"STATIC", text, WS_VISIBLE | WS_CHILD,
            x, y, w, h, hwnd, nullptr, hInstance, nullptr);
        SendMessageW(lbl, WM_SETFONT, (WPARAM)(title ? hFontTitle : hFontNormal), TRUE);
        return lbl;
    };

    auto createBtn = [&](const wchar_t* text, int x, int y, int w, int h, int id) {
        HWND btn = CreateWindowExW(0, L"BUTTON", text,
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            x, y, w, h, hwnd, (HMENU)(INT_PTR)id, hInstance, nullptr);
        SendMessageW(btn, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
        return btn;
    };

    createLabel(L"FuturaLink", 290, 12, 200, 35, true);

    hwndSearchInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        20, 55, 180, 35, hwnd, (HMENU)IDC_SEARCH_INPUT, hInstance, nullptr);
    SendMessageW(hwndSearchInput, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    SetWindowLongPtrW(hwndSearchInput, GWLP_USERDATA, (LONG_PTR)CreateSolidBrush(COLOR_SIDEBAR));

    hwndSearchButton = createBtn(L"Search", 210, 55, 70, 35, IDC_SEARCH_BUTTON);
    hwndAddContact = createBtn(L"+ Add", 20, 100, 260, 35, IDC_ADD_CONTACT);

    createLabel(L"Contacts", 20, 150, 200, 25);
    hwndContactsList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        20, 180, 260, 350, hwnd, (HMENU)IDC_CONTACTS_LIST, hInstance, nullptr);
    SendMessageW(hwndContactsList, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    createLabel(L"Search Results", 20, 545, 200, 25);
    hwndSearchResults = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        20, 570, 260, 120, hwnd, (HMENU)IDC_SEARCH_RESULTS, hInstance, nullptr);
    SendMessageW(hwndSearchResults, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    hwndMessageList = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        310, 20, 960, 580, hwnd, (HMENU)IDC_MESSAGE_LIST, hInstance, nullptr);
    SendMessageW(hwndMessageList, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    hwndMessageInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_WANTRETURN,
        310, 610, 850, 100, hwnd, (HMENU)IDC_MESSAGE_INPUT, hInstance, nullptr);
    SendMessageW(hwndMessageInput, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    hwndSendButton = createBtn(L"Send", 1170, 610, 100, 45, IDC_SEND_BUTTON);
    hwndSendFileButton = createBtn(L"File", 1170, 665, 100, 45, IDC_SEND_FILE);

    hwndStatusBar = CreateWindowExW(0, L"STATIC", L"Connecting...",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        310, 725, 960, 30, hwnd, (HMENU)IDC_STATUS_BAR, hInstance, nullptr);
    SendMessageW(hwndStatusBar, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    SetWindowLongPtrW(hwndStatusBar, GWLP_USERDATA, COLOR_TEXT_MUTED);

    auto& db = Database::getInstance();
    db.setCurrentUser(username);

    if (network.connect(serverHost, 8765)) {
        network.registerOnline(username);
        std::wstring status = L"● Connected to " + std::wstring(serverHost.begin(), serverHost.end()) +
            L":8765 — messages delivered in real time";
        updateStatusBar(status);
        SetTimer(hwnd, TIMER_POLL, POLL_INTERVAL_MS, nullptr);
    } else {
        updateStatusBar(L"○ Server unreachable — working offline");
    }

    loadContacts();

    std::wstring welcome = L"      Welcome to FuturaLink!\n\n"
        L"      ● Select a contact to start chatting\n"
        L"      ● Use search to find new users\n"
        L"      ● Press Enter to send, Shift+Enter for new line\n";
    SetWindowTextW(hwndMessageList, welcome.c_str());

    return true;
}

void MainWindow::show() {
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

void MainWindow::updateStatusBar(const std::wstring& text) {
    if (hwndStatusBar) SetWindowTextW(hwndStatusBar, text.c_str());
}

std::string MainWindow::wideToUtf8(const wchar_t* wstr) {
    if (!wstr || !wstr[0]) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], size, nullptr, nullptr);
    return result;
}

std::wstring MainWindow::formatTimestamp(const std::string& ts) {
    try {
        long long ms = std::stoll(ts);
        time_t t = static_cast<time_t>(ms / 1000);
        struct tm timeinfo;
        localtime_s(&timeinfo, &t);
        wchar_t buf[64];
        wcsftime(buf, 64, L"%H:%M", &timeinfo);
        return buf;
    } catch (...) {
        return L"";
    }
}

LRESULT CALLBACK MainWindow::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    pThis = (MainWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (pThis) return pThis->handleMessage(msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;
            LONG_PTR color = GetWindowLongPtrW(hCtrl, GWLP_USERDATA);
            
            if (hCtrl == hwndMessageList || hCtrl == hwndMessageInput) {
                SetTextColor(hdc, COLOR_TEXT);
                SetBkColor(hdc, COLOR_CHAT);
                return (LRESULT)hBrushChat;
            } else if (hCtrl == hwndContactsList || hCtrl == hwndSearchResults) {
                SetTextColor(hdc, COLOR_TEXT);
                SetBkColor(hdc, COLOR_SIDEBAR);
                return (LRESULT)hBrushSidebar;
            } else if (color == COLOR_TEXT_MUTED) {
                SetTextColor(hdc, COLOR_TEXT_MUTED);
                SetBkColor(hdc, COLOR_CHAT);
                return (LRESULT)hBrushChat;
            } else {
                SetTextColor(hdc, COLOR_TEXT);
                SetBkColor(hdc, COLOR_INPUT);
                return (LRESULT)hBrushInput;
            }
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, COLOR_ACCENT);
            return (LRESULT)CreateSolidBrush(COLOR_ACCENT);
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SEND_BUTTON) {
                sendMessage();
            } else if (LOWORD(wParam) == IDC_SEND_FILE) {
                sendFile();
            } else if (LOWORD(wParam) == IDC_SEARCH_BUTTON) {
                searchUsers();
            } else if (LOWORD(wParam) == IDC_ADD_CONTACT) {
                addSelectedContact();
            } else if (LOWORD(wParam) == IDC_CONTACTS_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
                int index = SendMessageW(hwndContactsList, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR) {
                    wchar_t buffer[256];
                    SendMessageW(hwndContactsList, LB_GETTEXT, index, (LPARAM)buffer);
                    currentContact = wideToUtf8(buffer);
                    lastMessageTimestamp = 0;
                    syncFromServer(currentContact);
                    loadMessages(currentContact);
                }
            }
            break;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN && GetFocus() == hwndMessageInput) {
                if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
                    sendMessage();
                    return 0;
                }
            }
            break;

        case WM_TIMER:
            if (wParam == TIMER_POLL) pollAllContacts();
            break;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                SetWindowPos(hwndSearchResults, NULL, 20, height - 190, 260, 120, SWP_NOZORDER);
                SetWindowPos(hwndMessageList, NULL, 310, 20, width - 350, height - 200, SWP_NOZORDER);
                SetWindowPos(hwndMessageInput, NULL, 310, height - 170, width - 460, 90, SWP_NOZORDER);
                SetWindowPos(hwndSendButton, NULL, width - 160, height - 170, 100, 40, SWP_NOZORDER);
                SetWindowPos(hwndSendFileButton, NULL, width - 160, height - 125, 100, 40, SWP_NOZORDER);
                SetWindowPos(hwndStatusBar, NULL, 310, height - 50, width - 350, 30, SWP_NOZORDER);
            }
            break;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_POLL);
            PostQuitMessage(0);
            break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void MainWindow::syncFromServer(const std::string& contact) {
    if (!network.isConnected() || contact.empty()) return;

    auto incoming = network.fetchMessages(currentUser, contact, 0);
    auto& db = Database::getInstance();
    for (const auto& msg : incoming) {
        if (!db.hasMessage(msg)) db.saveMessage(msg);
        if (msg.from != currentUser) db.addContact(msg.from);
        try {
            long long ts = std::stoll(msg.timestamp);
            if (ts > lastMessageTimestamp) lastMessageTimestamp = ts;
        } catch (...) {}
    }
}

void MainWindow::pollMessages() {
    if (!network.isConnected() || currentContact.empty()) return;

    auto incoming = network.fetchMessages(currentUser, currentContact, lastMessageTimestamp);
    if (incoming.empty()) return;

    auto& db = Database::getInstance();
    bool updated = false;
    for (const auto& msg : incoming) {
        if (!db.hasMessage(msg)) {
            db.saveMessage(msg);
            updated = true;
        }
        if (msg.from != currentUser) db.addContact(msg.from);
        try {
            long long ts = std::stoll(msg.timestamp);
            if (ts > lastMessageTimestamp) lastMessageTimestamp = ts;
        } catch (...) {}
    }

    if (updated) loadMessages(currentContact);
}

void MainWindow::pollAllContacts() {
    if (!network.isConnected()) return;

    auto& db = Database::getInstance();
    std::vector<std::string> contacts = db.getContacts();
    if (!currentContact.empty()) {
        bool found = false;
        for (const auto& c : contacts) {
            if (c == currentContact) { found = true; break; }
        }
        if (!found) contacts.push_back(currentContact);
    }

    bool updatedCurrent = false;
    for (const auto& contact : contacts) {
        long long since = (contact == currentContact) ? lastMessageTimestamp : 0;
        auto incoming = network.fetchMessages(currentUser, contact, since);
        for (const auto& msg : incoming) {
            if (!db.hasMessage(msg)) {
                db.saveMessage(msg);
                if (contact == currentContact) updatedCurrent = true;
            }
            if (msg.from != currentUser) db.addContact(msg.from);
            if (contact == currentContact) {
                try {
                    long long ts = std::stoll(msg.timestamp);
                    if (ts > lastMessageTimestamp) lastMessageTimestamp = ts;
                } catch (...) {}
            }
        }
    }

    if (updatedCurrent && !currentContact.empty()) {
        loadMessages(currentContact);
        loadContacts();
    }
}

void MainWindow::searchUsers() {
    wchar_t searchText[256];
    GetWindowTextW(hwndSearchInput, searchText, 255);
    if (wcslen(searchText) == 0) return;

    std::string searchStr = wideToUtf8(searchText);
    SendMessageW(hwndSearchResults, LB_RESETCONTENT, 0, 0);

    auto& db = Database::getInstance();
    for (const auto& user : db.searchUsers(searchStr)) {
        std::wstring wuser(user.begin(), user.end());
        SendMessageW(hwndSearchResults, LB_ADDSTRING, 0, (LPARAM)wuser.c_str());
    }
}

void MainWindow::addSelectedContact() {
    int index = SendMessageW(hwndSearchResults, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR) {
        MessageBoxW(hwnd, L"Select a user from search results", L"FuturaLink", MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t buffer[256];
    SendMessageW(hwndSearchResults, LB_GETTEXT, index, (LPARAM)buffer);
    std::string contact = wideToUtf8(buffer);

    auto& db = Database::getInstance();
    if (db.addContact(contact)) {
        loadContacts();
        SendMessageW(hwndSearchResults, LB_RESETCONTENT, 0, 0);
        SetWindowTextW(hwndSearchInput, L"");
        MessageBoxW(hwnd, L"Contact added!", L"FuturaLink", MB_OK | MB_ICONINFORMATION);
    }
}

void MainWindow::loadContacts() {
    SendMessageW(hwndContactsList, LB_RESETCONTENT, 0, 0);
    auto& db = Database::getInstance();
    for (const auto& contact : db.getContacts()) {
        std::wstring wcontact(contact.begin(), contact.end());
        SendMessageW(hwndContactsList, LB_ADDSTRING, 0, (LPARAM)wcontact.c_str());
    }
}

void MainWindow::loadMessages(const std::string& contact) {
    auto& db = Database::getInstance();
    auto messages = db.getMessages(contact);

    std::wstring displayText;
    for (const auto& msg : messages) {
        bool isMine = (msg.from == currentUser);
        std::wstring content(msg.content.begin(), msg.content.end());
        std::wstring timeStr = formatTimestamp(msg.timestamp);
        
        if (isMine) {
            displayText += L"┌─────────────────────────────────────┐\r\n";
            displayText += L"│  " + content + L"\r\n";
            displayText += L"│  \u001b[90m" + timeStr + L"\u001b[0m\r\n";
            displayText += L"└─────────────────────────────────────┘\r\n\r\n";
        } else {
            std::wstring sender(msg.from.begin(), msg.from.end());
            displayText += L"┌─ " + sender + L" ─────────────────────────────┐\r\n";
            displayText += L"│  " + content + L"\r\n";
            displayText += L"│  \u001b[90m" + timeStr + L"\u001b[0m\r\n";
            displayText += L"└─────────────────────────────────────┘\r\n\r\n";
        }
    }

    SetWindowTextW(hwndMessageList, displayText.c_str());

    int len = GetWindowTextLengthW(hwndMessageList);
    SendMessageW(hwndMessageList, EM_SETSEL, len, len);
    SendMessageW(hwndMessageList, EM_SCROLLCARET, 0, 0);
}

void MainWindow::sendMessage() {
    if (currentContact.empty()) {
        MessageBoxW(hwnd, L"First select a contact", L"FuturaLink", MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t buffer[1001];
    GetWindowTextW(hwndMessageInput, buffer, 1000);
    if (wcslen(buffer) == 0) return;

    std::string msgContent = wideToUtf8(buffer);
    Message msg(currentUser, currentContact, msgContent);

    auto& db = Database::getInstance();
    db.saveMessage(msg);

    if (network.isConnected()) {
        if (!network.sendMessage(msg)) {
            MessageBoxW(hwnd, L"Failed to send message to server", L"FuturaLink", MB_OK | MB_ICONWARNING);
        }
    }

    try {
        long long ts = std::stoll(msg.timestamp);
        if (ts > lastMessageTimestamp) lastMessageTimestamp = ts;
    } catch (...) {}

    SetWindowTextW(hwndMessageInput, L"");
    loadMessages(currentContact);
}

void MainWindow::sendFile() {
    if (currentContact.empty()) {
        MessageBoxW(hwnd, L"First select a contact", L"FuturaLink", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAMEW ofn = {};
    wchar_t fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        std::string fileNameStr = wideToUtf8(fileName);
        size_t slash = fileNameStr.find_last_of("\\/");
        std::string shortNameStr = (slash != std::string::npos) ? fileNameStr.substr(slash + 1) : fileNameStr;

        Message msg(currentUser, currentContact, "[File: " + shortNameStr + "]");
        msg.isFile = true;
        msg.filePath = fileNameStr;

        auto& db = Database::getInstance();
        db.saveMessage(msg);

        if (network.isConnected()) network.sendMessage(msg);

        loadMessages(currentContact);
    }
}

void MainWindow::updateMessageList() {
    if (!currentContact.empty()) loadMessages(currentContact);
}
