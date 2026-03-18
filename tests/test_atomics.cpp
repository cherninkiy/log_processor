#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>

#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/stats.h"
#include "common/scheduler.h"

// Слот, выровненный по кэш-линии — дублирует определение из main.cpp
// для тестирования свойств самой структуры.
struct alignas(64) ThreadSlot {
    LogStats stats;
};

// Тестовые строки лога
static const std::vector<std::string> kLines = {
    R"(1.1.1.1 - - [01/Jan/2024:00:00:00 +0000] "GET /a HTTP/1.1" 200 100 "-" "-")",
    R"(2.2.2.2 - - [01/Jan/2024:00:00:01 +0000] "POST /b HTTP/1.1" 404 200 "-" "-")",
    R"(3.3.3.3 - - [01/Jan/2024:00:00:02 +0000] "GET /c HTTP/1.1" 200 300 "-" "-")",
    R"(4.4.4.4 - - [01/Jan/2024:00:00:03 +0000] "GET /a HTTP/1.1" 200 100 "-" "-")",
};

// Вспомогательная функция: step3-логика с thread-local слотами
static LogStats runParallel(const std::vector<std::string>& lines, size_t n_threads) {
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
    LogStats result;
    for (auto& slot : slots) result.merge(slot.stats);
    return result;
}

// Вспомогательная функция: последовательная обработка
static LogStats runSequential(const std::vector<std::string>& lines) {
    LogStats result;
    for (const auto& line : lines) {
        result.total_lines++;
        if (auto entry = parse_log_line(line)) {
            result.parsed_ok++;
            accumulate(result, *entry);
        } else {
            result.parse_errors++;
        }
    }
    return result;
}

// ----- Тест 1: thread-local результат совпадает с последовательным -----

TEST(ThreadLocalTest, ResultMatchesSequential) {
    LogStats seq = runSequential(kLines);
    LogStats par = runParallel(kLines, 4);

    EXPECT_EQ(par.total_lines,  seq.total_lines);
    EXPECT_EQ(par.parsed_ok,    seq.parsed_ok);
    EXPECT_EQ(par.parse_errors, seq.parse_errors);
    EXPECT_EQ(par.total_bytes,  seq.total_bytes);
    EXPECT_EQ(par.status_codes, seq.status_codes);
    EXPECT_EQ(par.ip_counts,    seq.ip_counts);
    EXPECT_EQ(par.url_counts,   seq.url_counts);
}

// ----- Тест 2: число потоков больше числа строк — лишние слоты нулевые -----

TEST(ThreadLocalTest, MoreThreadsThanLines) {
    const std::vector<std::string> lines = {
        R"(1.1.1.1 - - [01/Jan/2024:00:00:00 +0000] "GET /x HTTP/1.1" 200 50 "-" "-")",
        R"(bad line)",
    };
    // 8 потоков на 2 строки: потоки 2..7 не запускаются, slots[2..7] остаются нулевыми
    LogStats par = runParallel(lines, 8);
    LogStats seq = runSequential(lines);

    EXPECT_EQ(par.total_lines,  seq.total_lines);
    EXPECT_EQ(par.parsed_ok,    seq.parsed_ok);
    EXPECT_EQ(par.parse_errors, seq.parse_errors);
    EXPECT_EQ(par.total_bytes,  seq.total_bytes);
}

// ----- Тест 3: ThreadSlot выровнен по 64 байтам (sizeof кратен 64) -----

TEST(ThreadLocalTest, AlignedSlotSize) {
    // alignas(64) гарантирует выравнивание и кратность sizeof
    EXPECT_EQ(alignof(ThreadSlot), 64u);
    EXPECT_EQ(sizeof(ThreadSlot) % 64, 0u);

    // Соседние элементы вектора не делят кэш-линий:
    // &slots[i+1] - &slots[i] == sizeof(ThreadSlot) == N * 64
    std::vector<ThreadSlot> slots(4);
    const auto diff = reinterpret_cast<const char*>(&slots[1])
                    - reinterpret_cast<const char*>(&slots[0]);
    EXPECT_EQ(diff % 64, 0);
}

// ----- Тест 4: порядок слияния слотов не влияет на результат -----

TEST(ThreadLocalTest, MergeOrderIndependent) {
    const size_t n_threads = 4;
    std::vector<ThreadSlot> slots(n_threads);

    parallel_for_indexed(kLines.size(), n_threads, [&](size_t start, size_t end, size_t tid) {
        LogStats& local = slots[tid].stats;
        for (size_t i = start; i < end; ++i) {
            local.total_lines++;
            if (auto entry = parse_log_line(kLines[i])) {
                local.parsed_ok++;
                accumulate(local, *entry);
            } else {
                local.parse_errors++;
            }
        }
    });

    // Прямой порядок
    LogStats forward;
    for (size_t i = 0; i < n_threads; ++i) forward.merge(slots[i].stats);

    // Обратный порядок
    LogStats reverse;
    for (size_t i = n_threads; i-- > 0; ) reverse.merge(slots[i].stats);

    EXPECT_EQ(forward.total_lines,  reverse.total_lines);
    EXPECT_EQ(forward.parsed_ok,    reverse.parsed_ok);
    EXPECT_EQ(forward.total_bytes,  reverse.total_bytes);
    EXPECT_EQ(forward.status_codes, reverse.status_codes);
    EXPECT_EQ(forward.ip_counts,    reverse.ip_counts);
    EXPECT_EQ(forward.url_counts,   reverse.url_counts);
}
