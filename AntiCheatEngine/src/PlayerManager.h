#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>

struct PlayerState {
    double lastX = 0, lastY = 0, lastZ = 0;
    bool onGround = true;
    float vl = 0.0f;
    long long lastPing = 0;
    bool initialized = false;
    double tokens = 50.0;
    unsigned long long lastUpdateTime = 0;
    bool lastOnGround = true;
    double lastDistance = 0.0;
};

class PlayerManager {
public:
    PlayerState getPlayer(const std::string& name);
    void updatePlayer(const std::string& name, const PlayerState& state);
    void updateVL(const std::string& name, float delta);
    void decayVL(const std::string& name);

private:
    std::unordered_map<std::string, PlayerState> m_players;
    mutable std::shared_mutex m_mutex;
};

#endif // PLAYER_MANAGER_H
