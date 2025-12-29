# Takaro-Palworld Integration Project

**Version:** 0.1.0
**Date:** 2025-12-29
**Status:** Working - Commands executing successfully

## Project Overview

A C++ DLL mod for Palworld dedicated servers that integrates with Takaro platform for server management. The mod provides:
- WebSocket connection to Takaro (wss://connect.takaro.io/)
- Command execution via both Palworld REST API and RCON
- Custom in-game commands via chat/RCON
- REST API server for mod-specific features
- Anti-cheat and damage validation systems

## Critical Configuration

### Servers
- **Test Server (Local)**: `C:\Program Files (x86)\Steam\steamapps\common\PalServer`
- **Production Server**: `\\server\c$\gameservers\palworld` (DO NOT MODIFY WITHOUT USER PERMISSION)

### Ports
- **Palworld REST API**: 8202 (Basic Auth: admin:456456)
- **Palworld RCON**: 8201 (Password: 456456)
- **Mod REST API**: 8003 (Bearer Token: 456456) - Currently not used
- **Takaro WebSocket**: wss://connect.takaro.io/

### Authentication
- **Palworld REST API**: Basic HTTP Auth (username: "admin", password: "456456")
- **Palworld RCON**: Password authentication ("456456")
- **Takaro WebSocket**: Token-based (from TakaroConfig.json)

## Project Structure

### Root Directory
```
/home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration/
```

### Documentation Files
- `README.md` - Project overview and setup instructions
- `BUILD.md` - Build instructions for MinGW cross-compilation
- `DEPLOYMENT.md` - Deployment procedures
- `IMPLEMENTATION.md` - Implementation details
- `PROJECT_SUMMARY.md` - Project summary
- `QUICK_START.md` - Quick start guide
- `RENAME_SUMMARY.md` - Rename history
- `Prompt.md` - This file

### Additional Documentation
- `/home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/COMMANDS.md` - Command reference
- `/home/zmedh/Takaro-Projects/Takaro-instructions/Websocket Connection Instructions.md` - WebSocket protocol

### Core Headers
```
include/TakaroPalworld/Core/
├── Config.h          - Configuration management (MainConfig, TakaroConfig, RESTConfig, WhiteListConfig)
├── Logger.h          - Logging system with spdlog
├── PalAPI.h          - Main API for Palworld game functions
└── Types.h           - Common type definitions (PlayerInfo, Position, GuildInfo, etc.)
```

### Core Implementation
```
src/Core/
├── Config.cpp        - Config loading/saving from JSON
├── Logger.cpp        - Logger initialization
├── Main.cpp          - DLL entry point (DllMain, Initialize, MainLoop)
└── PalAPI.cpp        - Command routing, REST API client, RCON client
```

### Takaro Integration
```
include/TakaroPalworld/Takaro/
└── TakaroClient.h    - Takaro WebSocket client

src/Takaro/
└── TakaroClient.cpp  - WebSocket connection, ping/pong, request handling
```

### REST API Server (Mod's Own API - Port 8003)
```
include/TakaroPalworld/API/
├── RestServer.h      - Crow-based REST server
└── Middleware.h      - CORS, Auth, Access middleware

src/API/
├── RestServer.cpp    - REST endpoints (version, guilds, players, give item)
├── Middleware.cpp    - Middleware implementations
└── Routes.cpp        - Route definitions
```

### Commands System
```
src/Commands/
├── CommandManager.cpp    - Command registration and routing
├── PlayerCommands.cpp    - Player-related commands
├── AdminCommands.cpp     - Admin commands
└── ClearInventory.cpp    - Inventory management
```

### Hooks System
```
src/Hooks/
├── HookManager.cpp   - Hook registration
├── ChatHooks.cpp     - Chat message interception
├── DamageHooks.cpp   - Damage calculation hooks
└── RCONHooks.cpp     - RCON command hooks
```

### Anti-Cheat
```
src/AntiCheat/
├── AntiCheatManager.cpp  - Anti-cheat coordination
└── DamageValidator.cpp   - Damage validation
```

### Configuration Files
```
config/Takaro-Palworld-Integration/
├── Config.json                    - Main mod configuration
├── TakaroConfig.json             - Takaro connection settings
├── WhiteList.json                - Whitelisted players
└── RESTAPI/
    └── RESTConfig.json           - REST API settings
```

### Build System
```
CMakeLists.txt                    - CMake build configuration
build/                            - Build output directory
├── bin/
│   └── Takaro-Palworld-Integration.dll  - Compiled DLL
└── CMakeFiles/                   - CMake generated files
```

### External Dependencies
```
external/
├── crow/                         - Crow web framework (REST API)
├── json/                         - nlohmann/json (JSON parsing)
├── spdlog/                       - spdlog (logging)
└── websocketpp/                  - WebSocket++ (Takaro connection)
```

## Command Routing Logic

### Command Execution Flow (PalAPI.cpp:267-331)

1. **Parse Command**: Split command name and arguments
2. **Check if REST API Command**:
   - `info` / `getversion` → `/v1/api/info` (Palworld REST API)
   - `showplayers` / `getplayers` → `/v1/api/players` (Palworld REST API)
3. **If REST API Command**:
   - Send HTTP GET request to `127.0.0.1:8202`
   - Use Basic Auth header: `Authorization: Basic base64(admin:456456)`
   - Return JSON response
4. **If NOT REST API Command**:
   - Treat as in-game mod command
   - Add `/` prefix if not present
   - Send via RCON to port 8201
   - Return RCON response

### REST API Implementation (PalAPI.cpp:636-740)

- Uses WinHTTP API for HTTP requests
- Connects to `127.0.0.1:8202` (Palworld's built-in REST API)
- Base64 encodes credentials for Basic Auth
- Returns raw JSON response or error message

### RCON Implementation (PalAPI.cpp:320-590)

- Source RCON protocol implementation
- Connects to `127.0.0.1:8201`
- Authenticates with password "456456"
- Sends commands and receives responses
- Used for in-game commands like `/getpos`

## Takaro Integration Details

### WebSocket Protocol (TakaroClient.cpp)

**Message Types**:
- `identify` - Initial authentication with Takaro
- `identifyResponse` - Takaro confirms connection
- `connected` - Connection established
- `ping` / `pong` - Keep-alive heartbeat
- `request` - Command execution request from Takaro
- `response` - Command execution result to Takaro
- `gameEvent` - Game events sent to Takaro

**Request Handling**:
1. Takaro sends `request` with `requestId`, `eventType`, and `data`
2. For `executeCommand` events:
   - Extract command from `data.arguments[0]`
   - Call `PalAPI::ExecuteCommand(command)`
   - Return response with `success: true` and `rawResult: <output>`
3. Response includes original `requestId` for matching

### Connection Maintenance
- Ping sent every 30 seconds to keep connection alive
- Pong responses handled automatically
- Reconnection logic for dropped connections

## Build Instructions

### Prerequisites
- MinGW-w64 cross-compiler: `x86_64-w64-mingw32-g++`
- CMake 3.15+
- Dependencies in `external/` directory

### Build Commands
```bash
cd /home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration/build
cmake --build . --config Release
```

### Output
- DLL: `build/bin/Takaro-Palworld-Integration.dll`
- Size: ~5.3 MB

## Deployment

### Test Server (Local)
```bash
# Stop server
cmd.exe /c "taskkill /F /IM PalServer-Win64-Shipping-Cmd.exe"

# Deploy DLL
cp build/bin/Takaro-Palworld-Integration.dll \
   "/mnt/c/Program Files (x86)/Steam/steamapps/common/PalServer/Pal/Binaries/Win64/Takaro-Palworld-Integration.dll"

# Start server
cd "/mnt/c/Program Files (x86)/Steam/steamapps/common/PalServer"
cmd.exe /c "start PalServer.exe"
```

### Production Server (DO NOT MODIFY)
- **CRITICAL**: Never stop/start/restart production server processes
- User handles ALL process management on production server
- Only modify files when explicitly instructed

## Known Issues & Solutions

### Issue: Commands returning "Unauthorized"
**Solution**: Changed from Bearer token to Basic Auth
- Old: `Authorization: Bearer 456456`
- New: `Authorization: Basic base64(admin:456456)` = `Authorization: Basic YWRtaW46NDU2NDU2`

### Issue: REST API 404 errors
**Solution**: Use correct Palworld REST API endpoints
- Wrong: `/v1/pdapi/version`
- Correct: `/v1/api/info`

### Issue: Port confusion (8003 vs 8080 vs 8202)
**Solution**:
- Port 8202: Palworld's built-in REST API (USE THIS)
- Port 8201: Palworld's RCON (USE THIS)
- Port 8003: Mod's own REST API (NOT CURRENTLY USED)

## Current Status

### Working Features ✅
- Takaro WebSocket connection established
- Server shows online in Takaro
- Commands execute successfully via Takaro
- REST API commands: `info`, `showplayers`
- RCON commands: `getpos`, and other mod commands
- Basic Auth authentication working
- Dual command routing (REST API + RCON)

### Not Implemented ❌
- Guild management endpoints (Palworld doesn't have guilds API)
- Player position tracking
- BaseCamp management
- Pal spawning
- Many PalAPI functions are stubs (TODO markers)

## Testing Commands

### From Takaro Dashboard
```
info              → Returns server version and info
showplayers       → Returns list of online players
getpos            → Gets your position (via RCON)
getpos steam_123  → Gets player's position (via RCON)
```

### Expected Responses
```json
// info command
{
    "version": "v0.7.0.84578",
    "servername": "Default Palworld Server",
    "description": "",
    "worldguid": "92477DBE4F4CE73E11AFFBA5EBE39C0C"
}

// showplayers command
{
    "players": [
        {
            "name": "PlayerName",
            "playerId": "steam_76500000000000000",
            "userId": "steam_76500000000000000",
            "ip": "127.0.0.1",
            "ping": 25.0,
            "location_x": 0.0,
            "location_y": 0.0,
            "level": 1
        }
    ]
}
```

## Important Files for Future Work

### To Add New REST API Endpoints
1. Edit `src/Core/PalAPI.cpp` - Add to command routing (line 288-295)
2. Update `/v1/api/` endpoint mapping

### To Add New In-Game Commands
1. Commands automatically route to RCON if not in REST API list
2. Ensure command exists in game mod
3. Test via Takaro dashboard

### To Modify Takaro Integration
1. Edit `src/Takaro/TakaroClient.cpp`
2. Update message handling (line ~200-400)
3. Rebuild and deploy

### To Change Authentication
1. Edit `src/Core/PalAPI.cpp` - Lines 640-641 (username/password)
2. Update base64_encode call at line 685-687

## Version History

### v0.1.0 (2025-12-29)
- Initial working version
- Takaro WebSocket connection established
- Command execution via REST API and RCON
- Basic Auth implementation
- Dual command routing system
- Commands working: info, showplayers, getpos

## Development Notes

### Code Patterns
- Singleton pattern used for managers (PalAPI, TakaroClient, RestServer, ConfigManager)
- JSON parsing with nlohmann/json library
- Async operations with std::thread
- Windows API for file operations and process management

### Logging
- All logs go to console and rotating file
- Log levels: TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL
- Located in mod directory: `Takaro-Palworld-Integration/logs/`

### Error Handling
- Functions return `std::optional<T>` for nullable results
- Empty strings indicate failure in command execution
- HTTP errors logged with status codes

## References

- [Palworld REST API Documentation](https://docs.palworldgame.com/api/rest-api/)
- [Takaro Platform](https://takaro.io/)
- [WebSocket Connection Instructions](../Takaro-instructions/Websocket%20Connection%20Instructions.md)
- [Commands Reference](../COMMANDS.md)
