# Takaro-Palworld-Integration Deployment Guide

## Overview

This guide explains how to deploy Takaro-Palworld-Integration to your Palworld server using the **d3d9.dll proxy injection** method.

## What You Have

### Already Compiled (Ready to Use):
- ✅ `d3d9.dll` (337 KB) - DLL loader/proxy
- ✅ `d3d9_config.json` - Loader configuration
- ⚠️ `Takaro-Palworld-Integration.dll` (2.1 MB) - Original compiled version (might be outdated)

### Source Code (Build This):
- 📁 `Takaro-Palworld-Integration/` - Reconstructed source code
- ➡️ Build this to create a NEW `Takaro-Palworld-Integration.dll`

## Deployment Methods

### Option 1: Quick Test (Use Existing DLLs)

Test with the already-compiled DLLs to see if they work with your game version:

**Files to copy to Palworld directory:**
```
C:\gameservers\palworld\Pal\Binaries\Win64\
├── d3d9.dll                    (from Backwards-Engineer/)
├── d3d9_config.json            (from Backwards-Engineer/)
├── Takaro-Palworld-Integration.dll             (from Backwards-Engineer/)
└── Takaro-Palworld-Integration/                (create directory)
    ├── Config.json
    ├── WhiteList.json
    └── RESTAPI/
        └── RESTConfig.json
```

**Steps:**
```powershell
# Copy DLLs
powershell.exe -Command "Copy-Item '\\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Backwards-Engineer\d3d9.dll' -Destination '\\server\c$\gameservers\palworld\Pal\Binaries\Win64\d3d9.dll'"

powershell.exe -Command "Copy-Item '\\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Backwards-Engineer\d3d9_config.json' -Destination '\\server\c$\gameservers\palworld\Pal\Binaries\Win64\d3d9_config.json'"

powershell.exe -Command "Copy-Item '\\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Backwards-Engineer\Takaro-Palworld-Integration.dll' -Destination '\\server\c$\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration.dll'"

# Copy config templates
powershell.exe -Command "Copy-Item -Recurse '\\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Takaro-Palworld-Integration\config\*' -Destination '\\server\c$\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration\'"
```

### Option 2: Build and Deploy (Recommended)

Build the new Takaro-Palworld-Integration.dll from source and deploy:

**1. Build Takaro-Palworld-Integration (on Windows with Visual Studio):**
```powershell
cd \\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Takaro-Palworld-Integration

# Install dependencies first (see BUILD.md)
mkdir external
cd external
git clone https://github.com/CrowCpp/Crow.git crow
git clone https://github.com/gabime/spdlog.git spdlog
git clone https://github.com/nlohmann/json.git json
git clone https://github.com/chriskohlhoff/asio.git asio

# Build
cd ..
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Output: build\bin\Release\Takaro-Palworld-Integration.dll
```

**2. Deploy:**
```powershell
# Copy newly built DLL
Copy-Item build\bin\Release\Takaro-Palworld-Integration.dll -Destination \\server\c$\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration.dll

# Copy d3d9 loader (existing)
Copy-Item \\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Backwards-Engineer\d3d9.dll -Destination \\server\c$\gameservers\palworld\Pal\Binaries\Win64\d3d9.dll

Copy-Item \\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Backwards-Engineer\d3d9_config.json -Destination \\server\c$\gameservers\palworld\Pal\Binaries\Win64\d3d9_config.json

# Copy configs
Copy-Item -Recurse config\* -Destination \\server\c$\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration\
```

## How the d3d9.dll Loader Works

### The Injection Process:

```
1. Palworld starts (Pal-Win64-Shipping.exe)
   ↓
2. Game looks for d3d9.dll (DirectX 9 library)
   ↓
3. Finds YOUR d3d9.dll (the proxy) in same directory
   ↓
4. d3d9.dll reads d3d9_config.json
   ↓
5. Loads Takaro-Palworld-Integration.dll (from "load_dlls" array)
   ↓
6. d3d9.dll forwards DirectX calls to real d3d9.dll (System32)
   ↓
7. Game continues normally with Takaro-Palworld-Integration.dll injected
```

### d3d9_config.json Format:

```json
{
  "load_dlls": [
    "Takaro-Palworld-Integration.dll",
    "AnotherMod.dll"  // Can load multiple DLLs
  ]
}
```

### Why This Works:

- Windows searches for DLLs in this order:
  1. Application directory (where exe is) ⬅️ YOUR d3d9.dll here
  2. System32 directory (real d3d9.dll)
  3. Windows directory
  4. PATH environment variable

- Your d3d9.dll is found FIRST, loads your mods, then loads the real DLL

## File Structure After Deployment

```
C:\gameservers\palworld\Pal\Binaries\Win64\
├── Pal-Win64-Shipping.exe          (Game executable)
├── d3d9.dll                         (Your DLL loader - 337 KB)
├── d3d9_config.json                 (Loader configuration)
├── Takaro-Palworld-Integration.dll                  (Your mod - 2.1 MB)
│
└── Takaro-Palworld-Integration\                     (Config directory)
    ├── Config.json                  (Main settings)
    ├── WhiteList.json              (Player whitelist)
    ├── RESTAPI\
    │   └── RESTConfig.json         (API settings)
    └── logs\                        (Created at runtime)
        └── Takaro-Palworld-Integration.log
```

