#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <numeric>
#include <thread>
#include <vector>

#include "common/thread_pool.h"
#include "common/scheduler.h"

// ----- Тест 1: пул выполняет N задач, результаты корректны -----

TEST(ThreadPoolTest, BasicTaskExecution) {
    ThreadPool pool(4);
    std::atomic<size_t> counter{0};
    constexpr size_t kTasks = 100;

    for (size_t i = 0; i < kTasks; ++i) {
        pool.enqueue_detach([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    pool.wait_all();

    EXPECT_EQ(counter.load(), kTasks);
}

// ----- Тест 2: пул с 1 потоком эквивалентен последовательному -----

TEST(ThreadPoolTest, SingleThreadMatchesSequential) {
    ThreadPool pool(1);
    std::atomic<size_t> counter{0};

    for (size_t i = 0; i < 10; ++i) {
        pool.enqueue_detach([&counter] {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    pool.wait_all();

    EXPECT_EQ(counter.load(), 10u);
}

// ----- Тест 3: повторный enqueue после wait_all корректен -----

TEST(ThreadPoolTest, ReuseAfterWait) {
    ThreadPool pool(2);

    for (int round = 0; round < 3; ++round) {
        std::atomic<size_t> counter{0};
        for (size_t i = 0; i < 5; ++i) {
            pool.enqueue_detach([&counter] {
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
        pool.wait_all();
        EXPECT_EQ(counter.load(), 5u);
    }
}

// ----- Тест 4: enqueue с возвращаемым значением (future) -----

TEST(ThreadPoolTest, EnqueueWithReturnValue) {
    ThreadPool pool(4);
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.enqueue([i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return i * i;
        }));
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

// ----- Тест 5: parallel_for_pool совпадает с sequential -----

#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/stats.h"

// Слот, выровненный по кэш-линии (дублирует определение из main.cpp)
struct alignas(64) TpThreadSlot {
    LogStats stats;
};

static const std::vector<std::string> kTestLines = {
    R"(1.1.1.1 - - [01/Jan/2024:00:00:00 +0000] "GET /a HTTP/1.1" 200 100 "-" "-")",
    R"(2.2.2.2 - - [01/Jan/2024:00:00:01 +0000] "POST /b HTTP/1.1" 404 200 "-" "-")",
    R"(3.3.3.3 - - [01/Jan/2024:00:00:02 +0000] "GET /c HTTP/1.1" 200 300 "-" "-")",
    R"(4.4.4.4 - - [01/Jan/2024:00:00:03 +0000] "GET /a HTTP/1.1" 200 100 "-" "-")",
};

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

static LogStats runWithPool(const std::vector<std::string>& lines, size_t n_threads) {
    std::vector<TpThreadSlot> slots(n_threads);
    {
        ThreadPool pool(n_threads);
        parallel_for_pool(pool, lines.size(), n_threads,
            [&](size_t start, size_t end, size_t tid) {
                LogStats& local = slots[tid].stats;
                // Преобразуем string_view в string для совместимости с parse_log_line
                for (size_t i = start; i < end; ++i) {
                    local.total_lines++;
                    if (auto entry = parse_log_line(lines[i])) {
                        local.parsed_ok++;
                        accumulate(local, *entry);
                    } else {
                        local.parse_errors++;
                    }
                }
            }
        );
    }
    LogStats result;
    for (auto& slot : slots) result.merge(slot.stats);
    return result;
}

TEST(ThreadPoolTest, ParallelForPoolMatchesSequential) {
    LogStats seq = runSequential(kTestLines);
    LogStats par = runWithPool(kTestLines, 4);

    EXPECT_EQ(par.total_lines,  seq.total_lines);
    EXPECT_EQ(par.parsed_ok,    seq.parsed_ok);
    EXPECT_EQ(par.parse_errors, seq.parse_errors);
    EXPECT_EQ(par.total_bytes,  seq.total_bytes);
    EXPECT_EQ(par.status_codes, seq.status_codes);
    EXPECT_EQ(par.ip_counts,    seq.ip_counts);
    EXPECT_EQ(par.url_counts,   seq.url_counts);
}

// ----- Тест 6: parallel_for_pool с 1 потоком = sequential -----

TEST(ThreadPoolTest, SingleThreadPoolMatchesSequential) {
    LogStats seq = runSequential(kTestLines);
    LogStats par = runWithPool(kTestLines, 1);

    EXPECT_EQ(par.total_lines,  seq.total_lines);
    EXPECT_EQ(par.parsed_ok,    seq.parsed_ok);
    EXPECT_EQ(par.parse_errors, seq.parse_errors);
    EXPECT_EQ(par.total_bytes,  seq.total_bytes);
}

// ----- Тест 7: parallel_for_pool с большим числом потоков корректна -----

TEST(ThreadPoolTest, ManyThreadsCorrectness) {
    LogStats seq = runSequential(kTestLines);
    LogStats par = runWithPool(kTestLines, 8);

    EXPECT_EQ(par.total_lines,  seq.total_lines);
    EXPECT_EQ(par.parsed_ok,    seq.parsed_ok);
    EXPECT_EQ(par.parse_errors, seq.parse_errors);
}