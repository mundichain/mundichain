#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "crypto/json.hpp"

namespace BitMundiGUI {

enum TabIndex {
    TAB_OVERVIEW = 0,
    TAB_SEND,
    TAB_RECEIVE,
    TAB_TRANSACTIONS,
    TAB_MINING,
    TAB_WALLETS,
    TAB_SECURITY,
    TAB_CONSOLE,
    TAB_COUNT
};

class WalletWindow {
public:
    WalletWindow(HINSTANCE hInstance);
    ~WalletWindow();

    bool Create();
    void Show(int nCmdShow);
    int RunMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    void CreateSidebar();
    void CreateOverviewTab();
    void CreateSendTab();
    void CreateReceiveTab();
    void CreateTransactionsTab();
    void CreateMiningTab();
    void CreateWalletsTab();
    void CreateSecurityTab();
    void CreateConsoleTab();

    void SwitchTab(TabIndex tab);
    void StartBackgroundPoller();
    void StopBackgroundPoller();
    void PollDaemon();
    void UpdateUIWithData();

    void CopyToClipboard(const std::string& text);
    void ExecuteRpcConsole(const std::string& cmd);
    void DoSendTransaction();
    void DoGenerateAddress();
    void DoDumpPrivKey();
    void DoImportPrivKey();
    void DoToggleMining();
    void DoInstantMine();
    void DoLoadWallet();
    void DoCreateWallet();
    void DoBackupWallet();

    HINSTANCE m_hInstance{NULL};
    HWND m_hwnd{NULL};
    HWND m_hwndSidebar{NULL};

    // Sub-containers for tabs
    HWND m_tabPanels[TAB_COUNT]{NULL};
    HWND m_sidebarBtns[TAB_COUNT]{NULL};
    TabIndex m_currentTab{TAB_OVERVIEW};

    // Overview controls
    HWND m_lblMatureBal{NULL};
    HWND m_lblImmatureBal{NULL};
    HWND m_lblTotalBal{NULL};
    HWND m_lblHeight{NULL};
    HWND m_lblDiff{NULL};
    HWND m_lblRecentTxs{NULL};

    // Send controls
    HWND m_txtSendTo{NULL};
    HWND m_txtSendAmount{NULL};
    HWND m_txtSendFee{NULL};
    HWND m_lblSendStatus{NULL};

    // Receive controls
    HWND m_txtRecvAddr{NULL};
    HWND m_txtRecvPubkey{NULL};

    // Transactions controls
    HWND m_lvTransactions{NULL};

    // Mining controls
    HWND m_txtMiningThreads{NULL};
    HWND m_btnMiningToggle{NULL};
    HWND m_lblMiningHashrate{NULL};
    HWND m_lblMiningBlocks{NULL};
    HWND m_txtMiningLog{NULL};

    // Wallets controls
    HWND m_lblActiveWallet{NULL};
    HWND m_listWallets{NULL};
    HWND m_txtNewWalletName{NULL};

    // Security controls
    HWND m_txtSecPassphrase{NULL};
    HWND m_txtSecHexKey{NULL};
    HWND m_txtSecWifKey{NULL};
    HWND m_txtImportKeyInput{NULL};
    HWND m_txtImportKeyLabel{NULL};
    HWND m_txtImportKeyPass{NULL};
    HWND m_lblSecStatus{NULL};

    // Console controls
    HWND m_txtConsoleOutput{NULL};
    HWND m_txtConsoleInput{NULL};

    // State cache
    std::atomic<bool> m_pollingActive{false};
    std::thread m_pollThread;
    nlohmann::json m_lastInfo;
    nlohmann::json m_lastWalletInfo;
    nlohmann::json m_lastMiningInfo;
    nlohmann::json m_lastTransactions;

    // Fonts and brushes
    HFONT m_hFontRegular{NULL};
    HFONT m_hFontBold{NULL};
    HFONT m_hFontTitle{NULL};
    HFONT m_hFontBigNum{NULL};
    HFONT m_hFontMono{NULL};

    HBRUSH m_hBrBg{NULL};
    HBRUSH m_hBrSidebar{NULL};
    HBRUSH m_hBrCard{NULL};
};

} // namespace BitMundiGUI
