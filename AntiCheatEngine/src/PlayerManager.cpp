#include "PlayerManager.h"
#include <algorithm>

PlayerState PlayerManager::getPlayer(const std::string& uuid) {
    std::shared_lock lock(m_mutex);
    if (m_players.find(uuid) == m_players.end()) {
        return PlayerState{};
    }
    return m_players.at(uuid);
}

void PlayerManager::updatePlayer(const std::string& uuid, const PlayerState& state) {
    std::unique_lock lock(m_mutex);
    m_players[uuid] = state;
}

void PlayerManager::updateVL(const std::string& uuid, float delta) {
    std::unique_lock lock(m_mutex);
    m_players[uuid].vl = std::max(0.0f, m_players[uuid].vl + delta);
}

void PlayerManager::decayVL(const std::string& uuid) {
    std::unique_lock lock(m_mutex);
    if (m_players.find(uuid) != m_players.end()) {
        m_players[uuid].vl = std::max(0.0f, m_players[uuid].vl - 0.1f);
    }
}
