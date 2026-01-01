#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include "Types.h"

namespace TakaroPalworld {

class SaveParser {
public:
    // Parse guilds from save file with optional player whitelist
    // If playerNames is empty, uses a default hardcoded list
    static std::vector<GuildInfo> ParseGuildsFromSave(const std::string& savePath,
                                                       const std::vector<std::string>& playerNames = {});

private:
    static std::string ReadString(std::ifstream& file);
    static uint32_t ReadUInt32(std::ifstream& file);
    static std::vector<uint8_t> DecompressIfNeeded(const std::vector<uint8_t>& data);
    static std::vector<GuildInfo> ExtractGuildData(const std::vector<uint8_t>& data,
                                                     const std::vector<std::string>& playerNames);
};

} // namespace TakaroPalworld
