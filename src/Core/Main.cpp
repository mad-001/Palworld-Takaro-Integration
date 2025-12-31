// Step 4: Add Config, PalAPI, TakaroClient, and REST API headers
#include "TakaroPalworld/Core/Config.h"
#include "TakaroPalworld/Core/Logger.h"
#include "TakaroPalworld/Core/PalAPI.h"
#include "TakaroPalworld/Takaro/TakaroClient.h"
#include "TakaroPalworld/API/RestServer.h"
#include <windows.h>
#include <string>

// C++ logging with std::string
static void WriteBootLog(const std::string& message) {
    HANDLE hFile = CreateFileA(
        "Takaro-Boot.txt",
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hFile, message.c_str(), message.length(), &written, NULL);
        WriteFile(hFile, "\r\n", 2, &written, NULL);
        FlushFileBuffers(hFile);  // FORCE flush to disk
        CloseHandle(hFile);
    }
}

// Global state
static bool g_Running = false;
static HANDLE g_MainThread = NULL;

void Initialize() {
    WriteBootLog("===========================================");
    WriteBootLog("WEBSOCKET VERSION - 2025-12-31 02:30 - WITH WS DEBUG");
    WriteBootLog("Initialize() START - Testing WebSocket Build");
    WriteBootLog("===========================================");

    try {
        // Initialize Logger first
        WriteBootLog("Initializing Logger...");
        auto& logger = TakaroPalworld::Logger::GetInstance();
        logger.Initialize("./logs", 1024 * 1024 * 5, 3);  // 5MB per file, 3 files
        WriteBootLog("Logger initialized successfully!");

        // Load Config
        WriteBootLog("Loading ConfigManager...");
        auto& config = TakaroPalworld::ConfigManager::GetInstance();
        WriteBootLog("ConfigManager instance created");

        WriteBootLog("Loading configurations from ./Takaro-Palworld-Integration...");
        config.LoadConfigurations("./Takaro-Palworld-Integration");
        WriteBootLog("Configurations loaded successfully!");

        // Debug: Log bearer tokens
        {
            auto cfg = config.GetRESTConfig();
            WriteBootLog("Bearer tokens loaded: " + std::to_string(cfg.authentication.bearerTokens.size()));
            for (size_t i = 0; i < cfg.authentication.bearerTokens.size(); i++) {
                WriteBootLog("Token " + std::to_string(i) + ": " + cfg.authentication.bearerTokens[i]);
            }
        }

        // Initialize PalAPI
        WriteBootLog("Initializing PalAPI...");
        auto& palAPI = TakaroPalworld::PalAPI::GetInstance();
        if (palAPI.Initialize()) {
            WriteBootLog("PalAPI initialized successfully!");
        } else {
            WriteBootLog("WARNING: PalAPI initialization returned false");
        }

        // Initialize TakaroClient
        WriteBootLog("Initializing TakaroClient...");
        auto& takaroClient = TakaroPalworld::TakaroClient::GetInstance();
        auto takaroConfig = config.GetTakaroConfig();

        // Log Takaro config details
        {
            HANDLE hFile = CreateFileA("TAKARO_CONNECTION.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                std::string info = "Takaro Config:\n";
                info += "Enabled: " + std::string(takaroConfig.enabled ? "true" : "false") + "\n";
                info += "WebSocket URL: " + takaroConfig.websocketUrl + "\n";
                info += "Identity Token: " + takaroConfig.identityToken + "\n";
                info += "Registration Token: " + takaroConfig.registrationToken + "\n\n";
                DWORD written;
                WriteFile(hFile, info.c_str(), info.length(), &written, NULL);
                CloseHandle(hFile);
            }
        }

        if (!takaroConfig.enabled) {
            WriteBootLog("TakaroClient is DISABLED in config");
        } else if (takaroClient.Initialize(takaroConfig.websocketUrl, takaroConfig.identityToken, takaroConfig.registrationToken)) {
            WriteBootLog("TakaroClient initialized successfully!");
            WriteBootLog("Connecting to Takaro...");
            if (takaroClient.Connect()) {
                WriteBootLog("TakaroClient connection started!");
            } else {
                WriteBootLog("WARNING: TakaroClient connection failed");
            }
        } else {
            WriteBootLog("WARNING: TakaroClient initialization failed");
        }

        // Initialize REST API Server
        WriteBootLog("Initializing REST API Server...");
        auto& restServer = TakaroPalworld::RestServer::GetInstance();
        auto restConfig = config.GetRESTConfig();

        // Write port to separate file
        {
            HANDLE hFile = CreateFileA("PORT_INFO.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                std::string portInfo = "Port: " + std::to_string(restConfig.port) + "\n";
                portInfo += "Host: " + restConfig.host + "\n";
                portInfo += "Tokens: " + std::to_string(restConfig.authentication.bearerTokens.size()) + "\n";
                for (size_t i = 0; i < restConfig.authentication.bearerTokens.size(); i++) {
                    portInfo += "Token " + std::to_string(i) + ": " + restConfig.authentication.bearerTokens[i] + "\n";
                }
                DWORD written;
                WriteFile(hFile, portInfo.c_str(), portInfo.length(), &written, NULL);
                CloseHandle(hFile);
            }
        }

        if (restServer.Start(restConfig)) {
            WriteBootLog("REST API Server started successfully!");
        } else {
            WriteBootLog("REST API Server not started (disabled or failed)");
        }

        g_Running = true;
        WriteBootLog("BOOT COMPLETE - All components initialized!");
    } catch (const std::exception& e) {
        WriteBootLog(std::string("EXCEPTION: ") + e.what());
    }
}

