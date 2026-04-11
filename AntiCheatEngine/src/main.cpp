#include <iostream>
#include "Server.h"
#include "PlayerManager.h"
#include "Checks.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#ifdef _WIN32
void initializeWinsock() {
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup failed with error: " << res << std::endl;
        exit(1);
    }
}

void cleanupWinsock() {
    WSACleanup();
}
#else
void initializeWinsock() {}
void cleanupWinsock() {}
#endif

int main() {
    initializeWinsock();

    Server server(8080);
    PlayerManager playerManager;

    auto onMessage = [&](const std::string& rawMessage, SOCKET clientSocket) {
        try {
            json data = json::parse(rawMessage);
            
            if (data.contains("action") && data["action"] == "MOVE") {
                auto verdict = Checks::performChecks(data, playerManager);
                
                if (verdict) {
                    std::string response = verdict->toJson().dump() + "\n";
                    server.sendToClient(clientSocket, response);
                    std::cout << "[FLAG] " << verdict->player << " flagged for " << verdict->type << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing packet: " << e.what() << " | Raw: " << rawMessage << std::endl;
        }
    };

    server.start(onMessage);

    std::cout << "Anti-Cheat Engine started. Press Enter to shutdown..." << std::endl;
    std::cin.get();

    server.stop();
    cleanupWinsock();

    return 0;
}
