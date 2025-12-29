# Implementation Guide

This guide explains how to implement the stub components and integrate Takaro-Palworld-Integration with Palworld's game engine.

## Overview

The reconstructed source code contains complete implementations for:
- ✅ Configuration system
- ✅ Logging system
- ✅ REST API framework
- ✅ Authentication middleware

These components need implementation:
- ⚠️ Game hooks (UE4/Palworld function hooking)
- ⚠️ Player management (game-specific operations)
- ⚠️ Command handlers
- ⚠️ Anti-cheat logic

## Prerequisites

Before implementing game integration, you need:

1. **UE4SS** or similar Unreal Engine hooking framework
   - Download: https://github.com/UE4SS-RE/RE-UE4SS
   - Provides function hooking and UObject access

2. **Palworld SDK/Headers** (if available)
   - Game-specific class definitions
   - Function signatures

3. **Debugging Tools**
   - x64dbg or similar debugger
   - Cheat Engine for memory analysis
   - IDA Pro or Ghidra for reverse engineering

## Step 1: Setting Up UE4SS Integration

### Include UE4SS Headers

Add to `CMakeLists.txt`:
```cmake
include_directories(
    ${CMAKE_SOURCE_DIR}/external/UE4SS/include
)

target_link_libraries(Takaro-Palworld-Integration PRIVATE
    ${CMAKE_SOURCE_DIR}/external/UE4SS/lib/UE4SS.lib
)
```

### Initialize UE4SS in Main.cpp

```cpp
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>

void Initialize() {
    // ... existing initialization ...

    // Initialize UE4SS
    RC::UObjectGlobals::InitUObjectArray();

    // Initialize PalAPI
    if (!PalAPI::GetInstance().Initialize()) {
        LOG_ERROR("Failed to initialize PalAPI");
        return;
    }

    // ... rest of initialization ...
}
```

## Step 2: Implementing PalAPI Game Functions

### Finding Game Objects

Edit `src/Core/PalAPI.cpp`:

```cpp
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/AActor.hpp>

bool PalAPI::Initialize() {
    LOG_INFO("Initializing PalAPI...");

    try {
        // Find game state
        auto gameState = RC::UObjectGlobals::FindFirstOf(L"PalGameStateInGame");
        if (!gameState) {
            LOG_ERROR("Failed to find PalGameStateInGame");
            return false;
        }

        // Find player controller
        auto playerController = RC::UObjectGlobals::FindFirstOf(L"PalPlayerController");
        if (!playerController) {
            LOG_ERROR("Failed to find PalPlayerController");
            return false;
        }

        initialized_ = true;
        LOG_INFO("PalAPI initialized successfully");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Exception during PalAPI initialization: {}", e.what());
        return false;
    }
}
```

### Implementing Player Position Tracking

```cpp
std::optional<Position> PalAPI::GetPlayerPosition(const std::string& playerId) {
    try {
        // Find player by ID
        auto playerState = GetPlayerState(playerId);
        if (!playerState) {
            LOG_ERROR("Failed to retrieve APalPlayerState for {}", playerId);
            return std::nullopt;
        }

        // Get player character
        auto playerCharacter = playerState->GetPlayerCharacter();
        if (!playerCharacter) {
            LOG_ERROR("Failed to retrieve APalPlayerCharacter for {}", playerId);
            return std::nullopt;
        }

        // Get actor location
        auto location = playerCharacter->GetActorLocation();

        Position pos;
        pos.x = location.X;
        pos.y = location.Y;
        pos.z = location.Z;

        return pos;

    } catch (const std::exception& e) {
        LOG_ERROR("Exception in GetPlayerPosition: {}", e.what());
        return std::nullopt;
    }
}
```

### Implementing Teleportation

```cpp
bool PalAPI::TeleportPlayer(const std::string& playerId, const Position& position) {
    try {
        auto playerController = GetPlayerController(playerId);
        if (!playerController) {
            return false;
        }

        auto playerCharacter = GetPlayerCharacter(playerId);
        if (!playerCharacter) {
            return false;
        }

        // Create FVector for new location
        FVector newLocation;
        newLocation.X = position.x;
        newLocation.Y = position.y;
        newLocation.Z = position.z;

        // Teleport
        playerCharacter->SetActorLocation(newLocation);

        LOG_INFO("Teleported player {} to ({:.2f}, {:.2f}, {:.2f})",
                 playerId, position.x, position.y, position.z);

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Exception in TeleportPlayer: {}", e.what());
        return false;
    }
}
```

## Step 3: Implementing Game Hooks

### Hook Manager Implementation

Edit `src/Hooks/HookManager.cpp`:

