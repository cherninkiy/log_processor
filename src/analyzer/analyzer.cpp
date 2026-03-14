#include "analyzer.h"

void accumulate(LogStats& stats, const LogEntry& entry) {
    stats.total_bytes += entry.bytes;
    stats.status_codes[entry.status]++;
    stats.ip_counts[entry.ip]++;
    stats.url_counts[entry.url]++;
}