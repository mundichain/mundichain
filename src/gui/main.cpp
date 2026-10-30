#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "gui/wallet_window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();

    BitMundiGUI::WalletWindow window(hInstance);
    if (!window.Create()) {
        MessageBoxW(NULL, L"Failed to create BitMundi Wallet native window!", L"Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    window.Show(nCmdShow);
    return window.RunMessageLoop();
}
