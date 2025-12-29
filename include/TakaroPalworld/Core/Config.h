#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace TakaroPalworld {

using json = nlohmann::json;

// Main configuration structure
struct MainConfig {
    bool allowAdminCheats;
    int pvpMaxToBuildingDamage;
    bool preventAdminPasswordInChat;
    bool enableRenaming;
    bool enablePalRenaming;
    bool bShowPlayerList;

    struct LoggingConfig {
        std::string level;
        struct RotatingFileSink {
            bool enabled;
            size_t maxSize;
            size_t maxFiles;
        } rotatingFileSink;
    } logging;

    struct AntiCheatConfig {
        float maxDamageMultiplier;
        bool preventNegativeDamage;
        bool preventCrossPlayerDamage;
        bool preventPalOwnedByPlayerDamage;
    } antiCheat;

    MainConfig();
    void LoadFromJson(const json& j);
    json ToJson() const;
};

// WhiteList configuration
struct WhiteListConfig {
    struct WhiteListedPlayer {
        std::string playerId;
        std::string playerName;
        std::string addedBy;
        std::string addedDate;
    };

    std::vector<WhiteListedPlayer> whitelistedPlayers;
    std::vector<std::string> whitelistedAdminIPs;

    void LoadFromJson(const json& j);
    json ToJson() const;
    bool IsPlayerWhitelisted(const std::string& playerId) const;
    bool IsIPWhitelisted(const std::string& ip) const;
};

// REST API configuration
struct RESTConfig {
    bool enabled;
    std::string host;
    int port;

    struct CORSConfig {
        bool enabled;
        std::vector<std::string> allowedOrigins;
        std::vector<std::string> allowedMethods;
    } cors;

    struct AuthenticationConfig {
        std::vector<std::string> bearerTokens;
    } authentication;

    struct EndpointsConfig {
        bool version;
        bool guilds;
        bool players;
        bool items;
    } endpoints;

    struct StaticFilesConfig {
        bool enabled;
        std::string rootDirectory;
        std::string defaultFile;
    } staticFiles;

    RESTConfig();
    void LoadFromJson(const json& j);
    json ToJson() const;
    bool IsTokenValid(const std::string& token) const;
};

// Takaro integration configuration
struct TakaroConfig {
    bool enabled;
    std::string websocketUrl;
    std::string identityToken;
    std::string registrationToken;

    TakaroConfig();
    void LoadFromJson(const json& j);
    json ToJson() const;
};

// Configuration manager
class ConfigManager {
public:
    static ConfigManager& GetInstance();

    bool LoadConfigurations(const std::string& configDir);
    bool SaveConfigurations(const std::string& configDir);
    void ReloadConfigurations();

    MainConfig& GetMainConfig() { return mainConfig_; }
    WhiteListConfig& GetWhiteListConfig() { return whiteListConfig_; }
    RESTConfig& GetRESTConfig() { return restConfig_; }
    TakaroConfig& GetTakaroConfig() { return takaroConfig_; }

    const MainConfig& GetMainConfig() const { return mainConfig_; }
    const WhiteListConfig& GetWhiteListConfig() const { return whiteListConfig_; }
    const RESTConfig& GetRESTConfig() const { return restConfig_; }
    const TakaroConfig& GetTakaroConfig() const { return takaroConfig_; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool LoadMainConfig(const std::string& path);
    bool LoadWhiteListConfig(const std::string& path);
    bool LoadRESTConfig(const std::string& path);
    bool LoadTakaroConfig(const std::string& path);

    MainConfig mainConfig_;
    WhiteListConfig whiteListConfig_;
    RESTConfig restConfig_;
    TakaroConfig takaroConfig_;
    std::string configDir_;
};

} // namespace TakaroPalworld
