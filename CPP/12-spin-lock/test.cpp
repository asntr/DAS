#include <gtest/gtest.h>
#include <spin_lock.h>
#include <chrono>
#include <vector>
#include <thread>

using namespace std::chrono;

TEST(Correctness, Simple) {
    SpinLock spin;
    std::thread first([&spin]() {
        spin.lock();
        std::this_thread::sleep_for(milliseconds(1500));
        spin.unlock();
    });
    std::this_thread::sleep_for(milliseconds(300));
    std::thread second([&spin]() {
        auto start = high_resolution_clock::now();
        spin.lock();
        auto end = high_resolution_clock::now();
        auto duration = (duration_cast<seconds> (end - start)).count();
        ASSERT_LT(0.9, duration);
        ASSERT_LT(duration, 1.1);
        spin.unlock();
    });
    first.join();
    second.join();
}

TEST(Concurrency, Simple) {
    int threads_count = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    int counter = 0;
    SpinLock spin;
    for (int i = 0; i < threads_count; ++i)
        threads.emplace_back([&counter, &spin]() {
            for (int j = 0; j < 100; ++j) {
                spin.lock();
                ++counter;
                spin.unlock();
            }
        });

    for (auto& thread : threads)
        thread.join();

    ASSERT_EQ(100 * threads_count, counter);
}
