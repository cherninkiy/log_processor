#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <vector>

#include "reader/reader.h"
#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/scheduler.h"
#include "common/stats.h"
#include "common/timer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <logfile> [threads]\n";
        return 1;
    }

    const std::string filename = argv[1];
    // Число потоков: необязательный второй аргумент или hardware_concurrency
    const size_t n_threads = (argc >= 3)
        ? static_cast<size_t>(std::stoul(argv[2]))
        : static_cast<size_t>(std::thread::hardware_concurrency());

    Timer timer;

    // Шаг 1: читаем весь файл в память (reader не менялся)
    LogReader reader(filename);
    const auto lines = reader.readAllLines();
    const size_t total = lines.size();

    // Шаг 2: параллельный парсинг и агрегация — каждый поток обрабатывает
    // свой диапазон строк в локальный LogStats, затем делает один merge
    LogStats global_stats;
    std::mutex stats_mutex;

    parallel_for(total, n_threads, [&](size_t start, size_t end) {
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
        // Один merge на весь чанк — мьютекс держится минимальное время
        std::lock_guard<std::mutex> lock(stats_mutex);
        global_stats.merge(local);
    });

    double elapsed = timer.elapsed();
    global_stats.print(filename, elapsed);
    return 0;
}
