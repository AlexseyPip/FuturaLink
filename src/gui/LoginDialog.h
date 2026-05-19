#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>

class LoginDialog {
public:
    LoginDialog();
    ~LoginDialog();
    bool show(HINSTANCE hInstance);
    std::string getLoggedInUser() const { return loggedInUser; }
    std::string getServerHost() const { return serverHost; }
    
private:
    std::string loggedInUser;
    std::string serverHost;
    HWND hwnd;
    HWND hwndUsername;
    HWND hwndPassword;
    HWND hwndServerHost;
    HFONT hFontTitle;
    HFONT hFontNormal;
    HBRUSH hBrushBg;
    HBRUSH hBrushCard;
    
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    bool login(const std::string& username, const std::string& password);
    bool registerUser(const std::string& username, const std::string& password);
    void applyTheme();
};
