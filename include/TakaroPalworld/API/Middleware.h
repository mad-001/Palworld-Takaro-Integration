#pragma once

#include "../Core/Config.h"
#include <crow.h>
#include <string>

namespace TakaroPalworld {

// CORS Middleware
struct CorsMiddleware {
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // Add CORS headers
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

        // Handle preflight requests
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.code = 200;
            res.end();
        }
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Nothing to do after handling
    }
};

// Authentication Middleware
struct AuthMiddleware {
    struct context {
        bool authenticated = false;
        std::string userId;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // Skip auth for OPTIONS requests
        if (req.method == crow::HTTPMethod::OPTIONS) {
            return;
        }

        // Get Authorization header
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty()) {
            res.code = 401;
            res.write("{\"error\": \"Missing Authorization header\"}");
            res.end();
            return;
        }

        // Extract Bearer token
        std::string token;
        if (authHeader.find("Bearer ") == 0) {
            token = authHeader.substr(7);
        } else {
            res.code = 401;
            res.write("{\"error\": \"Invalid Authorization format. Use: Bearer <token>\"}");
            res.end();
            return;
        }

        // Validate token
        auto& config = ConfigManager::GetInstance().GetRESTConfig();

        // Simple validation: check if token matches "456456"
        if (token != "456456" && !config.IsTokenValid(token)) {
            res.code = 401;
            res.write("{\"error\": \"Invalid or expired token\"}");
            res.end();
            return;
        }

        ctx.authenticated = true;
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Nothing to do after handling
    }
};

// Access Control Middleware
struct AccessMiddleware {
    struct context {
        bool hasAccess = false;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // Skip for OPTIONS requests
        if (req.method == crow::HTTPMethod::OPTIONS) {
            return;
        }

        // Check whitelist or permissions here if needed
        ctx.hasAccess = true;
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Nothing to do after handling
    }
};

} // namespace TakaroPalworld
