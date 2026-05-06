#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// ThreadPool — постоянный пул потоков для параллельного выполнения задач.
//
// Создаёт N рабочих потоков, которые ожидают задачи в очереди.
// enqueue() добавляет задачу, wait_all() дожидается завершения всех.
// Деструктор останавливает пул и join'ит потоки.
//
// В отличие от создания std::thread под каждый чанк (шаги 2-3),
// пул переиспользует потоки, устраняя накладные расходы на thread::join
// и позволяя динамически распределять чанки между потоками.
// ---------------------------------------------------------------------------
class ThreadPool {
public:
    explicit ThreadPool(size_t n_threads)
        : stop_{false}, active_tasks_{0}
    {
        workers_.reserve(n_threads);
        for (size_t i = 0; i < n_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        cv_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty())
                            return;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                    cv_all_.notify_one();
                }
            });
        }
    }

    ~ThreadPool()
    {
        stop();
    }

    // Запрещаем копирование и перемещение
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // -------------------------------------------------------------------
    // enqueue — добавить задачу в очередь.
    // Задача будет выполнена одним из рабочих потоков.
    // Поддерживает как void-функции, так и возвращающие значение (через future).
    // -------------------------------------------------------------------
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>
    {
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks_.emplace([task]() { (*task)(); });
        }
        active_tasks_.fetch_add(1, std::memory_order_release);
        cv_.notify_one();
        return result;
    }

    // enqueue_detach — добавить задачу без future (void, без возврата).
    template <typename F>
    void enqueue_detach(F&& f)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (stop_)
                throw std::runtime_error("enqueue_detach on stopped ThreadPool");
            tasks_.emplace(std::forward<F>(f));
        }
        active_tasks_.fetch_add(1, std::memory_order_release);
        cv_.notify_one();
    }

    // -------------------------------------------------------------------
    // wait_all — дождаться завершения всех активных задач.
    // -------------------------------------------------------------------
    void wait_all()
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        cv_all_.wait(lock, [this] {
            return tasks_.empty() && active_tasks_.load(std::memory_order_acquire) == 0;
        });
    }

    // -------------------------------------------------------------------
    // stop — запретить новые задачи, дождаться завершения, join'ить потоки.
    // -------------------------------------------------------------------
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;      // для новых задач
    std::condition_variable cv_all_;  // для wait_all

    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_tasks_{0};
};