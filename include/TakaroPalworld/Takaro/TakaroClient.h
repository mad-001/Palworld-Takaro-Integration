#pragma once

#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

namespace TakaroPalworld {

using json = nlohmann::json;

class TakaroClient {
public:
    static TakaroClient& GetInstance();

    // Initialize with Takaro configuration
    bool Initialize(const std::string& wsUrl, const std::string& identityToken, const std::string& registrationToken);

    // Start WebSocket connection
    bool Connect();

    // Disconnect from Takaro
    void Disconnect();

    // Send event to Takaro
    void SendEvent(const std::string& eventType, const json& payload);

    // Check if connected
    bool IsConnected() const { return isConnected_; }

    // Shutdown
    void Shutdown();

private:
    TakaroClient() = default;
    ~TakaroClient();
    TakaroClient(const TakaroClient&) = delete;
    TakaroClient& operator=(const TakaroClient&) = delete;

    // WebSocket connection thread
    void RunConnection();

    // Send identify message
    void SendIdentify();

    // Handle incoming messages
    void HandleMessage(const std::string& message);

    // Send pong response to ping
    void SendPong();

    // Handle request from Takaro
    void HandleRequest(const json& message);

    // Send response to Takaro request
    void SendResponse(const std::string& requestId, const json& payload);

    // Reconnection logic
    void ScheduleReconnect();

    std::string wsUrl_;
    std::string identityToken_;
    std::string registrationToken_;

    std::atomic<bool> isConnected_{false};
    std::atomic<bool> shouldRun_{false};

    std::unique_ptr<std::thread> connectionThread_;

    void* hSession_{nullptr};  // HINTERNET
    void* hConnection_{nullptr}; // HINTERNET
    void* hWebSocket_{nullptr}; // HINTERNET

    // Reconnection state
    int reconnectAttempts_{0};
    static constexpr int MAX_RECONNECT_DELAY_MS = 60000;  // 60 seconds
    static constexpr int BASE_RECONNECT_DELAY_MS = 3000;  // 3 seconds
};

} // namespace TakaroPalworld
