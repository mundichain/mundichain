#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSE_SOCKET(s) closesocket(s)
#define IS_INVALID_SOCKET(s) ((s) == INVALID_SOCKET)
#define SOCKET_ERR_VAL SOCKET_ERROR
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int socket_t;
#define CLOSE_SOCKET(s) close(s)
#define IS_INVALID_SOCKET(s) ((s) < 0)
#define SOCKET_ERR_VAL (-1)
#endif

#include "rpc/server.h"
#include <iostream>
#include <sstream>

namespace BitMundi {

RPCServer::RPCServer(Blockchain& blockchain, Miner& miner, int port)
    : m_blockchain(blockchain), m_miner(miner), m_port(port) {
}

RPCServer::~RPCServer() {
    Stop();
}

void RPCServer::Start() {
    if (m_running) return;
    m_running = true;
    m_serverThread = std::thread(&RPCServer::ServerLoop, this);
}

void RPCServer::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_listenSocket != 0) {
        CLOSE_SOCKET((socket_t)m_listenSocket);
        m_listenSocket = 0;
    }
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

void RPCServer::ServerLoop() {
#if defined(_WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[RPC ERROR] WSAStartup failed\n";
        return;
    }
#endif

    socket_t listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (IS_INVALID_SOCKET(listenSock)) {
        std::cerr << "[RPC ERROR] socket() failed\n";
#if defined(_WIN32)
        WSACleanup();
#endif
        return;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((uint16_t)m_port);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERR_VAL) {
        std::cerr << "[RPC ERROR] bind() failed on port " << m_port << "\n";
        CLOSE_SOCKET(listenSock);
#if defined(_WIN32)
        WSACleanup();
#endif
        return;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERR_VAL) {
        std::cerr << "[RPC ERROR] listen() failed\n";
        CLOSE_SOCKET(listenSock);
#if defined(_WIN32)
        WSACleanup();
#endif
        return;
    }

    m_listenSocket = (uintptr_t)listenSock;
    std::cout << "[RPC SERVER] Listening natively on http://127.0.0.1:" << m_port << std::endl;

    while (m_running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        socket_t clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientLen);
        if (IS_INVALID_SOCKET(clientSock)) {
            if (!m_running) break;
            continue;
        }

        std::thread(&RPCServer::HandleClient, this, (uintptr_t)clientSock).detach();
    }

    CLOSE_SOCKET(listenSock);
#if defined(_WIN32)
    WSACleanup();
#endif
}

void RPCServer::HandleClient(uintptr_t clientSocket) {
    socket_t sock = (socket_t)clientSocket;
    std::vector<char> buffer(65536, 0);
    int bytesRead = recv(sock, buffer.data(), (int)buffer.size() - 1, 0);
    if (bytesRead <= 0) {
        CLOSE_SOCKET(sock);
        return;
    }
    buffer[bytesRead] = '\0';
    std::string requestStr(buffer.data(), bytesRead);

    // Extract headers and body
    size_t headerEnd = requestStr.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        CLOSE_SOCKET(sock);
        return;
    }

    std::string headers = requestStr.substr(0, headerEnd);
    std::string body = requestStr.substr(headerEnd + 4);

    // Support HTTP OPTIONS preflight for browser CORS
    if (requestStr.rfind("OPTIONS ", 0) == 0) {
        std::stringstream optResp;
        optResp << "HTTP/1.1 204 No Content\r\n"
                << "Access-Control-Allow-Origin: *\r\n"
                << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n"
                << "Access-Control-Max-Age: 86400\r\n"
                << "Content-Length: 0\r\n"
                << "Connection: close\r\n\r\n";
        std::string s = optResp.str();
        send(sock, s.data(), (int)s.length(), 0);
        CLOSE_SOCKET(sock);
        return;
    }

    // If body is shorter than Content-Length, read remainder
    size_t clPos = headers.find("Content-Length: ");
    if (clPos != std::string::npos) {
        size_t clEnd = headers.find("\r\n", clPos);
        int expectedLen = std::stoi(headers.substr(clPos + 16, clEnd - (clPos + 16)));
        while ((int)body.length() < expectedLen) {
            int more = recv(sock, buffer.data(), (int)buffer.size() - 1, 0);
            if (more <= 0) break;
            body.append(buffer.data(), more);
        }
    }

    nlohmann::json responseJson;
    nlohmann::json reqId = nullptr;

    try {
        nlohmann::json req = nlohmann::json::parse(body);
        std::string method = req.value("method", "");
        nlohmann::json params = req.value("params", nlohmann::json::array());
        if (req.contains("id")) reqId = req["id"];

        nlohmann::json result = HandleMethod(method, params);
        responseJson["result"] = result;
        responseJson["error"] = nullptr;
        responseJson["id"] = reqId;
    } catch (const std::exception& e) {
        responseJson["result"] = nullptr;
        responseJson["error"] = e.what();
        responseJson["id"] = reqId;
    }

    std::string respBody = responseJson.dump();
    std::stringstream httpResp;
    httpResp << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With\r\n"
             << "Content-Length: " << respBody.length() << "\r\n"
             << "Connection: close\r\n\r\n"
             << respBody;

    std::string s = httpResp.str();
    send(sock, s.data(), (int)s.length(), 0);
    CLOSE_SOCKET(sock);
}

