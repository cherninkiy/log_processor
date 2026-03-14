#include <gtest/gtest.h>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>

#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/stats.h"

// Вспомогательная функция: обрабатывает диапазон строк [start, end) в локальный LogStats
// (зеркало логики main.cpp из step2)
static LogStats processRange(const std::vector<std::string>& lines,
                              size_t start, size_t end) {
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
    return local;
}

// Вспомогательная функция: разбивает total строк на ranges для n_threads потоков
static std::vector<std::pair<size_t,size_t>> makeRanges(size_t total, size_t n_threads) {
    const size_t chunk = (total + n_threads - 1) / n_threads;
    std::vector<std::pair<size_t,size_t>> ranges;
    for (size_t t = 0; t < n_threads; ++t) {
        size_t start = t * chunk;
        size_t end   = std::min(start + chunk, total);
        if (start >= total) break;
        ranges.emplace_back(start, end);
    }
    return ranges;
}

// ----- Тест 1: параллельная обработка даёт тот же результат, что и последовательная -----

TEST(ThreadingTest, ParallelMatchesSequential) {
    const std::vector<std::string> lines = {
        R"(1.1.1.1 - - [01/Jan/2024:00:00:00 +0000] "GET /a HTTP/1.1" 200 100 "-" "-")",
        R"(2.2.2.2 - - [01/Jan/2024:00:00:01 +0000] "POST /b HTTP/1.1" 404 200 "-" "-")",
        R"(3.3.3.3 - - [01/Jan/2024:00:00:02 +0000] "GET /c HTTP/1.1" 200 300 "-" "-")",
        R"(4.4.4.4 - - [01/Jan/2024:00:00:03 +0000] "GET /a HTTP/1.1" 200 100 "-" "-")",
    };

    // Последовательно
    LogStats seq = processRange(lines, 0, lines.size());

    // Параллельно: 2 потока
    LogStats global;
    std::mutex mtx;
    std::vector<std::thread> threads;
    for (auto [start, end] : makeRanges(lines.size(), 2)) {
        threads.emplace_back([&, start, end]() {
            LogStats local = processRange(lines, start, end);
            std::lock_guard<std::mutex> lock(mtx);
            global.merge(local);
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(global.total_lines,  seq.total_lines);
    EXPECT_EQ(global.parsed_ok,    seq.parsed_ok);
    EXPECT_EQ(global.parse_errors, seq.parse_errors);
    EXPECT_EQ(global.total_bytes,  seq.total_bytes);
    EXPECT_EQ(global.status_codes, seq.status_codes);
    EXPECT_EQ(global.url_counts,   seq.url_counts);
    EXPECT_EQ(global.ip_counts,    seq.ip_counts);
}

// ----- Тест 2: то же, но с числом потоков > числа строк (edge case) -----

TEST(ThreadingTest, MoreThreadsThanLines) {
    const std::vector<std::string> lines = {
        R"(5.5.5.5 - - [01/Jan/2024:00:00:00 +0000] "GET /x HTTP/1.1" 200 50 "-" "-")",
        R"(6.6.6.6 - - [01/Jan/2024:00:00:01 +0000] "GET /y HTTP/1.1" 200 50 "-" "-")",
    };

    LogStats seq = processRange(lines, 0, lines.size());

    // 8 потоков на 2 строки — лишние потоки просто не создаются
    LogStats global;
    std::mutex mtx;
    std::vector<std::thread> threads;
    for (auto [start, end] : makeRanges(lines.size(), 8)) {
        threads.emplace_back([&, start, end]() {
            LogStats local = processRange(lines, start, end);
            std::lock_guard<std::mutex> lock(mtx);
            global.merge(local);
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(global.total_lines, seq.total_lines);
    EXPECT_EQ(global.parsed_ok,   seq.parsed_ok);
    EXPECT_EQ(global.total_bytes, seq.total_bytes);
}

// ----- Тест 3: корректность разбиения диапазонов -----

TEST(ThreadingTest, ChunkRangesAreContiguousAndCoverAll) {
    const size_t total = 17, n_threads = 4;
    auto ranges = makeRanges(total, n_threads);

    // Все диапазоны вместе покрывают [0, total) без пропусков и перекрытий
    ASSERT_EQ(ranges.front().first, 0u);
    ASSERT_EQ(ranges.back().second, total);
    for (size_t i = 1; i < ranges.size(); ++i) {
        EXPECT_EQ(ranges[i].first, ranges[i-1].second);
    }
}

// ----- Тест 4: строки с ошибками парсинга считаются в parse_errors -----

TEST(ThreadingTest, ParseErrorsCountedCorrectly) {
    const std::vector<std::string> lines = {
        R"(1.1.1.1 - - [01/Jan/2024:00:00:00 +0000] "GET /ok HTTP/1.1" 200 100 "-" "-")",
        "INVALID LINE",
        "ANOTHER BAD LINE",
        R"(2.2.2.2 - - [01/Jan/2024:00:00:00 +0000] "GET /ok HTTP/1.1" 200 200 "-" "-")",
    };

    // Параллельно: 2 потока
    LogStats global;
    std::mutex mtx;
    std::vector<std::thread> threads;
    for (auto [start, end] : makeRanges(lines.size(), 2)) {
        threads.emplace_back([&, start, end]() {
            LogStats local = processRange(lines, start, end);
            std::lock_guard<std::mutex> lock(mtx);
            global.merge(local);
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(global.total_lines,  4u);
    EXPECT_EQ(global.parsed_ok,    2u);
    EXPECT_EQ(global.parse_errors, 2u);
}
