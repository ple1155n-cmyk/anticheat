#ifndef CHECKS_H
#define CHECKS_H

#include <nlohmann/json.hpp>
#include "PlayerManager.h"
#include <string>
#include <optional>

using json = nlohmann::json;

struct Verdict {
    std::string player;
    std::string verdict;
    std::string type;
    float vl;
    bool cancelMove;
    std::string message;

    json toJson() const {
        return {
            {"player", player},
            {"verdict", verdict},
            {"type", type},
            {"vl", vl},
            {"cancelMove", cancelMove},
            {"message", message}
        };
    }
};

class Checks {
public:
    static std::optional<Verdict> performChecks(const json& data, PlayerManager& manager);

private:
    static std::optional<Verdict> checkSpeed(const std::string& uuid, double x, double y, double z, bool onGround, const std::string& blockBelow, long long ping, PlayerManager& manager);
    static std::optional<Verdict> checkFly(const std::string& uuid, double x, double y, double z, bool onGround, PlayerManager& manager);
};

#endif // CHECKS_H
