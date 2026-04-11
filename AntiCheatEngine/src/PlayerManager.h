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
};

class PlayerManager {
public:
    PlayerState getPlayer(const std::string& uuid);
    void updatePlayer(const std::string& uuid, const PlayerState& state);
    void updateVL(const std::string& uuid, float delta);
    void decayVL(const std::string& uuid);

private:
    std::unordered_map<std::string, PlayerState> m_players;
    mutable std::shared_mutex m_mutex;
};

#endif // PLAYER_MANAGER_H