```cpp
#include "Takaro-Palworld-Integration/Hooks/HookManager.h"
#include "Takaro-Palworld-Integration/Core/Logger.h"
#include <Windows.h>
#include <detours.h>  // Microsoft Detours for hooking

namespace Takaro-Palworld-Integration {

class HookManager {
public:
    static HookManager& GetInstance() {
        static HookManager instance;
        return instance;
    }

    bool Initialize() {
        LOG_INFO("Initializing hook manager...");

        // Hook damage functions
        if (!HookDamageFunctions()) {
            LOG_ERROR("Failed to hook damage functions");
            return false;
        }

        // Hook RCON functions
        if (!HookRCONFunctions()) {
            LOG_ERROR("Failed to hook RCON functions");
            return false;
        }

        // Hook chat functions
        if (!HookChatFunctions()) {
            LOG_ERROR("Failed to hook chat functions");
            return false;
        }

        LOG_INFO("Hook manager initialized successfully");
        return true;
    }

private:
    bool HookDamageFunctions();
    bool HookRCONFunctions();
    bool HookChatFunctions();
};

} // namespace Takaro-Palworld-Integration
```

### Damage Hook Implementation

Edit `src/Hooks/DamageHooks.cpp`:

```cpp
#include "Takaro-Palworld-Integration/Core/Logger.h"
#include "Takaro-Palworld-Integration/Core/Config.h"
#include "Takaro-Palworld-Integration/AntiCheat/DamageValidator.h"
#include <Windows.h>

namespace Takaro-Palworld-Integration {

// Original function pointers
using DamageToSelfPlayer_t = void(*)(void*, float, void*, void*);
DamageToSelfPlayer_t Original_DamageToSelfPlayer = nullptr;

using DamageToNPC_t = void(*)(void*, float, void*, void*);
DamageToNPC_t Original_DamageToNPC = nullptr;

// Hook function for damage to self
void Hooked_DamageToSelfPlayer(void* thisPtr, float damage, void* attacker, void* weapon) {
    auto& config = ConfigManager::GetInstance().GetMainConfig();

    // Validate damage
    if (config.antiCheat.preventNegativeDamage && damage < 0) {
        LOG_WARNING("Attempted to deal negative damage ({})", damage);
        return;  // Block the damage
    }

    // Check extreme damage
    // TODO: Get attacker and target levels
    float maxDamage = 100.0f * config.antiCheat.maxDamageMultiplier;
    if (damage > maxDamage) {
        LOG_WARNING("Attempted to deal extreme amount of damage ({})", damage);
        return;  // Block the damage
    }

    // Call original function
    Original_DamageToSelfPlayer(thisPtr, damage, attacker, weapon);
}

// Hook function for damage to NPC
void Hooked_DamageToNPC(void* thisPtr, float damage, void* attacker, void* weapon) {
    auto& config = ConfigManager::GetInstance().GetMainConfig();

    // Similar validation
    if (config.antiCheat.preventNegativeDamage && damage < 0) {
        LOG_WARNING("Attempted to deal negative damage to NPC ({})", damage);
        return;
    }

    // Call original function
    Original_DamageToNPC(thisPtr, damage, attacker, weapon);
}

bool HookDamageFunctions() {
    // TODO: Find function addresses using pattern scanning
    // Example addresses (these need to be found for your game version):
    HMODULE gameModule = GetModuleHandle(L"Pal-Win64-Shipping.exe");
    if (!gameModule) {
        LOG_ERROR("Failed to get game module handle");
        return false;
    }

    // Pattern scan for DamageReactionComponent_ProcessDamage_ToServer_ToSelfPlayer
    // TODO: Implement pattern scanning
    uintptr_t damageToSelfAddr = 0x140000000;  // Placeholder

    // Hook using Microsoft Detours or similar
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    Original_DamageToSelfPlayer = (DamageToSelfPlayer_t)damageToSelfAddr;
    DetourAttach(&(PVOID&)Original_DamageToSelfPlayer, Hooked_DamageToSelfPlayer);

    LONG error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        LOG_ERROR("Failed to hook damage functions: {}", error);
        return false;
    }

    LOG_INFO("Successfully hooked damage functions");
    return true;
}

} // namespace Takaro-Palworld-Integration
```

### RCON Hook Implementation

Edit `src/Hooks/RCONHooks.cpp`:

