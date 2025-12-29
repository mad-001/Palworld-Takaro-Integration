#include "TakaroPalworld/Core/Config.h"
#include "TakaroPalworld/Core/Logger.h"
#include <fstream>
#include <filesystem>

namespace TakaroPalworld {

// MainConfig implementation
MainConfig::MainConfig()
    : allowAdminCheats(true)
    , pvpMaxToBuildingDamage(0)
    , preventAdminPasswordInChat(true)
    , enableRenaming(false)
    , enablePalRenaming(false)
    , bShowPlayerList(true)
{
    logging.level = "info";
    logging.rotatingFileSink.enabled = true;
    logging.rotatingFileSink.maxSize = 10485760; // 10 MB
    logging.rotatingFileSink.maxFiles = 5;

    antiCheat.maxDamageMultiplier = 10.0f;
    antiCheat.preventNegativeDamage = true;
    antiCheat.preventCrossPlayerDamage = true;
    antiCheat.preventPalOwnedByPlayerDamage = true;
}

void MainConfig::LoadFromJson(const json& j) {
    if (j.contains("allowAdminCheats"))
        allowAdminCheats = j["allowAdminCheats"];
    if (j.contains("pvpMaxToBuildingDamage"))
        pvpMaxToBuildingDamage = j["pvpMaxToBuildingDamage"];
    if (j.contains("preventAdminPasswordInChat"))
        preventAdminPasswordInChat = j["preventAdminPasswordInChat"];
    if (j.contains("enableRenaming"))
        enableRenaming = j["enableRenaming"];
    if (j.contains("enablePalRenaming"))
        enablePalRenaming = j["enablePalRenaming"];
    if (j.contains("bShowPlayerList"))
        bShowPlayerList = j["bShowPlayerList"];

    if (j.contains("logging")) {
        auto& log = j["logging"];
        if (log.contains("level"))
            logging.level = log["level"];
        if (log.contains("rotatingFileSink")) {
            auto& sink = log["rotatingFileSink"];
            if (sink.contains("enabled"))
                logging.rotatingFileSink.enabled = sink["enabled"];
            if (sink.contains("maxSize"))
                logging.rotatingFileSink.maxSize = sink["maxSize"];
            if (sink.contains("maxFiles"))
                logging.rotatingFileSink.maxFiles = sink["maxFiles"];
        }
    }

    if (j.contains("antiCheat")) {
        auto& ac = j["antiCheat"];
        if (ac.contains("maxDamageMultiplier"))
            antiCheat.maxDamageMultiplier = ac["maxDamageMultiplier"];
        if (ac.contains("preventNegativeDamage"))
            antiCheat.preventNegativeDamage = ac["preventNegativeDamage"];
        if (ac.contains("preventCrossPlayerDamage"))
            antiCheat.preventCrossPlayerDamage = ac["preventCrossPlayerDamage"];
        if (ac.contains("preventPalOwnedByPlayerDamage"))
            antiCheat.preventPalOwnedByPlayerDamage = ac["preventPalOwnedByPlayerDamage"];
    }
}

json MainConfig::ToJson() const {
    json j;
    j["allowAdminCheats"] = allowAdminCheats;
    j["pvpMaxToBuildingDamage"] = pvpMaxToBuildingDamage;
    j["preventAdminPasswordInChat"] = preventAdminPasswordInChat;
    j["enableRenaming"] = enableRenaming;
    j["enablePalRenaming"] = enablePalRenaming;
    j["bShowPlayerList"] = bShowPlayerList;

    j["logging"]["level"] = logging.level;
    j["logging"]["rotatingFileSink"]["enabled"] = logging.rotatingFileSink.enabled;
    j["logging"]["rotatingFileSink"]["maxSize"] = logging.rotatingFileSink.maxSize;
    j["logging"]["rotatingFileSink"]["maxFiles"] = logging.rotatingFileSink.maxFiles;

    j["antiCheat"]["maxDamageMultiplier"] = antiCheat.maxDamageMultiplier;
    j["antiCheat"]["preventNegativeDamage"] = antiCheat.preventNegativeDamage;
    j["antiCheat"]["preventCrossPlayerDamage"] = antiCheat.preventCrossPlayerDamage;
    j["antiCheat"]["preventPalOwnedByPlayerDamage"] = antiCheat.preventPalOwnedByPlayerDamage;

    return j;
}

// WhiteListConfig implementation
void WhiteListConfig::LoadFromJson(const json& j) {
    whitelistedPlayers.clear();
    whitelistedAdminIPs.clear();

    if (j.contains("whitelistedPlayers")) {
        for (const auto& player : j["whitelistedPlayers"]) {
            WhiteListedPlayer wp;
            wp.playerId = player.value("playerId", "");
            wp.playerName = player.value("playerName", "");
            wp.addedBy = player.value("addedBy", "");
            wp.addedDate = player.value("addedDate", "");
            whitelistedPlayers.push_back(wp);
        }
    }

    if (j.contains("whitelistedAdminIPs")) {
        whitelistedAdminIPs = j["whitelistedAdminIPs"].get<std::vector<std::string>>();
    }
}

json WhiteListConfig::ToJson() const {
    json j;
    j["whitelistedPlayers"] = json::array();
    for (const auto& player : whitelistedPlayers) {
        json p;
        p["playerId"] = player.playerId;
        p["playerName"] = player.playerName;
        p["addedBy"] = player.addedBy;
        p["addedDate"] = player.addedDate;
        j["whitelistedPlayers"].push_back(p);
    }
    j["whitelistedAdminIPs"] = whitelistedAdminIPs;
    return j;
}

bool WhiteListConfig::IsPlayerWhitelisted(const std::string& playerId) const {
    for (const auto& player : whitelistedPlayers) {
        if (player.playerId == playerId) {
            return true;
        }
    }
    return false;
}

bool WhiteListConfig::IsIPWhitelisted(const std::string& ip) const {
    for (const auto& whitelistedIP : whitelistedAdminIPs) {
        if (whitelistedIP == ip) {
            return true;
        }
    }
    return false;
}

// TakaroConfig implementation
TakaroConfig::TakaroConfig()
    : enabled(false)
    , websocketUrl("")
    , identityToken("")
    , registrationToken("")
{
}

void TakaroConfig::LoadFromJson(const json& j) {
    if (j.contains("enabled"))
        enabled = j["enabled"];
    if (j.contains("websocketUrl"))
        websocketUrl = j["websocketUrl"];
    if (j.contains("identityToken"))
        identityToken = j["identityToken"];
    if (j.contains("registrationToken"))
        registrationToken = j["registrationToken"];
}

json TakaroConfig::ToJson() const {
    return json{
        {"enabled", enabled},
        {"websocketUrl", websocketUrl},
        {"identityToken", identityToken},
        {"registrationToken", registrationToken}
    };
}

// RESTConfig implementation
RESTConfig::RESTConfig()
    : enabled(true)
    , host("0.0.0.0")
    , port(8080)
{
    cors.enabled = true;
    cors.allowedOrigins = {"*"};
    cors.allowedMethods = {"GET", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"};

    endpoints.version = true;
    endpoints.guilds = true;
    endpoints.players = true;
    endpoints.items = true;

    staticFiles.enabled = false;
    staticFiles.rootDirectory = "./static";
    staticFiles.defaultFile = "index.html";
}

void RESTConfig::LoadFromJson(const json& j) {
    if (j.contains("enabled"))
        enabled = j["enabled"];
    if (j.contains("host"))
        host = j["host"];
    if (j.contains("port"))
        port = j["port"];

    if (j.contains("cors")) {
        auto& c = j["cors"];
        if (c.contains("enabled"))
            cors.enabled = c["enabled"];
        if (c.contains("allowedOrigins"))
            cors.allowedOrigins = c["allowedOrigins"].get<std::vector<std::string>>();
        if (c.contains("allowedMethods"))
            cors.allowedMethods = c["allowedMethods"].get<std::vector<std::string>>();
    }

    if (j.contains("authentication")) {
        auto& auth = j["authentication"];
        if (auth.contains("bearerTokens"))
            authentication.bearerTokens = auth["bearerTokens"].get<std::vector<std::string>>();
    }

    if (j.contains("endpoints")) {
        auto& ep = j["endpoints"];
        if (ep.contains("version"))
            endpoints.version = ep["version"];
        if (ep.contains("guilds"))
            endpoints.guilds = ep["guilds"];
        if (ep.contains("players"))
            endpoints.players = ep["players"];
        if (ep.contains("items"))
            endpoints.items = ep["items"];
    }

    if (j.contains("staticFiles")) {
        auto& sf = j["staticFiles"];
        if (sf.contains("enabled"))
            staticFiles.enabled = sf["enabled"];
        if (sf.contains("rootDirectory"))
            staticFiles.rootDirectory = sf["rootDirectory"];
        if (sf.contains("defaultFile"))
            staticFiles.defaultFile = sf["defaultFile"];
    }
}

json RESTConfig::ToJson() const {
    json j;
    j["enabled"] = enabled;
    j["host"] = host;
    j["port"] = port;

    j["cors"]["enabled"] = cors.enabled;
    j["cors"]["allowedOrigins"] = cors.allowedOrigins;
    j["cors"]["allowedMethods"] = cors.allowedMethods;

    j["authentication"]["bearerTokens"] = authentication.bearerTokens;

    j["endpoints"]["version"] = endpoints.version;
    j["endpoints"]["guilds"] = endpoints.guilds;
    j["endpoints"]["players"] = endpoints.players;
    j["endpoints"]["items"] = endpoints.items;

    j["staticFiles"]["enabled"] = staticFiles.enabled;
    j["staticFiles"]["rootDirectory"] = staticFiles.rootDirectory;
    j["staticFiles"]["defaultFile"] = staticFiles.defaultFile;

    return j;
}

bool RESTConfig::IsTokenValid(const std::string& token) const {
    for (const auto& validToken : authentication.bearerTokens) {
        if (validToken == token) {
            return true;
        }
    }
    return false;
}

// ConfigManager implementation
ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadConfigurations(const std::string& configDir) {
    configDir_ = configDir;

    bool success = true;
    success &= LoadMainConfig(configDir + "/Config.json");
    success &= LoadWhiteListConfig(configDir + "/WhiteList.json");
    success &= LoadRESTConfig(configDir + "/RESTAPI/RESTConfig.json");
    success &= LoadTakaroConfig(configDir + "/TakaroConfig.json");

    return success;
}

bool ConfigManager::LoadMainConfig(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_WARNING("Config.json not found at '{}', creating default config", path);

            // Create directory if it doesn't exist
            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

            // Save default config
            std::ofstream file(path);
            file << mainConfig_.ToJson().dump(4);
            file.close();

            LOG_INFO("Created default Config.json");
            return true;
        }

        std::ifstream file(path);
        json j;
        file >> j;
        mainConfig_.LoadFromJson(j);

        LOG_INFO("Loaded Config.json");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load Config.json: {}", e.what());
        return false;
    }
}

