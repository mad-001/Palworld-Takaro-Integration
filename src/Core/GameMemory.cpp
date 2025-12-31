#include "TakaroPalworld/Core/GameMemory.h"
#include "TakaroPalworld/Core/Logger.h"
#include <windows.h>
#include <psapi.h>

namespace TakaroPalworld {

// Static members
uintptr_t GameMemory::palworldBase_ = 0;
void* GameMemory::gWorld_ = nullptr;
bool GameMemory::initialized_ = false;

bool GameMemory::Initialize() {
    if (initialized_) {
        return true;
    }

    LOG_INFO("Initializing GameMemory...");

    // Get Palworld base address
    HMODULE palModule = GetModuleHandleA("Pal-Win64-Shipping.exe");
    if (!palModule) {
        palModule = GetModuleHandleA("PalServer-Win64-Shipping.exe");
    }

    if (!palModule) {
        LOG_ERROR("Failed to find Palworld executable module");
        return false;
    }

    palworldBase_ = reinterpret_cast<uintptr_t>(palModule);
    LOG_INFO("Palworld base address: 0x{:X}", palworldBase_);

    // Find GWorld pointer via pattern scanning
    uintptr_t gWorldAddr = FindGWorld();
    if (gWorldAddr == 0) {
        LOG_ERROR("Failed to find GWorld");
        return false;
    }

    gWorld_ = reinterpret_cast<void*>(gWorldAddr);
    LOG_INFO("Found GWorld at address: 0x{:X}", gWorldAddr);

    initialized_ = true;
    LOG_INFO("GameMemory initialized successfully");
    return true;
}

void GameMemory::Shutdown() {
    LOG_INFO("Shutting down GameMemory...");
    initialized_ = false;
    gWorld_ = nullptr;
    palworldBase_ = 0;
}

UWorld* GameMemory::GetWorld() {
    if (!initialized_ || !gWorld_) {
        return nullptr;
    }

    // GWorld is a pointer to UWorld*
    UWorld** gWorldPtr = static_cast<UWorld**>(gWorld_);
    if (IsBadReadPtr(gWorldPtr, sizeof(UWorld*))) {
        return nullptr;
    }

    return *gWorldPtr;
}

APalGameStateInGame* GameMemory::GetGameState() {
    UWorld* world = GetWorld();
    if (!world) {
        return nullptr;
    }

    // Try multiple possible offsets for GameState
    const uintptr_t offsets[] = {0x140, 0x150, 0x160, 0x170, 0x180};

    for (uintptr_t offset : offsets) {
        APalGameStateInGame* gameState = ReadPointer<APalGameStateInGame>(world, offset);
        if (IsValidPointer(gameState)) {
            return gameState;
        }
    }

    return nullptr;
}

uintptr_t GameMemory::FindPattern(const char* moduleName, const char* pattern, const char* mask) {
    HMODULE module = GetModuleHandleA(moduleName);
    if (!module) {
        LOG_ERROR("Module not found: {}", moduleName);
        return 0;
    }

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), module, &modInfo, sizeof(MODULEINFO))) {
        LOG_ERROR("Failed to get module info for: {}", moduleName);
        return 0;
    }

    uintptr_t base = reinterpret_cast<uintptr_t>(module);
    uintptr_t size = modInfo.SizeOfImage;

    size_t patternLen = strlen(mask);

    for (uintptr_t i = 0; i < size - patternLen; i++) {
        bool found = true;
        for (size_t j = 0; j < patternLen; j++) {
            if (mask[j] != '?' && pattern[j] != *reinterpret_cast<char*>(base + i + j)) {
                found = false;
                break;
            }
        }

        if (found) {
            LOG_INFO("Pattern found at offset: 0x{:X}", i);
            return base + i;
        }
    }

    return 0;
}

uintptr_t GameMemory::FindGWorld() {
    LOG_INFO("Searching for GWorld...");

    // Pattern: mov rax, [GWorld]
    // 48 8B 05 ?? ?? ?? ?? 48 85 C0 74
    const char* pattern = "\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74";
    const char* mask = "xxx????xxxx";

    const char* moduleNames[] = {
        "Pal-Win64-Shipping.exe",
        "PalServer-Win64-Shipping.exe"
    };

    for (const char* moduleName : moduleNames) {
        uintptr_t patternAddr = FindPattern(moduleName, pattern, mask);
        if (patternAddr != 0) {
            // Extract RIP-relative offset
            // Pattern: 48 8B 05 [XX XX XX XX] - offset is at +3
            int32_t offset = *reinterpret_cast<int32_t*>(patternAddr + 3);

            // Calculate actual GWorld address
            // RIP = instruction after the mov (patternAddr + 7)
            uintptr_t gWorldAddr = patternAddr + 7 + offset;

            LOG_INFO("GWorld found via pattern at: 0x{:X}", gWorldAddr);
            return gWorldAddr;
        }
    }

    LOG_ERROR("GWorld pattern not found");
    return 0;
}

bool GameMemory::IsValidPointer(void* ptr) {
    if (!ptr) return false;

    // Check if pointer is in valid memory range
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
        return false;
    }

    // Check if memory is committed and accessible
    if (mbi.State != MEM_COMMIT) {
        return false;
    }

    if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE) {
        return false;
    }

    return true;
}

} // namespace TakaroPalworld
