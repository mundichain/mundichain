#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <sstream>
#include "crypto/json.hpp"

namespace BitMundiGUI {

class RpcClient {
public:
    static nlohmann::json Call(const std::string& method, const nlohmann::json& params = nlohmann::json::array(), const std::string& host = "127.0.0.1", int port = 18445) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Socket creation failed");
        }

        // Set timeout 2s
        DWORD timeout = 2000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
        serverAddr.sin_port = htons((uint16_t)port);

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            throw std::runtime_error("Daemon offline");
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

        closesocket(sock);
        WSACleanup();

        size_t headerEnd = responseStr.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            throw std::runtime_error("Invalid HTTP response");
        }

        std::string jsonBody = responseStr.substr(headerEnd + 4);
        nlohmann::json j = nlohmann::json::parse(jsonBody);
        if (j.contains("error") && !j["error"].is_null()) {
            throw std::runtime_error(j["error"].get<std::string>());
        }
        return j.value("result", nlohmann::json());
    }
};

} // namespace BitMundiGUI
