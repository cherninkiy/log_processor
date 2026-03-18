#!/bin/bash
set -e

# Определяем корень проекта
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"  # переходим в корень, чтобы все относительные пути работали

# Определяем ветку Git
BRANCH=$(git symbolic-ref --short HEAD 2>/dev/null || echo "unknown")
TIMESTAMP=$(date +%Y-%m-%d_%H-%M-%S)
OUTDIR="results/$BRANCH"
mkdir -p "$OUTDIR"

# Разбор аргументов командной строки
TEST_FILE=""
BUILD_TYPE="release"
for arg in "$@"; do
    case "$arg" in
        --test-file=*) TEST_FILE="${arg#*=}" ;;
        --build-type=*) BUILD_TYPE="${arg#*=}" ;;
        -*) echo "Unknown option: $arg"; exit 1 ;;
        *)  [ -z "$TEST_FILE" ] && TEST_FILE="$arg" ;;
    esac
done

# Запасной вариант: переменная окружения или файл по умолчанию
TEST_FILE="${TEST_FILE:-${TEST_LOG_FILE:-data/access.log}}"

OS_TAG=$(uname -s | tr '[:upper:]' '[:lower:]')
FILENAME="${TIMESTAMP}_${OS_TAG}_${BUILD_TYPE}"
if [ ! -f "$TEST_FILE" ]; then
    echo "❌ Test file $TEST_FILE not found. Run generate_logs.py first."
    exit 1
fi

# Проверяем, собран ли проект
BENCHMARK_EXE="./build/linux/benchmarks/log_benchmark"
if [ ! -f "$BENCHMARK_EXE" ]; then
    echo "⚠️  Benchmark executable not found. Building..."
    ./build-linux.sh Release --no-tests --no-benchmarks
fi

# Сохраняем информацию о системе
{
    echo "Branch: $BRANCH"
    echo "Timestamp: $TIMESTAMP"
    echo "OS: $OS_TAG"
    echo "Build type: $BUILD_TYPE"
    echo "Test file: $TEST_FILE"
    echo "--- System ---"
    uname -a
    echo "--- Compiler ---"
    ${CXX:-g++} --version | head -n 1
    echo "--- Git commit ---"
    git rev-parse HEAD
} > "$OUTDIR/${FILENAME}_info.txt"

# Запускаем бенчмарки с сохранением в JSON
echo "🚀 Running benchmarks..."
"$BENCHMARK_EXE" \
    --test_file="$TEST_FILE" \
    --benchmark_format=json \
    --benchmark_out="$OUTDIR/${FILENAME}_benchmark.json"

echo "✅ Results saved to $OUTDIR/${FILENAME}_benchmark.json"
echo "   System info saved to $OUTDIR/${FILENAME}_info.txt"