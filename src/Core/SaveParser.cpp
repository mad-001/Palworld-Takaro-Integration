#include "TakaroPalworld/Core/SaveParser.h"
#include "TakaroPalworld/Core/Logger.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace TakaroPalworld {

std::string SaveParser::ReadString(std::ifstream& file) {
    int32_t length;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));

    if (length <= 0) return "";

    std::string str(length - 1, '\0');
    file.read(&str[0], length - 1);
    file.seekg(1, std::ios::cur); // Skip null terminator

    return str;
}

uint32_t SaveParser::ReadUInt32(std::ifstream& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

std::vector<uint8_t> SaveParser::DecompressIfNeeded(const std::vector<uint8_t>& data) {
    // For now, return as-is. Add zlib decompression if needed
    return data;
}

std::vector<GuildInfo> SaveParser::ExtractGuildData(const std::vector<uint8_t>& data,
                                                     const std::vector<std::string>& playerNames) {
    std::vector<GuildInfo> guilds;

    LOG_INFO("Searching for guild data in save file");

    // Search for "GroupMap" which contains guild data (actual marker in Palworld saves)
    const char* groupMapMarker = "GroupMap";
    size_t groupMapLen = strlen(groupMapMarker);

    size_t groupMapOffset = 0;
    bool foundGroupMap = false;

    for (size_t i = 0; i < data.size() - groupMapLen; i++) {
        if (memcmp(&data[i], groupMapMarker, groupMapLen) == 0) {
            groupMapOffset = i;
            foundGroupMap = true;
            LOG_INFO("Found 'GroupMap' at offset: {}", i);
            break;
        }
    }

    if (!foundGroupMap) {
        LOG_WARNING("GroupMap not found in save file");
        return guilds;
    }

    // Search for "Guild" markers which indicate guild entries
    const char* groupNameMarker = "Guild";
    size_t groupNameLen = strlen(groupNameMarker);

    size_t searchStart = groupMapOffset;
    size_t searchEnd = std::min(data.size(), groupMapOffset + 500000); // Search ~500KB after map start

    for (size_t i = searchStart; i < searchEnd - groupNameLen; i++) {
        if (memcmp(&data[i], groupNameMarker, groupNameLen) == 0) {
            LOG_INFO("Found 'Guild' marker at offset: {}", i);

            GuildInfo info;
            info.dataOffset = i;  // Save guild marker offset for member search

            // Look for GUID pattern after Guild marker (format: !XXXXXXXXXXXXXXXX)
            size_t guidSearchStart = i + groupNameLen;
            size_t guidSearchEnd = std::min(data.size(), guidSearchStart + 500);
            size_t guidEndPos = guidSearchStart;

            for (size_t j = guidSearchStart; j < guidSearchEnd; j++) {
                if (data[j] == '!' && j + 17 < data.size()) {
                    // Found '!' which precedes a GUID in save files
                    std::string guid(reinterpret_cast<const char*>(&data[j]), 17);
                    info.guildId = guid;
                    guidEndPos = j + 17; // Mark end of GUID
                    LOG_INFO("Extracted guild ID: {}", guid);
                    break;
                }
            }

            // Search for readable guild name AFTER the GUID (appears ~50-150 bytes after GUID)
            // Take FIRST valid name with space in limited range to avoid player names
            size_t nameSearchStart = guidEndPos + 10;
            size_t nameSearchEnd = std::min(data.size(), nameSearchStart + 150);  // Reduced from 400 to 150

            std::string bestName = "";
            bool bestHasSpace = false;

            for (size_t j = nameSearchStart; j < nameSearchEnd; j++) {
                if (data[j] >= 'A' && data[j] <= 'z' && j + 3 < data.size()) {
                    size_t nameEnd = j;
                    while (nameEnd < nameSearchEnd &&
                           ((data[nameEnd] >= 'A' && data[nameEnd] <= 'z') ||
                            (data[nameEnd] >= '0' && data[nameEnd] <= '9') ||
                            data[nameEnd] == ' ' || data[nameEnd] == '_')) {
                        nameEnd++;
                    }

                    size_t nameLen = nameEnd - j;
                    if (nameLen >= 3 && nameLen <= 50) {
                        std::string potentialName(reinterpret_cast<const char*>(&data[j]), nameLen);

                        bool looksLikeHex = true;
                        bool validUTF8 = true;
                        bool hasSpace = potentialName.find(' ') != std::string::npos;

                        for (char c : potentialName) {
                            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || c == ' ')) {
                                looksLikeHex = false;
                            }
                            if (static_cast<unsigned char>(c) > 127) {
                                validUTF8 = false;
                            }
                        }

                        if (!looksLikeHex && validUTF8 && potentialName != "Unnamed") {
                            // Take FIRST name with space >= 5 chars (guild name comes before player names)
                            if (hasSpace && potentialName.length() >= 5 && bestName.empty()) {
                                bestName = potentialName;
                                bestHasSpace = hasSpace;
                                LOG_INFO("Found guild name: '{}'", potentialName);
                                break;  // Stop at first match
                            }
                        }
                    }
                    j = nameEnd - 1;
                }
            }

            if (!bestName.empty()) {
                info.guildName = bestName;
                LOG_INFO("Selected guild name: '{}'", bestName);
            }

            // Extract admin ID (appears shortly after guild name as !XXXXXXXX)
            size_t adminSearchStart = nameSearchEnd;
            size_t adminSearchEnd = std::min(data.size(), adminSearchStart + 200);

            for (size_t j = adminSearchStart; j < adminSearchEnd; j++) {
                if (data[j] == '!' && j + 9 < adminSearchEnd) {
                    // Check if it's a different GUID than the guild ID (admin player ID)
                    std::string potentialAdminId(reinterpret_cast<const char*>(&data[j]), 9);

                    // Validate UTF-8
                    bool validUTF8 = true;
                    for (char c : potentialAdminId) {
                        if (static_cast<unsigned char>(c) > 127) {
                            validUTF8 = false;
                            break;
                        }
                    }

                    if (validUTF8 && potentialAdminId != info.guildId && info.adminId.empty()) {
                        info.adminId = potentialAdminId;
                        LOG_INFO("Extracted admin ID: {}", potentialAdminId);
                        break;
                    }
                }
            }

            // Set defaults if not found
            if (info.guildId.empty()) {
                info.guildId = "GUILD_" + std::to_string(guilds.size() + 1);
            }
            if (info.guildName.empty()) {
                info.guildName = "Guild " + std::to_string(guilds.size() + 1);
            }

            // Members will be populated in second pass
            info.memberIds = {};
            info.memberCount = 0;
            guilds.push_back(info);

            // Move search forward to avoid finding the same guild again
            i = nameSearchEnd;
        }
    }

    LOG_INFO("Extracted {} guilds from save data (members will be populated next)", guilds.size());

    // Use provided player names, or fall back to hardcoded list for testing
    std::vector<std::string> allPlayerNames = playerNames;

    if (allPlayerNames.empty()) {
        // Fallback: hardcoded list for testing/development
        allPlayerNames = {
            "Mad",
            "Gangsa Monke0",
            "DoubleTap",
            "KittyKatWithABAT",
            "NnOxX69",
            "Henny",
            "SlipperyWhenWet"
        };
        LOG_INFO("Using fallback hardcoded player list ({} players)", allPlayerNames.size());
    } else {
        LOG_INFO("Using provided player whitelist ({} players)", allPlayerNames.size());
    }

    // Second pass: For each player, find their guild ID and assign them to the correct guild
    std::map<std::string, std::string> playerToGuild;

    for (const auto& playerName : allPlayerNames) {
        const char* nameToFind = playerName.c_str();
        size_t nameLen = playerName.length();

        // Search entire save file for this player name
        for (size_t i = 100; i < data.size() - nameLen; i++) {
            if (memcmp(&data[i], nameToFind, nameLen) == 0) {
                // Found player name, now look for CLOSEST guild ID BEFORE the player name (no distance limit)
                std::string closestGuildId = "";
                size_t closestDistance = SIZE_MAX;

                for (size_t j = 0; j < i; j++) {
                    if (data[j] == '!' && j + 17 < data.size()) {
                        std::string guildId(reinterpret_cast<const char*>(&data[j]), 17);

                        // Check if this guild ID matches any of our known guilds
                        for (const auto& guild : guilds) {
                            if (guild.guildId == guildId) {
                                size_t distance = i - j;
                                if (distance < closestDistance) {
                                    closestDistance = distance;
                                    closestGuildId = guildId;
                                }
                                break;
                            }
                        }
                    }
                }

                if (!closestGuildId.empty()) {
                    if (playerToGuild.find(playerName) == playerToGuild.end()) {
                        playerToGuild[playerName] = closestGuildId;
                        LOG_INFO("Player '{}' belongs to guild '{}' (found {} bytes before player name)",
                                 playerName, closestGuildId, closestDistance);
                    }
                }
                break;  // Found player, move to next
            }
        }
    }

    // Third pass: Populate guild members from player → guild map
    for (auto& guild : guilds) {
        for (const auto& [playerName, guildId] : playerToGuild) {
            if (guildId == guild.guildId) {
                guild.memberIds.push_back(playerName);
            }
        }
        guild.memberCount = static_cast<int>(guild.memberIds.size());
        LOG_INFO("Guild '{}' has {} members", guild.guildName, guild.memberCount);
    }

    // Fourth pass: Create solo guilds for players not in any guild
    std::set<std::string> playersInGuilds;
    for (const auto& [playerName, guildId] : playerToGuild) {
        playersInGuilds.insert(playerName);
    }

    for (const auto& playerName : allPlayerNames) {
        if (playersInGuilds.find(playerName) == playersInGuilds.end()) {
            // Player not in any guild - create solo guild
            GuildInfo soloGuild;
            soloGuild.guildId = "SOLO_" + playerName;
            soloGuild.guildName = playerName;
            soloGuild.memberIds.push_back(playerName);
            soloGuild.memberCount = 1;
            guilds.push_back(soloGuild);
            LOG_INFO("Created solo guild for player '{}'", playerName);
        }
    }

    LOG_INFO("Extracted {} total guilds (including solo guilds)", guilds.size());
    return guilds;
}

