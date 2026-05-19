#pragma once
#include "../core/NetworkClient.h"
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();
    bool create(HINSTANCE hInstance, const std::string& username, const std::string& serverHost = "127.0.0.1");
    void show();
    
private:
    HWND hwnd;
    std::string currentUser;
    std::string currentContact;
    NetworkClient network;
    long long lastMessageTimestamp = 0;
    
    HWND hwndMessageList;
    HWND hwndMessageInput;
    HWND hwndContactsList;
    HWND hwndSendButton;
    HWND hwndSendFileButton;
    HWND hwndSearchInput;
    HWND hwndSearchButton;
    HWND hwndSearchResults;
    HWND hwndAddContact;
    HWND hwndStatusBar;
    HWND hwndHeader;
    HFONT hFontTitle;
    HFONT hFontNormal;
    HBRUSH hBrushBg;
    HBRUSH hBrushSidebar;
    HBRUSH hBrushChat;
    HBRUSH hBrushInput;
    
    static const UINT TIMER_POLL = 1;
    static const int POLL_INTERVAL_MS = 1000;
    
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    void loadContacts();
    void loadMessages(const std::string& contact);
    void sendMessage();
    void sendFile();
    void pollMessages();
    void syncFromServer(const std::string& contact);
    void pollAllContacts();
    static std::string wideToUtf8(const wchar_t* wstr);
    void updateMessageList();
    void searchUsers();
    void addSelectedContact();
    void updateStatusBar(const std::wstring& text);
    std::wstring formatTimestamp(const std::string& ts);
    void applyTheme();
};
