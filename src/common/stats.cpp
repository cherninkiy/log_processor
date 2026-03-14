#include "stats.h"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

void LogStats::merge(const LogStats& other)
{
    total_lines  += other.total_lines;
    parsed_ok    += other.parsed_ok;
    parse_errors += other.parse_errors;
    total_bytes  += other.total_bytes;

    for (const auto& [code, cnt] : other.status_codes)
        status_codes[code] += cnt;
    for (const auto& [ip, cnt] : other.ip_counts)
        ip_counts[ip] += cnt;
    for (const auto& [url, cnt] : other.url_counts)
        url_counts[url] += cnt;
}

void LogStats::print(const std::string& filename, double elapsed_sec) const
{
    constexpr int W = 20;
    const double mb = static_cast<double>(total_bytes) / (1024.0 * 1024.0);

    std::cout << "\n=== Log Processing Results ===\n";
    std::cout << std::left << std::setw(W) << "File:" << filename << "\n";
    std::cout << std::setw(W) << "Total lines:"   << total_lines   << "\n";
    std::cout << std::setw(W) << "Parsed OK:"     << parsed_ok     << "\n";
    std::cout << std::setw(W) << "Parse errors:"  << parse_errors  << "\n";

    std::cout << "\n";
    std::cout << std::setw(W) << "Total requests:" << parsed_ok << "\n";
    std::cout << std::setw(W) << "Unique IPs:"     << ip_counts.size()  << "\n";
    std::cout << std::setw(W) << "Total bytes:"
              << std::fixed << std::setprecision(1) << mb << " MB\n";

    // HTTP status codes
    std::cout << "\nHTTP status codes:\n";
    std::vector<std::pair<int, uint64_t>> codes(status_codes.begin(), status_codes.end());
    std::sort(codes.begin(), codes.end());
    for (const auto& [code, cnt] : codes) {
        double pct = parsed_ok ? 100.0 * cnt / parsed_ok : 0.0;
        std::cout << "  " << std::setw(6) << code << ":  "
                  << std::setw(12) << cnt
                  << "  (" << std::setw(5) << std::fixed << std::setprecision(1) << pct << "%)\n";
    }

    // Top-10 URLs
    std::cout << "\nTop 10 URLs:\n";
    std::vector<std::pair<std::string, uint64_t>> urls(url_counts.begin(), url_counts.end());
    std::sort(urls.begin(), urls.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    for (int i = 0; i < std::min(10, static_cast<int>(urls.size())); ++i) {
        std::cout << "  " << std::setw(3) << (i + 1) << ".  "
                  << std::setw(40) << std::left << urls[i].first
                  << "  —  " << urls[i].second << " hits\n";
    }

    // Timing
    double throughput_mb = elapsed_sec > 0 ? mb / elapsed_sec : 0.0;
    std::cout << "\n";
    std::cout << std::setw(W) << "Processing time:"
              << std::fixed << std::setprecision(2) << elapsed_sec << " sec\n";
    std::cout << std::setw(W) << "Throughput:"
              << std::fixed << std::setprecision(1) << throughput_mb << " MB/s\n";
    std::cout << std::flush;
}