#!/usr/bin/env pwsh
# build-windows.ps1 - Windows build script with .env support

param(
    [Parameter(Position=0)]
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "",
    
    [ValidateSet("gcc", "clang", "")]
    [string]$Compiler = "",
    
    [switch]$NoTests,
    [switch]$NoBenchmarks,
    [int]$Jobs = 0,
    [string]$EnvFile = ".env",
    [switch]$Clean,
    [switch]$Sanitize,
    [switch]$Tsan,
    [switch]$Ubsan,
    [switch]$Help
)

# Help function
function Show-Help {
    Write-Host @"

${CYAN}Usage:${NC} .\build-windows.ps1 [BUILD_TYPE] [OPTIONS]

${CYAN}Build Types:${NC}
  Debug        - Debug build
  Release      - Release build with optimizations (default from .env)
  RelWithDebInfo - Release with debug info
  MinSizeRel   - Release optimized for size

${CYAN}Compiler:${NC}
  gcc          - Use GCC (MinGW)
  clang        - Use Clang
  (если не указан, берется из .env DEFAULT_COMPILER)

${CYAN}Options:${NC}
  -NoTests        Disable building tests
  -NoBenchmarks   Disable building benchmarks
  -Jobs N         Use N parallel jobs (default: from .env or CPU count)
  -Clean          Clean build directory before building
  -Sanitize       Enable AddressSanitizer (Debug only)
  -Tsan           Enable ThreadSanitizer (Debug only)
  -Ubsan          Enable UndefinedBehaviorSanitizer (Debug only)
  -Help           Show this help

${CYAN}Examples:${NC}
  .\build-windows.ps1                    # Build with settings from .env
  .\build-windows.ps1 Release             # Force Release build
  .\build-windows.ps1 Debug -Compiler gcc # Debug with GCC

"@
    exit 0
}

# Colors for output
$Green = [System.ConsoleColor]::Green
$Red = [System.ConsoleColor]::Red
$Yellow = [System.ConsoleColor]::Yellow
$Blue = [System.ConsoleColor]::Blue
$Cyan = [System.ConsoleColor]::Cyan
$Gray = [System.ConsoleColor]::Gray

# Show help if requested
if ($Help) {
    Show-Help
}

# Load environment from .env file
function Load-EnvFile {
    param([string]$FilePath)
    
    if (-not (Test-Path $FilePath)) {
        Write-Host "⚠️  File $FilePath not found, using defaults" -ForegroundColor Yellow
        return @{}
    }
    
    Write-Host "📁 Loading settings from $FilePath" -ForegroundColor Cyan
    $envVars = @{}
    
    Get-Content $FilePath | ForEach-Object {
        $line = $_.Trim()
        if ($line -and -not $line.StartsWith("#")) {
            $parts = $line.Split('=', 2)
            if ($parts.Count -eq 2) {
                $key = $parts[0].Trim()
                $value = $parts[1].Trim()
                $envVars[$key] = $value
                Write-Host "  ✓ $key = $value" -ForegroundColor Gray
            }
        }
    }
    
    return $envVars
}

# Load configuration
$envConfig = Load-EnvFile -FilePath $EnvFile

# Apply settings with priority: command line > .env > defaults

# Build type
if (-not $Config) {
    $Config = if ($envConfig.ContainsKey("BUILD_TYPE")) { $envConfig["BUILD_TYPE"] } else { "Debug" }
}

# Compiler
if (-not $Compiler) {
    $Compiler = if ($envConfig.ContainsKey("DEFAULT_COMPILER")) { $envConfig["DEFAULT_COMPILER"] } else { "gcc" }
}

# MSYS2 root
$msysRoot = if ($envConfig.ContainsKey("MSYS2_ROOT")) { 
    $envConfig["MSYS2_ROOT"] 
} else { 
    "D:/msys64" 
}

# Clang root (if different)
$clangRoot = if ($envConfig.ContainsKey("CLANG_ROOT")) { 
    $envConfig["CLANG_ROOT"] 
} else { 
    $msysRoot 
}

