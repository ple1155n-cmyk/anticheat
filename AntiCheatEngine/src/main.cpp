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

            // --- Firework Tracking ---
            if (parts.size() >= 2 && parts[0] == "FIREWORK") {
                std::string playerName = parts[1];
                PlayerState state = playerManager.getPlayer(playerName);
                state.fireworkTicks = 40; // Grant 2 seconds of acceleration buffer (40 ticks)
                playerManager.updatePlayer(playerName, state);
                return;
            }

            // --- Advanced Movement Logic (Fly & Elytra) ---
            if (parts.size() >= 11 && parts[0] == "POS") {
                std::string playerName = parts[1];
                double x = std::stod(parts[2]);
                double y = std::stod(parts[3]);
                double z = std::stod(parts[4]);
                // Yaw/Pitch (parts[5], parts[6]) not used for current heuristics
                bool onGround = (parts[7] == "true");
                bool validElytra = (parts[8] == "true");
                int speedLevel = std::stoi(parts[9]);
                bool inLiquid = (parts[10] == "true");

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
                    std::string alertPacket = "ALERT|" + playerName + "|Timer Hack Detected (Packet Rate Too High)\n";
                    server.sendToClient(clientSocket, alertPacket);
                    std::cout << "[ALERT] " << playerName << " triggered Timer Hack (tokens: " << state.tokens << ")" << std::endl;
                    
                    state.tokens = globalMaxTokens; // Reset tokens to avoid kick spam
                    playerManager.updatePlayer(playerName, state);
                    return; // Stop processing this packet
                }

                // --- 2. Advanced Movement Detection ---
                if (state.initialized) {
                    double dx = x - state.lastX;
                    double dy = y - state.lastY;
                    double dz = z - state.lastZ;
                    double distance = std::sqrt(dx * dx + dz * dz);

                    // Update Air Ticks
                    if (!onGround) state.airTicks++; else state.airTicks = 0;

                    // Update Sliding Window Momentum (Max size 8)
                    state.dyHistory.push_back(dy);
                    if (state.dyHistory.size() > 8) state.dyHistory.pop_front();

                    // --- Layer 2: Vanilla Fly Check ---
                    if (!onGround && !validElytra && !inLiquid) {
                        if (dy >= 0.0) {
                            if (state.airTicks > 10) state.flyVL += 1.0; 
                        } else {
                            state.flyVL = std::max(0.0, state.flyVL - 0.05); // Forgive when falling
                        }

                        if (state.flyVL > 10.0) {
                            std::string alertPacket = "ALERT|" + playerName + "|Fly Hack Detected\n";
                            server.sendToClient(clientSocket, alertPacket);
                            std::cout << "[ALERT] " << playerName << " triggered Fly Hack (VL: " << state.flyVL << ")" << std::endl;
                            state.flyVL = 0.0;
                        }
                    }

                    // --- Layer 1: Movement Heuristics (Speed & Elytra) ---
                    double limit = 0.36; // Default ground speed

                    if (validElytra) {
                        double oldestDy = state.dyHistory.empty() ? 0.0 : state.dyHistory.front();
                        double maxHorizontalCap;

                        if (state.fireworkTicks > 0) {
                            maxHorizontalCap = 4.0; // Override for firework!
                        } else if (dy < -0.1) {
                            maxHorizontalCap = 3.8; // Vanilla diving
                        } else if (oldestDy < -0.05 && dy >= 0) {
                            maxHorizontalCap = 0.8; // Bounce penalty
                        } else {
                            maxHorizontalCap = 1.8; // Level flight without fireworks
                        }

                        double accel = (state.fireworkTicks > 0) ? 0.5 : 0.05;
                        limit = std::min(maxHorizontalCap, (state.lastDistance * 0.99) + accel);
                    } else {
                        // Standard Ground/Air friction logic
                        if (onGround) {
                            limit = 0.36;
                        } else if (state.lastOnGround) {
                            limit = 0.62; // Initial Jump
                        } else {
                            limit = (state.lastDistance * 0.91) + 0.026; // Air friction
                        }
                    }

                    // Apply Potion Multiplier and lag buffer
                    limit = (limit * (1.0 + (0.20 * speedLevel))) + 0.03;

                    if (distance > limit) {
                        state.speedVL += 1.0;
                        std::cout << "[DEBUG] " << playerName << " " << (validElytra ? "Elytra" : "Speed") 
                                  << " VL: " << state.speedVL << " (Dist: " << distance << " > " << limit << ")" << std::endl;

                        if (state.speedVL > globalSpeedVlKick) {
                            std::string alertType = validElytra ? "Elytra Exploit" : "Speed Hack";
                            std::string alertPacket = "ALERT|" + playerName + "|" + alertType + " Detected\n";
                            server.sendToClient(clientSocket, alertPacket);
                            std::cout << "[ALERT] " << playerName << " exceeded " << alertType << " threshold." << std::endl;
                            state.speedVL = 0.0;
                        }
                    } else {
                        state.speedVL = std::max(0.0, state.speedVL - 0.05);
                    }

                    state.lastDistance = distance;
                }

                // Update movement state
                state.lastX = x;
                state.lastY = y;
                state.lastZ = z;
                state.lastOnGround = onGround;
                state.onGround = onGround;
                if (state.fireworkTicks > 0) state.fireworkTicks--;
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
