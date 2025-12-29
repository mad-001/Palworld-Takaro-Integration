# Takaro-Palworld-Integration Build Script (Windows PowerShell)
# Run this from Windows PowerShell if you have Visual Studio installed

$ErrorActionPreference = "Stop"

$ProjectDir = "\\wsl.localhost\Ubuntu\home\zmedh\Takaro-Projects\Palworld-Takaro-Integration\Palworld-Takaro-Integration"
Set-Location $ProjectDir

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Takaro-Palworld-Integration Builder" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Check for CMake
Write-Host "[1/5] Checking build tools..." -ForegroundColor Yellow
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Host "✓ CMake found: $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "ERROR: CMake not found!" -ForegroundColor Red
    Write-Host "Download from: https://cmake.org/download/" -ForegroundColor Yellow
    exit 1
}

# Check for Visual Studio
try {
    $vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
        -latest -property installationPath
    Write-Host "✓ Visual Studio found: $vsPath" -ForegroundColor Green
} catch {
    Write-Host "WARNING: Visual Studio not detected, will try default paths" -ForegroundColor Yellow
}

Write-Host ""

# Step 2: Download dependencies
Write-Host "[2/5] Setting up dependencies..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path "external" | Out-Null
Set-Location external

if (-not (Test-Path "crow")) {
    Write-Host "Downloading Crow..."
    git clone --depth 1 https://github.com/CrowCpp/Crow.git crow
} else {
    Write-Host "✓ Crow already exists" -ForegroundColor Green
}

if (-not (Test-Path "spdlog")) {
    Write-Host "Downloading spdlog..."
    git clone --depth 1 https://github.com/gabime/spdlog.git spdlog
} else {
    Write-Host "✓ spdlog already exists" -ForegroundColor Green
}

if (-not (Test-Path "json")) {
    Write-Host "Downloading nlohmann/json..."
    git clone --depth 1 https://github.com/nlohmann/json.git json
} else {
    Write-Host "✓ nlohmann/json already exists" -ForegroundColor Green
}

if (-not (Test-Path "asio")) {
    Write-Host "Downloading ASIO..."
    git clone --depth 1 https://github.com/chriskohlhoff/asio.git asio
} else {
    Write-Host "✓ ASIO already exists" -ForegroundColor Green
}

Set-Location $ProjectDir
Write-Host ""

# Step 3: Configure with CMake
Write-Host "[3/5] Configuring build..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path "build" | Out-Null
Set-Location build

cmake .. -G "Visual Studio 17 2022" -A x64

Write-Host ""

# Step 4: Build
Write-Host "[4/5] Building..." -ForegroundColor Yellow
cmake --build . --config Release

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Build Complete!" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Output DLL: $(Get-Location)\bin\Release\Takaro-Palworld-Integration.dll" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Copy the DLL to your Palworld server"
Write-Host "2. Copy d3d9.dll and d3d9_config.json"
Write-Host "3. Copy the config directory"
Write-Host ""