# Tests
if (-not $NoTests) {
    $buildTests = if ($envConfig.ContainsKey("BUILD_TESTS")) { 
        $envConfig["BUILD_TESTS"] 
    } else { 
        "ON" 
    }
} else {
    $buildTests = "OFF"
}

# Benchmarks
if (-not $NoBenchmarks) {
    $buildBenchmarks = if ($envConfig.ContainsKey("BUILD_BENCHMARKS")) { 
        $envConfig["BUILD_BENCHMARKS"] 
    } else { 
        "ON" 
    }
} else {
    $buildBenchmarks = "OFF"
}

# Parallel jobs
$parallelJobs = if ($Jobs -gt 0) { 
    $Jobs 
} elseif ($envConfig.ContainsKey("CMAKE_BUILD_PARALLEL_LEVEL") -and [int]$envConfig["CMAKE_BUILD_PARALLEL_LEVEL"] -gt 0) { 
    [int]$envConfig["CMAKE_BUILD_PARALLEL_LEVEL"] 
} else { 
    (Get-CimInstance -ClassName Win32_ComputerSystem).NumberOfLogicalProcessors
}

# Clean from env if not overridden by command line
if (-not $Clean -and $envConfig.ContainsKey("CLEAN_BEFORE_BUILD") -and $envConfig["CLEAN_BEFORE_BUILD"] -eq "ON") {
    $Clean = $true
}

# Sanitizers from env if not overridden
if (-not $Sanitize -and $envConfig.ContainsKey("ENABLE_ASAN") -and $envConfig["ENABLE_ASAN"] -eq "ON") {
    $Sanitize = $true
}
if (-not $Tsan -and $envConfig.ContainsKey("ENABLE_TSAN") -and $envConfig["ENABLE_TSAN"] -eq "ON") {
    $Tsan = $true
}
if (-not $Ubsan -and $envConfig.ContainsKey("ENABLE_UBSAN") -and $envConfig["ENABLE_UBSAN"] -eq "ON") {
    $Ubsan = $true
}

# Force setup PATH for MSYS2
$env:Path = "$msysRoot/ucrt64/bin;$msysRoot/usr/bin;$env:Path"

# Check DLL conflicts
$conflicts = @("zlib1.dll", "libwinpthread-1.dll", "libgcc_s_seh-1.dll")
foreach ($dll in $conflicts) {
    $sysPath = "C:\Windows\System32\$dll"
    if (Test-Path $sysPath) {
        Write-Host "⚠️  Warning: $dll found in System32 - may cause conflicts" -ForegroundColor Yellow
    }
}

# Set compiler paths based on choice
if ($Compiler -eq "clang") {
    # Check if path already contains /ucrt64
    if ($clangRoot -match "ucrt64$") {
        $compilerPath = "$clangRoot/bin/clang.exe"
        $compilerCxxPath = "$clangRoot/bin/clang++.exe"
    } else {
        $compilerPath = "$clangRoot/ucrt64/bin/clang.exe"
        $compilerCxxPath = "$clangRoot/ucrt64/bin/clang++.exe"
    }
    $compilerName = "Clang"
} else {
    $compilerPath = "$msysRoot/ucrt64/bin/gcc.exe"
    $compilerCxxPath = "$msysRoot/ucrt64/bin/g++.exe"
    $compilerName = "GCC"
}
$makePath = "$msysRoot/ucrt64/bin/mingw32-make.exe"

# Auto-disable tests and benchmarks when sanitizers are enabled
if ($Sanitize -or $Tsan -or $Ubsan) {
    if ($buildTests -eq "ON") {
        Write-Host "⚠️  Sanitizers enabled, disabling tests automatically" -ForegroundColor Yellow
        $buildTests = "OFF"
    }
    if ($buildBenchmarks -eq "ON") {
        Write-Host "⚠️  Sanitizers enabled, disabling benchmarks automatically" -ForegroundColor Yellow
        $buildBenchmarks = "OFF"
    }
}

