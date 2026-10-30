#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include "gui/wallet_window.h"
#include "gui/rpc_client.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <iomanip>
#include <sstream>
#include <iostream>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

#define WM_BITMUNDI_UPDATE (WM_APP + 1)
#define ID_SIDEBAR_BTN_BASE 2000
#define ID_BTN_SEND_TX 2100
#define ID_BTN_COPY_RECV_ADDR 2101
#define ID_BTN_GEN_NEW_ADDR 2102
#define ID_BTN_TOGGLE_MINING 2103
#define ID_BTN_INSTANT_MINE 2104
#define ID_BTN_LOAD_WALLET 2105
#define ID_BTN_CREATE_WALLET 2106
#define ID_BTN_BACKUP_WALLET 2107
#define ID_BTN_DUMP_PRIVKEY 2108
#define ID_BTN_COPY_HEX_KEY 2109
#define ID_BTN_COPY_WIF_KEY 2110
#define ID_BTN_IMPORT_PRIVKEY 2111
#define ID_BTN_SET_PASSWORD 2112
#define ID_BTN_CONSOLE_EXEC 2113

namespace BitMundiGUI {

WalletWindow::WalletWindow(HINSTANCE hInstance)
    : m_hInstance(hInstance) {
    m_hBrBg = CreateSolidBrush(RGB(11, 14, 20));
    m_hBrSidebar = CreateSolidBrush(RGB(16, 20, 29));
    m_hBrCard = CreateSolidBrush(RGB(22, 27, 38));

    m_hFontRegular = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hFontBold = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hFontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hFontBigNum = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    m_hFontMono = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
}

WalletWindow::~WalletWindow() {
    StopBackgroundPoller();

    DeleteObject(m_hBrBg);
    DeleteObject(m_hBrSidebar);
    DeleteObject(m_hBrCard);

    DeleteObject(m_hFontRegular);
    DeleteObject(m_hFontBold);
    DeleteObject(m_hFontTitle);
    DeleteObject(m_hFontBigNum);
    DeleteObject(m_hFontMono);
}

bool WalletWindow::Create() {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WalletWindow::WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = m_hBrBg;
    wc.lpszClassName = L"BitMundiNativeWalletClass";

    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        0,
        L"BitMundiNativeWalletClass",
        L"BitMundi Core - Institutional C++20 Node & Native Wallet (BMD)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1180, 800,
        NULL, NULL, m_hInstance, this
    );

    if (!m_hwnd) return false;

    // Windows 10/11 Dark Titlebar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &darkMode, sizeof(darkMode));

    InitControls();
    SwitchTab(TAB_OVERVIEW);
    StartBackgroundPoller();

    return true;
}

