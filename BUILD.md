# Building Takaro-Palworld-Integration

This guide provides detailed instructions for building Takaro-Palworld-Integration from source.

## Prerequisites

### Required Tools

- **CMake** (>= 3.20)
  - Download: https://cmake.org/download/
- **C++20 Compiler**
  - Windows: Visual Studio 2022 or later
  - Linux: GCC 11+ or Clang 14+
- **Git** - For cloning dependencies

### Required Libraries

Takaro-Palworld-Integration depends on several third-party libraries:

1. **Crow** - HTTP web framework (header-only)
2. **spdlog** - Fast logging library (header-only)
3. **nlohmann/json** - JSON parser (header-only)
4. **ASIO** - Asynchronous I/O (header-only)
5. **OpenSSL** - For HTTPS support (optional)

## Setting Up Dependencies

### Option 1: Manual Setup (Recommended)

Clone the dependencies into the `external/` directory:

```bash
cd Takaro-Palworld-Integration
mkdir -p external
cd external

# Clone Crow
git clone https://github.com/CrowCpp/Crow.git crow

# Clone spdlog
git clone https://github.com/gabime/spdlog.git spdlog

# Clone nlohmann/json
git clone https://github.com/nlohmann/json.git json

# Clone ASIO
git clone https://github.com/chriskohlhoff/asio.git asio
```

### Option 2: Using vcpkg (Windows)

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install crow spdlog nlohmann-json asio openssl

# Integrate with Visual Studio
.\vcpkg integrate install
```

### Option 3: Using package managers (Linux)

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install libssl-dev
# For header-only libraries, manual download to external/ is still recommended
```

#### Fedora/RHEL:
```bash
sudo dnf install openssl-devel
```

## Building on Windows

### Using Visual Studio 2022

1. **Open CMake Project:**
   ```
   File > Open > CMake... > Select CMakeLists.txt
   ```

2. **Configure Project:**
   - CMake will automatically configure
   - Select build configuration (Debug/Release)

3. **Build:**
   ```
   Build > Build All
   ```

### Using Command Line (MSVC)

```powershell
# Open Visual Studio Developer Command Prompt

mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Output will be in build/bin/Release/Takaro-Palworld-Integration.dll
```

### Using MinGW

```bash
mkdir build
cd build

cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Building on Linux

```bash
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Output will be in build/bin/Takaro-Palworld-Integration.dll
```

### Cross-Compiling for Windows from Linux

```bash
sudo apt install mingw-w64

mkdir build
cd build

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build .
```

Create `toolchain-mingw64.cmake`:
```cmake
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
```

## Build Configurations

### Debug Build

Includes debug symbols and assertions:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Release Build

Optimized for performance:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### RelWithDebInfo

Optimized with debug info:

```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build .
```

## CMake Options

Customize the build with CMake options:

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_TESTS=OFF
```

### Available Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build configuration (Debug/Release) | `Release` |
| `CMAKE_CXX_STANDARD` | C++ standard version | `20` |
| `BUILD_SHARED_LIBS` | Build as shared library (DLL) | `ON` |

## Troubleshooting Build Issues

### Missing Dependencies

**Error:** `Could not find nlohmann/json`

**Solution:**
```bash
cd external
git clone https://github.com/nlohmann/json.git json
```

### OpenSSL Not Found

**Windows:**
```powershell
# Using vcpkg
vcpkg install openssl:x64-windows
```

**Linux:**
```bash
sudo apt install libssl-dev
```

### C++20 Not Supported

**Error:** `C++20 features not available`

**Solution:** Update your compiler:
- Windows: Install Visual Studio 2022
- Linux: `sudo apt install g++-11` or later

### Crow Compilation Errors

**Error:** Missing ASIO headers

**Solution:**
```bash
cd external
git clone https://github.com/chriskohlhoff/asio.git asio
```

Ensure CMakeLists.txt includes:
```cmake
include_directories(${CMAKE_SOURCE_DIR}/external/asio/asio/include)
```

### Link Errors on Windows

**Error:** `unresolved external symbol`

**Solution:** Ensure you're linking against:
```cmake
target_link_libraries(Takaro-Palworld-Integration PRIVATE ws2_32)
```

## Verifying the Build

After building, verify the DLL:

### Windows

```powershell
# Check DLL exports
dumpbin /EXPORTS build\bin\Release\Takaro-Palworld-Integration.dll

# Should show: GetAPI
```

### Linux (Cross-Compile)

```bash
# Check DLL structure
x86_64-w64-mingw32-objdump -p build/bin/Takaro-Palworld-Integration.dll | grep "Export Address"
```

## Installation After Build

1. **Copy DLL:**
   ```bash
   cp build/bin/Release/Takaro-Palworld-Integration.dll /path/to/palworld/
   ```

2. **Create Configuration:**
   ```bash
   mkdir -p /path/to/palworld/Takaro-Palworld-Integration/RESTAPI
   ```

3. **Run Configuration:**
   - First run will auto-generate default config files
   - Edit as needed

## Build Output

Successful build produces:

```
build/
├── bin/
│   └── Release/
│       └── Takaro-Palworld-Integration.dll      # Main DLL
├── lib/                          # Static libraries (if any)
└── CMakeFiles/                   # Build artifacts
```

## Performance Optimization

### Compiler Flags

For maximum performance, add to CMakeLists.txt:

```cmake
if(MSVC)
    add_compile_options(/O2 /GL /arch:AVX2)
    add_link_options(/LTCG)
else()
    add_compile_options(-O3 -march=native -flto)
    add_link_options(-flto)
endif()
```

### Link Time Optimization (LTO)

```cmake
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

## Build Size Optimization

To reduce DLL size:

```cmake
if(MSVC)
    add_link_options(/OPT:REF /OPT:ICF)
else()
    add_link_options(-Wl,--gc-sections -Wl,--strip-all)
endif()
```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Build Takaro-Palworld-Integration

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3

      - name: Setup dependencies
        run: |
          git clone https://github.com/CrowCpp/Crow.git external/crow
          git clone https://github.com/gabime/spdlog.git external/spdlog
          git clone https://github.com/nlohmann/json.git external/json
          git clone https://github.com/chriskohlhoff/asio.git external/asio

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Upload artifact
        uses: actions/upload-artifact@v3
        with:
          name: Takaro-Palworld-Integration.dll
          path: build/bin/Release/Takaro-Palworld-Integration.dll
```

## Next Steps

After successfully building:

1. Read [IMPLEMENTATION.md](IMPLEMENTATION.md) to implement game hooks
2. Test the DLL with a Palworld server
3. Configure API endpoints and authentication
4. Set up monitoring and logging

## Getting Help

If you encounter build issues:

1. Check the error message carefully
2. Verify all dependencies are installed
3. Review CMake output for missing libraries
4. Check [GitHub Issues](https://github.com/yourrepo/issues)
5. Ask in the community Discord

## Build Time Expectations

Typical build times (Release configuration):

| System | Time |
|--------|------|
| Modern Desktop (8 cores) | ~2-3 minutes |
| Laptop (4 cores) | ~5-7 minutes |
| CI/CD Pipeline | ~3-5 minutes |

## Advanced: Custom Build Targets

### Build only Core library

```bash
cmake --build . --target Takaro-Palworld-Integration_Core
```

### Build with specific compiler

```bash
cmake .. -DCMAKE_CXX_COMPILER=clang++
```

### Build with sanitizers (Debug)

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"
```

---

**Last Updated:** 2024-12-28 | **Tested On:** Windows 11, Ubuntu 22.04
