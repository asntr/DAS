#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <chrono>

#include <gtest/gtest.h>
#include <rw_lock.h>

std::chrono::high_resolution_clock::time_point now() {
    return std::chrono::high_resolution_clock::now();
}

double elapsed_time(std::chrono::high_resolution_clock::time_point start) {
    using std::chrono::duration;
    duration<double> time_span = std::chrono::duration_cast<duration<double>>(now() - start);
    return time_span.count();
}

TEST(Correctness, Increment) {
    RWLock rw_lock;
    int threads_count = 4;
    double time_limit = 1.0;
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    int write_count = 0;
    std::atomic<int> read_count(0);

    for (int i = 0; i < threads_count; ++i)
        if (i % 2 == 0) {
            threads.emplace_back([time_limit, &read_count, &rw_lock]() {
                auto start = now();
                while (elapsed_time(start) < time_limit) {
                    rw_lock.read([&read_count]() {
                        ++read_count;
                    });
                }
            });
        } else {
            threads.emplace_back([time_limit, &write_count, &rw_lock]() {
                auto start = now();
                while (elapsed_time(start) < time_limit) {
                    rw_lock.write([&write_count]() {
                        ++write_count;
                    });
                }
            });
        }

    for (auto& cur : threads)
        cur.join();

    std::cerr << "read count " << read_count << ", write count " << write_count << "\n";
}

TEST(Concurrency, RLock) {
    double time_limit = 0.5;
    RWLock rw_lock;
    int max_threads_count = std::thread::hardware_concurrency();
    int threads_count = std::max(4, max_threads_count);
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    auto start = now();
    for (int i = 0; i < threads_count; ++i)
        threads.emplace_back([time_limit, &rw_lock]() {
            rw_lock.read([time_limit]() {
                auto start = now();
                while (elapsed_time(start) < time_limit) {
                    // do nothing
                }
            });
        });

    for (auto& thread : threads)
        thread.join();

    auto elapsed = elapsed_time(start);
    ASSERT_LT(elapsed, time_limit * 2);
}
