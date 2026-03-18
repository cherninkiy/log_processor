#pragma once

#include <algorithm>
#include <thread>
#include <vector>

// Запускает n_threads потоков, каждый обрабатывает диапазон индексов [start, end).
// worker должен быть callable: worker(size_t start, size_t end)
//
// Это простейший планировщик шага 2: статическое равномерное разбиение.
// В step4 интерфейс остаётся тем же, но внутри будет постоянный thread pool.
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

// Расширенная версия: дополнительно передаёт индекс потока (tid) воркеру.
// worker должен быть callable: worker(size_t start, size_t end, size_t tid)
//
// Позволяет каждому потоку обращаться к собственному слоту в предварительно
// выделенном массиве без мьютекса — основа step3 (thread-local буферы).
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
