#include <benchmark/benchmark.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <mutex>
#include <thread>

#include "reader/reader.h"
#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/scheduler.h"
#include "common/stats.h"
#ifdef HAVE_TBB
#  include "common/concurrent_stats.h"
#endif

// Путь к тестовому файлу — задаётся через --test_file=<path> (см. main),
// при отсутствии аргумента берётся из переменной окружения TEST_LOG_FILE,
// иначе — data/sample.log.
static std::string g_test_file;

// -----------------------------------------------------------------------------
// Глобальные данные для бенчмарков (загружаются один раз)
// -----------------------------------------------------------------------------
static std::vector<std::string> testLines;        // строки из тестового файла
static std::vector<LogEntry> testEntries;         // распарсенные записи (для бенчмарков агрегации)
static bool dataLoaded = false;

// Загружает строки из файла (путь задан в g_test_file к моменту первого вызова)
static void LoadTestData() {
    if (dataLoaded) return;

    std::ifstream file(g_test_file);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open test file " << g_test_file << "\n";
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
              << testEntries.size() << " parsed entries from " << g_test_file << "\n";
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
    for (auto _ : state) {
        // Полностью воспроизводим логику main.cpp (для step1)
        LogReader reader(g_test_file);
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
    for (auto _ : state) {
        LogReader reader(g_test_file);
        auto lines = reader.readAllLines();
        benchmark::DoNotOptimize(lines.size());
    }
}
// Загружает весь файл в память — одна итерация достаточна для файлов >= 100 МБ
BENCHMARK(BM_ReadFile)->Iterations(1)->UseRealTime();

// -----------------------------------------------------------------------------
// Бенчмарк: многопоточная обработка файла (step2 — N потоков, mutex merge)
// Использует hardware_concurrency потоков
// -----------------------------------------------------------------------------
static void BM_ProcessFileThreaded(benchmark::State& state) {
    const size_t n_threads = static_cast<size_t>(std::thread::hardware_concurrency());

    for (auto _ : state) {
        LogReader reader(g_test_file);
        const auto lines = reader.readAllLines();

        LogStats   global_stats;
        std::mutex stats_mutex;

        parallel_for(lines.size(), n_threads, [&](size_t start, size_t end) {
            LogStats local;
            for (size_t i = start; i < end; ++i) {
                local.total_lines++;
                if (auto entry = parse_log_line(lines[i])) {
                    local.parsed_ok++;
                    accumulate(local, *entry);
                } else {
                    local.parse_errors++;
                }
            }
            std::lock_guard<std::mutex> lock(stats_mutex);
            global_stats.merge(local);
        });

        benchmark::DoNotOptimize(global_stats);
    }
}
BENCHMARK(BM_ProcessFileThreaded)->Iterations(1)->UseRealTime();

// -----------------------------------------------------------------------------
// Бенчмарк: многопоточная обработка файла (step3 — thread-local слоты, нет mutex)
// Каждый поток пишет в свой alignas(64) слот, слияние после join в главном потоке.
// -----------------------------------------------------------------------------
static void BM_ProcessFileAtomicLocal(benchmark::State& state) {
    const size_t n_threads = static_cast<size_t>(std::thread::hardware_concurrency());

    struct alignas(64) ThreadSlot { LogStats stats; };

    for (auto _ : state) {
        LogReader reader(g_test_file);
        const auto lines = reader.readAllLines();

        std::vector<ThreadSlot> slots(n_threads);

        parallel_for_indexed(lines.size(), n_threads, [&](size_t start, size_t end, size_t tid) {
            LogStats& local = slots[tid].stats;
            for (size_t i = start; i < end; ++i) {
                local.total_lines++;
                if (auto entry = parse_log_line(lines[i])) {
                    local.parsed_ok++;
                    accumulate(local, *entry);
                } else {
                    local.parse_errors++;
                }
            }
        });

        LogStats global_stats;
        for (auto& slot : slots) global_stats.merge(slot.stats);

        benchmark::DoNotOptimize(global_stats);
    }
}
BENCHMARK(BM_ProcessFileAtomicLocal)->Iterations(1)->UseRealTime();

// Кастомный main: извлекает --test_file=<path> из argv до передачи оставшихся
// -----------------------------------------------------------------------------
// Бенчмарк: TBB concurrent_hash_map (без thread-local, без mutex в hot path)
// Каждый поток пишет напрямую в единственную ConcurrentStats через accessor
// (RAII-lock на один бакет). Нет финального слияния — но есть fine-grained
// locking при каждой вставке/обновлении ключа.
// -----------------------------------------------------------------------------
static void BM_ProcessFileConcurrentTBB(benchmark::State& state) {
    const size_t n_threads = static_cast<size_t>(std::thread::hardware_concurrency());

    for (auto _ : state) {
        LogReader reader(g_test_file);
        const auto lines = reader.readAllLines();

        ConcurrentStats cs;

        parallel_for(lines.size(), n_threads, [&](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                if (auto entry = parse_log_line(lines[i])) {
                    cs.add_entry(*entry, entry->bytes);
                } else {
                    cs.add_error();
                }
            }
        });

        auto result = cs.to_log_stats();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ProcessFileConcurrentTBB)->Iterations(1)->UseRealTime();
// Кастомный main: извлекает --test_file=<path> из argv до передачи оставшихся
// аргументов в Google Benchmark. Это позволяет запускать бенчмарк как:
//   log_benchmark --test_file=data/access.log --benchmark_filter=BM_ProcessFile
int main(int argc, char** argv) {
    // Разбираем --test_file= вручную и убираем его из argv
    std::vector<char*> filtered;
    filtered.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.starts_with("--test_file=")) {
            g_test_file = std::string(arg.substr(12));
        } else {
            filtered.push_back(argv[i]);
        }
    }
    // Резервный вариант: переменная окружения или дефолтный путь
    if (g_test_file.empty()) {
        if (const char* env = std::getenv("TEST_LOG_FILE"))
            g_test_file = env;
        else
            g_test_file = "data/sample.log";
    }

    int new_argc = static_cast<int>(filtered.size());
    ::benchmark::Initialize(&new_argc, filtered.data());
    if (::benchmark::ReportUnrecognizedArguments(new_argc, filtered.data())) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}