```cpp
#include "Takaro-Palworld-Integration/Core/Logger.h"

namespace Takaro-Palworld-Integration {

// Original function pointers
using RCONHandlePacket_t = void(*)(void*, void*);
RCONHandlePacket_t Original_RCONHandlePacket = nullptr;

using RCONSendMessage_t = void(*)(void*, const char*);
RCONSendMessage_t Original_RCONSendMessage = nullptr;

// Hook for RCON packet handling
void Hooked_RCONHandlePacket(void* thisPtr, void* packet) {
    LOG_DEBUG("RCON packet received");

    // TODO: Parse packet and intercept commands if needed

    // Call original function
    Original_RCONHandlePacket(thisPtr, packet);
}

// Hook for RCON message sending
void Hooked_RCONSendMessage(void* thisPtr, const char* message) {
    LOG_DEBUG("RCON sending message: {}", message);

    // Call original function
    Original_RCONSendMessage(thisPtr, message);
}

bool HookRCONFunctions() {
    // TODO: Find RCON function addresses
    // TODO: Hook using Detours or similar

    LOG_INFO("RCON hooks setup (stub implementation)");
    return true;
}

} // namespace Takaro-Palworld-Integration
```

## Step 4: Finding Game Function Addresses

### Pattern Scanning

Create `src/Core/PatternScanner.h`:

```cpp
#pragma once
#include <Windows.h>
#include <vector>
#include <string>

class PatternScanner {
public:
    static uintptr_t FindPattern(const char* moduleName, const char* pattern, const char* mask) {
        HMODULE module = GetModuleHandleA(moduleName);
        if (!module) return 0;

        MODULEINFO moduleInfo;
        GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof(MODULEINFO));

        uintptr_t base = (uintptr_t)module;
        size_t size = moduleInfo.SizeOfImage;

        size_t patternLength = strlen(mask);

        for (size_t i = 0; i < size - patternLength; i++) {
            bool found = true;
            for (size_t j = 0; j < patternLength; j++) {
                if (mask[j] != '?' && pattern[j] != *(char*)(base + i + j)) {
                    found = false;
                    break;
                }
            }

            if (found) {
                return base + i;
            }
        }

        return 0;
    }
};
```

### Finding Function Patterns

Use tools like x64dbg or IDA Pro to find unique byte patterns:

```cpp
// Example usage
uintptr_t damageFunc = PatternScanner::FindPattern(
    "Pal-Win64-Shipping.exe",
    "\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57",
    "xxxx?xxxx?xxxx?x"
);
```

## Step 5: Implementing Commands

### Command Registration

Edit `src/Commands/CommandManager.cpp`:

```cpp
#include "Takaro-Palworld-Integration/Commands/CommandManager.h"
#include "Takaro-Palworld-Integration/Core/PalAPI.h"
#include "Takaro-Palworld-Integration/Core/Logger.h"

namespace Takaro-Palworld-Integration {

class CommandManager {
public:
    static CommandManager& GetInstance() {
        static CommandManager instance;
        return instance;
    }

    void RegisterCommands() {
        // Register getpos command
        Command getpos;
        getpos.name = "getpos";
        getpos.description = "Get player position";
        getpos.minArgs = 1;
        getpos.requiresAdmin = true;
        getpos.handler = [](const CommandContext& ctx) {
            std::string playerId = ctx.GetString(0);
            auto& api = PalAPI::GetInstance();

            auto posOpt = api.GetPlayerPosition(playerId);
            if (!posOpt.has_value()) {
                LOG_ERROR("Failed to get position for player {}", playerId);
                return;
            }

            auto& pos = posOpt.value();
            LOG_INFO("Player {} position: ({:.2f}, {:.2f}, {:.2f})",
                     playerId, pos.x, pos.y, pos.z);
        };
        commands_["getpos"] = getpos;

        // Register teleport command
        // TODO: Implement teleport command

        // Register give item command
        // TODO: Implement give item command
    }

    bool ExecuteCommand(const std::string& commandName, const CommandContext& ctx) {
        auto it = commands_.find(commandName);
        if (it == commands_.end()) {
            LOG_ERROR("Command '{}' not found", commandName);
            return false;
        }

        auto& cmd = it->second;

        // Check permissions
        if (cmd.requiresAdmin && !ctx.isAdmin) {
            LOG_WARNING("Insufficient permission to execute command '{}'", commandName);
            return false;
        }

        // Check arguments
        if (ctx.arguments.size() < cmd.minArgs) {
            LOG_ERROR("Command '{}' requires at least {} arguments", commandName, cmd.minArgs);
            return false;
        }

        // Execute
        try {
            cmd.handler(ctx);
            LOG_INFO("Command '{}' executed successfully", commandName);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Command '{}' execution failed: {}", commandName, e.what());
            return false;
        }
    }

private:
    std::map<std::string, Command> commands_;
};

} // namespace Takaro-Palworld-Integration
```

