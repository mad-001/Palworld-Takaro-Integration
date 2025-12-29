# Quick Start - Building Takaro-Palworld-Integration

## Prerequisites (Install First)

You need to install build tools before building. Choose ONE option:

### Option 1: Build in WSL (Ubuntu)

**Install required tools:**
```bash
sudo apt update
sudo apt install -y cmake mingw-w64 git
```

**Then run:**
```bash
cd /home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration
chmod +x setup-and-build.sh
./setup-and-build.sh
```

### Option 2: Build in Windows (Recommended)

**Install required tools:**
1. **CMake:** https://cmake.org/download/ (Windows x64 Installer)
2. **Visual Studio 2022:** https://visualstudio.microsoft.com/downloads/ (Community Edition is free)
   - During install, select "Desktop development with C++"
3. **Git:** https://git-scm.com/download/win

**Then run in PowerShell:**
```powershell
cd \\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Palworld-Takaro-Integration
.\setup-and-build.ps1
```

## What the Build Scripts Do

Both scripts automatically:
1. ✅ Check for required build tools
2. ✅ Download dependencies (Crow, spdlog, nlohmann/json, ASIO)
3. ✅ Configure CMake
4. ✅ Build the DLL
5. ✅ Output: `build/bin/Release/Takaro-Palworld-Integration.dll`

## Manual Build (If Scripts Don't Work)

### WSL/Linux:
```bash
cd /home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration

# Install dependencies
mkdir -p external && cd external
git clone https://github.com/CrowCpp/Crow.git crow
git clone https://github.com/gabime/spdlog.git spdlog
git clone https://github.com/nlohmann/json.git json
git clone https://github.com/chriskohlhoff/asio.git asio
cd ..

# Build
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Windows:
```powershell
cd \\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Palworld-Takaro-Integration

# Install dependencies
mkdir external
cd external
git clone https://github.com/CrowCpp/Crow.git crow
git clone https://github.com/gabime/spdlog.git spdlog
git clone https://github.com/nlohmann/json.git json
git clone https://github.com/chriskohlhoff/asio.git asio
cd ..

# Build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## After Building

Your DLL will be at:
- WSL: `/home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration/build/bin/Takaro-Palworld-Integration.dll`
- Windows: `\\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Palworld-Takaro-Integration\build\bin\Release\Takaro-Palworld-Integration.dll`

## Deploy to Palworld

Copy these files to your Palworld server:
```
C:\gameservers\palworld\Pal\Binaries\Win64\
├── d3d9.dll                                    (from Backwards-Engineer/)
├── d3d9_config.json                            (from Backwards-Engineer/)
├── Takaro-Palworld-Integration.dll             (from build output)
└── Takaro-Palworld-Integration/                (from config/)
    ├── Config.json
    ├── WhiteList.json
    └── RESTAPI/RESTConfig.json
```

See DEPLOYMENT.md for detailed deployment instructions.

---

**TL;DR:**
1. Install CMake + Visual Studio (Windows) OR CMake + MinGW (WSL)
2. Run `setup-and-build.ps1` (Windows) OR `setup-and-build.sh` (WSL)
3. Copy output DLL + configs to Palworld directory
