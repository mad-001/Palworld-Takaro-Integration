#include "TakaroPalworld/Core/PalAPI.h"
#include "TakaroPalworld/Core/Logger.h"
#include "TakaroPalworld/Core/GameMemory.h"
#include "TakaroPalworld/Core/SaveParser.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

namespace TakaroPalworld {

PalAPI& PalAPI::GetInstance() {
    static PalAPI instance;
    return instance;
}

bool PalAPI::Initialize() {
    LOG_INFO("Initializing PalAPI...");

    // Initialize game memory access
    if (!GameMemory::Initialize()) {
        LOG_ERROR("Failed to initialize GameMemory");
        return false;
    }

    initialized_ = true;
    LOG_INFO("PalAPI initialized successfully");
    return true;
}

void PalAPI::Shutdown() {
    LOG_INFO("Shutting down PalAPI...");
    GameMemory::Shutdown();
    initialized_ = false;
}

// Player information
std::optional<PlayerInfo> PalAPI::GetPlayerInfo(const std::string& playerId) {
    if (!ValidatePlayerId(playerId)) {
        LOG_ERROR("Invalid PlayerUId: {}", playerId);
        return std::nullopt;
    }

    // TODO: Implement actual player info retrieval from game
    LOG_WARNING("GetPlayerInfo not yet implemented");
    return std::nullopt;
}

std::optional<PlayerInfo> PalAPI::GetPlayerInfoByName(const std::string& playerName) {
    // TODO: Convert player name to PlayerUId
    // TODO: Call GetPlayerInfo with the converted ID
    LOG_WARNING("GetPlayerInfoByName not yet implemented");
    return std::nullopt;
}

std::string PalAPI::ConvertPlayerNameToUId(const std::string& playerName) {
    // TODO: Implement player name to UID conversion
    LOG_WARNING("Player name '{}' to PlayerUId conversion not yet implemented", playerName);
    return "";
}

bool PalAPI::IsPlayerOnServer(const std::string& playerId) {
    // TODO: Check if player exists on server
    return false;
}

// Player state access (stubs - these would interface with UE4/Palworld)
APalPlayerState* PalAPI::GetPlayerState(const std::string& playerId) {
    // TODO: Retrieve APalPlayerState from game
    LOG_DEBUG("Failed to retrieve APalPlayerState for {}", playerId);
    return nullptr;
}

APalPlayerController* PalAPI::GetPlayerController(const std::string& playerId) {
    // TODO: Retrieve APalPlayerController from game
    LOG_DEBUG("Failed to retrieve APalPlayerController for {}", playerId);
    return nullptr;
}

APalPlayerCharacter* PalAPI::GetPlayerCharacter(const std::string& playerId) {
    // TODO: Retrieve APalPlayerCharacter from game
    LOG_DEBUG("Failed to retrieve APalPlayerCharacter for {}", playerId);
    return nullptr;
}

// Position and teleportation
std::optional<Position> PalAPI::GetPlayerPosition(const std::string& playerId) {
    // TODO: Get player character and extract position
    LOG_WARNING("GetPlayerPosition not yet implemented");
    return std::nullopt;
}

bool PalAPI::TeleportPlayer(const std::string& playerId, const Position& position) {
    // TODO: Implement teleportation via game function
    LOG_INFO("Teleporting player {} to ({:.2f}, {:.2f}, {:.2f})",
             playerId, position.x, position.y, position.z);
    return false;
}

bool PalAPI::TeleportToBaseCamp(const std::string& playerId) {
    // TODO: Find player's base camp and teleport
    LOG_WARNING("TeleportToBaseCamp not yet implemented");
    return false;
}

// Moderation
bool PalAPI::KickPlayer(const std::string& playerId, const std::string& reason) {
    // TODO: Implement kick via RCON or game function
    LOG_INFO("Kicking player {}: {}", playerId, reason);
    return false;
}

bool PalAPI::BanPlayer(const std::string& playerId, const std::string& reason) {
    // TODO: Implement ban via RCON or game function
    LOG_INFO("Banning player {}: {}", playerId, reason);
    return false;
}

bool PalAPI::UnbanPlayer(const std::string& playerId) {
    // TODO: Implement unban
    LOG_INFO("Unbanning player {}", playerId);
    return false;
}

std::vector<std::string> PalAPI::GetBannedPlayers() {
    // TODO: Retrieve ban list
    return {};
}

// Inventory management
bool PalAPI::GiveItem(const std::string& playerId, const std::string& itemId, int quantity) {
    if (!IsPlayerOnServer(playerId)) {
        LOG_ERROR("Player '{}' does not exist on your server", playerId);
        return false;
    }

    // TODO: Implement item giving via game function
    LOG_INFO("Giving {} {}x '{}' not yet implemented", playerId, quantity, itemId);
    return false;
}

bool PalAPI::ClearInventory(const std::string& playerId) {
    // TODO: Implement inventory clearing with backup
    LOG_WARNING("ClearInventory not yet implemented");
    return false;
}

std::string PalAPI::GetItemQuantity(const std::string& playerId, const std::string& itemId) {
    auto inventory = GetPlayerInventory(playerId);

    for (const auto& [itemName, count] : inventory) {
        if (itemName == itemId) {
            return std::to_string(count);
        }
    }

    return "0";
}

std::vector<std::pair<std::string, int>> PalAPI::GetPlayerInventory(const std::string& playerId) {
    std::vector<std::pair<std::string, int>> inventory;

    APalGameStateInGame* gameState = GameMemory::GetGameState();
    if (!gameState) {
        LOG_ERROR("Failed to get game state");
        return inventory;
    }

    // Try to find PlayerArray
    const uintptr_t playerArrayOffsets[] = {0x2A0, 0x2B0, 0x2C0, 0x2D0};
    TArray<APlayerState*>* playerArray = nullptr;

    for (uintptr_t offset : playerArrayOffsets) {
        auto* arr = reinterpret_cast<TArray<APlayerState*>*>(
            reinterpret_cast<uintptr_t>(gameState) + offset
        );
        if (arr && arr->IsValid()) {
            playerArray = arr;
            LOG_DEBUG("Found PlayerArray at offset 0x{:X}", offset);
            break;
        }
    }

    if (!playerArray || !playerArray->IsValid()) {
        LOG_ERROR("Failed to find PlayerArray");
        return inventory;
    }

    // Find the player
    APlayerState* targetPlayerState = nullptr;
    for (int32_t i = 0; i < playerArray->Num(); i++) {
        APlayerState* playerState = (*playerArray)[i];
        if (!GameMemory::IsValidPointer(playerState)) {
            continue;
        }

        // Try to find PlayerId (Steam ID)
        const uintptr_t idOffsets[] = {0x300, 0x310, 0x320, 0x330};
        for (uintptr_t offset : idOffsets) {
            FString* steamId = reinterpret_cast<FString*>(
                reinterpret_cast<uintptr_t>(playerState) + offset
            );
            if (steamId && steamId->Data) {
                std::string idStr = steamId->ToString();
                if (idStr == playerId) {
                    targetPlayerState = playerState;
                    LOG_DEBUG("Found player {} in PlayerArray", playerId);
                    break;
                }
            }
        }

        if (targetPlayerState) break;
    }

    if (!targetPlayerState) {
        LOG_WARNING("Player {} not found in PlayerArray", playerId);
        return inventory;
    }

    // Get InventoryData
    const uintptr_t invDataOffsets[] = {0x400, 0x410, 0x420, 0x430, 0x440, 0x450};
    UPalPlayerInventoryData* invData = nullptr;

    for (uintptr_t offset : invDataOffsets) {
        invData = GameMemory::ReadPointer<UPalPlayerInventoryData>(targetPlayerState, offset);
        if (GameMemory::IsValidPointer(invData)) {
            LOG_DEBUG("Found InventoryData at offset 0x{:X}", offset);
            break;
        }
    }

    if (!invData) {
        LOG_ERROR("Failed to find InventoryData for player {}", playerId);
        return inventory;
    }

    // Try to get Containers array
    const uintptr_t containerOffsets[] = {0x28, 0x30, 0x38, 0x40, 0x48, 0x50};
    TArray<void*>* containers = nullptr;

    for (uintptr_t offset : containerOffsets) {
        auto* arr = reinterpret_cast<TArray<void*>*>(
            reinterpret_cast<uintptr_t>(invData) + offset
        );
        if (arr && arr->IsValid()) {
            containers = arr;
            LOG_DEBUG("Found Containers array at offset 0x{:X} with {} containers",
                      offset, arr->Num());
            break;
        }
    }

    if (!containers || !containers->IsValid()) {
        LOG_WARNING("No containers found for player {}", playerId);
        return inventory;
    }

    // Iterate through containers (0 = main inventory, 1+ = other slots)
    for (int32_t containerIdx = 0; containerIdx < containers->Num() && containerIdx < 5; containerIdx++) {
        void* container = (*containers)[containerIdx];
        if (!GameMemory::IsValidPointer(container)) {
            continue;
        }

        // Try to find slots in container
        // Assuming slots array is at offset 0x28-0x50 in container
        const uintptr_t slotOffsets[] = {0x28, 0x30, 0x38, 0x40, 0x48, 0x50};
        TArray<FPalContainerSlot>* slots = nullptr;

        for (uintptr_t offset : slotOffsets) {
            auto* arr = reinterpret_cast<TArray<FPalContainerSlot>*>(
                reinterpret_cast<uintptr_t>(container) + offset
            );
            if (arr && arr->IsValid() && arr->Num() > 0) {
                slots = arr;
                break;
            }
        }

        if (!slots || !slots->IsValid()) {
            continue;
        }

        // Iterate through slots
        for (int32_t slotIdx = 0; slotIdx < slots->Num() && slotIdx < 50; slotIdx++) {
            FPalContainerSlot& slot = (*slots)[slotIdx];

            if (slot.StackCount > 0 && slot.ItemId) {
                // Try to get item ID string
                // ItemId is likely a struct with an FName or FString
                const uintptr_t itemIdOffsets[] = {0x0, 0x8, 0x10, 0x18};
                for (uintptr_t offset : itemIdOffsets) {
                    FString* itemIdStr = reinterpret_cast<FString*>(
                        reinterpret_cast<uintptr_t>(slot.ItemId) + offset
                    );
                    if (itemIdStr && itemIdStr->Data) {
                        std::string itemName = itemIdStr->ToString();
                        if (!itemName.empty()) {
                            inventory.push_back({itemName, slot.StackCount});
                            break;
                        }
                    }
                }
            }
        }
    }

    LOG_INFO("Retrieved {} items from player {} inventory", inventory.size(), playerId);
    return inventory;
}

// Technology management
bool PalAPI::UnlockTechnology(const std::string& playerId, const std::string& techId) {
    // TODO: Check if player already has technology
    // TODO: Unlock technology via game function
    LOG_WARNING("UnlockTechnology not yet implemented");
    return false;
}

bool PalAPI::HasTechnology(const std::string& playerId, const std::string& techId) {
    // TODO: Check player's technology list
    return false;
}

// Stats
bool PalAPI::AddStatPoints(const std::string& playerId, int points) {
    LOG_INFO("An Admin added {} unused Stat-Points to player {}", points, playerId);
    // TODO: Implement stat point addition
    return false;
}

bool PalAPI::RemoveStatPoints(const std::string& playerId, int points) {
    LOG_INFO("An Admin removed {} unused Stat-Points from player {}", points, playerId);
    // TODO: Implement stat point removal
    return false;
}

// Admin features
bool PalAPI::SetGodMode(const std::string& playerId, bool enabled) {
    if (enabled) {
        LOG_INFO("Godmode has been enabled for player {}!", playerId);
    } else {
        LOG_INFO("Godmode has been disabled for player {}!", playerId);
    }
    // TODO: Implement godmode toggle
    return false;
}

bool PalAPI::SetAdminMode(const std::string& playerId, bool enabled) {
    // TODO: Implement admin mode toggle
    return false;
}

// Guild management - with player cache
namespace {
    // Cache for player names (cleared when stale or when players change guilds)
    std::vector<std::string> cachedPlayerNames;
    std::chrono::steady_clock::time_point cacheTimestamp;
    const int CACHE_DURATION_MINUTES = 5;  // Cache valid for 5 minutes
}

std::vector<GuildInfo> PalAPI::GetGuilds() {
    LOG_INFO("GetGuilds() - Reading from save file");

    // Use save file parser instead of memory access
    std::string savePath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\PalServer\\Pal\\Saved\\SaveGames\\0\\D7EE49E04E615D88090F9380049E9855\\Level.sav";

    // Check if cache is still valid
    auto now = std::chrono::steady_clock::now();
    auto cacheAge = std::chrono::duration_cast<std::chrono::minutes>(now - cacheTimestamp).count();

    if (cachedPlayerNames.empty() || cacheAge >= CACHE_DURATION_MINUTES) {
        LOG_INFO("Player cache is stale or empty, refreshing from RCON...");

        try {
            // Get player list from RCON 'players' command
            std::string playersResponse = ExecuteCommand("players");

            if (!playersResponse.empty()) {
                // Parse JSON response
                auto playersJson = json::parse(playersResponse);

                cachedPlayerNames.clear();

                if (playersJson.contains("players") && playersJson["players"].is_array()) {
                    for (const auto& player : playersJson["players"]) {
                        if (player.contains("name")) {
                            std::string characterName = player["name"];
                            cachedPlayerNames.push_back(characterName);
                        }
                    }

                    LOG_INFO("Loaded {} player names from RCON into cache", cachedPlayerNames.size());
                } else {
                    LOG_WARNING("Players response missing 'players' array, using fallback");
                }
            } else {
                LOG_WARNING("Empty response from 'players' command, using fallback");
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to get players from RCON: {}, using fallback", e.what());
        }

        cacheTimestamp = now;
    } else {
        LOG_INFO("Using cached player list ({} players, cache age: {} minutes)",
                 cachedPlayerNames.size(), cacheAge);
    }

    std::vector<GuildInfo> guilds = SaveParser::ParseGuildsFromSave(savePath, cachedPlayerNames);

    LOG_INFO("Retrieved {} guilds from save file", guilds.size());
    return guilds;
}

std::optional<GuildInfo> PalAPI::GetGuildInfo(const std::string& guildId) {
    std::vector<GuildInfo> allGuilds = GetGuilds();

    for (const auto& guild : allGuilds) {
        if (guild.guildId == guildId) {
            return guild;
        }
    }

    return std::nullopt;
}

std::optional<GuildInfo> PalAPI::GetPlayerGuild(const std::string& playerId) {
    std::vector<GuildInfo> allGuilds = GetGuilds();

    for (const auto& guild : allGuilds) {
        for (const auto& memberId : guild.memberIds) {
            if (memberId == playerId) {
                return guild;
            }
        }
    }

    return std::nullopt;
}

// BaseCamp management
std::vector<BaseCampInfo> PalAPI::GetPlayerBaseCamps(const std::string& playerId) {
    // TODO: Retrieve player's base camps
    return {};
}

std::optional<BaseCampInfo> PalAPI::GetNearestBaseCamp(const Position& position) {
    // TODO: Find nearest base camp to position
    return std::nullopt;
}

bool PalAPI::DestroyBaseCamp(const std::string& campId) {
    // TODO: Destroy base camp
    LOG_WARNING("DestroyBaseCamp not yet implemented");
    return false;
}

// Pal spawning
bool PalAPI::SpawnPal(const std::string& palId, const Position& position, const std::string& saveParams) {
    if (!ValidateCharacterId(palId)) {
        LOG_ERROR("Invalid CharacterID: '{}'", palId);
        return false;
    }

    LOG_INFO("Spawning '{}' at {:.2f} {:.2f} {:.2f}", palId, position.x, position.y, position.z);

    // TODO: Implement pal spawning
    // TODO: Call GetPalSaveParam
    // TODO: Spawn pal at position

    LOG_ERROR("Failed to spawn Pal '{}'. Reason: GetPalSaveParam failed", palId);
    return false;
}

// Game state
APalGameStateInGame* PalAPI::GetGameState() {
    // TODO: Retrieve game state
    LOG_DEBUG("Failed to retrieve the APalGameStateInGame!");
    return nullptr;
}

UPalGameWorldSettings* PalAPI::GetWorldSettings() {
    // TODO: Retrieve world settings
    return nullptr;
}

UPalCharacterManager* PalAPI::GetCharacterManager() {
    // TODO: Retrieve character manager
    LOG_DEBUG("Failed to retrieve UPalCharacterManager");
    return nullptr;
}

UPalOilrigManager* PalAPI::GetOilrigManager() {
    // TODO: Retrieve oil rig manager
    return nullptr;
}

// Commands - Execute via REST API
std::string PalAPI::ExecuteCommand(const std::string& command) {
    LOG_INFO("Executing command: {}", command);

    // Parse command and arguments
    std::string cmd = command;
    std::string args;
    size_t spacePos = command.find(' ');
    if (spacePos != std::string::npos) {
        cmd = command.substr(0, spacePos);
        args = command.substr(spacePos + 1);
    }

    // Convert to lowercase for comparison
    std::string cmdLower = cmd;
    for (char& c : cmdLower) c = tolower(c);

    // Special handler for getGuilds - call C++ function directly
    if (cmdLower == "getguilds") {
        LOG_INFO("getGuilds command - calling GetGuilds()");
        std::vector<GuildInfo> guilds = GetGuilds();

        // Format each guild on separate lines
        std::string output = "";
        for (size_t i = 0; i < guilds.size(); i++) {
            if (i > 0) output += "\n";
            output += "Guild: " + guilds[i].guildName + "\n";
            output += "Members: " + std::to_string(guilds[i].memberCount) + "\n";
            for (const auto& member : guilds[i].memberIds) {
                output += "  - " + member + "\n";
            }
        }

        LOG_INFO("getGuilds returning: {}", output);
        return output;
    }

    // Built-in help command
    if (cmdLower == "help" || cmdLower == "commands") {
        std::string helpText = R"(
=== Palworld Server Commands ===

SERVER INFORMATION (GET):
  info, getversion
    Description: Get server version and info
    Example: info

  players, showplayers, getplayers
    Description: List all online players
    Example: players

  settings, getsettings
    Description: View server configuration settings
    Example: settings

  metrics, getmetrics
    Description: View server performance metrics
    Example: metrics

SERVER ACTIONS (POST):
  announce <message>
    Description: Broadcast a message to all players
    Arguments: <message> - The message to announce
    Example: announce Server will restart in 5 minutes

  kick <userid> [message]
    Description: Kick a player from the server
    Arguments: <userid> - Steam ID of player to kick
               [message] - Optional kick reason message
    Example: kick steam_76561198012345678
    Example: kick steam_76561198012345678 Violation of server rules

  ban <userid> [message]
    Description: Ban a player from the server
    Arguments: <userid> - Steam ID of player to ban
               [message] - Optional ban reason message
    Example: ban steam_76561198012345678
    Example: ban steam_76561198012345678 Cheating detected

  unban <userid>
    Description: Remove a player from the ban list
    Arguments: <userid> - Steam ID of player to unban
    Example: unban steam_76561198012345678

  save, saveworld
    Description: Force save the game world
    Example: save

  shutdown
    Description: Gracefully shutdown the server
    Example: shutdown

  stop, forcestop
    Description: Force stop the server immediately
    Example: stop

OTHER COMMANDS:
  help, commands
    Description: Show this help message
    Example: help

Note: All commands are case-insensitive.
For RCON commands not listed above, they will be forwarded directly to the game server.
)";
        return helpText;
    }

    // Check if it's a REST API command
    std::string endpoint;
    std::string method = "GET";
    std::string body;
    bool isRESTCommand = false;

    // GET endpoints
    if (cmdLower == "showplayers" || cmdLower == "getplayers" || cmdLower == "players") {
        endpoint = "/v1/api/players";
        isRESTCommand = true;
    }
    else if (cmdLower == "info" || cmdLower == "getversion") {
        endpoint = "/v1/api/info";
        isRESTCommand = true;
    }
    else if (cmdLower == "settings" || cmdLower == "getsettings") {
        endpoint = "/v1/api/settings";
        isRESTCommand = true;
    }
    else if (cmdLower == "metrics" || cmdLower == "getmetrics") {
        endpoint = "/v1/api/metrics";
        isRESTCommand = true;
    }
    // POST endpoints
    else if (cmdLower == "announce") {
        endpoint = "/v1/api/announce";
        method = "POST";
        // Build JSON body: {"message": "args"}
        body = "{\"message\":\"" + args + "\"}";
        isRESTCommand = true;
    }
    else if (cmdLower == "kick") {
        endpoint = "/v1/api/kick";
        method = "POST";
        // Parse args as userid and optional message
        // Format: kick userid [message]
        std::string userid = args;
        std::string message;
        size_t msgPos = args.find(' ');
        if (msgPos != std::string::npos) {
            userid = args.substr(0, msgPos);
            message = args.substr(msgPos + 1);
            body = "{\"userid\":\"" + userid + "\",\"message\":\"" + message + "\"}";
        } else {
            body = "{\"userid\":\"" + userid + "\"}";
        }
        isRESTCommand = true;
    }
    else if (cmdLower == "ban") {
        endpoint = "/v1/api/ban";
        method = "POST";
        // Parse args as userid and optional message
        // Format: ban userid [message]
        std::string userid = args;
        std::string message;
        size_t msgPos = args.find(' ');
        if (msgPos != std::string::npos) {
            userid = args.substr(0, msgPos);
            message = args.substr(msgPos + 1);
            body = "{\"userid\":\"" + userid + "\",\"message\":\"" + message + "\"}";
        } else {
            body = "{\"userid\":\"" + userid + "\"}";
        }
        isRESTCommand = true;
    }
    else if (cmdLower == "unban") {
        endpoint = "/v1/api/unban";
        method = "POST";
        // Build JSON body: {"userid": "args"}
        body = "{\"userid\":\"" + args + "\"}";
        isRESTCommand = true;
    }
    else if (cmdLower == "save" || cmdLower == "saveworld") {
        endpoint = "/v1/api/save";
        method = "POST";
        body = "{}";  // Empty JSON object
        isRESTCommand = true;
    }
    else if (cmdLower == "shutdown") {
        endpoint = "/v1/api/shutdown";
        method = "POST";
        // Optional: time in seconds and message
        // For now, send empty body
        body = "{}";
        isRESTCommand = true;
    }
    else if (cmdLower == "stop" || cmdLower == "forcestop") {
        endpoint = "/v1/api/stop";
        method = "POST";
        body = "{}";
        isRESTCommand = true;
    }

    // If it's a REST API command, use REST API
    if (isRESTCommand) {
        LOG_INFO("Executing via REST API: {} {} (body: {})", method, endpoint, body.empty() ? "none" : body);
        std::string response = CallRESTAPI(method, endpoint, body);
        if (!response.empty()) {
            LOG_INFO("REST API response: {}", response);
            return response;
        } else {
            LOG_ERROR("REST API call failed for command: {}", command);
            return "";
        }
    }

    // Otherwise, it's an in-game command - send via RCON
    LOG_INFO("Executing in-game command via RCON: {}", command);

    // Add / prefix if not present
    std::string rconCommand = command;
    if (rconCommand[0] != '/') {
        rconCommand = "/" + rconCommand;
    }

    std::string response = ExecuteRCON(rconCommand);

    if (!response.empty()) {
        LOG_INFO("RCON command response: {}", response);
        return response;
    } else {
        LOG_ERROR("RCON command failed: {}", command);
        return "";
    }
}

bool PalAPI::SendChatMessage(const std::string& message) {
    // Use RCON Broadcast command
    std::string command = "Broadcast " + message;
    std::string result = ExecuteCommand(command);
    return !result.empty();
}

bool PalAPI::SendRCONCommand(const std::string& command) {
    std::string result = ExecuteRCON(command);
    return !result.empty();
}

// RCON Implementation
bool PalAPI::SendRCONPacket(void* socket, const RCONPacket& packet) {
    // Calculate packet size (id + type + body + null terminators)
    int32_t size = 10 + packet.body.length();

    // Send size
    if (send((SOCKET)socket, (char*)&size, 4, 0) != 4) return false;

    // Send id
    if (send((SOCKET)socket, (char*)&packet.id, 4, 0) != 4) return false;

    // Send type
    if (send((SOCKET)socket, (char*)&packet.type, 4, 0) != 4) return false;

    // Send body
    if (send((SOCKET)socket, packet.body.c_str(), packet.body.length(), 0) != (int)packet.body.length()) return false;

    // Send null terminators
    char nulls[2] = {0, 0};
    if (send((SOCKET)socket, nulls, 2, 0) != 2) return false;

    return true;
}

bool PalAPI::ReceiveRCONPacket(void* socket, RCONPacket& packet) {
    // Debug logging
    HANDLE hFile = CreateFileA("RCON_DEBUG.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    auto logDebug = [&](const std::string& msg) {
        if (hFile != INVALID_HANDLE_VALUE) {
            std::string logMsg = "[RCV] " + msg + "\n";
            DWORD written;
            WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
            FlushFileBuffers(hFile);
        }
    };

    // Helper to receive exact number of bytes
    auto recvExact = [&](char* buffer, int size) -> bool {
        int totalReceived = 0;
        while (totalReceived < size) {
            int bytesReceived = recv((SOCKET)socket, buffer + totalReceived, size - totalReceived, 0);
            if (bytesReceived <= 0) {
                logDebug("recv failed: bytes=" + std::to_string(bytesReceived) + ", WSA error=" + std::to_string(WSAGetLastError()));
                return false;
            }
            totalReceived += bytesReceived;
            logDebug("Received " + std::to_string(bytesReceived) + " bytes, total=" + std::to_string(totalReceived) + "/" + std::to_string(size));
        }
        return true;
    };

    // Receive size (4 bytes)
    if (!recvExact((char*)&packet.size, 4)) {
        logDebug("ERROR: Failed to receive size");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        return false;
    }
    logDebug("Size: " + std::to_string(packet.size));

    // Receive id (4 bytes)
    if (!recvExact((char*)&packet.id, 4)) {
        logDebug("ERROR: Failed to receive id");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        return false;
    }
    logDebug("ID: " + std::to_string(packet.id));

    // Receive type (4 bytes)
    if (!recvExact((char*)&packet.type, 4)) {
        logDebug("ERROR: Failed to receive type");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        return false;
    }
    logDebug("Type: " + std::to_string(packet.type));

    // Receive body (size - 10 bytes for id, type, and null terminators)
    int bodySize = packet.size - 10;
    logDebug("Body size: " + std::to_string(bodySize));

    if (bodySize > 0 && bodySize < 4096) {
        char buffer[4096];
        if (!recvExact(buffer, bodySize)) {
            logDebug("ERROR: Failed to receive body");
            if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
            return false;
        }
        packet.body = std::string(buffer, bodySize - 2); // Exclude null terminators
        logDebug("Body: " + packet.body);
    } else if (bodySize <= 0) {
        packet.body = "";
        logDebug("Empty body");
    } else {
        logDebug("ERROR: Body size too large: " + std::to_string(bodySize));
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        return false;
    }

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    return true;
}

std::string PalAPI::ExecuteRCON(const std::string& command) {
    // RCON configuration
    const char* host = "127.0.0.1";
    const int port = 8201;
    const char* password = "456456";

    // Debug logging to file
    HANDLE hFile = CreateFileA("RCON_DEBUG.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    auto logDebug = [&](const std::string& msg) {
        if (hFile != INVALID_HANDLE_VALUE) {
            std::string logMsg = msg + "\n";
            DWORD written;
            WriteFile(hFile, logMsg.c_str(), logMsg.length(), &written, NULL);
            FlushFileBuffers(hFile);
        }
    };

    logDebug("=== RCON ExecuteCommand: " + command + " ===");

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WSAStartup failed");
        logDebug("ERROR: WSAStartup failed");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        return "";
    }

