#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <chrono>
#include "Server.h"
#include "PlayerManager.h"

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

// Global Config (Synced from Java)
double globalMaxTokens = 50.0;
double globalSpeedVlKick = 5.0;

// Helper to get current time in ms
unsigned long long getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

int main() {
    initializeWinsock();

    Server server(25577);
    PlayerManager playerManager;

    auto onMessage = [&](const std::string& rawMessage, SOCKET clientSocket) {
        if (rawMessage.empty()) return;

        std::vector<std::string> parts;
        std::stringstream ss(rawMessage);
        std::string item;
        while (std::getline(ss, item, '|')) {
            parts.push_back(item);
        }

        try {
            // --- Configuration Sync ---
            if (parts.size() >= 3 && parts[0] == "CONFIG") {
                globalMaxTokens = std::stod(parts[1]);
                globalSpeedVlKick = std::stod(parts[2]);
                std::cout << "[INFO] Received config from Java: max_tokens=" << globalMaxTokens 
                          << ", speed_vl=" << globalSpeedVlKick << std::endl;
                return;
            }

            if (parts.size() >= 7 && parts[0] == "MOVE") {
                std::string playerName = parts[1];
                double x = std::stod(parts[2]);
                double y = std::stod(parts[3]);
                double z = std::stod(parts[4]);
                bool onGround = (parts[5] == "true");
                int speedLevel = std::stoi(parts[6]);

                unsigned long long now = getCurrentTimeMs();
                PlayerState state = playerManager.getPlayer(playerName);

                // --- 1. Timer / TickShift Detection (Token Bucket) ---
                if (state.lastUpdateTime != 0) {
                    unsigned long long elapsed = now - state.lastUpdateTime;
                    
                    // Regenerate tokens: 1 token per 50ms (legal tick rate)
                    state.tokens += (double)elapsed / 50.0;
                    if (state.tokens > globalMaxTokens) state.tokens = globalMaxTokens; // Clamp max buffer
                }

                state.tokens -= 1.0; // Consume 1 token per movement packet
                state.lastUpdateTime = now;

                if (state.tokens < 0.0) {
                    std::string kickPacket = "KICK|" + playerName + "|Timer Hack Detected (Packet Rate Too High)\n";
                    server.sendToClient(clientSocket, kickPacket);
                    std::cout << "[KICK] " << playerName << " triggered Timer Hack (tokens: " << state.tokens << ")" << std::endl;
                    
                    state.tokens = globalMaxTokens; // Reset tokens to avoid kick spam
                    playerManager.updatePlayer(playerName, state);
                    return; // Stop processing this packet
                }

                // --- 2. Dynamic Speed Detection ---
                if (state.initialized) {
                    double dx = x - state.lastX;
                    double dz = z - state.lastZ;
                    double distance = std::sqrt(dx * dx + dz * dz);

                    double limit = 0.36;

                    if (onGround) {
                        limit = 0.36;
                    } else if (state.lastOnGround) {
                        // Jump (First air tick)
                        limit = 0.62;
                    } else {
                        // Air (Subsequent air ticks) - Friction math
                        limit = (state.lastDistance * 0.91) + 0.026;
                    }

                    // Apply Potion Multiplier and lag buffer
                    limit = (limit * (1.0 + (0.20 * speedLevel))) + 0.03;

                    if (distance > limit) {
                        state.speedVL += 1.0;
                        std::cout << "[DEBUG] " << playerName << " Speed VL: " << state.speedVL << std::endl;

                        if (state.speedVL > globalSpeedVlKick) {
                            std::string kickPacket = "KICK|" + playerName + "|Speed Hack Detected\n";
                            server.sendToClient(clientSocket, kickPacket);
                            std::cout << "[KICK] " << playerName << " exceeded Speed VL threshold (" << globalSpeedVlKick << ")." << std::endl;
                            state.speedVL = 0.0;
                        }
                    } else {
                        state.speedVL -= 0.05;
                        if (state.speedVL < 0.0) state.speedVL = 0.0;
                    }

                    state.lastDistance = distance;
                }

                // Update movement state
                state.lastX = x;
                state.lastY = y;
                state.lastZ = z;
                state.lastOnGround = onGround;
                state.onGround = onGround;
                state.initialized = true;
                playerManager.updatePlayer(playerName, state);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing packet: " << e.what() << " | Raw: " << rawMessage << std::endl;
        }
    };

    server.start(onMessage);

    std::cout << "Anti-Cheat Engine (Speed Module) started on port 25577." << std::endl;
    std::cout << "Press Enter to shutdown..." << std::endl;
    std::cin.get();

    server.stop();
    cleanupWinsock();

    return 0;
}
