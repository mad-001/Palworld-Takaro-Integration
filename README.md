# Takaro-Palworld-Integration - Palworld Anti-Cheat & Server Management

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B20-orange)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

Takaro-Palworld-Integration is a comprehensive Palworld server management and anti-cheat system reconstructed from reverse-engineered binaries. It provides:

- 🛡️ **Anti-Cheat System** - Real-time damage validation and exploit prevention
- 🌐 **REST API** - Remote server management via HTTP/HTTPS
- 🎮 **Player Management** - Kick, ban, teleport, inventory management
- 👥 **Guild Tracking** - Monitor and manage guilds and base camps
- 📊 **Logging System** - Comprehensive event logging with rotation
- 🔐 **Authentication** - Bearer token-based API authentication
- ⚙️ **Configuration** - JSON-based configuration system

## ⚠️ Important Notice

**This is a reconstructed source code based on reverse engineering of compiled DLLs.**

The original source code was deleted, and this version was created by analyzing the binary files using tools like `strings`, `objdump`, and pattern matching. While the structure, APIs, and configuration formats match the original, **the game integration code (hooks) are stubs** that need to be filled in with actual implementation.

## Project Structure

```
Takaro-Palworld-Integration/
├── CMakeLists.txt           # Build system
├── README.md                # This file
├── BUILD.md                 # Build instructions
├── IMPLEMENTATION.md        # Implementation guide
├── include/                 # Public headers
│   └── Takaro-Palworld-Integration/
│       ├── Core/           # Core system headers
│       ├── API/            # REST API headers
│       ├── Commands/       # Command system headers
│       ├── Hooks/          # Game hooks headers
│       └── AntiCheat/      # Anti-cheat headers
├── src/                    # Source files
│   ├── Core/              # Core implementations
│   ├── API/               # REST API implementations
│   ├── Commands/          # Command handlers
│   ├── Hooks/             # Game hooks (STUBS)
│   └── AntiCheat/         # Anti-cheat logic (STUBS)
├── config/                # Configuration templates
└── external/              # Third-party libraries
```

## Features

### REST API Endpoints

All endpoints require Bearer token authentication via the `Authorization` header.

#### Version Information
```http
GET /v1/pdapi/version
```
Returns API version and game version information.

#### Guild Management
```http
GET /v1/pdapi/guilds
GET /v1/pdapi/guild/<guild_id>
```
Query guild information, members, and base camps.

#### Player Management
```http
GET /v1/pdapi/players
GET /v1/pdapi/player/<player_id>
```
Get player information including position, level, and stats.

#### Item Management
```http
POST /v1/pdapi/give
Content-Type: application/json

{
  "playerId": "player-uuid",
  "itemId": "Stone",
  "quantity": 100
}
```
Give items to players.

### Anti-Cheat Features

1. **Negative Damage Prevention** - Blocks negative damage values
2. **Extreme Damage Detection** - Prevents damage beyond configured multiplier
3. **Missing Entity Validation** - Ensures attacker and target exist
4. **Spoofed Attacker Detection** - Prevents identity spoofing
5. **PvP Protection** - Configurable PvP damage rules
6. **Building Damage Control** - Limit PvP building damage

### Configuration Files

#### Config.json (Main Settings)
```json
{
  "allowAdminCheats": true,
  "pvpMaxToBuildingDamage": 0,
  "preventAdminPasswordInChat": true,
  "enableRenaming": false,
  "enablePalRenaming": false,
  "bShowPlayerList": true,
  "logging": {
    "level": "info",
    "rotatingFileSink": {
      "enabled": true,
      "maxSize": 10485760,
      "maxFiles": 5
    }
  },
  "antiCheat": {
    "maxDamageMultiplier": 10.0,
    "preventNegativeDamage": true,
    "preventCrossPlayerDamage": true,
    "preventPalOwnedByPlayerDamage": true
  }
}
```

#### RESTAPI/RESTConfig.json
```json
{
  "enabled": true,
  "host": "0.0.0.0",
  "port": 8080,
  "cors": {
    "enabled": true,
    "allowedOrigins": ["*"],
    "allowedMethods": ["GET", "POST", "PUT", "DELETE", "PATCH", "OPTIONS"]
  },
  "authentication": {
    "bearerTokens": [
      "your-secure-token-here"
    ]
  }
}
```

