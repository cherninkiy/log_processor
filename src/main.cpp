#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "reader/reader.h"
#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/scheduler.h"
#include "common/stats.h"
#include "common/thread_pool.h"
#include "common/timer.h"

// Слот для статистики одного потока, выровненный по границе кэш-линии (64 байта).
// Предотвращает false sharing: соседние слоты в векторе не делят кэш-линий,
// поэтому независимые записи разных потоков не вызывают cache line bouncing.
struct alignas(64) ThreadSlot {
    LogStats stats;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <logfile> [threads]\n";
        return 1;
    }

    const std::string filename = argv[1];
    const size_t n_threads = (argc >= 3)
        ? static_cast<size_t>(std::stoul(argv[2]))
        : static_cast<size_t>(std::thread::hardware_concurrency());

    Timer timer;

    // Шаг 1: читаем весь файл одним блоком (один системный вызов read()),
    // затем расщепляем на string_view без аллокации std::string на каждую строку.
    // Это устраняет ~7 млн аллокаций при обработке 1 ГБ access.log.
    LogReader reader(filename);
    auto buf = reader.readRawBuffer();
    const auto lines = LogReader::getLineViews(buf);
    const size_t total = lines.size();

    // Шаг 2 (step4): параллельный парсинг через ThreadPool.
    // Потоки не создаются заново — переиспользуются из пула.
    // Каждый поток пишет в свой slot[tid] без мьютекса (shared-nothing).
    std::vector<ThreadSlot> slots(n_threads);
    {
        ThreadPool pool(n_threads);

        // Используем parallel_for_pool — статическое разбиение через пул.
        parallel_for_pool(pool, total, n_threads, [&](size_t start, size_t end, size_t tid) {
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
        // pool.wait_all() вызывается внутри parallel_for_pool.
        // При выходе из блока ~ThreadPool() join'ит потоки.
    }

    // Шаг 3: слияние выполняется главным потоком после завершения всех задач.
    // N-1 операций merge без конкуренции вместо N последовательных lock+merge.
    LogStats global_stats;
    for (auto& slot : slots) {
        global_stats.merge(slot.stats);
    }

    double elapsed = timer.elapsed();
    global_stats.print(filename, elapsed);
    return 0;
}
