#!/usr/bin/env pwsh
param(
    [string]$TestFile = "",
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"

# Определяем корень проекта (там, где лежат папки build, data, scripts и т.д.)
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Resolve-Path "$scriptPath\.."

# Переходим в корень проекта, чтобы все относительные пути работали
Set-Location $projectRoot

# Определяем ветку Git
$branch = git symbolic-ref --short HEAD 2>$null
if (-not $branch) {
    $branch = "unknown"
}

$timestamp = Get-Date -Format "yyyy-MM-dd_HH-mm-ss"
$outDir = "results\$branch"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

# Определяем тестовый файл
if ($TestFile) {
    $testFile = $TestFile
} elseif ($env:TEST_LOG_FILE) {
    $testFile = $env:TEST_LOG_FILE
} else {
    $testFile = "data\sample.log"
}

if (-not (Test-Path $testFile)) {
    Write-Host "❌ Test file $testFile not found. Run generate_logs.py first." -ForegroundColor Red
    exit 1
}

# Проверяем наличие бенчмарка
$benchmarkExe = "build\windows\benchmarks\log_benchmark.exe"
if (-not (Test-Path $benchmarkExe)) {
    if ($NoBuild) {
        Write-Host "❌ Benchmark executable not found and -NoBuild specified." -ForegroundColor Red
        exit 1
    }
    Write-Host "⚠️  Benchmark executable not found. Building..." -ForegroundColor Yellow
    & .\build-windows.ps1 -Config Release -NoTests -NoBenchmarks
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ Build failed." -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

# Сохраняем информацию о системе
$infoFile = "$outDir\${timestamp}_info.txt"
@"
Branch: $branch
Timestamp: $timestamp
Test file: $testFile
--- System ---
OS: $([Environment]::OSVersion)
Machine: $env:COMPUTERNAME
Processor: $((Get-CimInstance Win32_Processor).Name)
Cores: $((Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors)
--- Compiler ---
"@ | Out-File -FilePath $infoFile -Encoding utf8

# Определяем компилятор
if (Get-Command g++ -ErrorAction SilentlyContinue) {
    $compilerVersion = g++ --version | Select-Object -First 1
} elseif (Get-Command clang++ -ErrorAction SilentlyContinue) {
    $compilerVersion = clang++ --version | Select-Object -First 1
} else {
    $compilerVersion = "Unknown"
}
"$compilerVersion" | Out-File -FilePath $infoFile -Encoding utf8 -Append

$commit = git rev-parse HEAD 2>$null
if ($commit) {
    "--- Git commit ---" | Out-File -FilePath $infoFile -Encoding utf8 -Append
    $commit | Out-File -FilePath $infoFile -Encoding utf8 -Append
}

Write-Host "🚀 Running benchmarks..." -ForegroundColor Green
$env:TEST_LOG_FILE = $testFile
$benchmarkArgs = @(
    "--benchmark_format=json",
    "--benchmark_out=$outDir\${timestamp}_benchmark.json"
)

& $benchmarkExe $benchmarkArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Benchmarks failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "✅ Results saved to $outDir\${timestamp}_benchmark.json" -ForegroundColor Green
Write-Host "   System info saved to $infoFile" -ForegroundColor Green