## Step 6: Implementing Anti-Cheat Logic

### Damage Validator

Edit `src/AntiCheat/DamageValidator.cpp`:

```cpp
#include "Takaro-Palworld-Integration/AntiCheat/DamageValidator.h"
#include "Takaro-Palworld-Integration/Core/Config.h"
#include "Takaro-Palworld-Integration/Core/Logger.h"

namespace Takaro-Palworld-Integration {

class DamageValidator {
public:
    static DetectionResult ValidateDamage(const DamageEvent& event) {
        auto& config = ConfigManager::GetInstance().GetMainConfig().antiCheat;

        // Check for negative damage
        if (config.preventNegativeDamage && event.damageAmount < 0) {
            LOG_WARNING("Blocked: Negative damage ({}) to {}",
                        event.damageAmount, event.targetId);
            return DetectionResult::BLOCKED_NEGATIVE_DAMAGE;
        }

        // Check for extreme damage
        float maxDamage = event.attackerLevel * config.maxDamageMultiplier;
        if (event.damageAmount > maxDamage) {
            LOG_WARNING("Blocked: Extreme damage ({}) to {} with {}. Attacker Level={}, Max allowed={}",
                        event.damageAmount, event.targetId, event.weaponId,
                        event.attackerLevel, maxDamage);
            return DetectionResult::BLOCKED_EXTREME_DAMAGE;
        }

        // Check for missing entities
        if (event.attackerId.empty() || event.targetId.empty()) {
            LOG_WARNING("Blocked: Missing attacker or target");
            return DetectionResult::BLOCKED_MISSING_ENTITY;
        }

        // TODO: Implement more checks
        // - Spoofed attacker detection
        // - PvP protection
        // - Building damage limits

        return DetectionResult::ALLOWED;
    }
};

} // namespace Takaro-Palworld-Integration
```

## Step 7: Testing and Debugging

### Enable Debug Logging

In `Config.json`:
```json
{
  "logging": {
    "level": "debug"
  }
}
```

### Test API Endpoints

```bash
# Test version endpoint
curl -H "Authorization: Bearer your-token" \
     http://localhost:8080/v1/pdapi/version

# Test player position (once implemented)
curl -H "Authorization: Bearer your-token" \
     http://localhost:8080/v1/pdapi/player/<player_id>
```

### Monitor Logs

Check `./Takaro-Palworld-Integration/logs/Takaro-Palworld-Integration.log` for:
- Successful hook initialization
- API requests and responses
- Anti-cheat detections
- Error messages

## Common Issues and Solutions

### Issue: Hooks Not Working

**Symptoms:** Game functions are called but hooks don't trigger

**Solutions:**
1. Verify function addresses are correct for your game version
2. Check if another mod is conflicting
3. Use debugger to verify hook installation
4. Check if game has anti-tamper protection

### Issue: Crashes on Hook

**Symptoms:** Game crashes when hooked function is called

**Solutions:**
1. Verify function signature matches exactly
2. Check calling convention (stdcall, cdecl, fastcall)
3. Ensure thread-safety in hook function
4. Test with minimal hook first (just call original)

### Issue: Player Position Always Null

**Symptoms:** GetPlayerPosition returns std::nullopt

**Solutions:**
1. Verify player state object exists
2. Check if player is fully initialized
3. Validate player ID format
4. Add debug logging to track object retrieval

## Best Practices

1. **Always Validate Input**
   - Check player IDs before use
   - Validate damage values
   - Verify object pointers

2. **Thread Safety**
   - Use mutexes for shared data
   - Be careful with game thread vs hook thread
   - Don't block game thread in hooks

3. **Error Handling**
   - Catch exceptions in hooks
   - Log all errors with context
   - Fail gracefully

4. **Performance**
   - Minimize processing in hooks
   - Cache frequently accessed objects
   - Use async operations for slow tasks

## Resources

- **UE4SS Documentation:** https://docs.ue4ss.com/
- **Palworld Modding Wiki:** https://pwmodding.wiki/
- **Microsoft Detours:** https://github.com/microsoft/Detours
- **Reverse Engineering Tools:** IDA Pro, Ghidra, x64dbg

## Next Steps

1. Implement core hooks (damage, RCON, chat)
2. Test with local server
3. Implement remaining player management features
4. Add guild tracking functionality
5. Complete anti-cheat rules
6. Performance testing and optimization

---

**Note:** This is a complex integration project. Start with simple features (like player position tracking) and gradually add more complex hooks. Always test changes incrementally.

**Last Updated:** 2024-12-28
