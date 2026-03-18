#pragma once

// ---------------------------------------------------------------------------
// ConcurrentStats — step9: lock-free агрегация через tbb::concurrent_hash_map
//
// Предыдущие шаги хранили thread-local LogStats и сливали их после join().
// Это требует N копий карт и O(N*K) работы на слияние, где K — размер карты.
//
// tbb::concurrent_hash_map обеспечивает fine-grained locking: каждый бакет
// защищён своим спинлоком. Несколько потоков могут писать в разные бакеты
// одновременно — конфликт только если два потока попадают в один бакет.
//
// Итог: нет thread-local буферов, нет финального слияния. Каждый поток
// пишет напрямую в единственную общую структуру через accessor-объект
// (RAII-lock на один бакет).
// ---------------------------------------------------------------------------

#include <atomic>
#include <cstdint>
#include <string>

#include <tbb/concurrent_hash_map.h>

#include "stats.h"  // LogEntry, LogStats, AtomicStats

// Тип карт: строка → счётчик
using ConcurrentMap = tbb::concurrent_hash_map<std::string, uint64_t>;

// ---------------------------------------------------------------------------
struct ConcurrentStats {
    // Скалярные счётчики — те же atomic, что в step3.
    // Каждое поле на отдельной кэш-линии (см. AtomicStats).
    alignas(64) std::atomic<uint64_t> total_lines  {0};
    alignas(64) std::atomic<uint64_t> parsed_ok    {0};
    alignas(64) std::atomic<uint64_t> parse_errors {0};
    alignas(64) std::atomic<uint64_t> total_bytes  {0};

    // Карты с fine-grained locking (tbb::concurrent_hash_map)
    ConcurrentMap status_codes;  // "200" -> count
    ConcurrentMap ip_counts;
    ConcurrentMap url_counts;

    // -------------------------------------------------------------------
    // Добавить успешно распарсенную запись.
    // accessor блокирует ровно один бакет на время операции.
    // -------------------------------------------------------------------
    void add_entry(const LogEntry& e, uint64_t bytes_val)
    {
        total_lines .fetch_add(1, std::memory_order_relaxed);
        parsed_ok   .fetch_add(1, std::memory_order_relaxed);
        total_bytes .fetch_add(bytes_val, std::memory_order_relaxed);

        increment(status_codes, std::to_string(e.status));
        increment(ip_counts,    e.ip);
        increment(url_counts,   e.url);
    }

    void add_error()
    {
        total_lines  .fetch_add(1, std::memory_order_relaxed);
        parse_errors .fetch_add(1, std::memory_order_relaxed);
    }

    // -------------------------------------------------------------------
    // Конвертировать в LogStats для печати (вызывается однократно).
    // -------------------------------------------------------------------
    LogStats to_log_stats() const
    {
        LogStats ls;
        ls.total_lines  = total_lines .load(std::memory_order_relaxed);
        ls.parsed_ok    = parsed_ok   .load(std::memory_order_relaxed);
        ls.parse_errors = parse_errors.load(std::memory_order_relaxed);
        ls.total_bytes  = total_bytes .load(std::memory_order_relaxed);

        for (auto it = status_codes.begin(); it != status_codes.end(); ++it)
            ls.status_codes[std::stoi(it->first)] = it->second;

        for (auto it = ip_counts.begin(); it != ip_counts.end(); ++it)
            ls.ip_counts[it->first] = it->second;

        for (auto it = url_counts.begin(); it != url_counts.end(); ++it)
            ls.url_counts[it->first] = it->second;

        return ls;
    }

private:
    // Атомарный инкремент значения в concurrent_hash_map.
    // insert_or_visit + lambda — атомарнее, чем find + insert отдельно.
    static void increment(ConcurrentMap& m, const std::string& key)
    {
        ConcurrentMap::accessor acc;   // write-lock на бакет
        if (m.insert(acc, key))
            acc->second = 1;           // новый ключ
        else
            ++acc->second;             // существующий
    }
};
