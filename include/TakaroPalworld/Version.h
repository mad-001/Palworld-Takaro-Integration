#pragma once

#define TAKARO_PALWORLD_VERSION_MAJOR 0
#define TAKARO_PALWORLD_VERSION_MINOR 1
#define TAKARO_PALWORLD_VERSION_PATCH 0
#define TAKARO_PALWORLD_VERSION_STRING "0.1.0"
#define TAKARO_PALWORLD_VERSION_DATE "2025-12-29"

namespace TakaroPalworld {

struct Version {
    static constexpr int MAJOR = TAKARO_PALWORLD_VERSION_MAJOR;
    static constexpr int MINOR = TAKARO_PALWORLD_VERSION_MINOR;
    static constexpr int PATCH = TAKARO_PALWORLD_VERSION_PATCH;
    static constexpr const char* STRING = TAKARO_PALWORLD_VERSION_STRING;
    static constexpr const char* DATE = TAKARO_PALWORLD_VERSION_DATE;

    static const char* GetVersionString() {
        return TAKARO_PALWORLD_VERSION_STRING;
    }

    static const char* GetBuildDate() {
        return TAKARO_PALWORLD_VERSION_DATE;
    }
};

} // namespace TakaroPalworld