    logDebug("Winsock initialized");

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        LOG_ERROR("Failed to create socket");
        logDebug("ERROR: Failed to create socket");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        WSACleanup();
        return "";
    }

    logDebug("Socket created");

    // Set timeout
    DWORD timeout = 5000; // 5 seconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    logDebug("Timeouts set");

    // Connect to server
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host, &serverAddr.sin_addr);

    logDebug("Connecting to " + std::string(host) + ":" + std::to_string(port));

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        LOG_ERROR("Failed to connect to RCON server at {}:{}", host, port);
        logDebug("ERROR: Failed to connect, WSA error: " + std::to_string(err));
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    LOG_DEBUG("Connected to RCON server");
    logDebug("Connected successfully");

    // Authenticate
    logDebug("Sending authentication...");
    RCONPacket authPacket;
    authPacket.id = 1;
    authPacket.type = 3; // SERVERDATA_AUTH
    authPacket.body = password;

    if (!SendRCONPacket((void*)sock, authPacket)) {
        LOG_ERROR("Failed to send auth packet");
        logDebug("ERROR: Failed to send auth packet");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    logDebug("Auth packet sent, waiting for response...");

    // Receive FIRST auth response (empty packet from server)
    RCONPacket authResponse1;
    if (!ReceiveRCONPacket((void*)sock, authResponse1)) {
        LOG_ERROR("Failed to receive first auth response");
        logDebug("ERROR: Failed to receive first auth response");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    logDebug("First auth response: ID=" + std::to_string(authResponse1.id) + ", Type=" + std::to_string(authResponse1.type));

    // Receive SECOND auth response (actual auth confirmation)
    RCONPacket authResponse2;
    if (!ReceiveRCONPacket((void*)sock, authResponse2)) {
        LOG_ERROR("Failed to receive second auth response");
        logDebug("ERROR: Failed to receive second auth response");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    logDebug("Second auth response: ID=" + std::to_string(authResponse2.id) + ", Type=" + std::to_string(authResponse2.type));

    if (authResponse2.id == -1) {
        LOG_ERROR("RCON authentication failed");
        logDebug("ERROR: Authentication failed (ID=-1)");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    LOG_DEBUG("RCON authenticated successfully");
    logDebug("Authenticated successfully");

    // Send command
    logDebug("Sending command: " + command);
    RCONPacket cmdPacket;
    cmdPacket.id = 2;
    cmdPacket.type = 2; // SERVERDATA_EXECCOMMAND
    cmdPacket.body = command;

    if (!SendRCONPacket((void*)sock, cmdPacket)) {
        LOG_ERROR("Failed to send command packet");
        logDebug("ERROR: Failed to send command packet");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    logDebug("Command packet sent, waiting for response...");

    // Receive the response (Palworld seems to send single response)
    RCONPacket cmdResponse;
    if (!ReceiveRCONPacket((void*)sock, cmdResponse)) {
        LOG_ERROR("Failed to receive command response");
        logDebug("ERROR: Failed to receive command response");
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        closesocket(sock);
        WSACleanup();
        return "";
    }

    logDebug("Response received - ID: " + std::to_string(cmdResponse.id) + ", Type: " + std::to_string(cmdResponse.type) + ", Body: " + cmdResponse.body);
    LOG_INFO("RCON command executed: {} -> {}", command, cmdResponse.body);

    // Cleanup
    closesocket(sock);
    WSACleanup();

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

    return cmdResponse.body;
}

// Base64 encoding helper
static std::string base64_encode(const std::string& input) {
    static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = input.length();
    const char* bytes_to_encode = input.c_str();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

// REST API HTTP Client
std::string PalAPI::CallRESTAPI(const std::string& method, const std::string& endpoint, const std::string& body) {
    const wchar_t* host = L"127.0.0.1";
    const int port = 8202;  // Palworld's REST API port
    const std::string username = "admin";
    const std::string password = "456456";

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(L"Takaro-Palworld/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) {
        LOG_ERROR("WinHttpOpen failed");
        return "";
    }

    // Connect to server
    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect) {
        LOG_ERROR("WinHttpConnect failed");
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Convert endpoint to wide string
    std::wstring wEndpoint(endpoint.begin(), endpoint.end());

    // Convert method to wide string
    std::wstring wMethod(method.begin(), method.end());

    // Open request
    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
        wMethod.c_str(),
        wEndpoint.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest) {
        LOG_ERROR("WinHttpOpenRequest failed");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Add Basic Authorization header
    std::string credentials = username + ":" + password;
    std::string encodedCreds = base64_encode(credentials);
    std::string authHeader = "Authorization: Basic " + encodedCreds;
    std::wstring wAuthHeader(authHeader.begin(), authHeader.end());
    WinHttpAddRequestHeaders(hRequest, wAuthHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

    // Add Content-Type header for POST requests with body
    if (!body.empty()) {
        std::wstring contentTypeHeader = L"Content-Type: application/json";
        WinHttpAddRequestHeaders(hRequest, contentTypeHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
    }

    // Send request
    BOOL result = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(),
        body.length(),
        body.length(),
        0);

    if (!result) {
        LOG_ERROR("WinHttpSendRequest failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Receive response
    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        LOG_ERROR("WinHttpReceiveResponse failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Read data
    std::string response;
    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    char buffer[4096];

    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            LOG_ERROR("WinHttpQueryDataAvailable failed");
            break;
        }

        if (bytesAvailable > 0) {
            DWORD bytesToRead = (bytesAvailable < sizeof(buffer)) ? bytesAvailable : sizeof(buffer);
            if (!WinHttpReadData(hRequest, buffer, bytesToRead, &bytesRead)) {
                LOG_ERROR("WinHttpReadData failed");
                break;
            }
            response.append(buffer, bytesRead);
        }
    } while (bytesAvailable > 0);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

// Helper functions
bool PalAPI::ValidatePlayerId(const std::string& playerId) {
    return !playerId.empty();
}

bool PalAPI::ValidateCharacterId(const std::string& charId) {
    return !charId.empty();
}

} // namespace TakaroPalworld