std::vector<GuildInfo> SaveParser::ParseGuildsFromSave(const std::string& savePath,
                                                        const std::vector<std::string>& playerNames) {
    std::vector<GuildInfo> guilds;

    LOG_INFO("Attempting to parse guilds from: {}", savePath);

    // Convert Windows path to WSL path format
    // Example: C:\path\to\file.sav -> /mnt/c/path/to/file.sav
    std::string wslPath = savePath;
    if (wslPath.length() >= 2 && wslPath[1] == ':') {
        char drive = std::tolower(wslPath[0]);
        wslPath = "/mnt/" + std::string(1, drive) + wslPath.substr(2);
        // Convert backslashes to forward slashes
        std::replace(wslPath.begin(), wslPath.end(), '\\', '/');
    }

    // Call Python parser script via WSL
    std::string pythonScript = "/home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration/parse_guilds.py";
    std::string command = "wsl python3 " + pythonScript + " \"" + wslPath + "\"";

    LOG_INFO("Executing: {}", command);

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("Failed to execute Python parser");
        return guilds;
    }

    // Parse output line by line
    char buffer[4096];
    GuildInfo currentGuild;
    bool inGuild = false;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        // Remove trailing newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        if (line.rfind("Guild: ", 0) == 0) {
            // Save previous guild if exists
            if (inGuild && !currentGuild.memberIds.empty()) {
                guilds.push_back(currentGuild);
            }

            // Start new guild
            currentGuild = GuildInfo();
            currentGuild.guildName = line.substr(7); // Remove "Guild: " prefix
            currentGuild.guildId = "GUILD_" + std::to_string(guilds.size() + 1);
            inGuild = true;
        }
        else if (line.rfind("Members: ", 0) == 0) {
            // Parse member count
            currentGuild.memberCount = std::stoi(line.substr(9));
        }
        else if (line.rfind("  - ", 0) == 0) {
            // Add member name
            std::string memberName = line.substr(4);
            currentGuild.memberIds.push_back(memberName);
        }
    }

    // Add final guild
    if (inGuild && !currentGuild.memberIds.empty()) {
        guilds.push_back(currentGuild);
    }

    int status = pclose(pipe);
    if (status != 0) {
        LOG_WARNING("Python parser exited with status: {}", status);
    }

    LOG_INFO("Found {} guilds from Python parser", guilds.size());
    return guilds;
}