void Shutdown() {
    WriteBootLog("Shutdown() called");
    g_Running = false;
}

void MainLoop() {
    while (g_Running) {
        Sleep(100);
    }
}

// Thread entry point
static DWORD WINAPI MainThreadProc(LPVOID lpParam) {
    WriteBootLog("MainThreadProc started - NO SLEEP");

    WriteBootLog("About to call Initialize()...");
    try {
        Initialize();
        WriteBootLog("Initialize() returned successfully!");
    } catch (const std::exception& e) {
        WriteBootLog(std::string("EXCEPTION in Initialize(): ") + e.what());
    } catch (...) {
        WriteBootLog("UNKNOWN EXCEPTION in Initialize()");
    }

    WriteBootLog("Entering MainLoop()...");
    try {
        MainLoop();
    } catch (...) {
        WriteBootLog("EXCEPTION in MainLoop");
    }

    WriteBootLog("MainThreadProc exiting");
    return 0;
}

// DLL Entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            {
                WriteBootLog("***********************************************************");
                WriteBootLog("DLL_PROCESS_ATTACH - Takaro-Palworld-Integration.dll loading");
                WriteBootLog("***********************************************************");

                HANDLE hFile = CreateFileA(
                    "TAKARO_LOADED.txt",
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                if (hFile != INVALID_HANDLE_VALUE) {
                    const char* msg = "DLL LOADED!\n";
                    DWORD written;
                    WriteFile(hFile, msg, strlen(msg), &written, NULL);
                    CloseHandle(hFile);
                    WriteBootLog("Created TAKARO_LOADED.txt marker");
                }

                WriteBootLog("Disabling thread library calls...");
                DisableThreadLibraryCalls(hModule);

                WriteBootLog("Creating initialization thread...");
                g_MainThread = CreateThread(
                    NULL,
                    0,
                    MainThreadProc,
                    NULL,
                    0,
                    NULL
                );

                if (g_MainThread) {
                    WriteBootLog("Initialization thread created successfully");
                } else {
                    WriteBootLog("ERROR: Failed to create initialization thread");
                }
            }
            break;

        case DLL_PROCESS_DETACH:
            WriteBootLog("***********************************************************");
            WriteBootLog("DLL_PROCESS_DETACH - Shutting down");
            WriteBootLog("***********************************************************");

            g_Running = false;

            if (g_MainThread) {
                WriteBootLog("Waiting for initialization thread to exit...");
                WaitForSingleObject(g_MainThread, 2000);
                CloseHandle(g_MainThread);
                g_MainThread = NULL;
                WriteBootLog("Initialization thread closed");
            }

            WriteBootLog("DLL_PROCESS_DETACH complete");
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

// Export function for API access (optional)
extern "C" __declspec(dllexport) void* GetAPI() {
    return nullptr; // Temporarily disabled
}