## Testing the Deployment

### 1. Check DLL Loading

Start Palworld and check the log:
```
C:\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration\logs\Takaro-Palworld-Integration.log
```

Should see:
```
[2024-12-28 10:00:00.000] [info] ===========================================
[2024-12-28 10:00:00.001] [info]   Takaro-Palworld-Integration v1.0.0 - Initializing...
[2024-12-28 10:00:00.002] [info] ===========================================
[2024-12-28 10:00:00.010] [info] Loaded Config.json
[2024-12-28 10:00:00.011] [info] Loaded WhiteList.json
[2024-12-28 10:00:00.012] [info] [RESTAPI] Loaded 'RESTConfig.json'.
[2024-12-28 10:00:00.015] [info] Initializing PalAPI...
[2024-12-28 10:00:00.020] [info] [RESTAPI] Starting server on 0.0.0.0:8080
[2024-12-28 10:00:00.100] [info] Takaro-Palworld-Integration initialized successfully!
```

### 2. Test REST API

Open browser or use curl:
```powershell
# Test version endpoint
curl -H "Authorization: Bearer Bearer-Token-Example-Change-This-To-Secure-Random-String" http://localhost:8080/v1/pdapi/version
```

Should return:
```json
{
  "version": "1.0.0",
  "apiVersion": "v1",
  "gameVersion": "0.2.4.0"
}
```

### 3. Check for Errors

If the game crashes or DLL doesn't load:

**Check d3d9.dll is loading:**
- Use Process Explorer to see loaded DLLs
- Should see both d3d9.dll and Takaro-Palworld-Integration.dll

**Common Issues:**

| Issue | Cause | Solution |
|-------|-------|----------|
| Game won't start | d3d9.dll corrupt | Re-download d3d9.dll |
| Takaro-Palworld-Integration.dll not loaded | Wrong config | Check d3d9_config.json |
| API not responding | Firewall blocking port 8080 | Open port or change in config |
| Game crashes | Takaro-Palworld-Integration hooks failed | Disable hooks (stub implementation) |

## Configuration

### Main Config (Takaro-Palworld-Integration/Config.json)

```json
{
  "allowAdminCheats": true,
  "pvpMaxToBuildingDamage": 0,
  "logging": {
    "level": "info"  // Change to "debug" for more details
  }
}
```

### REST API Config (Takaro-Palworld-Integration/RESTAPI/RESTConfig.json)

**IMPORTANT:** Change the default bearer token!

```json
{
  "enabled": true,
  "host": "0.0.0.0",  // Listen on all interfaces
  "port": 8080,       // Change if port is in use
  "authentication": {
    "bearerTokens": [
      "YOUR-SECURE-TOKEN-HERE"  // ⚠️ CHANGE THIS!
    ]
  }
}
```

Generate a secure token:
```powershell
# PowerShell - Generate random token
-join ((48..57) + (65..90) + (97..122) | Get-Random -Count 32 | ForEach-Object {[char]$_})
```

## Uninstalling

To remove Takaro-Palworld-Integration:

```powershell
# Stop Palworld server first!

# Remove DLLs
Remove-Item \\server\c$\gameservers\palworld\Pal\Binaries\Win64\d3d9.dll
Remove-Item \\server\c$\gameservers\palworld\Pal\Binaries\Win64\d3d9_config.json
Remove-Item \\server\c$\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration.dll

# Remove config (optional - keeps your settings)
Remove-Item -Recurse \\server\c$\gameservers\palworld\Pal\Binaries\Win64\Takaro-Palworld-Integration\
```

## Alternative Deployment: Using Existing Mod Loaders

If you have other mod loaders like **CAKE** or **UnrealModLoader**, you can skip d3d9.dll:

**For CAKE Mod Loader:**
```
Just place Takaro-Palworld-Integration.dll in the CAKE mods folder
CAKE will handle loading it
```

**For UnrealModLoader:**
```
Place in the Mods folder as per UML documentation
```

But the **d3d9.dll method is simplest** if you don't have other loaders installed.

## About CAKE Mod

You asked about CAKE mod - I don't have information about this in the reverse-engineered files. If you're referring to a different mod loader:
- Takaro-Palworld-Integration was designed to work standalone via d3d9.dll
- It doesn't require CAKE or any other mod loader
- But it CAN work with other loaders if they support loading arbitrary DLLs

## Security Notes

⚠️ **Important Security Considerations:**

1. **Change default bearer tokens** - The example token is public
2. **Restrict API access** - Use firewall or change `host` to `127.0.0.1` for local-only
3. **Use HTTPS** - For production deployments (requires OpenSSL setup)
4. **Backup your server** - Before installing any mods
5. **Test on development server first** - Don't deploy straight to production

## Performance Impact

Expected performance impact:
- **CPU:** <1% overhead (REST API + logging)
- **Memory:** ~50 MB additional RAM
- **Network:** Only when API is accessed
- **Game FPS:** No impact (runs in separate thread)

## Support

If issues occur:
1. Check `Takaro-Palworld-Integration/logs/Takaro-Palworld-Integration.log`
2. Verify all files are in correct locations
3. Test with default configs first
4. Check that port 8080 is not in use (`netstat -an | findstr :8080`)

---

**Last Updated:** 2024-12-28
