cmake_minimum_required(VERSION 3.20)
project(Takaro-Palworld-Integration VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX- /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find packages
find_package(Threads REQUIRED)
find_package(OpenSSL)

# Configure Crow without SSL if OpenSSL is not found
if(NOT OPENSSL_FOUND)
    message(STATUS "OpenSSL not found - building without HTTPS support")
    add_compile_definitions(CROW_DISABLE_SSL)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/crow/include
    ${CMAKE_SOURCE_DIR}/external/spdlog/include
    ${CMAKE_SOURCE_DIR}/external/json/include
    ${CMAKE_SOURCE_DIR}/external/asio/asio/include
)

# Source files - Adding Config, Logger, PalAPI, TakaroClient, and REST API
set(SOURCES
    src/Core/Main.cpp
    src/Core/Config.cpp
    src/Core/Logger.cpp
    src/Core/PalAPI.cpp
    src/Core/GameMemory.cpp
    src/Core/SaveParser.cpp

    src/API/RestServer.cpp
    src/API/Middleware.cpp
    src/API/Routes.cpp

    src/Takaro/TakaroClient.cpp

    src/Commands/CommandManager.cpp
    src/Commands/AdminCommands.cpp
    src/Commands/PlayerCommands.cpp
    src/Commands/ClearInventory.cpp

    src/Hooks/HookManager.cpp
    src/Hooks/DamageHooks.cpp
    src/Hooks/RCONHooks.cpp
    src/Hooks/ChatHooks.cpp

    src/AntiCheat/DamageValidator.cpp
    src/AntiCheat/AntiCheatManager.cpp
)

# Header files (only include files that exist)
set(HEADERS
    include/TakaroPalworld/Core/Config.h
    include/TakaroPalworld/Core/Logger.h
    include/TakaroPalworld/Core/PalAPI.h
    include/TakaroPalworld/Core/Types.h
    include/TakaroPalworld/Core/GameMemory.h
    include/TakaroPalworld/Core/SaveParser.h

    include/TakaroPalworld/API/RestServer.h
    include/TakaroPalworld/API/Middleware.h

    include/TakaroPalworld/Takaro/TakaroClient.h
)

# Create DLL
add_library(Takaro-Palworld-Integration SHARED ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(Takaro-Palworld-Integration
    PRIVATE
        Threads::Threads
)

# Link OpenSSL if available
if(OPENSSL_FOUND)
    target_link_libraries(Takaro-Palworld-Integration
        PRIVATE
            OpenSSL::SSL
            OpenSSL::Crypto
    )
endif()

# Platform-specific settings
if(WIN32)
    target_link_libraries(Takaro-Palworld-Integration PRIVATE ws2_32 mswsock winhttp)
    set_target_properties(Takaro-Palworld-Integration PROPERTIES
        SUFFIX ".dll"
        PREFIX ""
        LINK_FLAGS "-Wl,--subsystem,windows -static-libgcc -static-libstdc++ -static"
    )
endif()

# Install rules
install(TARGETS Takaro-Palworld-Integration
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install config files
install(DIRECTORY config/
    DESTINATION config
    FILES_MATCHING PATTERN "*.json"
)

# Print build info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
