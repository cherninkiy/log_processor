#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

#include "thread_pool.h"

// ---------------------------------------------------------------------------
// parallel_for — статическое равномерное разбиение с созданием std::thread.
// Используется в шагах 2-3. Сохранён для обратной совместимости тестов.
// worker: void(size_t start, size_t end)
// ---------------------------------------------------------------------------
template <typename Worker>
void parallel_for(size_t total, size_t n_threads, Worker worker) {
    if (total == 0 || n_threads == 0) return;

    const size_t chunk = (total + n_threads - 1) / n_threads;

    std::vector<std::thread> threads;
    threads.reserve(n_threads);

    for (size_t t = 0; t < n_threads; ++t) {
        const size_t start = t * chunk;
        const size_t end   = std::min(start + chunk, total);
        if (start >= total) break;
        threads.emplace_back(worker, start, end);
    }

    for (auto& th : threads) th.join();
}

// ---------------------------------------------------------------------------
// parallel_for_indexed — как parallel_for, но передаёт tid воркеру.
// worker: void(size_t start, size_t end, size_t tid)
// Сохранён для обратной совместимости тестов step3.
// ---------------------------------------------------------------------------
template <typename IndexedWorker>
void parallel_for_indexed(size_t total, size_t n_threads, IndexedWorker worker) {
    if (total == 0 || n_threads == 0) return;

    const size_t chunk = (total + n_threads - 1) / n_threads;

    std::vector<std::thread> threads;
    threads.reserve(n_threads);

    for (size_t t = 0; t < n_threads; ++t) {
        const size_t start = t * chunk;
        const size_t end   = std::min(start + chunk, total);
        if (start >= total) break;
        threads.emplace_back(worker, start, end, t);
    }

    for (auto& th : threads) th.join();
}

// ---------------------------------------------------------------------------
// parallel_for_pool — статическое разбиение через ThreadPool.
// worker: void(size_t start, size_t end, size_t tid)
//
// В отличие от parallel_for_indexed, потоки не создаются заново —
// используются заранее созданные рабочие потоки из пула.
// Это устраняет накладные расходы на thread::join (шаг 4+).
// ---------------------------------------------------------------------------
template <typename IndexedWorker>
void parallel_for_pool(ThreadPool& pool, size_t total, size_t n_threads, IndexedWorker worker)
{
    if (total == 0 || n_threads == 0) return;

    const size_t chunk = (total + n_threads - 1) / n_threads;

    for (size_t t = 0; t < n_threads; ++t) {
        const size_t start = t * chunk;
        const size_t end   = std::min(start + chunk, total);
        if (start >= total) break;
        pool.enqueue_detach([worker, start, end, t] {
            worker(start, end, t);
        });
    }

    pool.wait_all();
}