bool ConfigManager::LoadWhiteListConfig(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_WARNING("WhiteList.json not found, creating default");

            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

            std::ofstream file(path);
            file << whiteListConfig_.ToJson().dump(4);
            file.close();

            return true;
        }

        std::ifstream file(path);
        json j;
        file >> j;
        whiteListConfig_.LoadFromJson(j);

        LOG_INFO("Loaded WhiteList.json");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load WhiteList.json: {}", e.what());
        return false;
    }
}

bool ConfigManager::LoadRESTConfig(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_WARNING("RESTConfig.json not found, creating default");

            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

            // Generate example token
            restConfig_.authentication.bearerTokens.clear();
            restConfig_.authentication.bearerTokens.push_back("Bearer-Token-Example-Change-This");

            std::ofstream file(path);
            file << restConfig_.ToJson().dump(4);
            file.close();

            LOG_INFO("[RESTAPI] Created default RESTConfig.json");

            // Create TokenExample.json
            json tokenExample;
            tokenExample["note"] = "This is an example token file. It is ignored by authentication.";
            tokenExample["instructions"] = "Copy tokens to RESTConfig.json authentication.bearerTokens array";
            tokenExample["exampleTokens"] = json::array();
            tokenExample["exampleTokens"].push_back("Bearer-Token-Example-1-Change-This-To-Random-String");
            tokenExample["exampleTokens"].push_back("Bearer-Token-Example-2-Use-Cryptographically-Secure-Random");

            std::ofstream tokenFile(std::filesystem::path(path).parent_path() / "TokenExample.json");
            tokenFile << tokenExample.dump(4);
            tokenFile.close();

            LOG_INFO("Did not find any Bearer tokens. Generating 'TokenExample.json'. (which is ignored by the authentication!)");

            return true;
        }

        std::ifstream file(path);
        json j;
        file >> j;
        restConfig_.LoadFromJson(j);

        LOG_INFO("[RESTAPI] Loaded 'RESTConfig.json'.");
        LOG_INFO("Loaded {} Bearer tokens.", restConfig_.authentication.bearerTokens.size());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load RESTConfig.json: {}", e.what());
        return false;
    }
}