Write-Host "`n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Blue
Write-Host "🔧 Windows Build Script" -ForegroundColor Green
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Blue
Write-Host "  • Build type:     " -NoNewline; Write-Host "$Config" -ForegroundColor Cyan
Write-Host "  • Compiler:       " -NoNewline; Write-Host "$compilerName" -ForegroundColor Cyan
Write-Host "  • MSYS2 root:     " -NoNewline; Write-Host "$msysRoot" -ForegroundColor Cyan
Write-Host "  • Tests:          " -NoNewline; Write-Host "$buildTests" -ForegroundColor Cyan
Write-Host "  • Benchmarks:     " -NoNewline; Write-Host "$buildBenchmarks" -ForegroundColor Cyan
Write-Host "  • Parallel jobs:  " -NoNewline; Write-Host "$parallelJobs" -ForegroundColor Cyan
if ($Sanitize) {
    Write-Host "  • AddressSanitizer: " -NoNewline; Write-Host "ON" -ForegroundColor Yellow
}
if ($Tsan) {
    Write-Host "  • ThreadSanitizer: " -NoNewline; Write-Host "ON" -ForegroundColor Yellow
}
if ($Ubsan) {
    Write-Host "  • UBSan:           " -NoNewline; Write-Host "ON" -ForegroundColor Yellow
}
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Blue

# Verify tools
$missingTools = @()
if (-not (Test-Path $compilerPath)) { $missingTools += "$compilerName compiler" }
if (-not (Test-Path $compilerCxxPath)) { $missingTools += "$compilerName C++ compiler" }
if (-not (Test-Path $makePath)) { $missingTools += "mingw32-make.exe" }

if ($missingTools.Count -gt 0) {
    Write-Host "`n❌ Error: Missing components:" -ForegroundColor Red
    $missingTools | ForEach-Object { Write-Host "   • $_" -ForegroundColor Red }
    Write-Host "`nPlease check MSYS2_ROOT in .env file or install required packages" -ForegroundColor Yellow
    if ($Compiler -eq "clang") {
        Write-Host "  For Clang: pacman -S mingw-w64-ucrt-x86_64-clang" -ForegroundColor Yellow
    }
    exit 1
}

Write-Host "`n✅ All components found" -ForegroundColor Green

# Show compiler version
Write-Host "`n🔧 Compiler info:" -ForegroundColor Blue
& "$compilerCxxPath" --version | Select-Object -First 1

# Check for optional tools
Write-Host "`n📦 Optional tools:" -ForegroundColor Blue
if (Get-Command ccache -ErrorAction SilentlyContinue) {
    Write-Host "  ✅ ccache found - will speed up rebuilds" -ForegroundColor Green
    $env:CCACHE_DIR = "$env:USERPROFILE\.ccache"
    $env:USE_CCACHE = "1"
} else {
    Write-Host "  ⚠️  ccache not found (optional, for faster rebuilds)" -ForegroundColor Yellow
}

# Warn about sanitizers with GCC on Windows
if ($Compiler -eq "gcc" -and ($Sanitize -or $Tsan -or $Ubsan)) {
    Write-Host "`n⚠️  Warning: Sanitizers with GCC on Windows may not work properly" -ForegroundColor Yellow
    Write-Host "   For better sanitizer support, use: -Compiler clang or WSL" -ForegroundColor Yellow
}

# Warn about any sanitizers on Windows (they're experimental)
if ($Sanitize -or $Tsan -or $Ubsan) {
    Write-Host "`n⚠️  Note: Sanitizers on Windows are experimental and may not work" -ForegroundColor Yellow
    Write-Host "   For reliable sanitizer support, use WSL: ./build-linux.sh Debug --sanitize" -ForegroundColor Yellow
}

# Clean build if requested
if ($Clean) {
    Write-Host "`n🧹 Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force build\windows -ErrorAction SilentlyContinue
}

# Create build directory
New-Item -ItemType Directory -Force -Path build\windows | Out-Null