bool SaveParser::WriteGuildsCache(const std::vector<GuildInfo>& guilds, const std::string& cacheFilePath) {
    LOG_INFO("Writing guilds cache to: {}", cacheFilePath);

    std::ofstream file(cacheFilePath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open cache file for writing: {}", cacheFilePath);
        return false;
    }

    // Write JSON array
    file << "[\n";
    for (size_t i = 0; i < guilds.size(); i++) {
        const auto& guild = guilds[i];
        file << "  {\n";
        file << "    \"guildId\": \"" << guild.guildId << "\",\n";
        file << "    \"guildName\": \"" << guild.guildName << "\",\n";
        file << "    \"memberCount\": " << guild.memberCount << ",\n";
        file << "    \"members\": [\n";

        for (size_t j = 0; j < guild.memberIds.size(); j++) {
            file << "      \"" << guild.memberIds[j] << "\"";
            if (j < guild.memberIds.size() - 1) file << ",";
            file << "\n";
        }

        file << "    ]\n";
        file << "  }";
        if (i < guilds.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";

    file.close();
    LOG_INFO("Successfully wrote {} guilds to cache", guilds.size());
    return true;
}

std::vector<GuildInfo> SaveParser::ReadGuildsCache(const std::string& cacheFilePath) {
    std::vector<GuildInfo> guilds;

    LOG_INFO("Reading guilds cache from: {}", cacheFilePath);

    std::ifstream file(cacheFilePath);
    if (!file.is_open()) {
        LOG_WARNING("Cache file not found: {}", cacheFilePath);
        return guilds;
    }

    // Simple JSON parsing (expecting exact format from WriteGuildsCache)
    std::string line;
    GuildInfo currentGuild;
    bool inGuild = false;
    bool inMembers = false;

    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line.find("\"guildId\":") != std::string::npos) {
            inGuild = true;
            size_t valueStart = line.find("\"", line.find(":") + 1) + 1;
            size_t valueEnd = line.find("\"", valueStart);
            currentGuild.guildId = line.substr(valueStart, valueEnd - valueStart);
        }
        else if (line.find("\"guildName\":") != std::string::npos) {
            size_t valueStart = line.find("\"", line.find(":") + 1) + 1;
            size_t valueEnd = line.find("\"", valueStart);
            currentGuild.guildName = line.substr(valueStart, valueEnd - valueStart);
        }
        else if (line.find("\"memberCount\":") != std::string::npos) {
            size_t valueStart = line.find(":") + 1;
            size_t valueEnd = line.find(",", valueStart);
            if (valueEnd == std::string::npos) valueEnd = line.length();
            currentGuild.memberCount = std::stoi(line.substr(valueStart, valueEnd - valueStart));
        }
        else if (line.find("\"members\":") != std::string::npos) {
            inMembers = true;
            currentGuild.memberIds.clear();
        }
        else if (inMembers && line.find("\"") != std::string::npos && line.find("]") == std::string::npos) {
            size_t valueStart = line.find("\"") + 1;
            size_t valueEnd = line.find("\"", valueStart);
            if (valueEnd != std::string::npos) {
                currentGuild.memberIds.push_back(line.substr(valueStart, valueEnd - valueStart));
            }
        }
        else if (line.find("]") != std::string::npos && inMembers) {
            inMembers = false;
        }
        else if (line.find("}") != std::string::npos && inGuild && !inMembers) {
            guilds.push_back(currentGuild);
            currentGuild = GuildInfo();
            inGuild = false;
        }
    }

    file.close();
    LOG_INFO("Successfully read {} guilds from cache", guilds.size());
    return guilds;
}

} // namespace TakaroPalworld
