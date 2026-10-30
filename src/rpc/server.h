#pragma once
#include <string>
#include <thread>
#include <atomic>
#include "consensus/blockchain.h"
#include "consensus/miner.h"
#include "crypto/json.hpp"

namespace BitMundi {

class RPCServer {
public:
    RPCServer(Blockchain& blockchain, Miner& miner, int port = 18445);
    ~RPCServer();

    void Start();
    void Stop();

private:
    void ServerLoop();
    void HandleClient(uintptr_t clientSocket);
    nlohmann::json HandleMethod(const std::string& method, const nlohmann::json& params);

    Blockchain& m_blockchain;
    Miner& m_miner;
    int m_port;
    std::atomic<bool> m_running{false};
    std::thread m_serverThread;
    uintptr_t m_listenSocket{0};
};

} // namespace BitMundi
