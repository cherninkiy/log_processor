#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>

#include "reader/reader.h"
#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/stats.h"

// -----------------------------------------------------------------------------
// Глобальные данные для бенчмарков (загружаются один раз)
// -----------------------------------------------------------------------------
static std::vector<std::string> testLines;        // строки из тестового файла
static std::vector<LogEntry> testEntries;         // распарсенные записи (для бенчмарков агрегации)
static bool dataLoaded = false;

// Загружает строки из файла (путь жёстко задан или берётся из переменной окружения)
static void LoadTestData() {
    if (dataLoaded) return;

    const char* filename = std::getenv("TEST_LOG_FILE");
    if (!filename) filename = "data/sample.log";   // запасной вариант

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open test file " << filename << "\n";
        std::exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        testLines.push_back(line);
        if (auto entry = parse_log_line(line)) {
            testEntries.push_back(*entry);
        }
    }
    dataLoaded = true;
    std::cout << "Loaded " << testLines.size() << " lines, "
              << testEntries.size() << " parsed entries from " << filename << "\n";
}

// -----------------------------------------------------------------------------
// Микро-бенчмарк: парсинг одной строки
// -----------------------------------------------------------------------------
static void BM_ParseLine(benchmark::State& state) {
    LoadTestData();
    size_t idx = 0;
    for (auto _ : state) {
        auto result = parse_log_line(testLines[idx]);
        benchmark::DoNotOptimize(result);
        if (++idx >= testLines.size()) idx = 0;
    }
}
BENCHMARK(BM_ParseLine);

// -----------------------------------------------------------------------------
// Микро-бенчмарк: агрегация одной записи (accumulate)
// -----------------------------------------------------------------------------
static void BM_Accumulate(benchmark::State& state) {
    LoadTestData();
    if (testEntries.empty()) {
        state.SkipWithError("No parsed entries available");
        return;
    }
    LogStats stats;
    size_t idx = 0;
    for (auto _ : state) {
        accumulate(stats, testEntries[idx]);
        benchmark::DoNotOptimize(stats);
        if (++idx >= testEntries.size()) idx = 0;
    }
}
BENCHMARK(BM_Accumulate);

// -----------------------------------------------------------------------------
// Бенчмарк: полная обработка файла (чтение, парсинг, агрегация)
// Использует небольшой файл (sample.log) и измеряет реальное время.
// -----------------------------------------------------------------------------
static void BM_ProcessFile(benchmark::State& state) {
    const char* filename = std::getenv("TEST_LOG_FILE");
    if (!filename) filename = "data/sample.log";

    for (auto _ : state) {
        // Полностью воспроизводим логику main.cpp (для step1)
        LogReader reader(filename);
        auto lines = reader.readAllLines();          // читает весь файл в память

        LogStats stats;
        for (const auto& line : lines) {
            stats.total_lines++;
            if (auto entry = parse_log_line(line)) {
                stats.parsed_ok++;
                accumulate(stats, *entry);
            } else {
                stats.parse_errors++;
            }
        }
        benchmark::DoNotOptimize(stats);
    }
}
// Загружает весь файл в память на каждой итерации — одна итерация достаточна для файлов >= 100 МБ
BENCHMARK(BM_ProcessFile)->Iterations(1)->UseRealTime();

// -----------------------------------------------------------------------------
// Дополнительно: бенчмарк только чтения файла (без парсинга)
// -----------------------------------------------------------------------------
static void BM_ReadFile(benchmark::State& state) {
    const char* filename = std::getenv("TEST_LOG_FILE");
    if (!filename) filename = "data/sample.log";

    for (auto _ : state) {
        LogReader reader(filename);
        auto lines = reader.readAllLines();
        benchmark::DoNotOptimize(lines.size());
    }
}
// Загружает весь файл в память — одна итерация достаточна для файлов >= 100 МБ
BENCHMARK(BM_ReadFile)->Iterations(1)->UseRealTime();

BENCHMARK_MAIN();