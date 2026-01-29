#pragma once

#include "../Core/Types.h"
#include "../Core/Config.h"
#include <crow.h>
#include <memory>
#include <thread>

namespace TakaroPalworld {

// Forward declaration
class AuthMiddleware;
class CorsMiddleware;
class AccessMiddleware;

// REST API Server
class RestServer {
public:
    static RestServer& GetInstance();

    bool Start(const RESTConfig& config);
    void Stop();
    bool IsRunning() const { return running_; }

    void SetupRoutes();

private:
    RestServer() = default;
    ~RestServer() = default;
    RestServer(const RestServer&) = delete;
    RestServer& operator=(const RestServer&) = delete;

    // Route handlers
    crow::response HandleVersion();
    crow::response HandleGetGuilds();
    crow::response HandleGetGuild(const std::string& guildId);
    crow::response HandleGiveItem(const crow::request& req);
    crow::response HandleGetPlayers();
    crow::response HandleGetPlayer(const std::string& playerId);
    crow::response HandleGetPlayerInventory(const std::string& playerId);
    crow::response HandleTeleportPlayer(const crow::request& req);
    crow::response HandleKickPlayer(const crow::request& req);
    crow::response HandleBanPlayer(const crow::request& req);

    // Middleware
    std::shared_ptr<AuthMiddleware> authMiddleware_;
    std::shared_ptr<CorsMiddleware> corsMiddleware_;
    std::shared_ptr<AccessMiddleware> accessMiddleware_;

    // Crow app
    using App = crow::App<CorsMiddleware, AuthMiddleware, AccessMiddleware>;
    std::unique_ptr<App> app_;
    std::thread serverThread_;

    bool running_ = false;
    RESTConfig config_;
};

} // namespace TakaroPalworld
