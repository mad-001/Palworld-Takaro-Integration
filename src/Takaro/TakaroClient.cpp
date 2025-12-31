#include "TakaroPalworld/Takaro/TakaroClient.h"
#include "TakaroPalworld/Core/Logger.h"
#include "TakaroPalworld/Core/PalAPI.h"
#include <chrono>
#include <thread>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// Debug logging helper
static void WriteWSDebug(const std::string& msg) {
    HANDLE hFile = CreateFileA("TAKARO_WS_DEBUG.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        std::string timestamped = "[WS] " + msg;
        WriteFile(hFile, timestamped.c_str(), timestamped.length(), &written, NULL);
        WriteFile(hFile, "\r\n", 2, &written, NULL);
        FlushFileBuffers(hFile);
        CloseHandle(hFile);
    }
}

// WebSocket implementation using WinHTTP
namespace TakaroPalworld {

TakaroClient& TakaroClient::GetInstance() {
    static TakaroClient instance;
    return instance;
}

TakaroClient::~TakaroClient() {
    Shutdown();
}

bool TakaroClient::Initialize(const std::string& wsUrl, const std::string& identityToken, const std::string& registrationToken) {
    LOG_INFO("Initializing Takaro client");
    LOG_INFO("WebSocket URL: {}", wsUrl);
    LOG_INFO("Identity Token: {}", identityToken);

    wsUrl_ = wsUrl;
    identityToken_ = identityToken;
    registrationToken_ = registrationToken;

    if (wsUrl_.empty() || identityToken_.empty()) {
        LOG_ERROR("Takaro configuration incomplete - WebSocket URL and Identity Token are required");
        return false;
    }

    return true;
}

bool TakaroClient::Connect() {
    if (isConnected_) {
        LOG_WARNING("Already connected to Takaro");
        return true;
    }

    LOG_INFO("Starting Takaro WebSocket connection to {}", wsUrl_);

    shouldRun_ = true;
    connectionThread_ = std::make_unique<std::thread>(&TakaroClient::RunConnection, this);

    return true;
}

void TakaroClient::Disconnect() {
    LOG_INFO("Disconnecting from Takaro");
    shouldRun_ = false;
    isConnected_ = false;

    if (connectionThread_ && connectionThread_->joinable()) {
        connectionThread_->join();
    }
}

void TakaroClient::Shutdown() {
    LOG_INFO("Shutting down Takaro client");
    Disconnect();
}

void TakaroClient::RunConnection() {
    WriteWSDebug("Connection thread started");
    LOG_INFO("Takaro connection thread started");

    while (shouldRun_) {
        try {
            if (!isConnected_) {
                WriteWSDebug("Attempting connection (attempt " + std::to_string(reconnectAttempts_ + 1) + ")");
                LOG_INFO("Attempting to connect to Takaro (attempt {})", reconnectAttempts_ + 1);

                // Initialize WinHTTP
                hSession_ = WinHttpOpen(L"Takaro-Palworld-Integration/1.0",
                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS,
                    0);

                if (!hSession_) {
                    DWORD err = GetLastError();
                    WriteWSDebug("WinHttpOpen failed: " + std::to_string(err));
                    LOG_ERROR("WinHttpOpen failed: {}", err);
                    ScheduleReconnect();
                    continue;
                }
                WriteWSDebug("WinHttpOpen succeeded");

                // Connect to server
                hConnection_ = WinHttpConnect(static_cast<HINTERNET>(hSession_),
                    L"connect.takaro.io",
                    INTERNET_DEFAULT_HTTPS_PORT,
                    0);

                if (!hConnection_) {
                    DWORD err = GetLastError();
                    WriteWSDebug("WinHttpConnect failed: " + std::to_string(err));
                    LOG_ERROR("WinHttpConnect failed: {}", err);
                    WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
                    hSession_ = nullptr;
                    ScheduleReconnect();
                    continue;
                }
                WriteWSDebug("WinHttpConnect succeeded");

                // Open WebSocket request
                HINTERNET hRequest = WinHttpOpenRequest(static_cast<HINTERNET>(hConnection_),
                    L"GET",
                    L"/",
                    NULL,
                    WINHTTP_NO_REFERER,
                    WINHTTP_DEFAULT_ACCEPT_TYPES,
                    WINHTTP_FLAG_SECURE);

                if (!hRequest) {
                    LOG_ERROR("WinHttpOpenRequest failed: {}", GetLastError());
                    WinHttpCloseHandle(static_cast<HINTERNET>(hConnection_));
                    WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
                    hConnection_ = nullptr;
                    hSession_ = nullptr;
                    ScheduleReconnect();
                    continue;
                }

                // Upgrade to WebSocket
                BOOL result = WinHttpSetOption(hRequest,
                    WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET,
                    NULL,
                    0);

                if (!result) {
                    LOG_ERROR("Failed to set WebSocket upgrade option: {}", GetLastError());
                }

                // Send request
                result = WinHttpSendRequest(hRequest,
                    WINHTTP_NO_ADDITIONAL_HEADERS,
                    0,
                    WINHTTP_NO_REQUEST_DATA,
                    0,
                    0,
                    0);

                if (!result) {
                    LOG_ERROR("WinHttpSendRequest failed: {}", GetLastError());
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(static_cast<HINTERNET>(hConnection_));
                    WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
                    hConnection_ = nullptr;
                    hSession_ = nullptr;
                    ScheduleReconnect();
                    continue;
                }

                // Receive response
                result = WinHttpReceiveResponse(hRequest, NULL);
                if (!result) {
                    LOG_ERROR("WinHttpReceiveResponse failed: {}", GetLastError());
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(static_cast<HINTERNET>(hConnection_));
                    WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
                    hConnection_ = nullptr;
                    hSession_ = nullptr;
                    ScheduleReconnect();
                    continue;
                }

                // Complete WebSocket upgrade
                hWebSocket_ = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);
                WinHttpCloseHandle(hRequest);

                if (!hWebSocket_) {
                    LOG_ERROR("WinHttpWebSocketCompleteUpgrade failed: {}", GetLastError());
                    WinHttpCloseHandle(static_cast<HINTERNET>(hConnection_));
                    WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
                    hConnection_ = nullptr;
                    hSession_ = nullptr;
                    ScheduleReconnect();
                    continue;
                }

                WriteWSDebug("WebSocket connection established successfully!");
                LOG_INFO("WebSocket connection established successfully!");
                isConnected_ = true;
                reconnectAttempts_ = 0;

                // Send identify message
                WriteWSDebug("Sending identify message...");
                SendIdentify();
            }

            // Receive messages
            if (isConnected_ && hWebSocket_) {
                BYTE buffer[4096];
                DWORD bytesRead = 0;
                WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

                DWORD error = WinHttpWebSocketReceive(static_cast<HINTERNET>(hWebSocket_),
                    buffer,
                    sizeof(buffer),
                    &bytesRead,
                    &bufferType);

                if (error == ERROR_SUCCESS) {
                    if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
                        bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
                        std::string message(reinterpret_cast<char*>(buffer), bytesRead);
                        LOG_DEBUG("Received message from Takaro: {}", message);
                        HandleMessage(message);
                    }
                } else if (error != ERROR_WINHTTP_TIMEOUT) {
                    LOG_ERROR("WebSocket receive error: {}", error);
                    isConnected_ = false;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        } catch (const std::exception& e) {
            LOG_ERROR("Takaro connection error: {}", e.what());
            isConnected_ = false;

            if (hWebSocket_) {
                WinHttpCloseHandle(static_cast<HINTERNET>(hWebSocket_));
                hWebSocket_ = nullptr;
            }
            if (hConnection_) {
                WinHttpCloseHandle(static_cast<HINTERNET>(hConnection_));
                hConnection_ = nullptr;
            }
            if (hSession_) {
                WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
                hSession_ = nullptr;
            }

            ScheduleReconnect();
        }
    }

    // Cleanup
    if (hWebSocket_) {
        WinHttpWebSocketClose(static_cast<HINTERNET>(hWebSocket_), WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, NULL, 0);
        WinHttpCloseHandle(static_cast<HINTERNET>(hWebSocket_));
        hWebSocket_ = nullptr;
    }
    if (hConnection_) {
        WinHttpCloseHandle(static_cast<HINTERNET>(hConnection_));
        hConnection_ = nullptr;
    }
    if (hSession_) {
        WinHttpCloseHandle(static_cast<HINTERNET>(hSession_));
        hSession_ = nullptr;
    }

    LOG_INFO("Takaro connection thread stopped");
}

void TakaroClient::SendIdentify() {
    if (!isConnected_ || !hWebSocket_) {
        LOG_ERROR("Cannot send identify - not connected to Takaro");
        return;
    }

    try {
        json identifyMessage = {
            {"type", "identify"},
            {"payload", {
                {"identityToken", identityToken_}
            }}
        };

        if (!registrationToken_.empty()) {
            identifyMessage["payload"]["registrationToken"] = registrationToken_;
        }

        std::string message = identifyMessage.dump();
        LOG_INFO("Sending identify message to Takaro: {}", message);

        // Send via WebSocket
        DWORD error = WinHttpWebSocketSend(static_cast<HINTERNET>(hWebSocket_),
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)message.c_str(),
            (DWORD)message.length());

        if (error != ERROR_SUCCESS) {
            LOG_ERROR("Failed to send identify message: {}", error);
        } else {
            LOG_INFO("Identify message sent successfully");
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send identify message: {}", e.what());
    }
}

void TakaroClient::SendEvent(const std::string& eventType, const json& payload) {
    if (!isConnected_ || !hWebSocket_) {
        LOG_WARNING("Cannot send event '{}' - not connected to Takaro", eventType);
        return;
    }

    try {
        json eventMessage = {
            {"type", eventType},
            {"payload", payload}
        };

        std::string message = eventMessage.dump();
        LOG_DEBUG("Sending event to Takaro: {}", message);

        // Send via WebSocket
        DWORD error = WinHttpWebSocketSend(static_cast<HINTERNET>(hWebSocket_),
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)message.c_str(),
            (DWORD)message.length());

        if (error != ERROR_SUCCESS) {
            LOG_ERROR("Failed to send event: {}", error);
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send event '{}': {}", eventType, e.what());
    }
}

void TakaroClient::HandleMessage(const std::string& message) {
    try {
        json parsed = json::parse(message);

        std::string messageType = parsed.value("type", "unknown");
        LOG_DEBUG("Received message from Takaro: type={}", messageType);

        if (messageType == "identifyResponse") {
            if (parsed.contains("payload") && parsed["payload"].contains("error")) {
                LOG_ERROR("Identification failed: {}", parsed["payload"]["error"].dump());
            } else {
                LOG_INFO("Successfully identified with Takaro");
            }
        }
        else if (messageType == "connected") {
            LOG_INFO("Takaro confirmed connection");
        }
        else if (messageType == "ping") {
            // CRITICAL: Respond to ping immediately to stay online
            LOG_DEBUG("Received ping from Takaro, sending pong");
            SendPong();
        }
        else if (messageType == "request") {
            LOG_INFO("Received request from Takaro");
            HandleRequest(parsed);
        }
        else if (messageType == "error") {
            LOG_ERROR("Takaro error: {}", parsed.dump());
        }
        else {
            LOG_WARNING("Unknown message type from Takaro: {}", messageType);
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse Takaro message: {}", e.what());
    }
}

void TakaroClient::SendPong() {
    if (!isConnected_ || !hWebSocket_) {
        LOG_ERROR("Cannot send pong - not connected to Takaro");
        return;
    }

    try {
        json pongMessage = {
            {"type", "pong"}
        };

        std::string message = pongMessage.dump();

        DWORD error = WinHttpWebSocketSend(static_cast<HINTERNET>(hWebSocket_),
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)message.c_str(),
            (DWORD)message.length());

        if (error != ERROR_SUCCESS) {
            LOG_ERROR("Failed to send pong: {}", error);
        } else {
            LOG_DEBUG("Pong sent successfully");
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send pong: {}", e.what());
    }
}

void TakaroClient::HandleRequest(const json& message) {
    try {
        if (!message.contains("requestId") || !message.contains("payload")) {
            LOG_ERROR("Invalid request message - missing requestId or payload");
            return;
        }

        std::string requestId = message["requestId"];
        json payload = message["payload"];
        std::string action = payload.value("action", "");

        LOG_INFO("Handling request: {} (requestId: {})", action, requestId);

        // Write to boot log for debugging
        {
            HANDLE hFile = CreateFileA("TAKARO_REQUESTS.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                std::string logMsg = "REQUEST: " + action + " (requestId: " + requestId + ")\n";
                if (payload.contains("args")) {
                    logMsg += "ARGS: " + payload["args"].dump() + "\n";
                }
                DWORD written;
                WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
                FlushFileBuffers(hFile);
                CloseHandle(hFile);
            }
        }

        json responsePayload;

        if (action == "testReachability") {
            responsePayload = {
                {"connectable", true},
                {"reason", nullptr}
            };
        }
        else if (action == "executeCommand" || action == "executeConsoleCommand") {
            // Parse command from args
            std::string command;
            if (payload.contains("args")) {
                auto args = payload["args"];
                if (args.is_string()) {
                    // Args is JSON string, parse it
                    auto parsedArgs = json::parse(args.get<std::string>());
                    command = parsedArgs.value("command", "");
                } else if (args.is_object()) {
                    command = args.value("command", args.value("message", ""));
                }
            }

            // Log command execution
            {
                HANDLE hFile = CreateFileA("TAKARO_REQUESTS.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    std::string logMsg = "EXECUTING COMMAND: " + command + "\n";
                    DWORD written;
                    WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
                    FlushFileBuffers(hFile);
                    CloseHandle(hFile);
                }
            }

            // Execute the command via PalAPI
            auto& palAPI = PalAPI::GetInstance();
            std::string result = palAPI.ExecuteCommand(command);

            if (!result.empty()) {
                responsePayload = {
                    {"success", true},
                    {"rawResult", result}
                };

                // Log success
                {
                    HANDLE hFile = CreateFileA("TAKARO_REQUESTS.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                        std::string logMsg = "COMMAND SUCCESS: " + command + "\nRESPONSE: " + result + "\n";
                        DWORD written;
                        WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
                        FlushFileBuffers(hFile);
                        CloseHandle(hFile);
                    }
                }
            } else {
                responsePayload = {
                    {"success", false},
                    {"rawResult", "Failed to execute command: " + command}
                };

                // Log failure
                {
                    HANDLE hFile = CreateFileA("TAKARO_REQUESTS.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                        std::string logMsg = "COMMAND FAILED: " + command + "\n";
                        DWORD written;
                        WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
                        FlushFileBuffers(hFile);
                        CloseHandle(hFile);
                    }
                }
            }
        }
        else if (action == "getPlayers") {
            // Return empty players list for now
            responsePayload = json::array();
        }
        else if (action == "getGuilds") {
            try {
                PalAPI& palAPI = PalAPI::GetInstance();
                std::vector<GuildInfo> guilds = palAPI.GetGuilds();

                json guildsArray = json::array();
                for (const auto& guild : guilds) {
                    json guildObj = {
                        {"guildId", guild.guildId},
                        {"guildName", guild.guildName},
                        {"adminId", guild.adminId},
                        {"adminName", guild.adminName},
                        {"level", guild.level},
                        {"memberCount", guild.memberCount},
                        {"memberIds", guild.memberIds}
                    };
                    guildsArray.push_back(guildObj);
                }

                responsePayload = guildsArray;
                LOG_INFO("Returned {} guilds to Takaro", guilds.size());

            } catch (const std::exception& e) {
                LOG_ERROR("Failed to get guilds: {}", e.what());
                responsePayload = {
                    {"error", "Failed to retrieve guilds: " + std::string(e.what())}
                };
            }
        }
        else if (action == "getInventory") {
            try {
                if (!payload.contains("playerId")) {
                    responsePayload = {
                        {"error", "Missing required parameter: playerId"}
                    };
                } else {
                    std::string playerId = payload["playerId"];
                    PalAPI& palAPI = PalAPI::GetInstance();
                    auto inventory = palAPI.GetPlayerInventory(playerId);

                    json inventoryArray = json::array();
                    for (const auto& [itemId, count] : inventory) {
                        json itemObj = {
                            {"itemId", itemId},
                            {"count", count}
                        };
                        inventoryArray.push_back(itemObj);
                    }

                    responsePayload = inventoryArray;
                    LOG_INFO("Returned {} items for player {}", inventory.size(), playerId);
                }

            } catch (const std::exception& e) {
                LOG_ERROR("Failed to get inventory: {}", e.what());
                responsePayload = {
                    {"error", "Failed to retrieve inventory: " + std::string(e.what())}
                };
            }
        }
        else if (action == "getServerInfo") {
            responsePayload = {
                {"servername", "Takaro-Palworld Server"},
                {"version", "1.0.0"}
            };
        }
        else {
            responsePayload = {
                {"error", "Action not implemented yet: " + action}
            };
        }

        SendResponse(requestId, responsePayload);

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to handle request: {}", e.what());

        // Log error to boot log
        {
            HANDLE hFile = CreateFileA("TAKARO_REQUESTS.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                std::string logMsg = "ERROR handling request: " + std::string(e.what()) + "\n";
                DWORD written;
                WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
                FlushFileBuffers(hFile);
                CloseHandle(hFile);
            }
        }
    }
}

void TakaroClient::SendResponse(const std::string& requestId, const json& payload) {
    if (!isConnected_ || !hWebSocket_) {
        LOG_ERROR("Cannot send response - not connected to Takaro");
        return;
    }

    try {
        json responseMessage = {
            {"type", "response"},
            {"requestId", requestId},
            {"payload", payload}
        };

        std::string message = responseMessage.dump();

        DWORD error = WinHttpWebSocketSend(static_cast<HINTERNET>(hWebSocket_),
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)message.c_str(),
            (DWORD)message.length());

        if (error != ERROR_SUCCESS) {
            LOG_ERROR("Failed to send response: {}", error);
        } else {
            LOG_INFO("Response sent for requestId: {}", requestId);
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send response: {}", e.what());
    }
}

void TakaroClient::ScheduleReconnect() {
    reconnectAttempts_++;

    // Exponential backoff with max delay
    int delayMs = std::min(
        BASE_RECONNECT_DELAY_MS * (1 << (reconnectAttempts_ - 1)),
        MAX_RECONNECT_DELAY_MS
    );

    LOG_INFO("Scheduling reconnect attempt {} in {}ms", reconnectAttempts_, delayMs);

    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
}

} // namespace TakaroPalworld
