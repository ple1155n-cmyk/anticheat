#include "PlayerManager.h"
#include <algorithm>

PlayerState PlayerManager::getPlayer(const std::string& name) {
    std::shared_lock lock(m_mutex);
    if (m_players.find(name) == m_players.end()) {
        return PlayerState{};
    }
    return m_players.at(name);
}

void PlayerManager::updatePlayer(const std::string& name, const PlayerState& state) {
    std::unique_lock lock(m_mutex);
    m_players[name] = state;
}

void PlayerManager::updateVL(const std::string& name, float delta) {
    std::unique_lock lock(m_mutex);
    m_players[name].vl = std::max(0.0f, m_players[name].vl + delta);
}

void PlayerManager::decayVL(const std::string& name) {
    std::unique_lock lock(m_mutex);
    if (m_players.find(name) != m_players.end()) {
        m_players[name].vl = std::max(0.0f, m_players[name].vl - 0.1f);
    }
}
