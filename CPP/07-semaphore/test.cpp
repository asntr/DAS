#include <thread>
#include <vector>
#include <chrono>

#include <gtest/gtest.h>
#include <semaphore.h>

void run(int threads_count, int concurrency_level) {
    Semaphore semaphore(concurrency_level);
    for (int i = 0; i < concurrency_level; ++i)
        semaphore.enter();

    int time = 0;
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    for (int i = 0; i < threads_count; ++i) {
        threads.emplace_back([&time, &semaphore, i]() {
            semaphore.enter([&time, i](int& value) {
                auto cur_time = time++;
                ASSERT_EQ(i, cur_time);
                --value;
            });
            semaphore.leave();
        });
        std::this_thread::sleep_for(std::chrono::duration<int, std::milli> (200));
    }

    for (int i = 0; i < concurrency_level; ++i) {
        semaphore.leave();
    }

    for (int i = 0; i < threads_count; ++i) {
        threads[i].join();
    }
}

TEST(Order, Mutex) {
    run(4, 1);
}

TEST(Order, Semaphore) {
    run(4, 3);
}

TEST(Order, Advanced) {
    std::vector<std::thread> threads;
    threads.reserve(3);
    Semaphore semaphore(3);
    for (int i = 0; i < 3; ++i)
        semaphore.enter();

    int time = 0;

    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&semaphore, &time, i]() {
            semaphore.enter([&semaphore, &time, i](int& value) {
                int cur_time = time++;
                ASSERT_EQ(i, cur_time);
                if (i < 2)
                    std::this_thread::sleep_for(std::chrono::duration<int, std::milli> (400));
                --value;
            });
            semaphore.leave();
        });
        std::this_thread::sleep_for(std::chrono::duration<int, std::milli> (200));
    }

    for (int i = 0; i < 3; ++i)
        semaphore.leave();

    for (auto& cur : threads)
        cur.join();
}
