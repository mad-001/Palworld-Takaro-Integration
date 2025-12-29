#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace TakaroPalworld {

// Forward declarations
struct PlayerInfo;
struct GuildInfo;
struct BaseCampInfo;
struct Position;

// Position structure
struct Position {
    float x;
    float y;
    float z;

    Position() : x(0.0f), y(0.0f), z(0.0f) {}
    Position(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// Player information
struct PlayerInfo {
    std::string playerId;       // PlayerUId
    std::string playerName;     // Display name
    std::string gender;         // Character gender
    int level;                  // Player level
    int rank;                   // Player rank
    float hp;                   // Current HP
    int lucky;                  // Lucky stat
    std::vector<std::string> passives;  // Passive abilities
    Position position;          // Current position

    PlayerInfo() : level(0), rank(0), hp(0.0f), lucky(0) {}
};

// Guild information
struct GuildInfo {
    std::string guildId;
    std::string guildName;
    std::string adminName;
    std::string adminId;
    int level;
    int memberCount;
    std::vector<std::string> memberIds;

    GuildInfo() : level(0), memberCount(0) {}
};

// BaseCamp information
struct BaseCampInfo {
    std::string campId;
    std::string guildId;
    int level;
    Position position;

    BaseCampInfo() : level(0) {}
};

// Command context
struct CommandContext {
    std::string commandName;
    std::vector<std::string> arguments;
    std::string executorId;
    std::string executorName;
    bool isAdmin;
    bool isRCON;

    std::string GetString(size_t index = 0) const {
        return index < arguments.size() ? arguments[index] : "";
    }

    float GetFloat(size_t index = 0) const {
        if (index < arguments.size()) {
            try {
                return std::stof(arguments[index]);
            } catch (...) {}
        }
        return 0.0f;
    }

    int GetInt(size_t index = 0) const {
        if (index < arguments.size()) {
            try {
                return std::stoi(arguments[index]);
            } catch (...) {}
        }
        return 0;
    }
};

// Command handler function
using CommandHandler = std::function<void(const CommandContext&)>;

// Command structure
struct Command {
    std::string name;
    std::string description;
    int minArgs;
    std::vector<std::string> aliases;
    bool requiresAdmin;
    bool rconOnly;
    bool chatOnly;
    int chatCharLimit;
    CommandHandler handler;

    Command() : minArgs(0), requiresAdmin(false), rconOnly(false),
                chatOnly(false), chatCharLimit(256) {}
};

// Hook structure
struct Hook {
    std::string functionName;
    std::string className;
    void* originalFunction;
    void* hookFunction;
    bool isHooked;

    Hook() : originalFunction(nullptr), hookFunction(nullptr), isHooked(false) {}
};

// Damage event data
struct DamageEvent {
    std::string attackerId;
    std::string targetId;
    float damageAmount;
    std::string weaponId;
    int attackerLevel;
    int targetLevel;
    bool isSelfDamage;
    bool isNPC;

    DamageEvent() : damageAmount(0.0f), attackerLevel(0), targetLevel(0),
                    isSelfDamage(false), isNPC(false) {}
};

// Anti-cheat detection result
enum class DetectionResult {
    ALLOWED,
    BLOCKED_NEGATIVE_DAMAGE,
    BLOCKED_EXTREME_DAMAGE,
    BLOCKED_MISSING_ENTITY,
    BLOCKED_SPOOFED_ATTACKER,
    BLOCKED_PVP_PROTECTION,
    BLOCKED_BUILDING_DAMAGE
};

// API Response structure
struct APIResponse {
    int statusCode;
    std::string message;
    std::string data;
    bool success;

    APIResponse() : statusCode(200), success(true) {}

    static APIResponse Success(const std::string& msg, const std::string& responseData = "") {
        APIResponse resp;
        resp.statusCode = 200;
        resp.success = true;
        resp.message = msg;
        resp.data = responseData;
        return resp;
    }

    static APIResponse Error(int code, const std::string& msg) {
        APIResponse resp;
        resp.statusCode = code;
        resp.success = false;
        resp.message = msg;
        return resp;
    }
};

} // namespace TakaroPalworld