void WalletWindow::Show(int nCmdShow) {
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

int WalletWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WalletWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WalletWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        pThis = (WalletWindow*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (WalletWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT WalletWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= ID_SIDEBAR_BTN_BASE && id < ID_SIDEBAR_BTN_BASE + TAB_COUNT) {
            SwitchTab(static_cast<TabIndex>(id - ID_SIDEBAR_BTN_BASE));
            return 0;
        }

        switch (id) {
        case ID_BTN_SEND_TX: DoSendTransaction(); break;
        case ID_BTN_COPY_RECV_ADDR: {
            wchar_t buf[256];
            GetWindowTextW(m_txtRecvAddr, buf, 256);
            std::wstring ws(buf);
            CopyToClipboard(std::string(ws.begin(), ws.end()));
            MessageBoxW(m_hwnd, L"Address copied to Windows clipboard!", L"BitMundi", MB_OK | MB_ICONINFORMATION);
            break;
        }
        case ID_BTN_GEN_NEW_ADDR: DoGenerateAddress(); break;
        case ID_BTN_TOGGLE_MINING: DoToggleMining(); break;
        case ID_BTN_INSTANT_MINE: DoInstantMine(); break;
        case ID_BTN_LOAD_WALLET: DoLoadWallet(); break;
        case ID_BTN_CREATE_WALLET: DoCreateWallet(); break;
        case ID_BTN_BACKUP_WALLET: DoBackupWallet(); break;
        case ID_BTN_DUMP_PRIVKEY: DoDumpPrivKey(); break;
        case ID_BTN_COPY_HEX_KEY: {
            wchar_t buf[256];
            GetWindowTextW(m_txtSecHexKey, buf, 256);
            std::wstring ws(buf);
            CopyToClipboard(std::string(ws.begin(), ws.end()));
            MessageBoxW(m_hwnd, L"HEX Private Key copied to clipboard!", L"BitMundi", MB_OK | MB_ICONINFORMATION);
            break;
        }
        case ID_BTN_COPY_WIF_KEY: {
            wchar_t buf[256];
            GetWindowTextW(m_txtSecWifKey, buf, 256);
            std::wstring ws(buf);
            CopyToClipboard(std::string(ws.begin(), ws.end()));
            MessageBoxW(m_hwnd, L"WIF Private Key copied to clipboard!", L"BitMundi", MB_OK | MB_ICONINFORMATION);
            break;
        }
        case ID_BTN_IMPORT_PRIVKEY: DoImportPrivKey(); break;
        case ID_BTN_CONSOLE_EXEC: {
            wchar_t buf[512];
            GetWindowTextW(m_txtConsoleInput, buf, 512);
            std::wstring ws(buf);
            std::string cmd(ws.begin(), ws.end());
            ExecuteRpcConsole(cmd);
            SetWindowTextW(m_txtConsoleInput, L"");
            break;
        }
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(243, 244, 246));
        SetBkColor(hdc, RGB(22, 27, 38));
        return (LRESULT)m_hBrCard;
    }

    case WM_BITMUNDI_UPDATE:
        UpdateUIWithData();
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WalletWindow::InitControls() {
    CreateSidebar();

    for (int i = 0; i < TAB_COUNT; ++i) {
        m_tabPanels[i] = CreateWindowExW(
            0, L"STATIC", NULL,
            WS_CHILD | WS_VISIBLE,
            230, 10, 930, 740,
            m_hwnd, NULL, m_hInstance, NULL
        );
    }

    CreateOverviewTab();
    CreateSendTab();
    CreateReceiveTab();
    CreateTransactionsTab();
    CreateMiningTab();
    CreateWalletsTab();
    CreateSecurityTab();
    CreateConsoleTab();
}

void WalletWindow::CreateSidebar() {
    m_hwndSidebar = CreateWindowExW(
        0, L"STATIC", NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, 220, 800,
        m_hwnd, NULL, m_hInstance, NULL
    );

    HWND lblLogo = CreateWindowExW(
        0, L"STATIC", L"BITMUNDI CORE",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 20, 200, 30,
        m_hwndSidebar, NULL, m_hInstance, NULL
    );
    SendMessage(lblLogo, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblSubtitle = CreateWindowExW(
        0, L"STATIC", L"Native C++20 Standard",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 50, 200, 20,
        m_hwndSidebar, NULL, m_hInstance, NULL
    );
    SendMessage(lblSubtitle, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    const wchar_t* tabTitles[TAB_COUNT] = {
        L" Overview",
        L" Send BMD",
        L" Receive BMD",
        L" Transactions",
        L" Mining",
        L" Wallets",
        L" Security / Keys",
        L" RPC Console"
    };

    int y = 90;
    for (int i = 0; i < TAB_COUNT; ++i) {
        m_sidebarBtns[i] = CreateWindowExW(
            0, L"BUTTON", tabTitles[i],
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            12, y, 196, 42,
            m_hwndSidebar, (HMENU)(INT_PTR)(ID_SIDEBAR_BTN_BASE + i), m_hInstance, NULL
        );
        SendMessage(m_sidebarBtns[i], WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
        y += 48;
    }
}

void WalletWindow::SwitchTab(TabIndex tab) {
    m_currentTab = tab;
    for (int i = 0; i < TAB_COUNT; ++i) {
        ShowWindow(m_tabPanels[i], (i == tab) ? SW_SHOW : SW_HIDE);
    }
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void WalletWindow::CreateOverviewTab() {
    HWND panel = m_tabPanels[TAB_OVERVIEW];

    HWND lblCardTitle = CreateWindowExW(0, L"STATIC", L"BALANCE SUMMARY", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblCardTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblMatureTitle = CreateWindowExW(0, L"STATIC", L"Available (Mature) Balance:", WS_CHILD | WS_VISIBLE, 20, 55, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblMatureTitle, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_lblMatureBal = CreateWindowExW(0, L"STATIC", L"0.00000000 BMD", WS_CHILD | WS_VISIBLE, 20, 75, 450, 40, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblMatureBal, WM_SETFONT, (WPARAM)m_hFontBigNum, TRUE);

    HWND lblImmatureTitle = CreateWindowExW(0, L"STATIC", L"Pending / Immature Mining Reward:", WS_CHILD | WS_VISIBLE, 20, 130, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblImmatureTitle, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_lblImmatureBal = CreateWindowExW(0, L"STATIC", L"0.00000000 BMD", WS_CHILD | WS_VISIBLE, 20, 150, 400, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblImmatureBal, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblTotalTitle = CreateWindowExW(0, L"STATIC", L"Total Wallet Balance:", WS_CHILD | WS_VISIBLE, 20, 195, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTotalTitle, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_lblTotalBal = CreateWindowExW(0, L"STATIC", L"0.00000000 BMD", WS_CHILD | WS_VISIBLE, 20, 215, 400, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblTotalBal, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    // Blockchain stats
    m_lblHeight = CreateWindowExW(0, L"STATIC", L"Blocks: 0", WS_CHILD | WS_VISIBLE, 520, 75, 380, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblHeight, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_lblDiff = CreateWindowExW(0, L"STATIC", L"Difficulty: 1.0000", WS_CHILD | WS_VISIBLE, 520, 110, 380, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblDiff, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Recent activity
    HWND lblActTitle = CreateWindowExW(0, L"STATIC", L"RECENT ACTIVITY (TRANSFER STREAM)", WS_CHILD | WS_VISIBLE, 20, 270, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblActTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    m_lblRecentTxs = CreateWindowExW(0, L"EDIT", L"Connecting to node...", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 20, 305, 880, 410, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblRecentTxs, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);
}

void WalletWindow::CreateSendTab() {
    HWND panel = m_tabPanels[TAB_SEND];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"SEND BITMUNDI (BMD)", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblTo = CreateWindowExW(0, L"STATIC", L"Pay To (Destination Address):", WS_CHILD | WS_VISIBLE, 20, 60, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTo, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
    m_txtSendTo = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 85, 880, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtSendTo, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND lblAmt = CreateWindowExW(0, L"STATIC", L"Amount (BMD):", WS_CHILD | WS_VISIBLE, 20, 130, 200, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblAmt, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
    m_txtSendAmount = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0.00000000", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 155, 300, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtSendAmount, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND lblFee = CreateWindowExW(0, L"STATIC", L"Miner Fee (BMD):", WS_CHILD | WS_VISIBLE, 340, 130, 200, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblFee, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
    m_txtSendFee = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0.00010000", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 340, 155, 300, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtSendFee, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND btnSend = CreateWindowExW(0, L"BUTTON", L"SUBMIT TRANSACTION", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 210, 260, 44, panel, (HMENU)ID_BTN_SEND_TX, m_hInstance, NULL);
    SendMessage(btnSend, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_lblSendStatus = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 270, 880, 80, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblSendStatus, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
}

void WalletWindow::CreateReceiveTab() {
    HWND panel = m_tabPanels[TAB_RECEIVE];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"RECEIVE BITMUNDI (BMD)", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblAddr = CreateWindowExW(0, L"STATIC", L"Your Active BitMundi Address:", WS_CHILD | WS_VISIBLE, 20, 60, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblAddr, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_txtRecvAddr = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL, 20, 85, 700, 32, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtRecvAddr, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    HWND btnCopy = CreateWindowExW(0, L"BUTTON", L"COPY ADDRESS", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 730, 84, 170, 34, panel, (HMENU)ID_BTN_COPY_RECV_ADDR, m_hInstance, NULL);
    SendMessage(btnCopy, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND btnGen = CreateWindowExW(0, L"BUTTON", L"+ GENERATE NEW ADDRESS", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 140, 250, 40, panel, (HMENU)ID_BTN_GEN_NEW_ADDR, m_hInstance, NULL);
    SendMessage(btnGen, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblPub = CreateWindowExW(0, L"STATIC", L"Public Key:", WS_CHILD | WS_VISIBLE, 20, 200, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblPub, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_txtRecvPubkey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL, 20, 225, 880, 32, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtRecvPubkey, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);
}

void WalletWindow::CreateTransactionsTab() {
    HWND panel = m_tabPanels[TAB_TRANSACTIONS];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"TRANSACTION HISTORY", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    m_lvTransactions = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_VSCROLL,
        20, 55, 880, 660,
        panel, NULL, m_hInstance, NULL
    );
    SendMessage(m_lvTransactions, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    ListView_SetExtendedListViewStyle(m_lvTransactions, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.pszText = (LPWSTR)L"Status";
    col.cx = 120;
    ListView_InsertColumn(m_lvTransactions, 0, &col);

    col.pszText = (LPWSTR)L"Category";
    col.cx = 90;
    ListView_InsertColumn(m_lvTransactions, 1, &col);

    col.pszText = (LPWSTR)L"Amount (BMD)";
    col.cx = 140;
    ListView_InsertColumn(m_lvTransactions, 2, &col);

    col.pszText = (LPWSTR)L"Confs";
    col.cx = 70;
    ListView_InsertColumn(m_lvTransactions, 3, &col);

    col.pszText = (LPWSTR)L"Address";
    col.cx = 260;
    ListView_InsertColumn(m_lvTransactions, 4, &col);

    col.pszText = (LPWSTR)L"TxID";
    col.cx = 180;
    ListView_InsertColumn(m_lvTransactions, 5, &col);
}

void WalletWindow::CreateMiningTab() {
    HWND panel = m_tabPanels[TAB_MINING];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"PROOF OF WORK CPU MINER", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblThreads = CreateWindowExW(0, L"STATIC", L"CPU Mining Threads:", WS_CHILD | WS_VISIBLE, 20, 60, 200, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblThreads, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_txtMiningThreads = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"4", WS_CHILD | WS_VISIBLE | ES_NUMBER, 20, 85, 120, 32, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtMiningThreads, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_btnMiningToggle = CreateWindowExW(0, L"BUTTON", L"START MINING", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 160, 84, 180, 34, panel, (HMENU)ID_BTN_TOGGLE_MINING, m_hInstance, NULL);
    SendMessage(m_btnMiningToggle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND btnInstant = CreateWindowExW(0, L"BUTTON", L"INSTANT MINE 1 BLOCK", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 84, 220, 34, panel, (HMENU)ID_BTN_INSTANT_MINE, m_hInstance, NULL);
    SendMessage(btnInstant, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_lblMiningHashrate = CreateWindowExW(0, L"STATIC", L"Hashrate: 0 H/s", WS_CHILD | WS_VISIBLE, 20, 140, 300, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblMiningHashrate, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    m_lblMiningBlocks = CreateWindowExW(0, L"STATIC", L"Total Blocks Mined: 0", WS_CHILD | WS_VISIBLE, 340, 140, 300, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblMiningBlocks, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    HWND lblLog = CreateWindowExW(0, L"STATIC", L"Mining Activity Output:", WS_CHILD | WS_VISIBLE, 20, 180, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblLog, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_txtMiningLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 20, 205, 880, 500, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtMiningLog, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);
}

void WalletWindow::CreateWalletsTab() {
    HWND panel = m_tabPanels[TAB_WALLETS];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"MULTI-WALLET MANAGER", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    m_lblActiveWallet = CreateWindowExW(0, L"STATIC", L"Active Wallet: wallet.dat", WS_CHILD | WS_VISIBLE, 20, 60, 500, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblActiveWallet, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblAvail = CreateWindowExW(0, L"STATIC", L"Available Wallet Files (.dat):", WS_CHILD | WS_VISIBLE, 20, 95, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblAvail, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_listWallets = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 20, 120, 350, 200, panel, NULL, m_hInstance, NULL);
    SendMessage(m_listWallets, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND btnLoad = CreateWindowExW(0, L"BUTTON", L"LOAD SELECTED WALLET", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 390, 120, 230, 38, panel, (HMENU)ID_BTN_LOAD_WALLET, m_hInstance, NULL);
    SendMessage(btnLoad, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND btnBackup = CreateWindowExW(0, L"BUTTON", L"BACKUP ACTIVE WALLET", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 390, 170, 230, 38, panel, (HMENU)ID_BTN_BACKUP_WALLET, m_hInstance, NULL);
    SendMessage(btnBackup, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblNew = CreateWindowExW(0, L"STATIC", L"Create New Wallet Name:", WS_CHILD | WS_VISIBLE, 20, 340, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblNew, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_txtNewWalletName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"savings.dat", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 365, 350, 32, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtNewWalletName, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND btnCreate = CreateWindowExW(0, L"BUTTON", L"CREATE WALLET", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 390, 364, 230, 34, panel, (HMENU)ID_BTN_CREATE_WALLET, m_hInstance, NULL);
    SendMessage(btnCreate, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
}

void WalletWindow::CreateSecurityTab() {
    HWND panel = m_tabPanels[TAB_SECURITY];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"PRIVATE KEY & SECURITY", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    // Export section
    HWND lblExpTitle = CreateWindowExW(0, L"STATIC", L"EXPORT ACTIVE PRIVATE KEY (HEX & WIF):", WS_CHILD | WS_VISIBLE, 20, 55, 400, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblExpTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblPass = CreateWindowExW(0, L"STATIC", L"Wallet Passphrase (if encrypted):", WS_CHILD | WS_VISIBLE, 20, 85, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblPass, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    m_txtSecPassphrase = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL, 20, 105, 350, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtSecPassphrase, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND btnDump = CreateWindowExW(0, L"BUTTON", L"REVEAL KEYS", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 390, 104, 160, 32, panel, (HMENU)ID_BTN_DUMP_PRIVKEY, m_hInstance, NULL);
    SendMessage(btnDump, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblHex = CreateWindowExW(0, L"STATIC", L"Private Key (64-character HEX):", WS_CHILD | WS_VISIBLE, 20, 145, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblHex, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_txtSecHexKey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL, 20, 168, 680, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtSecHexKey, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    HWND btnCopyHex = CreateWindowExW(0, L"BUTTON", L"COPY HEX", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 715, 167, 140, 32, panel, (HMENU)ID_BTN_COPY_HEX_KEY, m_hInstance, NULL);
    SendMessage(btnCopyHex, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblWif = CreateWindowExW(0, L"STATIC", L"Private Key (Base58Check WIF):", WS_CHILD | WS_VISIBLE, 20, 210, 300, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblWif, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_txtSecWifKey = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL, 20, 233, 680, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtSecWifKey, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    HWND btnCopyWif = CreateWindowExW(0, L"BUTTON", L"COPY WIF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 715, 232, 140, 32, panel, (HMENU)ID_BTN_COPY_WIF_KEY, m_hInstance, NULL);
    SendMessage(btnCopyWif, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Import section
    HWND lblImpTitle = CreateWindowExW(0, L"STATIC", L"IMPORT PRIVATE KEY (HEX OR WIF):", WS_CHILD | WS_VISIBLE, 20, 290, 400, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblImpTitle, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND lblImpKey = CreateWindowExW(0, L"STATIC", L"Key String (HEX or WIF):", WS_CHILD | WS_VISIBLE, 20, 320, 250, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblImpKey, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_txtImportKeyInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 342, 680, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtImportKeyInput, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    HWND lblImpLbl = CreateWindowExW(0, L"STATIC", L"Label:", WS_CHILD | WS_VISIBLE, 20, 380, 100, 20, panel, NULL, m_hInstance, NULL);
    SendMessage(lblImpLbl, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);
    m_txtImportKeyLabel = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Imported Key", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 402, 300, 30, panel, NULL, m_hInstance, NULL);
    SendMessage(m_txtImportKeyLabel, WM_SETFONT, (WPARAM)m_hFontRegular, TRUE);

    HWND btnImport = CreateWindowExW(0, L"BUTTON", L"IMPORT KEY", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 340, 401, 180, 32, panel, (HMENU)ID_BTN_IMPORT_PRIVKEY, m_hInstance, NULL);
    SendMessage(btnImport, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_lblSecStatus = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 460, 850, 60, panel, NULL, m_hInstance, NULL);
    SendMessage(m_lblSecStatus, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
}

void WalletWindow::CreateConsoleTab() {
    HWND panel = m_tabPanels[TAB_CONSOLE];

    HWND lblTitle = CreateWindowExW(0, L"STATIC", L"BITCOIN CORE RPC TERMINAL", WS_CHILD | WS_VISIBLE, 20, 15, 400, 25, panel, NULL, m_hInstance, NULL);
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    m_txtConsoleOutput = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT",
        L"MundiChain Core Console [v1.5.0 Native C++]\r\nType 'getinfo', 'getmininginfo', 'help'...\r\n",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        20, 50, 880, 620, panel, NULL, m_hInstance, NULL
    );
    SendMessage(m_txtConsoleOutput, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    m_txtConsoleInput = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"getinfo",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        20, 680, 720, 32, panel, NULL, m_hInstance, NULL
    );
    SendMessage(m_txtConsoleInput, WM_SETFONT, (WPARAM)m_hFontMono, TRUE);

    HWND btnExec = CreateWindowExW(
        0, L"BUTTON", L"EXECUTE",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        750, 679, 150, 34, panel, (HMENU)ID_BTN_CONSOLE_EXEC, m_hInstance, NULL
    );
    SendMessage(btnExec, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
}

void WalletWindow::StartBackgroundPoller() {
    m_pollingActive = true;
    m_pollThread = std::thread(&WalletWindow::PollDaemon, this);
}

void WalletWindow::StopBackgroundPoller() {
    m_pollingActive = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
}

void WalletWindow::PollDaemon() {
    while (m_pollingActive) {
        try {
            m_lastInfo = RpcClient::Call("getinfo");
            m_lastWalletInfo = RpcClient::Call("getwalletinfo");
            m_lastMiningInfo = RpcClient::Call("getmininginfo");
            m_lastTransactions = RpcClient::Call("listtransactions");

            if (m_hwnd) {
                PostMessage(m_hwnd, WM_BITMUNDI_UPDATE, 0, 0);
            }
        } catch (...) {
            // Node might be starting or busy
        }

        for (int i = 0; i < 10 && m_pollingActive; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void WalletWindow::UpdateUIWithData() {
    if (!m_lastWalletInfo.empty()) {
        double mature = m_lastWalletInfo.value("balance", 0.0);
        double immature = m_lastWalletInfo.value("immature", 0.0);
        double total = m_lastWalletInfo.value("total", 0.0);
        std::string addr = m_lastWalletInfo.value("address", "");

        std::wstringstream ssMat, ssImm, ssTot;
        ssMat << std::fixed << std::setprecision(8) << mature << L" BMD";
        ssImm << std::fixed << std::setprecision(8) << immature << L" BMD";
        ssTot << std::fixed << std::setprecision(8) << total << L" BMD";

        SetWindowTextW(m_lblMatureBal, ssMat.str().c_str());
        SetWindowTextW(m_lblImmatureBal, ssImm.str().c_str());
        SetWindowTextW(m_lblTotalBal, ssTot.str().c_str());

        std::wstring wAddr(addr.begin(), addr.end());
        SetWindowTextW(m_txtRecvAddr, wAddr.c_str());
    }

    if (!m_lastInfo.empty()) {
        int blocks = m_lastInfo.value("blocks", 0);
        double diff = m_lastInfo.value("difficulty", 1.0);

        std::wstringstream ssH, ssD;
        ssH << L"Blockchain Height: #" << blocks;
        ssD << L"Network Difficulty: " << std::fixed << std::setprecision(4) << diff;

        SetWindowTextW(m_lblHeight, ssH.str().c_str());
        SetWindowTextW(m_lblDiff, ssD.str().c_str());
    }

    if (!m_lastMiningInfo.empty()) {
        int64_t hr = m_lastMiningInfo.value("hashrate", (int64_t)0);
        int64_t blks = m_lastMiningInfo.value("blocks_mined", (int64_t)0);
        bool active = m_lastMiningInfo.value("active", false);

        std::wstringstream ssHr, ssBlk;
        ssHr << L"Hashrate: " << hr << L" H/s";
        ssBlk << L"Total Blocks Mined: " << blks;

        SetWindowTextW(m_lblMiningHashrate, ssHr.str().c_str());
        SetWindowTextW(m_lblMiningBlocks, ssBlk.str().c_str());
        SetWindowTextW(m_btnMiningToggle, active ? L"STOP MINING" : L"START MINING");
    }

    // Update transactions ListView
    if (!m_lastTransactions.empty() && m_lastTransactions.is_array()) {
        ListView_DeleteAllItems(m_lvTransactions);
        int itemIndex = 0;
        std::wstringstream ssRecent;

        for (const auto& tx : m_lastTransactions) {
            std::string status = tx.value("status", "");
            std::string category = tx.value("category", "");
            double amount = tx.value("amount", 0.0);
            int confs = tx.value("confirmations", 0);
            std::string dest = tx.value("address", "");
            std::string txid = tx.value("txid", "");

            std::wstring wStatus(status.begin(), status.end());
            std::wstring wCategory(category.begin(), category.end());
            std::wstring wDest(dest.begin(), dest.end());
            std::wstring wTxid(txid.begin(), txid.end());

            std::wstringstream ssAmt;
            ssAmt << std::fixed << std::setprecision(8) << amount;

            LVITEMW lvi{};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = itemIndex;
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)wStatus.c_str();
            ListView_InsertItem(m_lvTransactions, &lvi);

            ListView_SetItemText(m_lvTransactions, itemIndex, 1, (LPWSTR)wCategory.c_str());
            ListView_SetItemText(m_lvTransactions, itemIndex, 2, (LPWSTR)ssAmt.str().c_str());
            ListView_SetItemText(m_lvTransactions, itemIndex, 3, (LPWSTR)std::to_wstring(confs).c_str());
            ListView_SetItemText(m_lvTransactions, itemIndex, 4, (LPWSTR)wDest.c_str());
            ListView_SetItemText(m_lvTransactions, itemIndex, 5, (LPWSTR)wTxid.c_str());

            if (itemIndex < 10) {
                ssRecent << L"[" << wStatus << L"] " << wCategory << L"  "
                         << ssAmt.str() << L" BMD  (TxID: " << wTxid.substr(0, 16) << L"...)\r\n";
            }

            itemIndex++;
        }

        SetWindowTextW(m_lblRecentTxs, ssRecent.str().c_str());
    }
}

void WalletWindow::CopyToClipboard(const std::string& text) {
    if (!OpenClipboard(m_hwnd)) return;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), text.length() + 1);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_TEXT, hGlob);
    }
    CloseClipboard();
}

void WalletWindow::DoSendTransaction() {
    wchar_t wTo[256], wAmt[64], wFee[64];
    GetWindowTextW(m_txtSendTo, wTo, 256);
    GetWindowTextW(m_txtSendAmount, wAmt, 64);
    GetWindowTextW(m_txtSendFee, wFee, 64);

    std::wstring wsTo(wTo), wsAmt(wAmt), wsFee(wFee);
    std::string to(wsTo.begin(), wsTo.end());
    double amt = std::stod(std::string(wsAmt.begin(), wsAmt.end()));
    double fee = std::stod(std::string(wsFee.begin(), wsFee.end()));

    try {
        nlohmann::json params = { to, amt, fee };
        nlohmann::json txid = RpcClient::Call("sendtoaddress", params);
        std::string sTxid = txid.get<std::string>();
        std::wstring wsResult = L"Success! Transaction broadcast to mempool.\nTxID: " + std::wstring(sTxid.begin(), sTxid.end());
        SetWindowTextW(m_lblSendStatus, wsResult.c_str());
        MessageBoxW(m_hwnd, wsResult.c_str(), L"Transaction Sent", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        std::string err = e.what();
        std::wstring wErr(err.begin(), err.end());
        SetWindowTextW(m_lblSendStatus, (L"Error: " + wErr).c_str());
        MessageBoxW(m_hwnd, wErr.c_str(), L"Transaction Failed", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoGenerateAddress() {
    try {
        nlohmann::json res = RpcClient::Call("getnewaddress", {"Generated via GUI"});
        std::string addr = res.get<std::string>();
        std::wstring wAddr(addr.begin(), addr.end());
        SetWindowTextW(m_txtRecvAddr, wAddr.c_str());
        MessageBoxW(m_hwnd, (L"New Address Generated:\n" + wAddr).c_str(), L"Address Ready", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        std::string err = e.what();
        MessageBoxA(m_hwnd, err.c_str(), "Error", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoDumpPrivKey() {
    wchar_t wPass[256];
    GetWindowTextW(m_txtSecPassphrase, wPass, 256);
    std::wstring wsPass(wPass);
    std::string pass(wsPass.begin(), wsPass.end());

    std::string activeAddr;
    if (!m_lastWalletInfo.empty()) {
        activeAddr = m_lastWalletInfo.value("address", "");
    }

    try {
        nlohmann::json res = RpcClient::Call("dumpprivkey", { activeAddr, pass });
        std::string hexKey = res.value("hex", "");
        std::string wifKey = res.value("wif", "");

        std::wstring wHex(hexKey.begin(), hexKey.end());
        std::wstring wWif(wifKey.begin(), wifKey.end());

        SetWindowTextW(m_txtSecHexKey, wHex.c_str());
        SetWindowTextW(m_txtSecWifKey, wWif.c_str());
        SetWindowTextW(m_lblSecStatus, L"Keys decrypted successfully! Copy them safely.");
    } catch (const std::exception& e) {
        std::string err = e.what();
        std::wstring wErr(err.begin(), err.end());
        SetWindowTextW(m_lblSecStatus, (L"Error: " + wErr).c_str());
        MessageBoxW(m_hwnd, wErr.c_str(), L"Key Export Failed", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoImportPrivKey() {
    wchar_t wKey[256], wLbl[128];
    GetWindowTextW(m_txtImportKeyInput, wKey, 256);
    GetWindowTextW(m_txtImportKeyLabel, wLbl, 128);

    std::wstring wsKey(wKey), wsLbl(wLbl);
    std::string key(wsKey.begin(), wsKey.end());
    std::string lbl(wsLbl.begin(), wsLbl.end());

    try {
        nlohmann::json res = RpcClient::Call("importprivkey", { key, lbl });
        std::string addr = res.value("address", "");
        std::wstring wAddr(addr.begin(), addr.end());
        SetWindowTextW(m_lblSecStatus, (L"Key imported successfully! Address: " + wAddr).c_str());
        MessageBoxW(m_hwnd, (L"Imported Key Address:\n" + wAddr).c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        std::string err = e.what();
        MessageBoxA(m_hwnd, err.c_str(), "Import Failed", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoToggleMining() {
    bool isCurrentlyMining = false;
    if (!m_lastMiningInfo.empty()) {
        isCurrentlyMining = m_lastMiningInfo.value("active", false);
    }

    wchar_t wThreads[32];
    GetWindowTextW(m_txtMiningThreads, wThreads, 32);
    int threads = _wtoi(wThreads);
    if (threads < 1) threads = 1;

    try {
        RpcClient::Call("setgenerate", { !isCurrentlyMining, threads });
    } catch (const std::exception& e) {
        MessageBoxA(m_hwnd, e.what(), "Mining Toggle Error", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoInstantMine() {
    try {
        nlohmann::json blk = RpcClient::Call("mine");
        int h = blk.value("height", 0);
        std::string hash = blk.value("hash", "");
        std::stringstream ss;
        ss << "[NATIVE MINER] Instant Block #" << h << " mined successfully! Hash: " << hash.substr(0, 24) << "...\r\n";
        
        wchar_t currentLog[4096];
        GetWindowTextW(m_txtMiningLog, currentLog, 4096);
        std::string sLog = ss.str();
        std::wstring wsLog(sLog.begin(), sLog.end());
        std::wstring combined = wsLog + currentLog;
        SetWindowTextW(m_txtMiningLog, combined.c_str());
    } catch (const std::exception& e) {
        MessageBoxA(m_hwnd, e.what(), "Mine Error", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoLoadWallet() {
    int sel = (int)SendMessage(m_listWallets, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) return;
    wchar_t buf[256];
    SendMessage(m_listWallets, LB_GETTEXT, sel, (LPARAM)buf);
    std::wstring ws(buf);
    try {
        RpcClient::Call("loadwallet", { std::string(ws.begin(), ws.end()) });
        MessageBoxW(m_hwnd, (L"Loaded wallet: " + ws).c_str(), L"Wallet Switched", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        MessageBoxA(m_hwnd, e.what(), "Error", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoCreateWallet() {
    wchar_t buf[256];
    GetWindowTextW(m_txtNewWalletName, buf, 256);
    std::wstring ws(buf);
    try {
        RpcClient::Call("createwallet", { std::string(ws.begin(), ws.end()) });
        MessageBoxW(m_hwnd, (L"Created new wallet: " + ws).c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        MessageBoxA(m_hwnd, e.what(), "Error", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::DoBackupWallet() {
    try {
        RpcClient::Call("backupwallet", { "wallet-backup.dat" });
        MessageBoxW(m_hwnd, L"Backup saved to wallet-backup.dat in wallets directory!", L"Backup Successful", MB_OK | MB_ICONINFORMATION);
    } catch (const std::exception& e) {
        MessageBoxA(m_hwnd, e.what(), "Error", MB_OK | MB_ICONERROR);
    }
}

void WalletWindow::ExecuteRpcConsole(const std::string& cmd) {
    if (cmd.empty()) return;
    std::stringstream ss(cmd);
    std::string method;
    ss >> method;
    std::vector<std::string> args;
    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }

    nlohmann::json params = nlohmann::json::array();
    for (const auto& a : args) {
        if (a == "true") params.push_back(true);
        else if (a == "false") params.push_back(false);
        else {
            try {
                if (a.find('.') != std::string::npos) {
                    params.push_back(std::stod(a));
                } else {
                    params.push_back(std::stoll(a));
                }
            } catch (...) {
                params.push_back(a);
            }
        }
    }

    std::stringstream output;
    output << "\r\n> " << cmd << "\r\n";
    try {
        nlohmann::json res = RpcClient::Call(method, params);
        output << res.dump(2) << "\r\n";
    } catch (const std::exception& e) {
        output << "Error: " << e.what() << "\r\n";
    }

    wchar_t current[16384];
    GetWindowTextW(m_txtConsoleOutput, current, 16384);
    std::string sOut = output.str();
    std::wstring wsOut(sOut.begin(), sOut.end());
    std::wstring combined = current + wsOut;
    SetWindowTextW(m_txtConsoleOutput, combined.c_str());
    SendMessage(m_txtConsoleOutput, EM_SETSEL, (WPARAM)combined.length(), (LPARAM)combined.length());
    SendMessage(m_txtConsoleOutput, EM_SCROLLCARET, 0, 0);
}

} // namespace BitMundiGUI
