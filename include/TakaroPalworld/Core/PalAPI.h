#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace TakaroPalworld {

// Forward declarations for UE4/Palworld classes
class UPalGameWorldSettings;
class APalPlayerState;
class APalPlayerController;
class APalPlayerCharacter;
class APalGameStateInGame;
class UPalCharacterManager;
class UPalOilrigManager;

// PalAPI - Interface to Palworld game functions
class PalAPI {
public:
    static PalAPI& GetInstance();

    // Initialization
    bool Initialize();
    void Shutdown();

    // Player management
    std::optional<PlayerInfo> GetPlayerInfo(const std::string& playerId);
    std::optional<PlayerInfo> GetPlayerInfoByName(const std::string& playerName);
    std::string ConvertPlayerNameToUId(const std::string& playerName);
    bool IsPlayerOnServer(const std::string& playerId);

    // Player state access
    APalPlayerState* GetPlayerState(const std::string& playerId);
    APalPlayerController* GetPlayerController(const std::string& playerId);
    APalPlayerCharacter* GetPlayerCharacter(const std::string& playerId);

    // Player position
    std::optional<Position> GetPlayerPosition(const std::string& playerId);
    bool TeleportPlayer(const std::string& playerId, const Position& position);
    bool TeleportToBaseCamp(const std::string& playerId);

    // Player moderation
    bool KickPlayer(const std::string& playerId, const std::string& reason = "Kicked by admin.");
    bool BanPlayer(const std::string& playerId, const std::string& reason = "Banned by admin.");
    bool UnbanPlayer(const std::string& playerId);
    std::vector<std::string> GetBannedPlayers();

    // Inventory management
    bool GiveItem(const std::string& playerId, const std::string& itemId, int quantity);
    bool ClearInventory(const std::string& playerId);
    std::string GetItemQuantity(const std::string& playerId, const std::string& itemId);
    std::vector<std::pair<std::string, int>> GetPlayerInventory(const std::string& playerId);

    // Technology management
    bool UnlockTechnology(const std::string& playerId, const std::string& techId);
    bool HasTechnology(const std::string& playerId, const std::string& techId);

    // Stat management
    bool AddStatPoints(const std::string& playerId, int points);
    bool RemoveStatPoints(const std::string& playerId, int points);

    // Admin features
    bool SetGodMode(const std::string& playerId, bool enabled);
    bool SetAdminMode(const std::string& playerId, bool enabled);

    // Guild management
    std::vector<GuildInfo> GetGuilds();
    std::optional<GuildInfo> GetGuildInfo(const std::string& guildId);
    std::optional<GuildInfo> GetPlayerGuild(const std::string& playerId);

    // BaseCamp management
    std::vector<BaseCampInfo> GetPlayerBaseCamps(const std::string& playerId);
    std::optional<BaseCampInfo> GetNearestBaseCamp(const Position& position);
    bool DestroyBaseCamp(const std::string& campId);

    // Pal spawning
    bool SpawnPal(const std::string& palId, const Position& position, const std::string& saveParams = "");

    // Game state
    APalGameStateInGame* GetGameState();
    UPalGameWorldSettings* GetWorldSettings();
    UPalCharacterManager* GetCharacterManager();
    UPalOilrigManager* GetOilrigManager();

    // Command execution
    std::string ExecuteCommand(const std::string& command);  // Returns command output
    bool SendChatMessage(const std::string& message);

    // RCON
    bool SendRCONCommand(const std::string& command);

private:
    PalAPI() = default;
    ~PalAPI() = default;
    PalAPI(const PalAPI&) = delete;
    PalAPI& operator=(const PalAPI&) = delete;

    bool initialized_ = false;

    // Helper functions
    bool ValidatePlayerId(const std::string& playerId);
    bool ValidateCharacterId(const std::string& charId);

    // RCON helpers
    struct RCONPacket {
        int32_t size;
        int32_t id;
        int32_t type;
        std::string body;
    };

    bool SendRCONPacket(void* socket, const RCONPacket& packet);
    bool ReceiveRCONPacket(void* socket, RCONPacket& packet);
    std::string ExecuteRCON(const std::string& command);

    // REST API helper
    std::string CallRESTAPI(const std::string& method, const std::string& endpoint, const std::string& body = "");
};

} // namespace TakaroPalworld
