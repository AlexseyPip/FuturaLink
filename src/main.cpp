#include "gui/LoginDialog.h"
#include "gui/MainWindow.h"
#include "core/Logger.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Logger::getInstance().log("Application started");
    
    LoginDialog loginDialog;
    if (!loginDialog.show(hInstance)) {
        Logger::getInstance().log("Login cancelled or failed");
        return 0;
    }
    
    Logger::getInstance().log("User logged in: " + loginDialog.getLoggedInUser());
    
    MainWindow mainWindow;
    if (!mainWindow.create(hInstance, loginDialog.getLoggedInUser(), loginDialog.getServerHost())) {
        Logger::getInstance().error("Failed to create main window");
        MessageBoxA(nullptr, "Failed to create main window!", "Error", MB_OK);
        return 1;
    }
    
    mainWindow.show();
    Logger::getInstance().log("Main window shown");
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    Logger::getInstance().log("Application exiting");
    return msg.wParam;
}
