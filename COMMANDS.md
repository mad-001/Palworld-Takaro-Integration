# Takaro-Palworld Integration Commands

## REST API Commands (Port 8003)

These commands call the **mod's REST API server** (port 8003) and return data.

**Important**: Do not confuse with Palworld's built-in REST API (port 8202) which has different endpoints.

### Player Commands

**ShowPlayers** / **GetPlayers**
- Description: Get list of all online players
- REST Endpoint: GET /v1/pdapi/players
- Returns: JSON array of player objects with name, steamId, characterId, etc.

**GetPlayer <playerId>**
- Description: Get detailed info about a specific player
- REST Endpoint: GET /v1/pdapi/player/<playerId>
- Returns: Player object with detailed stats

### Guild Commands

**GetGuilds**
- Description: Get list of all guilds on the server
- REST Endpoint: GET /v1/pdapi/guilds
- Returns: JSON array of guild objects

**GetGuild <guildId>**
- Description: Get detailed info about a specific guild
- REST Endpoint: GET /v1/pdapi/guild/<guildId>
- Returns: Guild object with members and details

### Server Commands

**Info** / **GetVersion**
- Description: Get server version and API information
- REST Endpoint: GET /v1/pdapi/version
- Returns: Version info, API version, game version

### Item Commands

**GiveItem <playerId> <itemId> <quantity>**
- Description: Give an item to a player
- REST Endpoint: POST /v1/pdapi/give
- Body: {"playerId": "...", "itemId": "...", "quantity": number}
- Returns: Success/failure status

## Palworld Native RCON Commands (Port 8201)

Note: RCON may not be actively used. Prefer REST API commands when possible.

**Broadcast <message>**
- Description: Send a broadcast message to all players
- RCON Command: `Broadcast <message>`

**Save**
- Description: Save the current world state
- RCON Command: `Save`

**Shutdown <seconds> <message>**
- Description: Shutdown server with countdown
- RCON Command: `Shutdown <seconds> <message>`

**KickPlayer <steamId>**
- Description: Kick a player from the server
- RCON Command: `KickPlayer <steamId>`

**BanPlayer <steamId>**
- Description: Ban a player from the server
- RCON Command: `BanPlayer <steamId>`

## Implementation Status

- [ ] REST API command execution
- [ ] Command parameter parsing
- [ ] Error handling and response formatting
- [ ] Takaro integration (executeCommand handler)
- [ ] RCON fallback for native commands (if needed)
