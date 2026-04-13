#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <deque>
#include "Server.h"
#include "PlayerManager.h"

#ifdef _WIN32
void initializeWinsock() {
    WSADATA wsaData;
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        exit(1);
    }
}
void cleanupWinsock() { WSACleanup(); }
#else
void initializeWinsock() {}
void cleanupWinsock() {}
#endif

double globalMaxTokens = 50.0;
double globalSpeedVlKick = 5.0;

unsigned long long getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main() {
    initializeWinsock();
    Server server(25577);
    PlayerManager playerManager;

    auto onMessage = [&](const std::string& rawMsg, SOCKET clientSocket) {
        if (rawMsg.empty()) return;
        std::stringstream ss_p(rawMsg);
        std::string pkt;
        while (std::getline(ss_p, pkt, '\n')) {
            if (pkt.empty()) continue;
            std::vector<std::string> parts;
            std::stringstream ss_t(pkt);
            std::string item;
            while (std::getline(ss_t, item, '|')) parts.push_back(item);
            if (parts.empty()) continue;
            const std::string& cmd = parts[0];

            if (cmd == "QUIT" && parts.size() >= 2) {
                playerManager.removePlayer(parts[1]);
            } else if (cmd == "CONFIG" && parts.size() >= 3) {
                try {
                    globalMaxTokens = std::stod(parts[1]);
                    globalSpeedVlKick = std::stod(parts[2]);
                } catch (...) {}
            } else if (cmd == "FIREWORK" && parts.size() >= 2) {
                PlayerState fState = playerManager.getPlayer(parts[1]);
                fState.fireworkTicks = 40;
                playerManager.updatePlayer(parts[1], fState);
            } else if (cmd == "POS" && parts.size() >= 11) {
                try {
                    const std::string& pNameAddress = parts[1];
                    double cX = std::stod(parts[2]);
                    double cY = std::stod(parts[3]);
                    double cZ = std::stod(parts[4]);
                    bool gr = (parts[7] == "true");
                    bool el = (parts[8] == "true");
                    int sLv = std::stoi(parts[9]);
                    bool liq = (parts[10] == "true");

                    unsigned long long nowTs = getCurrentTimeMs();
                    PlayerState playerObj = playerManager.getPlayer(pNameAddress);

                    if (playerObj.lastUpdateTime != 0) {
                        unsigned long long elp = nowTs - playerObj.lastUpdateTime;
                        playerObj.tokens += (double)elp / 50.0;
                        if (playerObj.tokens > globalMaxTokens) playerObj.tokens = globalMaxTokens;
                    }
                    playerObj.tokens -= 1.0;
                    playerObj.lastUpdateTime = nowTs;

                    if (playerObj.tokens < 0.0) {
                        server.sendToClient(clientSocket, "ALERT|" + pNameAddress + "|Timer Hack\n");
                        playerObj.tokens = globalMaxTokens;
                        playerManager.updatePlayer(pNameAddress, playerObj);
                    } else {
                        if (playerObj.initialized) {
                            double dx = cX - playerObj.posX;
                            double dy = cY - playerObj.posY;
                            double dz = cZ - playerObj.posZ;
                            double d2d = std::sqrt(dx * dx + dz * dz);
                            if (!gr) playerObj.airTicks++; else playerObj.airTicks = 0;
                            playerObj.dyHistory.push_back(dy);
                            if (playerObj.dyHistory.size() > 8) playerObj.dyHistory.pop_front();
                            if (!gr && !el && !liq) {
                                if (dy >= 0.0) { if (playerObj.airTicks > 10) playerObj.flyVL += 1.0; } 
                                else { playerObj.flyVL = std::max(0.0, playerObj.flyVL - 0.05); }
                                if (playerObj.flyVL > 10.0) { server.sendToClient(clientSocket, "ALERT|" + pNameAddress + "|Fly Hack\n"); playerObj.flyVL = 0.0; }
                            }
                            double lim = 0.36;
                            if (el) {
                                double oDy = playerObj.dyHistory.empty() ? 0.0 : playerObj.dyHistory.front();
                                double mH = (playerObj.fireworkTicks > 0) ? 4.0 : ((dy < -0.1) ? 3.8 : ((oDy < -0.05 && dy >= 0) ? 0.8 : 1.8));
                                double acc = (playerObj.fireworkTicks > 0) ? 2.0 : 0.05;
                                lim = std::min(mH, (playerObj.lastDistance * 0.99) + acc);
                            } else {
                                if (gr) lim = 0.36;
                                else if (playerObj.wasGrounded) lim = 0.62;
                                else lim = (playerObj.lastDistance * 0.91) + 0.026;
                            }
                            lim = (lim * (1.0 + (0.20 * sLv))) + 0.03;
                            if (d2d > lim) {
                                playerObj.speedVL += 1.0;
                                if (playerObj.speedVL > globalSpeedVlKick) {
                                    server.sendToClient(clientSocket, "ALERT|" + pNameAddress + "|" + (el ? "Elytra" : "Speed") + " Detected\n");
                                    playerObj.speedVL = 0.0;
                                }
                            } else { playerObj.speedVL = std::max(0.0, playerObj.speedVL - 0.05); }
                            playerObj.lastDistance = d2d;
                        }
                        playerObj.lastPosX = playerObj.posX;
                        playerObj.lastPosY = playerObj.posY;
                        playerObj.lastPosZ = playerObj.posZ;
                        playerObj.posX = cX;
                        playerObj.posY = cY;
                        playerObj.posZ = cZ;
                        playerObj.wasGrounded = playerObj.isGrounded;
                        playerObj.isGrounded = gr;
                        if (playerObj.fireworkTicks > 0) playerObj.fireworkTicks--;
                        playerObj.initialized = true;
                        playerManager.updatePlayer(pNameAddress, playerObj);
                    }
                } catch (...) {}
            }
        }
    };

    server.start(onMessage);
    std::cout << "Anti-Cheat Engine started on port 25577." << std::endl;
    std::cin.get();
    server.stop();
    cleanupWinsock();
    return 0;
}
