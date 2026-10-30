#if defined(_WIN32)
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

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "crypto/json.hpp"

static std::string SendRpcRequest(const std::string& host, int port, const std::string& method, const nlohmann::json& params) {
#if defined(_WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (IS_INVALID_SOCKET(sock)) {
#if defined(_WIN32)
        WSACleanup();
#endif
        throw std::runtime_error("socket creation failed");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
    serverAddr.sin_port = htons((uint16_t)port);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERR_VAL) {
        CLOSE_SOCKET(sock);
#if defined(_WIN32)
        WSACleanup();
#endif
        throw std::runtime_error("Could not connect to MundiChain daemon at " + host + ":" + std::to_string(port) + ". Is mundichaind running?");
    }

    nlohmann::json req;
    req["method"] = method;
    req["params"] = params;
    req["id"] = 1;

    std::string body = req.dump();
    std::stringstream httpReq;
    httpReq << "POST / HTTP/1.1\r\n"
            << "Host: " << host << ":" << port << "\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << body.length() << "\r\n"
            << "Connection: close\r\n\r\n"
            << body;

    std::string reqStr = httpReq.str();
    send(sock, reqStr.data(), (int)reqStr.length(), 0);

    std::string responseStr;
    std::vector<char> buffer(65536, 0);
    int bytesRead = 0;
    while ((bytesRead = recv(sock, buffer.data(), (int)buffer.size() - 1, 0)) > 0) {
        responseStr.append(buffer.data(), bytesRead);
    }

    CLOSE_SOCKET(sock);
#if defined(_WIN32)
    WSACleanup();
#endif

    size_t bodyPos = responseStr.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        return responseStr.substr(bodyPos + 4);
    }
    return responseStr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "MundiChain Core RPC Client v1.5.0\n"
                  << "Usage: mundichain-cli [options] <command> [params...]\n\n"
                  << "Commands:\n"
                  << "  getblockchaininfo         Get current blockchain status\n"
                  << "  getblockcount             Get total number of blocks\n"
                  << "  getbestblockhash          Get tip block hash\n"
                  << "  getblock <hash/height>    Get block data\n"
                  << "  getbalance                Get current wallet balance\n"
                  << "  getwalletinfo             Get wallet info and active status\n"
                  << "  getnewaddress             Get the active receive address\n"
                  << "  dumpprivkey               Export private key in WIF format\n"
                  << "  listwallets               List all wallet files in data/wallets/\n"
                  << "  loadwallet <name>         Load/switch to specific wallet\n"
                  << "  getmininginfo             Get miner status and hash rate\n"
                  << "  setgenerate <true|false>  Start or stop continuous mining\n"
                  << "  sendtoaddress <addr> <n>  Send BMD coins to an address\n"
                  << "  listunspent               List spendable transaction outputs\n";
        return 1;
    }

    std::string host = "127.0.0.1";
    int port = 18445;
    int argIdx = 1;

    std::string method = argv[argIdx++];
    nlohmann::json params = nlohmann::json::array();

    for (; argIdx < argc; ++argIdx) {
        std::string arg = argv[argIdx];
        if (arg == "true") params.push_back(true);
        else if (arg == "false") params.push_back(false);
        else {
            try {
                if (arg.find('.') != std::string::npos) {
                    params.push_back(std::stod(arg));
                } else {
                    params.push_back(std::stoll(arg));
                }
            } catch (...) {
                params.push_back(arg);
            }
        }
    }

    try {
        std::string rawJson = SendRpcRequest(host, port, method, params);
        auto parsed = nlohmann::json::parse(rawJson);
        if (parsed.contains("error") && !parsed["error"].is_null()) {
            std::cerr << "error: " << parsed["error"].dump(2) << std::endl;
            return 1;
        }
        std::cout << parsed["result"].dump(2) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
