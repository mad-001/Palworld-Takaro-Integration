#include "TakaroPalworld/API/RestServer.h"
#include "TakaroPalworld/API/Middleware.h"
#include "TakaroPalworld/Core/PalAPI.h"
#include "TakaroPalworld/Core/Logger.h"
#include <nlohmann/json.hpp>

namespace TakaroPalworld {

using json = nlohmann::json;

RestServer& RestServer::GetInstance() {
    static RestServer instance;
    return instance;
}

bool RestServer::Start(const RESTConfig& config) {
    if (running_) {
        LOG_WARNING("REST server is already running");
        return false;
    }

    config_ = config;

    if (!config_.enabled) {
        LOG_INFO("REST API is disabled in configuration");
        return false;
    }

    try {
        // Create Crow app with middleware
        app_ = std::make_unique<App>();

        // Set up routes
        SetupRoutes();

        // Start server in separate thread
        serverThread_ = std::thread([this]() {
            try {
                LOG_INFO("[RESTAPI] Starting server on {}:{}", config_.host, config_.port);
                LOG_INFO("[RESTAPI] Binding to address and port...");
                app_->bindaddr(config_.host).port(config_.port).multithreaded().run();
                LOG_INFO("[RESTAPI] Server stopped normally");
            } catch (const std::exception& e) {
                LOG_ERROR("[RESTAPI] Server thread exception: {}", e.what());
            } catch (...) {
                LOG_ERROR("[RESTAPI] Server thread unknown exception");
            }
        });

        // Give the server thread a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        running_ = true;
        LOG_INFO("[RESTAPI] Server thread launched, binding should be in progress");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("[RESTAPI] Failed to start server: {}", e.what());
        return false;
    }
}

void RestServer::Stop() {
    if (!running_) {
        return;
    }

    LOG_INFO("[RESTAPI] Stopping server...");

    if (app_) {
        app_->stop();
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    running_ = false;
    LOG_INFO("[RESTAPI] Server stopped");
}

void RestServer::SetupRoutes() {
    auto& app = *app_;

    // Version endpoint
    CROW_ROUTE(app, "/v1/pdapi/version")
    .methods("GET"_method, "OPTIONS"_method)
    ([this]() {
        return HandleVersion();
    });

    // Guilds endpoints
    CROW_ROUTE(app, "/v1/pdapi/guilds")
    .methods("GET"_method, "OPTIONS"_method)
    ([this]() {
        return HandleGetGuilds();
    });

    CROW_ROUTE(app, "/v1/pdapi/guild/<string>")
    .methods("GET"_method, "OPTIONS"_method)
    ([this](const std::string& guildId) {
        return HandleGetGuild(guildId);
    });

    // Give item endpoint
    CROW_ROUTE(app, "/v1/pdapi/give")
    .methods("POST"_method, "OPTIONS"_method)
    ([this](const crow::request& req) {
        return HandleGiveItem(req);
    });

    // Players endpoints
    CROW_ROUTE(app, "/v1/pdapi/players")
    .methods("GET"_method, "OPTIONS"_method)
    ([this]() {
        return HandleGetPlayers();
    });

    CROW_ROUTE(app, "/v1/pdapi/player/<string>")
    .methods("GET"_method, "OPTIONS"_method)
    ([this](const std::string& playerId) {
        return HandleGetPlayer(playerId);
    });

    LOG_INFO("[RESTAPI] Routes configured successfully");
}

crow::response RestServer::HandleVersion() {
    json response;
    response["version"] = "1.0.0";
    response["apiVersion"] = "v1";
    response["gameVersion"] = "0.2.4.0";  // Example version

    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.write(response.dump());
    return res;
}

crow::response RestServer::HandleGetGuilds() {
    auto& api = PalAPI::GetInstance();
    auto guilds = api.GetGuilds();

    json response;
    response["guilds"] = json::array();

    for (const auto& guild : guilds) {
        json g;
        g["guildId"] = guild.guildId;
        g["guildName"] = guild.guildName;
        g["adminName"] = guild.adminName;
        g["adminId"] = guild.adminId;
        g["level"] = guild.level;
        g["memberCount"] = guild.memberCount;
        response["guilds"].push_back(g);
    }

    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.write(response.dump());
    return res;
}

crow::response RestServer::HandleGetGuild(const std::string& guildId) {
    auto& api = PalAPI::GetInstance();
    auto guildOpt = api.GetGuildInfo(guildId);

    if (!guildOpt.has_value()) {
        json error;
        error["success"] = false;
        error["error"] = "Guild not found";

        crow::response res;
        res.code = 404;
        res.set_header("Content-Type", "application/json");
        res.write(error.dump());
        return res;
    }

    auto& guild = guildOpt.value();
    json response;
    response["guildId"] = guild.guildId;
    response["guildName"] = guild.guildName;
    response["admin"]["name"] = guild.adminName;
    response["admin"]["id"] = guild.adminId;
    response["level"] = guild.level;
    response["memberCount"] = guild.memberCount;

    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.write(response.dump());
    return res;
}

crow::response RestServer::HandleGiveItem(const crow::request& req) {
    try {
        auto body = json::parse(req.body);

        std::string playerId = body.value("playerId", "");
        std::string itemId = body.value("itemId", "");
        int quantity = body.value("quantity", 0);

        if (playerId.empty() || itemId.empty() || quantity <= 0) {
            json error;
            error["success"] = false;
            error["error"] = "Invalid request parameters";

            crow::response res;
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            res.write(error.dump());
            return res;
        }

        auto& api = PalAPI::GetInstance();
        bool success = api.GiveItem(playerId, itemId, quantity);

        json response;
        response["success"] = success;
        if (success) {
            response["message"] = "Item given successfully";
        } else {
            response["message"] = "Failed to give item";
        }

        crow::response res;
        res.code = success ? 200 : 500;
        res.set_header("Content-Type", "application/json");
        res.write(response.dump());
        return res;

    } catch (const std::exception& e) {
        json error;
        error["success"] = false;
        error["error"] = std::string("Error parsing request: ") + e.what();

        crow::response res;
        res.code = 400;
        res.set_header("Content-Type", "application/json");
        res.write(error.dump());
        return res;
    }
}

crow::response RestServer::HandleGetPlayers() {
    // TODO: Implement player list retrieval
    json response;
    response["players"] = json::array();

    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.write(response.dump());
    return res;
}

crow::response RestServer::HandleGetPlayer(const std::string& playerId) {
    auto& api = PalAPI::GetInstance();
    auto playerOpt = api.GetPlayerInfo(playerId);

    if (!playerOpt.has_value()) {
        json error;
        error["success"] = false;
        error["error"] = "Player not found";

        crow::response res;
        res.code = 404;
        res.set_header("Content-Type", "application/json");
        res.write(error.dump());
        return res;
    }

    auto& player = playerOpt.value();
    json response;
    response["playerId"] = player.playerId;
    response["playerName"] = player.playerName;
    response["level"] = player.level;
    response["position"]["x"] = player.position.x;
    response["position"]["y"] = player.position.y;
    response["position"]["z"] = player.position.z;

    crow::response res;
    res.code = 200;
    res.set_header("Content-Type", "application/json");
    res.write(response.dump());
    return res;
}

} // namespace TakaroPalworld
