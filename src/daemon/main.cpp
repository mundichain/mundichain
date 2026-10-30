#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include <iostream>
#include <filesystem>
#include <csignal>
#include <atomic>
#include <iomanip>
#include <chrono>
#include <thread>

#include "consensus/blockchain.h"
#include "consensus/miner.h"
#include "rpc/server.h"

using namespace BitMundi;

static std::atomic<bool> g_shutdownRequested{false};
static Blockchain* g_blockchain = nullptr;
static Miner* g_miner = nullptr;
static RPCServer* g_rpcServer = nullptr;

#if defined(_WIN32)
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT) {
        std::cout << "\n[SHUTDOWN] Intercepted signal, shutting down MundiChain native node safely..." << std::endl;
        g_shutdownRequested = true;
        if (g_rpcServer) g_rpcServer->Stop();
        if (g_miner) g_miner->StopMining();
        return TRUE;
    }
    return FALSE;
}
#else
void SignalHandler(int signum) {
    std::cout << "\n[SHUTDOWN] Intercepted signal (" << signum << "), shutting down MundiChain native node safely..." << std::endl;
    g_shutdownRequested = true;
    if (g_rpcServer) g_rpcServer->Stop();
    if (g_miner) g_miner->StopMining();
}
#endif

int main(int argc, char* argv[]) {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath(exePathBuf);
#else
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    char exePathBuf[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exePathBuf, PATH_MAX);
    std::filesystem::path exePath = (count > 0) ? std::filesystem::path(std::string(exePathBuf, count)) : (std::filesystem::current_path() / "mundichaind");
#endif

    std::filesystem::path baseDir = exePath.parent_path();
    std::filesystem::path dataDir = baseDir / "data";

    std::cout << "======================================================================" << std::endl;
    std::cout << "MUNDICHAIN CORE DAEMON (mundichaind) v1.5.0 NATIVE C++" << std::endl;
    std::cout << "Institutional Performance Engine - Zero Overhead Architecture" << std::endl;
    std::cout << "Network: MundiChain Mainnet -- Ticker: BMD" << std::endl;

    Blockchain blockchain(dataDir.string());
    blockchain.LoadOrInit();
    g_blockchain = &blockchain;

    Miner miner(blockchain);
    g_miner = &miner;

    BalanceResult bal = blockchain.GetBalance();
    size_t blockCount = blockchain.GetBlockCount();
    std::string primaryAddr = blockchain.GetWallet().GetPrimaryAddress();
    std::string activeWallet = blockchain.GetWallet().GetActiveWalletName();

    uint32_t lastBits = Consensus::Params::POW_LIMIT_BITS;
    if (blockCount > 0) {
        std::string bStr = blockchain.GetBlocks().back().value("bits", "0x1e0ffff0");
        lastBits = static_cast<uint32_t>(std::stoul(bStr, nullptr, 16));
    }
    double diff = Blockchain::CalculateDifficulty(lastBits);

    std::cout << "Current Block: #" << blockCount << " -- Balance: " << std::fixed << std::setprecision(8) << bal.total << " BMD" << std::endl;
    std::cout << "Active Wallet: " << activeWallet << std::endl;
    std::cout << "Active Address: " << primaryAddr << std::endl;
    std::cout << "Current Difficulty: " << std::fixed << std::setprecision(4) << diff << " (Bits: 0x" << std::hex << lastBits << std::dec << ")" << std::endl;
    std::cout << "Target Block Time: 60s (Satoshi Nakamoto Re-targeting)" << std::endl;

    RPCServer rpcServer(blockchain, miner, 18445);
    g_rpcServer = &rpcServer;
    rpcServer.Start();

    std::cout << "======================================================================" << std::endl;
    std::cout << "Node running in headless daemon mode. Press Ctrl+C to terminate." << std::endl;

    while (!g_shutdownRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    std::cout << "MundiChain native daemon stopped cleanly." << std::endl;
    return 0;
}
