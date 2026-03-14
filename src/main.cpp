#include <iostream>
#include <string>

#include "reader/reader.h"
#include "parser/parser.h"
#include "analyzer/analyzer.h"
#include "common/timer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <logfile>\n";
        return 1;
    }

    std::string filename = argv[1];

    Timer timer;  // начало замера

    // Чтение всего файла (для скелета)
    LogReader reader(filename);
    auto lines = reader.readAllLines();

    LogStats stats;

    for (const auto& line : lines) {
        stats.total_lines++;

        auto entry = parse_log_line(line);
        if (entry) {
            stats.parsed_ok++;
            accumulate(stats, *entry);
        } else {
            stats.parse_errors++;
        }
    }

    double elapsed = timer.elapsed();
    stats.print(filename, elapsed);

    return 0;
}