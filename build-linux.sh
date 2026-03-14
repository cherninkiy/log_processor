#!/bin/bash
# build-linux.sh - Linux/WSL build script unified with Windows version

set -e  # exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default values
CONFIG=""
COMPILER="gcc"  # gcc or clang (for Linux, both work)
BUILD_TESTS="ON"
BUILD_BENCHMARKS="ON"
PARALLEL_JOBS=0
CLEAN_BUILD=false
SANITIZE=false
TSAN=false
UBSAN=false
ENV_FILE=".env"
HELP=false

# Help function
show_help() {
    echo -e "${BLUE}Usage:${NC} $0 [BUILD_TYPE] [OPTIONS]"
    echo
    echo -e "${BLUE}Build Types:${NC}"
    echo "  Debug        - Debug build (default from .env)"
    echo "  Release      - Release build with optimizations"
    echo "  RelWithDebInfo - Release with debug info"
    echo "  MinSizeRel   - Release optimized for size"
    echo
    echo -e "${BLUE}Compiler:${NC}"
    echo "  gcc          - Use GCC (default)"
    echo "  clang        - Use Clang"
    echo "  (can be set in .env with DEFAULT_COMPILER)"
    echo
    echo -e "${BLUE}Options:${NC}"
    echo "  -h, --help     Show this help"
    echo "  -c, --clean    Clean build directory before building"
    echo "  -j N           Use N parallel jobs (default: number of CPUs)"
    echo "  --no-tests     Disable building tests"
    echo "  --no-benchmarks Disable building benchmarks"
    echo "  --sanitize     Enable AddressSanitizer (Debug only)"
    echo "  --tsan         Enable ThreadSanitizer (Debug only)"
    echo "  --ubsan        Enable UndefinedBehaviorSanitizer (Debug only)"
    echo "  --env FILE     Use specific env file (default: .env)"
    echo
    echo -e "${BLUE}Examples:${NC}"
    echo "  $0                     # Build with settings from .env"
    echo "  $0 Release             # Release build"
    echo "  $0 Debug --sanitize    # Debug with AddressSanitizer"
    echo "  $0 Release -j 8        # Release with 8 jobs"
    exit 0
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        Debug|Release|RelWithDebInfo|MinSizeRel)
            CONFIG="$1"
            shift
            ;;
        gcc|clang)
            COMPILER="$1"
            shift
            ;;
        -h|--help)
            HELP=true
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -j)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --no-benchmarks)
            BUILD_BENCHMARKS="OFF"
            shift
            ;;
        --sanitize)
            SANITIZE=true
            shift
            ;;
        --tsan)
            TSAN=true
            shift
            ;;
        --ubsan)
            UBSAN=true
            shift
            ;;
        --env)
            ENV_FILE="$2"
            shift 2
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Show help if requested
if [ "$HELP" = true ]; then
    show_help
fi