nlohmann::json RPCServer::HandleMethod(const std::string& method, const nlohmann::json& params) {
    if (method == "getinfo") {
        BalanceResult bal = m_blockchain.GetBalance();
        uint32_t lastBits = Consensus::Params::POW_LIMIT_BITS;
        if (m_blockchain.GetBlockCount() > 0) {
            std::string bStr = m_blockchain.GetBlocks().back().value("bits", "0x1e0ffff0");
            lastBits = static_cast<uint32_t>(std::stoul(bStr, nullptr, 16));
        }
        double diff = Blockchain::CalculateDifficulty(lastBits);

        nlohmann::json info;
        info["version"] = "1.5.0-bitmundi-native-cpp";
        info["protocolversion"] = 70015;
        info["blocks"] = m_blockchain.GetBlockCount();
        info["timeoffset"] = 0;
        info["connections"] = 1;
        info["difficulty"] = diff;
        info["target_block_time"] = Consensus::Params::TARGET_BLOCK_TIME;
        info["retarget_interval"] = Consensus::Params::RETARGET_INTERVAL;
        info["balance"] = bal.mature;
        info["unconfirmed"] = bal.immature;
        info["keypoololdest"] = Consensus::Params::GENESIS_TIME;
        info["paytxfee"] = 0.0001;
        info["relayfee"] = 0.0001;
        info["wallet_file"] = m_blockchain.GetWallet().GetActiveWalletName();
        info["wallets_dir"] = m_blockchain.GetWallet().GetWalletsDir();
        info["mempool_count"] = m_blockchain.GetMempoolCount();
        info["errors"] = "";
        return info;
    }
    else if (method == "getblockcount") {
        return m_blockchain.GetBlockCount();
    }
    else if (method == "getblock") {
        if (params.empty()) throw std::runtime_error("Missing block index or hash parameter!");
        if (params[0].is_number_integer()) {
            size_t idx = params[0].get<size_t>();
            return m_blockchain.GetBlock(idx);
        } else if (params[0].is_string()) {
            return m_blockchain.GetBlockByHash(params[0].get<std::string>());
        }
        throw std::runtime_error("Invalid block parameter type!");
    }
    else if (method == "getaddress" || method == "getaddressinfo") {
        std::string addr = "";
        if (!params.empty() && params[0].is_string()) {
            addr = params[0].get<std::string>();
        }
        BalanceResult bal = m_blockchain.GetBalance(addr);

        std::vector<nlohmann::json> txs;
        double totalReceived = 0.0;
        double totalSent = 0.0;

        if (!addr.empty()) {
            std::lock_guard<std::mutex> lock(m_blockchain.GetMutex());
            
            // 1. Mempool
            const auto& mempool = m_blockchain.GetMempool();
            for (auto it = mempool.rbegin(); it != mempool.rend(); ++it) {
                const auto& tx = *it;
                if (tx.value("is_coinbase", false)) continue;

                bool isSender = false;
                if (tx.contains("vin") && tx["vin"].is_array()) {
                    for (const auto& vi : tx["vin"]) {
                        if (vi.value("address", "") == addr) { isSender = true; break; }
                    }
                }
                if (!isSender && tx.contains("from_addrs") && tx["from_addrs"].is_array()) {
                    for (const auto& fa : tx["from_addrs"]) {
                        if (fa == addr) { isSender = true; break; }
                    }
                }

                bool isReceiver = false;
                double receivedAmt = 0.0;
                if (tx.contains("vout") && tx["vout"].is_array()) {
                    for (const auto& vo : tx["vout"]) {
                        if (vo.value("address", "") == addr) {
                            isReceiver = true;
                            receivedAmt += static_cast<double>(vo.value("value", (int64_t)0)) / Consensus::Params::COIN;
                        }
                    }
                }

                if (isSender) {
                    double sentAmt = 0.0;
                    std::string primaryDest = "";
                    if (tx.contains("vout") && tx["vout"].is_array()) {
                        for (const auto& vo : tx["vout"]) {
                            if (vo.value("address", "") != addr) {
                                sentAmt += static_cast<double>(vo.value("value", (int64_t)0)) / Consensus::Params::COIN;
                                if (primaryDest.empty()) primaryDest = vo.value("address", "");
                            }
                        }
                    }
                    if (sentAmt == 0.0 && tx.value("amount", (int64_t)0) > 0) {
                        sentAmt = static_cast<double>(tx.value("amount", (int64_t)0)) / Consensus::Params::COIN;
                    }
                    double fee = static_cast<double>(tx.value("fee", (int64_t)0)) / Consensus::Params::COIN;
                    totalSent += sentAmt;
                    txs.push_back({
                        {"txid", tx.value("txid", "")},
                        {"category", "send"},
                        {"amount", -sentAmt},
                        {"fee", fee},
                        {"confirmations", 0},
                        {"blockheight", -1},
                        {"blockhash", "mempool"},
                        {"time", tx.value("time", (int64_t)0)},
                        {"status", "Pendente na Mempool"},
                        {"from_address", addr},
                        {"to_address", primaryDest.empty() ? tx.value("to_addr", "") : primaryDest},
                        {"vin", tx.value("vin", nlohmann::json::array())},
                        {"vout", tx.value("vout", nlohmann::json::array())}
                    });
                } else if (isReceiver) {
                    std::string senderAddr = "";
                    if (tx.contains("vin") && tx["vin"].is_array() && !tx["vin"].empty()) {
                        senderAddr = tx["vin"][0].value("address", "");
                    }
                    if (senderAddr.empty() && tx.contains("from_addrs") && tx["from_addrs"].is_array() && !tx["from_addrs"].empty()) {
                        senderAddr = tx["from_addrs"][0].get<std::string>();
                    }
                    double fee = static_cast<double>(tx.value("fee", (int64_t)0)) / Consensus::Params::COIN;
                    totalReceived += receivedAmt;
                    txs.push_back({
                        {"txid", tx.value("txid", "")},
                        {"category", "receive"},
                        {"amount", receivedAmt},
                        {"fee", fee},
                        {"confirmations", 0},
                        {"blockheight", -1},
                        {"blockhash", "mempool"},
                        {"time", tx.value("time", (int64_t)0)},
                        {"status", "Pendente na Mempool"},
                        {"from_address", senderAddr},
                        {"to_address", addr},
                        {"vin", tx.value("vin", nlohmann::json::array())},
                        {"vout", tx.value("vout", nlohmann::json::array())}
                    });
                }
            }

            // 2. Confirmed blocks
            const auto& blocks = m_blockchain.GetBlocks();
            int32_t currentHeight = static_cast<int32_t>(blocks.size()) - 1;
            for (auto bit = blocks.rbegin(); bit != blocks.rend(); ++bit) {
                const auto& blk = *bit;
                int32_t h = blk.value("height", 0);
                if (!blk.contains("tx") || !blk["tx"].is_array()) continue;

                for (const auto& tx : blk["tx"]) {
                    if (tx.value("is_coinbase", false)) continue;

                    bool isSender = false;
                    if (tx.contains("vin") && tx["vin"].is_array()) {
                        for (const auto& vi : tx["vin"]) {
                            if (vi.value("address", "") == addr) { isSender = true; break; }
                        }
                    }
                    if (!isSender && tx.contains("from_addrs") && tx["from_addrs"].is_array()) {
                        for (const auto& fa : tx["from_addrs"]) {
                            if (fa == addr) { isSender = true; break; }
                        }
                    }

                    bool isReceiver = false;
                    double receivedAmt = 0.0;
                    if (tx.contains("vout") && tx["vout"].is_array()) {
                        for (const auto& vo : tx["vout"]) {
                            if (vo.value("address", "") == addr) {
                                isReceiver = true;
                                receivedAmt += static_cast<double>(vo.value("value", (int64_t)0)) / Consensus::Params::COIN;
                            }
                        }
                    }

                    int32_t confs = currentHeight - h + 1;
                    int64_t blockTime = blk.value("time", (int64_t)0);

                    if (isSender) {
                        double sentAmt = 0.0;
                        std::string primaryDest = "";
                        if (tx.contains("vout") && tx["vout"].is_array()) {
                            for (const auto& vo : tx["vout"]) {
                                if (vo.value("address", "") != addr) {
                                    sentAmt += static_cast<double>(vo.value("value", (int64_t)0)) / Consensus::Params::COIN;
                                    if (primaryDest.empty()) primaryDest = vo.value("address", "");
                                }
                            }
                        }
                        if (sentAmt == 0.0 && tx.value("amount", (int64_t)0) > 0) {
                            sentAmt = static_cast<double>(tx.value("amount", (int64_t)0)) / Consensus::Params::COIN;
                        }
                        double fee = static_cast<double>(tx.value("fee", (int64_t)0)) / Consensus::Params::COIN;
                        totalSent += sentAmt;
                        txs.push_back({
                            {"txid", tx.value("txid", "")},
                            {"category", "send"},
                            {"amount", -sentAmt},
                            {"fee", fee},
                            {"confirmations", confs},
                            {"blockheight", h},
                            {"blockhash", blk.value("hash", "")},
                            {"time", blockTime},
                            {"status", "Confirmada"},
                            {"from_address", addr},
                            {"to_address", primaryDest.empty() ? tx.value("to_addr", "") : primaryDest},
                            {"vin", tx.value("vin", nlohmann::json::array())},
                            {"vout", tx.value("vout", nlohmann::json::array())}
                        });
                    } else if (isReceiver) {
                        std::string senderAddr = "";
                        if (tx.contains("vin") && tx["vin"].is_array() && !tx["vin"].empty()) {
                            senderAddr = tx["vin"][0].value("address", "");
                        }
                        if (senderAddr.empty() && tx.contains("from_addrs") && tx["from_addrs"].is_array() && !tx["from_addrs"].empty()) {
                            senderAddr = tx["from_addrs"][0].get<std::string>();
                        }
                        double fee = static_cast<double>(tx.value("fee", (int64_t)0)) / Consensus::Params::COIN;
                        totalReceived += receivedAmt;
                        txs.push_back({
                            {"txid", tx.value("txid", "")},
                            {"category", "receive"},
                            {"amount", receivedAmt},
                            {"fee", fee},
                            {"confirmations", confs},
                            {"blockheight", h},
                            {"blockhash", blk.value("hash", "")},
                            {"time", blockTime},
                            {"status", "Confirmada"},
                            {"from_address", senderAddr},
                            {"to_address", addr},
                            {"vin", tx.value("vin", nlohmann::json::array())},
                            {"vout", tx.value("vout", nlohmann::json::array())}
                        });
                    }
                }
            }
        }

        return {
            {"address", addr},
            {"balance", bal.mature},
            {"immature", bal.immature},
            {"total", bal.total},
            {"total_received", totalReceived},
            {"total_sent", totalSent},
            {"volume", totalReceived + totalSent},
            {"tx_count", txs.size()},
            {"status", "Active on Mainnet"},
            {"transactions", txs}
        };
    }
    else if (method == "getrawtransaction" || method == "gettransaction") {
        if (params.empty() || !params[0].is_string()) {
            throw std::runtime_error("Missing txid parameter!");
        }
        std::string targetTxid = params[0].get<std::string>();
        
        std::lock_guard<std::mutex> lock(m_blockchain.GetMutex());
        const auto& mempool = m_blockchain.GetMempool();
        for (const auto& tx : mempool) {
            if (tx.value("txid", "") == targetTxid) {
                nlohmann::json res = tx;
                res["confirmations"] = 0;
                res["blockheight"] = -1;
                res["blockhash"] = "mempool";
                res["status"] = "Pending (In Mempool)";
                return res;
            }
        }

        const auto& blocks = m_blockchain.GetBlocks();
        int32_t currentHeight = static_cast<int32_t>(blocks.size()) - 1;
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            const auto& blk = *it;
            int32_t h = blk.value("height", 0);
            if (blk.contains("tx") && blk["tx"].is_array()) {
                for (const auto& tx : blk["tx"]) {
                    if (tx.value("txid", "") == targetTxid) {
                        nlohmann::json res = tx;
                        res["confirmations"] = currentHeight - h + 1;
                        res["blockheight"] = h;
                        res["blockhash"] = blk.value("hash", "");
                        res["time"] = blk.value("time", (int64_t)0);
                        res["status"] = "Confirmed";
                        return res;
                    }
                }
            }
            if (blk.value("hash", "") == targetTxid && blk.contains("tx") && blk["tx"].is_array() && !blk["tx"].empty()) {
                nlohmann::json res = blk["tx"][0];
                res["confirmations"] = currentHeight - h + 1;
                res["blockheight"] = h;
                res["blockhash"] = blk.value("hash", "");
                res["time"] = blk.value("time", (int64_t)0);
                res["status"] = "Confirmed";
                return res;
            }
        }
        throw std::runtime_error("Transaction not found: " + targetTxid);
    }
    else if (method == "getbalance") {
        std::string addr = "";
        if (!params.empty() && params[0].is_string()) {
            addr = params[0].get<std::string>();
        }
        return m_blockchain.GetBalance(addr).mature;
    }
    else if (method == "getwalletinfo") {
        BalanceResult bal = m_blockchain.GetBalance();
        std::string primaryAddr = m_blockchain.GetWallet().GetPrimaryAddress();
        std::string label = "Default";
        if (m_blockchain.GetWallet().HasAddress(primaryAddr)) {
            label = m_blockchain.GetWallet().GetAddressesMap().at(primaryAddr).label;
        }
        nlohmann::json winfo;
        winfo["wallet_file"] = m_blockchain.GetWallet().GetActiveWalletName();
        winfo["address"] = primaryAddr;
        winfo["label"] = label;
        winfo["balance"] = bal.mature;
        winfo["immature"] = bal.immature;
        winfo["total"] = bal.total;
        winfo["has_password"] = m_blockchain.GetWallet().HasPassword();
        winfo["wallets_dir"] = m_blockchain.GetWallet().GetWalletsDir();
        return winfo;
    }
    else if (method == "listwallets") {
        nlohmann::json res;
        res["wallets"] = m_blockchain.GetWallet().ListWallets();
        res["active"] = m_blockchain.GetWallet().GetActiveWalletName();
        res["wallets_dir"] = m_blockchain.GetWallet().GetWalletsDir();
        return res;
    }
    else if (method == "loadwallet") {
        std::string wname = params.at(0).get<std::string>();
        m_blockchain.GetWallet().LoadWallet(wname);
        return HandleMethod("getwalletinfo", nlohmann::json::array());
    }
    else if (method == "createwallet") {
        std::string wname = params.at(0).get<std::string>();
        m_blockchain.GetWallet().CreateWallet(wname);
        return HandleMethod("getwalletinfo", nlohmann::json::array());
    }
    else if (method == "importwallet") {
        std::string path = params.at(0).get<std::string>();
        m_blockchain.GetWallet().ImportWallet(path);
        return HandleMethod("getwalletinfo", nlohmann::json::array());
    }
    else if (method == "backupwallet") {
        std::string path = params.at(0).get<std::string>();
        return m_blockchain.GetWallet().BackupWallet(path);
    }
    else if (method == "getnewaddress") {
        std::string label = params.empty() ? "New Address" : params[0].get<std::string>();
        return m_blockchain.GetWallet().GenerateNewAddress(label);
    }
    else if (method == "getaddresses") {
        return m_blockchain.GetWallet().GetAddresses([this](const std::string& a) {
            return m_blockchain.GetBalance(a);
        });
    }
    else if (method == "dumpprivkey") {
        std::string addr = params.at(0).get<std::string>();
        std::string pwd = (params.size() > 1 && params[1].is_string()) ? params[1].get<std::string>() : "";
        return m_blockchain.GetWallet().DumpPrivKey(addr, pwd);
    }
    else if (method == "importprivkey") {
        std::string priv = params.at(0).get<std::string>();
        std::string label = (params.size() > 1 && params[1].is_string()) ? params[1].get<std::string>() : "Imported Key";
        std::string pwd = (params.size() > 2 && params[2].is_string()) ? params[2].get<std::string>() : "";
        return m_blockchain.GetWallet().ImportPrivKey(priv, label, pwd);
    }
    else if (method == "setwalletpassword") {
        std::string pwd = params.at(0).get<std::string>();
        return m_blockchain.GetWallet().SetPassword(pwd);
    }
    else if (method == "checkwalletpassword") {
        std::string pwd = params.at(0).get<std::string>();
        return m_blockchain.GetWallet().CheckPassword(pwd);
    }
    else if (method == "sendtoaddress") {
        std::string toAddr = params.at(0).get<std::string>();
        double amt = params.at(1).get<double>();
        double fee = (params.size() > 2) ? params[2].get<double>() : 0.0001;
        std::string fromAddr = (params.size() > 3 && params[3].is_string()) ? params[3].get<std::string>() : m_blockchain.GetWallet().GetPrimaryAddress();
        return m_blockchain.CreateTransaction(fromAddr, toAddr, amt, fee);
    }
    else if (method == "mine") {
        std::string minerAddr = (!params.empty() && params[0].is_string()) ? params[0].get<std::string>() : "";
        nlohmann::json blk = m_blockchain.MineSingleBlockSync(minerAddr);
        nlohmann::json res;
        res["height"] = blk["height"];
        res["hash"] = blk["hash"];
        res["reward"] = static_cast<double>(Consensus::Params::INITIAL_SUBSIDY) / Consensus::Params::COIN;
        return res;
    }
    else if (method == "getmininginfo") {
        return m_miner.GetMiningInfo();
    }
    else if (method == "setgenerate") {
        bool active = params.at(0).get<bool>();
        int threads = (params.size() > 1) ? params[1].get<int>() : 1;
        if (active) {
            m_miner.StartMining(threads);
        } else {
            m_miner.StopMining();
        }
        nlohmann::json res;
        res["active"] = m_miner.IsMining();
        res["threads"] = threads;
        return res;
    }
    else if (method == "listtransactions") {
        return m_blockchain.ListTransactions(m_blockchain.GetWallet().GetAddressesMap());
    }
    else if (method == "submitblock") {
        if (params.empty()) throw std::runtime_error("Missing block parameter!");
        nlohmann::json blk = params[0];
        m_blockchain.AddMinedBlock(blk);
        return {{"status", "accepted"}, {"height", blk.value("height", 0)}, {"hash", blk.value("hash", "")}};
    }

    throw std::runtime_error("Unknown RPC method: " + method);
}

} // namespace BitMundi
