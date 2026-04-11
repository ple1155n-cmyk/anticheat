#include "Checks.h"
#include <cmath>
#include <iostream>

std::optional<Verdict> Checks::performChecks(const json& data, PlayerManager& manager) {
    std::string uuid = data["player"];
    double x = data["x"];
    double y = data["y"];
    double z = data["z"];
    bool onGround = data["onGround"];
    std::string blockBelow = data["blockBelow"];
    long long ping = data["ping"];

    // 1. Run Checks
    auto speedVerdict = checkSpeed(uuid, x, y, z, onGround, blockBelow, ping, manager);
    if (speedVerdict) return speedVerdict;

    auto flyVerdict = checkFly(uuid, x, y, z, onGround, manager);
    if (flyVerdict) return flyVerdict;

    // 2. If no flags, decay VL and update state
    manager.decayVL(uuid);
    
    PlayerState state = manager.getPlayer(uuid);
    state.lastX = x;
    state.lastY = y;
    state.lastZ = z;
    state.onGround = onGround;
    state.lastPing = ping;
    state.initialized = true;
    manager.updatePlayer(uuid, state);

    return std::nullopt;
}

std::optional<Verdict> Checks::checkSpeed(const std::string& uuid, double x, double y, double z, bool onGround, const std::string& blockBelow, long long ping, PlayerManager& manager) {
    PlayerState lastState = manager.getPlayer(uuid);
    if (!lastState.initialized) return std::nullopt;

    double dx = x - lastState.lastX;
    double dz = z - lastState.lastZ;
    double dist2D = std::sqrt(dx * dx + dz * dz);

    // Basic speed limit (sprinting is ~0.28 blocks/tick)
    double limit = 0.35; 
    
    // Adjust for ICE friction
    if (blockBelow == "ICE" || blockBelow == "PACKED_ICE" || blockBelow == "BLUE_ICE") {
        limit = 0.6; // High friction/speed tolerance for ice
    }

    // Ping buffer (very basic: +0.01 per 100ms)
    limit += (ping / 100.0) * 0.01;

    if (dist2D > limit) {
        manager.updateVL(uuid, 1.0f);
        PlayerState current = manager.getPlayer(uuid);
        return Verdict{uuid, "FLAG", "SPEED", current.vl, true, "Moving too fast (dist: " + std::to_string(dist2D) + ")"};
    }

    return std::nullopt;
}

std::optional<Verdict> Checks::checkFly(const std::string& uuid, double x, double y, double z, bool onGround, PlayerManager& manager) {
    PlayerState lastState = manager.getPlayer(uuid);
    if (!lastState.initialized) return std::nullopt;

    double dy = y - lastState.lastY;

    // Standard jump height is ~1.25 blocks
    if (!onGround && !lastState.onGround && dy > 0.6) {
        manager.updateVL(uuid, 1.5f);
        PlayerState current = manager.getPlayer(uuid);
        return Verdict{uuid, "FLAG", "FLY", current.vl, true, "Impossible Y-delta while airborne"};
    }

    return std::nullopt;
}