# Load environment from .env file
load_env_file() {
    local file="$1"
    
    if [ ! -f "$file" ]; then
        echo -e "${YELLOW}⚠️  File $file not found, using defaults${NC}"
        return
    fi
    
    echo -e "${CYAN}📁 Loading settings from $file${NC}"
    
    while IFS= read -r line || [ -n "$line" ]; do
        # Remove carriage return and trim whitespace
        line=$(echo "$line" | tr -d '\r' | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')
        
        # Skip comments and empty lines
        if [[ -z "$line" ]] || [[ "$line" =~ ^# ]]; then
            continue
        fi
        
        # Check if line is a valid variable assignment
        if [[ "$line" =~ ^[[:alpha:]_][[:alnum:]_]*=.* ]]; then
            key="${line%%=*}"
            value="${line#*=}"
            echo -e "  ${GRAY}✓ $key = $value${NC}"
            
            # Special handling for CMAKE_BUILD_PARALLEL_LEVEL (do not export, store separately)
            if [ "$key" = "CMAKE_BUILD_PARALLEL_LEVEL" ]; then
                export CMAKE_BUILD_PARALLEL_LEVEL_FROM_ENV="$value"
                continue
            fi
            
            # Export all other variables
            export "$line"
        else
            echo -e "  ${YELLOW}⚠️  Skipping invalid line: $line${NC}"
        fi
    done < "$file"
}

# Load configuration
load_env_file "$ENV_FILE"

# Apply settings with priority: command line > .env > defaults

# Build type
if [ -z "$CONFIG" ]; then
    CONFIG="${BUILD_TYPE:-Debug}"
fi

# Compiler (for Linux, we'll set CC/CXX environment variables)
if [ "$COMPILER" = "clang" ]; then
    export CC=clang
    export CXX=clang++
    COMPILER_NAME="Clang"
else
    export CC=gcc
    export CXX=g++
    COMPILER_NAME="GCC"
fi

# Tests
if [ "$BUILD_TESTS" != "OFF" ]; then
    BUILD_TESTS="${BUILD_TESTS:-ON}"
fi

# Benchmarks
if [ "$BUILD_BENCHMARKS" != "OFF" ]; then
    BUILD_BENCHMARKS="${BUILD_BENCHMARKS:-ON}"
fi

# Parallel jobs
if [ "$PARALLEL_JOBS" -eq 0 ]; then
    # Try to use value from .env (stored in CMAKE_BUILD_PARALLEL_LEVEL_FROM_ENV)
    if [ -n "$CMAKE_BUILD_PARALLEL_LEVEL_FROM_ENV" ] && [ "$CMAKE_BUILD_PARALLEL_LEVEL_FROM_ENV" -gt 0 ]; then
        PARALLEL_JOBS="$CMAKE_BUILD_PARALLEL_LEVEL_FROM_ENV"
    else
        PARALLEL_JOBS=$(nproc)
    fi
fi

# Clean from env if not overridden
if [ "$CLEAN_BUILD" = false ] && [ "${CLEAN_BEFORE_BUILD:-OFF}" = "ON" ]; then
    CLEAN_BUILD=true
fi

# Sanitizers from env if not overridden
if [ "$SANITIZE" = false ] && [ "${ENABLE_ASAN:-OFF}" = "ON" ]; then
    SANITIZE=true
fi
if [ "$TSAN" = false ] && [ "${ENABLE_TSAN:-OFF}" = "ON" ]; then
    TSAN=true
fi
if [ "$UBSAN" = false ] && [ "${ENABLE_UBSAN:-OFF}" = "ON" ]; then
    UBSAN=true
fi

# Check if running in WSL
if grep -q Microsoft /proc/version 2>/dev/null; then
    echo -e "${YELLOW}⚠️  Running in WSL environment${NC}"
    WSL=1
else
    WSL=0
fi

# Auto-disable tests and benchmarks when sanitizers are enabled
if [ "$SANITIZE" = true ] || [ "$TSAN" = true ] || [ "$UBSAN" = true ]; then
    if [ "$BUILD_TESTS" = "ON" ]; then
        echo -e "${YELLOW}⚠️  Sanitizers enabled, disabling tests automatically (use --no-tests to keep off, or edit script to force)${NC}"
        BUILD_TESTS="OFF"
    fi
    if [ "$BUILD_BENCHMARKS" = "ON" ]; then
        echo -e "${YELLOW}⚠️  Sanitizers enabled, disabling benchmarks automatically${NC}"
        BUILD_BENCHMARKS="OFF"
    fi
fi

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}🔧 Linux Build Script${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "  • Build type:     ${CYAN}$CONFIG${NC}"
echo -e "  • Compiler:       ${CYAN}$COMPILER_NAME${NC}"
echo -e "  • Tests:          ${CYAN}$BUILD_TESTS${NC}"
echo -e "  • Benchmarks:     ${CYAN}$BUILD_BENCHMARKS${NC}"
echo -e "  • Parallel jobs:  ${CYAN}$PARALLEL_JOBS${NC}"
if [ "$SANITIZE" = true ]; then
    echo -e "  • AddressSanitizer: ${CYAN}ON${NC}"
fi
if [ "$TSAN" = true ]; then
    echo -e "  • ThreadSanitizer: ${CYAN}ON${NC}"
fi
if [ "$UBSAN" = true ]; then
    echo -e "  • UBSan:           ${CYAN}ON${NC}"
fi
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

# Check for required tools
echo -e "\n${BLUE}🔍 Checking required tools...${NC}"
MISSING_TOOLS=0

for tool in cmake make; do
    if ! command -v $tool &> /dev/null; then
        echo -e "  ${RED}❌ $tool not found${NC}"
        MISSING_TOOLS=1
    else
        echo -e "  ${GREEN}✅ $tool found: $(command -v $tool)${NC}"
    fi
done

# Check compiler
if ! command -v ${CC} &> /dev/null; then
    echo -e "  ${RED}❌ $CC not found${NC}"
    MISSING_TOOLS=1
else
    echo -e "  ${GREEN}✅ $CC found: $(command -v $CC)${NC}"
fi

if ! command -v ${CXX} &> /dev/null; then
    echo -e "  ${RED}❌ $CXX not found${NC}"
    MISSING_TOOLS=1
else
    echo -e "  ${GREEN}✅ $CXX found: $(command -v $CXX)${NC}"
fi

if [ $MISSING_TOOLS -eq 1 ]; then
    echo -e "\n${RED}❌ Missing required tools. Please install:${NC}"
    echo "  sudo apt update"
    echo "  sudo apt install build-essential cmake"
    exit 1
fi

# Show compiler version
echo -e "\n${BLUE}🔧 Compiler info:${NC}"
${CXX} --version | head -n 1

# Check for optional tools
echo -e "\n${BLUE}📦 Optional tools:${NC}"
if command -v ccache &> /dev/null; then
    echo -e "  ${GREEN}✅ ccache found - will speed up rebuilds${NC}"
    export CCACHE_DIR="$HOME/.ccache"
    export USE_CCACHE=1
else
    echo -e "  ${YELLOW}⚠️  ccache not found (optional, for faster rebuilds)${NC}"
    echo "     Install: sudo apt install ccache"
fi

if command -v ninja &> /dev/null; then
    echo -e "  ${GREEN}✅ ninja found - can be used as alternative generator${NC}"
    NINJA_FOUND=1
else
    NINJA_FOUND=0
fi

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "\n${YELLOW}🧹 Cleaning build directory...${NC}"
    rm -rf build/linux
fi

# Create build directory
mkdir -p build/linux

# Save build info
BUILD_INFO_FILE="build/linux/build_info.txt"
{
    echo "Build type: $CONFIG"
    echo "Compiler: $COMPILER_NAME"
    echo "Date: $(date)"
    echo "Compiler version: $(${CXX} --version | head -n 1)"
    echo "CMake: $(cmake --version | head -n 1)"
} > "$BUILD_INFO_FILE"

# CMake configuration
echo -e "\n${BLUE}⚙️  Running CMake configuration...${NC}"

# Build CMake arguments
CMAKE_ARGS=(
    "-S" "."
    "-B" "build/linux"
    "-DCMAKE_BUILD_TYPE=$CONFIG"
    "-DBUILD_TESTS=$BUILD_TESTS"
    "-DBUILD_BENCHMARKS=$BUILD_BENCHMARKS"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

# Add sanitizers if requested and Debug build
if [ "$CONFIG" = "Debug" ]; then
    if [ "$SANITIZE" = true ]; then
        CMAKE_ARGS+=("-DUSE_ASAN=ON")
        echo -e "  ${YELLOW}AddressSanitizer enabled${NC}"
    fi
    if [ "$TSAN" = true ]; then
        CMAKE_ARGS+=("-DUSE_TSAN=ON")
        echo -e "  ${YELLOW}ThreadSanitizer enabled${NC}"
    fi
    if [ "$UBSAN" = true ]; then
        CMAKE_ARGS+=("-DUSE_UBSAN=ON")
        echo -e "  ${YELLOW}UBSan enabled${NC}"
    fi
fi

# Use Ninja if available and not Debug (for consistency with Windows)
if [ $NINJA_FOUND -eq 1 ] && [ "$CONFIG" != "Debug" ]; then
    CMAKE_ARGS+=("-G" "Ninja")
    echo -e "  ${GREEN}Using Ninja generator${NC}"
else
    CMAKE_ARGS+=("-G" "Unix Makefiles")
fi

# Run CMake
cmake "${CMAKE_ARGS[@]}"

if [ $? -ne 0 ]; then
    echo -e "\n${RED}❌ CMake configuration failed${NC}"
    exit 1
fi

# Build
echo -e "\n${BLUE}🔨 Building project...${NC}"
echo -e "  Using ${CYAN}$PARALLEL_JOBS${NC} parallel jobs\n"

# Time the build
BUILD_START=$(date +%s)

if [ $NINJA_FOUND -eq 1 ] && [ "$CONFIG" != "Debug" ]; then
    if [ "$PARALLEL_JOBS" -gt 0 ]; then
        ninja -C build/linux -j "$PARALLEL_JOBS"
    else
        ninja -C build/linux
    fi
else
    if [ "$PARALLEL_JOBS" -gt 0 ]; then
        make -C build/linux -j "$PARALLEL_JOBS"
    else
        make -C build/linux
    fi
fi

BUILD_END=$(date +%s)
BUILD_TIME=$((BUILD_END - BUILD_START))

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}✅ Build completed successfully!${NC}"
    echo -e "   Build time: ${CYAN}${BUILD_TIME}s${NC}"
    echo -e "   Executable: ${CYAN}build/linux/src/log_processor${NC}"
    
    # Copy compile_commands.json for IDE support
    if [ -f build/linux/compile_commands.json ]; then
        cp build/linux/compile_commands.json .
        echo -e "   📝 ${GREEN}compile_commands.json copied for code completion${NC}"
    fi
    
    # Show executable size
    if [ -f build/linux/src/log_processor ]; then
        SIZE=$(ls -lh build/linux/src/log_processor | awk '{print $5}')
        echo -e "   📦 Executable size: ${CYAN}$SIZE${NC}"
    fi
    
    echo -e "\n${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}Next steps:${NC}"
    echo -e "  Run program:    ${CYAN}./build/linux/src/log_processor <logfile>${NC}"
    echo -e "  Run tests:      ${CYAN}cd build/linux && ctest --output-on-failure${NC}"
    echo -e "  Run benchmarks: ${CYAN}./build/linux/benchmarks/log_benchmark${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
else
    echo -e "\n${RED}❌ Build failed${NC}"
    exit 1
fi