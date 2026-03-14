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

# Путь к тестовому файлу (можно переопределить через переменную окружения)
TEST_FILE=${TEST_LOG_FILE:-data/sample.log}
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
    echo "Test file: $TEST_FILE"
    echo "--- System ---"
    uname -a
    echo "--- Compiler ---"
    ${CXX:-g++} --version | head -n 1
    echo "--- Git commit ---"
    git rev-parse HEAD
} > "$OUTDIR/${TIMESTAMP}_info.txt"

# Запускаем бенчмарки с сохранением в JSON
echo "🚀 Running benchmarks..."
export TEST_LOG_FILE="$TEST_FILE"
"$BENCHMARK_EXE" \
    --benchmark_format=json \
    --benchmark_out="$OUTDIR/${TIMESTAMP}_benchmark.json"

echo "✅ Results saved to $OUTDIR/${TIMESTAMP}_benchmark.json"
echo "   System info saved to $OUTDIR/${TIMESTAMP}_info.txt"