bool ConfigManager::LoadTakaroConfig(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_WARNING("TakaroConfig.json not found at '{}', creating default config", path);

            std::filesystem::create_directories(std::filesystem::path(path).parent_path());

            std::ofstream file(path);
            file << takaroConfig_.ToJson().dump(4);
            file.close();

            LOG_INFO("Created default TakaroConfig.json (disabled by default)");
            LOG_INFO("Configure your Takaro credentials and set enabled:true to connect");
            return true;
        }

        std::ifstream file(path);
        json j;
        file >> j;
        takaroConfig_.LoadFromJson(j);

        LOG_INFO("Loaded TakaroConfig.json");
        if (takaroConfig_.enabled) {
            LOG_INFO("Takaro integration enabled - will connect to: {}", takaroConfig_.websocketUrl);
        } else {
            LOG_INFO("Takaro integration disabled");
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load TakaroConfig.json: {}", e.what());
        return false;
    }
}

bool ConfigManager::SaveConfigurations(const std::string& configDir) {
    try {
        std::ofstream file1(configDir + "/Config.json");
        file1 << mainConfig_.ToJson().dump(4);
        file1.close();

        std::ofstream file2(configDir + "/WhiteList.json");
        file2 << whiteListConfig_.ToJson().dump(4);
        file2.close();

        std::filesystem::create_directories(configDir + "/RESTAPI");
        std::ofstream file3(configDir + "/RESTAPI/RESTConfig.json");
        file3 << restConfig_.ToJson().dump(4);
        file3.close();

        LOG_INFO("Saved all configurations");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save configurations: {}", e.what());
        return false;
    }
}

void ConfigManager::ReloadConfigurations() {
    LOG_INFO("Takaro-Palworld-Integration WhiteList and Configuration reloaded!");
    LoadConfigurations(configDir_);
}

} // namespace TakaroPalworld
