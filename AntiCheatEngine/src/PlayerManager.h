#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <deque>

struct PlayerState {
    double posX = 0, posY = 0, posZ = 0;
    double lastPosX = 0, lastPosY = 0, lastPosZ = 0;
    bool isGrounded = true;
    bool wasGrounded = true;
    float vl = 0.0f;
    long long lastPing = 0;
    bool initialized = false;
    double tokens = 50.0;
    unsigned long long lastUpdateTime = 0;
    double lastDistance = 0.0;
    double speedVL = 0.0;
    
    // New fields for Anti-Fly and Elytra Module
    double flyVL = 0.0;
    int airTicks = 0;
    int fireworkTicks = 0;
    std::deque<double> dyHistory; // Sliding window for Y-axis momentum (max size 8)
};

class PlayerManager {
public:
    PlayerState getPlayer(const std::string& name);
    void updatePlayer(const std::string& name, const PlayerState& state);
    void updateVL(const std::string& name, float delta);
    void decayVL(const std::string& name);
    void removePlayer(const std::string& name);

private:
    std::unordered_map<std::string, PlayerState> m_players;
    mutable std::shared_mutex m_mutex;
};

#endif // PLAYER_MANAGER_H