# Save build info
$buildInfo = @"
Build type: $Config
Compiler: $compilerName
Date: $(Get-Date)
Compiler version: $(& "$compilerCxxPath" --version | Select-Object -First 1)
"@
$buildInfo | Out-File -FilePath build\windows\build_info.txt

# CMake configuration
Write-Host "`n⚙️  Running CMake configuration..." -ForegroundColor Blue

# Build CMake arguments
$cmakeArgs = @(
    "-S", ".",
    "-B", "build/windows",
    "-G", "MinGW Makefiles",
    "-DCMAKE_C_COMPILER=$compilerPath",
    "-DCMAKE_CXX_COMPILER=$compilerCxxPath",
    "-DCMAKE_MAKE_PROGRAM=$makePath",
    "-DCMAKE_BUILD_TYPE=$Config",
    "-DBUILD_TESTS=$buildTests",
    "-DBUILD_BENCHMARKS=$buildBenchmarks",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

# Add sanitizers if requested and Debug build
if ($Config -eq "Debug") {
    if ($Sanitize) {
        $cmakeArgs += "-DUSE_ASAN=ON"
        Write-Host "  AddressSanitizer enabled${NC}" -ForegroundColor Yellow
    }
    if ($Tsan) {
        $cmakeArgs += "-DUSE_TSAN=ON"
        Write-Host "  ThreadSanitizer enabled${NC}" -ForegroundColor Yellow
    }
    if ($Ubsan) {
        $cmakeArgs += "-DUSE_UBSAN=ON"
        Write-Host "  UBSan enabled${NC}" -ForegroundColor Yellow
    }
}

# Run CMake
$configResult = cmake @cmakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n❌ CMake configuration failed" -ForegroundColor Red
    exit $LASTEXITCODE
}

# Build
Write-Host "`n🔨 Building project..." -ForegroundColor Blue
Write-Host "  Using $parallelJobs parallel jobs`n" -ForegroundColor Cyan

# Time the build
$buildStart = Get-Date

if ($parallelJobs -gt 0) {
    $buildResult = cmake --build build\windows -j $parallelJobs
} else {
    $buildResult = cmake --build build\windows
}

$buildEnd = Get-Date
$buildTime = ($buildEnd - $buildStart).TotalSeconds

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n✅ Build completed successfully!" -ForegroundColor Green
    Write-Host "   Build time: " -NoNewline; Write-Host "$([math]::Round($buildTime, 1))s" -ForegroundColor Cyan
    Write-Host "   Executable: " -NoNewline; Write-Host "build\windows\src\log_processor.exe" -ForegroundColor Cyan
    
    # Copy compile_commands.json for IDE support
    if (Test-Path build\windows\compile_commands.json) {
        Copy-Item build\windows\compile_commands.json . -Force
        Write-Host "   📝 compile_commands.json copied for code completion" -ForegroundColor Green
    }
    
    # Show executable size
    if (Test-Path build\windows\src\log_processor.exe) {
        $size = (Get-Item build\windows\src\log_processor.exe).Length
        $sizeMB = [math]::Round($size / 1MB, 2)
        Write-Host "   📦 Executable size: " -NoNewline; Write-Host "$sizeMB MB" -ForegroundColor Cyan
    }
    
    Write-Host "`n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Blue
    Write-Host "Next steps:" -ForegroundColor Green
    Write-Host "  Run program:    " -NoNewline; Write-Host ".\build\windows\src\log_processor.exe <logfile>" -ForegroundColor Cyan
    Write-Host "  Run tests:      " -NoNewline; Write-Host "cd build\windows && ctest --output-on-failure" -ForegroundColor Cyan
    Write-Host "  Run benchmarks: " -NoNewline; Write-Host ".\build\windows\benchmarks\log_benchmark.exe" -ForegroundColor Cyan
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Blue
} else {
    Write-Host "`n❌ Build failed" -ForegroundColor Red
    exit $LASTEXITCODE
}