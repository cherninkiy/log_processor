#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

// ---------------------------------------------------------------------------
// LogEntry — одна распарсенная строка лога
// ---------------------------------------------------------------------------
struct LogEntry {
    std::string ip;
    std::string time;
    std::string method;
    std::string url;
    std::string protocol;
    int status;
    uint64_t bytes;
    std::string referer;
    std::string user_agent;
};

// ---------------------------------------------------------------------------
// LogStats — агрегированная статистика по всему файлу (или его блоку)
// ---------------------------------------------------------------------------
struct LogStats {
    uint64_t total_lines   = 0;
    uint64_t parsed_ok     = 0;
    uint64_t parse_errors  = 0;
    uint64_t total_bytes   = 0;

    std::unordered_map<int, uint64_t>         status_codes;  // код → кол-во
    std::unordered_map<std::string, uint64_t> ip_counts;     // ip  → кол-во
    std::unordered_map<std::string, uint64_t> url_counts;    // url → кол-во

    // Слить другой экземпляр в this (используется при параллельной обработке)
    void merge(const LogStats& other);

    // Вывести результат на stdout
    void print(const std::string& filename, double elapsed_sec) const;
};