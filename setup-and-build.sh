#!/bin/bash
# Takaro-Palworld-Integration Build Script
# This script sets up dependencies and builds the project

set -e  # Exit on error

PROJECT_DIR="/home/zmedh/Takaro-Projects/Palworld-Takaro-Integration/Palworld-Takaro-Integration"
cd "$PROJECT_DIR"

echo "=========================================="
echo "  Takaro-Palworld-Integration Builder"
echo "=========================================="
echo ""

# Step 1: Check for required tools
echo "[1/5] Checking build tools..."
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found!"
    echo "Install with: sudo apt install cmake"
    exit 1
fi

if ! command -v x86_64-w64-mingw32-g++ &> /dev/null; then
    echo "ERROR: MinGW cross-compiler not found!"
    echo "Install with: sudo apt install mingw-w64"
    exit 1
fi

echo "✓ CMake found: $(cmake --version | head -1)"
echo "✓ MinGW found: $(x86_64-w64-mingw32-g++ --version | head -1)"
echo ""

# Step 2: Download dependencies
echo "[2/5] Setting up dependencies..."
mkdir -p external
cd external

if [ ! -d "crow" ]; then
    echo "Downloading Crow..."
    git clone --depth 1 https://github.com/CrowCpp/Crow.git crow
else
    echo "✓ Crow already exists"
fi

if [ ! -d "spdlog" ]; then
    echo "Downloading spdlog..."
    git clone --depth 1 https://github.com/gabime/spdlog.git spdlog
else
    echo "✓ spdlog already exists"
fi

if [ ! -d "json" ]; then
    echo "Downloading nlohmann/json..."
    git clone --depth 1 https://github.com/nlohmann/json.git json
else
    echo "✓ nlohmann/json already exists"
fi

if [ ! -d "asio" ]; then
    echo "Downloading ASIO..."
    git clone --depth 1 https://github.com/chriskohlhoff/asio.git asio
else
    echo "✓ ASIO already exists"
fi

cd "$PROJECT_DIR"
echo ""

# Step 3: Create toolchain file for cross-compilation
echo "[3/5] Creating CMake toolchain..."
cat > toolchain-mingw64.cmake << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF
echo "✓ Toolchain file created"
echo ""

# Step 4: Configure with CMake
echo "[4/5] Configuring build..."
mkdir -p build
cd build

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20

echo ""

# Step 5: Build
echo "[5/5] Building..."
cmake --build . -j$(nproc)

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="
echo ""
echo "Output DLL: $(pwd)/bin/Takaro-Palworld-Integration.dll"
echo ""
echo "Next steps:"
echo "1. Copy the DLL to your Palworld server"
echo "2. Copy d3d9.dll and d3d9_config.json"
echo "3. Copy the config directory"
echo ""
