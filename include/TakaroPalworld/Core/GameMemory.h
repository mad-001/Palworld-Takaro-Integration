#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

namespace TakaroPalworld {

// ============================================================================
// UE4/UE5 Core Structures
// ============================================================================

// Unreal Engine String (UTF-16)
struct FString {
    wchar_t* Data;
    int32_t Length;
    int32_t MaxLength;

    FString() : Data(nullptr), Length(0), MaxLength(0) {}

    std::string ToString() const {
        if (!Data || Length <= 0) return "";
        std::wstring wide(Data, Length);
        return std::string(wide.begin(), wide.end());
    }

    static FString FromStdString(const std::string& str) {
        FString result;
        std::wstring wide(str.begin(), str.end());
        result.Data = (wchar_t*)malloc((wide.size() + 1) * sizeof(wchar_t));
        wcscpy_s(result.Data, wide.size() + 1, wide.c_str());
        result.Length = static_cast<int32_t>(wide.size());
        result.MaxLength = static_cast<int32_t>(wide.size() + 1);
        return result;
    }

    void Free() {
        if (Data) {
            free(Data);
            Data = nullptr;
        }
        Length = 0;
        MaxLength = 0;
    }
};

// Unreal Engine Dynamic Array
template<typename T>
struct TArray {
    T* Data;
    int32_t Count;
    int32_t Max;

    TArray() : Data(nullptr), Count(0), Max(0) {}

    T& operator[](int32_t index) { return Data[index]; }
    const T& operator[](int32_t index) const { return Data[index]; }
    int32_t Num() const { return Count; }
    bool IsValid() const { return Data != nullptr && Count > 0; }
};

// Palworld Player UID (128-bit)
struct FPalPlayerUId {
    uint32_t A;
    uint32_t B;
    uint32_t C;
    uint32_t D;

    FPalPlayerUId() : A(0), B(0), C(0), D(0) {}

    bool IsZero() const {
        return A == 0 && B == 0 && C == 0 && D == 0;
    }

    std::string ToString() const {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%08X%08X%08X%08X", A, B, C, D);
        return std::string(buffer);
    }
};

// Palworld Chat Message
struct FPalChatMessage {
    FString Message;
    FString Sender;
    uint32_t Category;
    FPalPlayerUId SenderPlayerUId;
    FPalPlayerUId ReceiverPlayerUId;
};

// ============================================================================
// Palworld Game Classes (Forward Declarations)
// ============================================================================

class UObject {
public:
    virtual ~UObject() {}
    // UObject vtable...
};

class UWorld : public UObject {
public:
    // GameState typically at offset 0x140-0x180
    void* GameState; // APalGameStateInGame*
};

class APalGameStateInGame : public UObject {
public:
    // PlayerArray typically at offset 0x2A0-0x2D0
    TArray<void*> PlayerArray; // TArray<APlayerState*>

    // GuildManager typically at offset 0x300-0x350
    void* GuildManager; // UPalGroupGuildManager*
};

class APlayerState : public UObject {
public:
    // PlayerId (Steam ID) typically at offset 0x300-0x330
    FString PlayerId;

    // PlayerName typically at offset 0x340-0x370
    FString PlayerName;

    // PlayerUId typically at offset 0x380-0x3B0
    FPalPlayerUId PlayerUId;

    // InventoryData typically at offset 0x400-0x450
    void* InventoryData; // UPalPlayerInventoryData*
};

class UPalGroupGuildManager : public UObject {
public:
    // Guilds array typically at offset 0x28-0x50
    TArray<void*> Guilds; // TArray<UPalGroupGuildBase*>
};

struct FPalGuildPlayerInfo {
    FPalPlayerUId PlayerUId;
    FString PlayerName;
    uint32_t Permission; // 0=Member, 1=Admin, etc.
};

class UPalGroupGuildBase : public UObject {
public:
    // GroupId typically at offset 0x28-0x40
    FString GroupId;

    // GroupName typically at offset 0x48-0x60
    FString GroupName;

    // AdminPlayerUId typically at offset 0x68-0x80
    FPalPlayerUId AdminPlayerUId;

    // Players (members) typically at offset 0x88-0xA0
    TArray<FPalGuildPlayerInfo> Players;

    // BaseCamps typically at offset 0xA8-0xC0
    TArray<void*> BaseCamps; // TArray<UPalBaseCampModel*>
};

struct FPalContainerSlot {
    void* ItemId; // FPalItemId*
    int32_t StackCount;
    int32_t SlotIndex;
};

class UPalPlayerInventoryData : public UObject {
public:
    // Containers typically at offset 0x28-0x50
    // Container 0 = main inventory
    // Container 1 = equipment
    // Container 2 = palbox, etc.
    TArray<void*> Containers; // TArray<UPalItemContainer*>
};

// ============================================================================
// Game Memory Access
// ============================================================================

class GameMemory {
public:
    // Initialize memory access (find GWorld, etc.)
    static bool Initialize();
    static void Shutdown();

    // Get global pointers
    static UWorld* GetWorld();
    static APalGameStateInGame* GetGameState();

    // Pattern scanning
    static uintptr_t FindPattern(const char* moduleName, const char* pattern, const char* mask);
    static uintptr_t FindGWorld();

    // Memory utilities
    template<typename T>
    static T* ReadPointer(void* address, uintptr_t offset) {
        if (!address) return nullptr;
        uintptr_t ptr = reinterpret_cast<uintptr_t>(address) + offset;
        if (IsBadReadPtr(reinterpret_cast<void*>(ptr), sizeof(T*))) {
            return nullptr;
        }
        return *reinterpret_cast<T**>(ptr);
    }

    template<typename T>
    static T ReadValue(void* address, uintptr_t offset) {
        if (!address) return T{};
        uintptr_t ptr = reinterpret_cast<uintptr_t>(address) + offset;
        if (IsBadReadPtr(reinterpret_cast<void*>(ptr), sizeof(T))) {
            return T{};
        }
        return *reinterpret_cast<T*>(ptr);
    }

    static bool IsValidPointer(void* ptr);

private:
    static uintptr_t palworldBase_;
    static void* gWorld_;
    static bool initialized_;
};

} // namespace TakaroPalworld
