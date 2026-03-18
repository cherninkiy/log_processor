#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "reader/reader.h"
#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/scheduler.h"
#include "common/stats.h"
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

    // Шаг 1: читаем весь файл в память (reader не менялся)
    LogReader reader(filename);
    const auto lines = reader.readAllLines();
    const size_t total = lines.size();

    // Шаг 2: параллельный парсинг и агрегация без мьютекса.
    // Каждый поток обращается только к своему слоту — нет разделяемого состояния
    // в горячем пути, нет необходимости в lock_guard или std::atomic.
    std::vector<ThreadSlot> slots(n_threads);

    parallel_for_indexed(total, n_threads, [&](size_t start, size_t end, size_t tid) {
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
        // В step2 здесь был: std::lock_guard<std::mutex> lock(stats_mutex); global_stats.merge(local);
        // В step3 — никакой синхронизации: каждый слот принадлежит ровно одному потоку.
    });

    // Шаг 3: слияние выполняется главным потоком после завершения всех потоков.
    // N-1 операций merge без конкуренции вместо N последовательных lock+merge.
    LogStats global_stats;
    for (auto& slot : slots) {
        global_stats.merge(slot.stats);
    }

    double elapsed = timer.elapsed();
    global_stats.print(filename, elapsed);
    return 0;
}