## Dependencies

- **CMake** (>= 3.20) - Build system
- **C++20** compiler (MSVC, GCC, or Clang)
- **Crow** - HTTP web framework
- **spdlog** - Fast logging library
- **nlohmann/json** - JSON parsing
- **ASIO** - Asynchronous I/O
- **OpenSSL** - For HTTPS support (optional)

## Building

See [BUILD.md](BUILD.md) for detailed build instructions.

Quick start:
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Installation

1. Build the project (see BUILD.md)
2. Copy `Takaro-Palworld-Integration.dll` to your Palworld installation directory
3. Create configuration files in `./Takaro-Palworld-Integration/` directory:
   - `Config.json`
   - `WhiteList.json`
   - `RESTAPI/RESTConfig.json`
4. Configure your DLL loader (d3d9.dll proxy or similar) to load Takaro-Palworld-Integration.dll

## Implementation Status

### ✅ Completed Components

- Project structure and build system
- Configuration management (JSON parsing)
- Logging system with rotation
- REST API server framework
- Authentication middleware
- Error handling and validation

### ⚠️ Stub Components (Need Implementation)

- **Game Hooks** - Actual UE4/Palworld function hooking
- **Command System** - In-game command handlers
- **Player Management** - Game-specific player operations
- **Anti-Cheat Logic** - Real-time damage validation hooks
- **Guild System** - Guild data retrieval from game

See [IMPLEMENTATION.md](IMPLEMENTATION.md) for implementation guide.

## Usage

### Starting the DLL

The DLL automatically initializes when loaded by the game:
1. Loads configuration from `./Takaro-Palworld-Integration/` directory
2. Initializes logging system
3. Sets up game hooks (stub implementation)
4. Starts REST API server (if enabled)

### API Authentication

All API requests require Bearer token authentication:

```bash
curl -H "Authorization: Bearer your-token-here" \
     http://localhost:8080/v1/pdapi/version
```

### Managing Configuration

Reload configuration without restarting:
```bash
curl -X POST -H "Authorization: Bearer your-token-here" \
     http://localhost:8080/v1/pdapi/reload
```

## Security Considerations

1. **Use strong Bearer tokens** - Generate cryptographically secure random strings
2. **Restrict CORS origins** - Don't use `*` in production
3. **Enable HTTPS** - Use SSL/TLS for API communication
4. **Whitelist admin IPs** - Restrict admin access by IP address
5. **Review logs regularly** - Monitor for suspicious activity

## Troubleshooting

### DLL not loading
- Check that d3d9.dll proxy is correctly configured
- Verify Takaro-Palworld-Integration.dll is in the correct directory
- Check logs in `./Takaro-Palworld-Integration/logs/` for errors

### API not accessible
- Verify `RESTConfig.json` has `"enabled": true`
- Check firewall settings for the configured port
- Ensure bearer tokens are correctly set

### Hooks not working
- This is expected - hook implementations are stubs
- See [IMPLEMENTATION.md](IMPLEMENTATION.md) for hooking guide
- Requires UE4SS or similar framework for Unreal Engine hooking

## Contributing

This project was reconstructed from reverse-engineered binaries. Contributions welcome:

1. **Complete stub implementations** - Fill in game hooks and command handlers
2. **Add new features** - Extend the API or add new commands
3. **Improve documentation** - Add examples and tutorials
4. **Test and report issues** - Help identify bugs

## License

MIT License - See LICENSE file for details

## Acknowledgments

- Original Takaro-Palworld-Integration by Ultimeit (abandoned project)
- Reverse engineered from compiled DLLs using open-source tools
- Community contributions to Palworld modding

## Related Projects

- **TakaroChat** - Alternative Lua-based approach using UE4SS
- **Palworld Modding Wiki** - Community modding resources
- **UE4SS** - Unreal Engine scripting system

## Disclaimer

This project is provided "as is" for educational and research purposes. Use at your own risk. The authors are not responsible for any damage or consequences resulting from the use of this software. Respect game terms of service and applicable laws.

---

**Status:** Reconstructed from reverse-engineered binaries | **Version:** 1.0.0 | **Last Updated:** 2024-12-28
