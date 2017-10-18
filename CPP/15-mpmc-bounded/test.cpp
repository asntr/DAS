#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include <mpmc.h>

using namespace std::chrono;

TEST(Correctness, Enqueue) {
    MPMCBoundedQueue<int> queue(2);
    ASSERT_TRUE(queue.enqueue(2));
    ASSERT_TRUE(queue.enqueue(2));
    ASSERT_FALSE(queue.enqueue(2));
    ASSERT_FALSE(queue.enqueue(2));
}

TEST(Correctness, Dequeue) {
    int val;
    MPMCBoundedQueue<int> queue(2);
    ASSERT_FALSE(queue.dequeue(val));
    ASSERT_FALSE(queue.dequeue(val));
}

TEST(Correctness, EnqueueDequeue) {
    int val = 0;
    MPMCBoundedQueue<int> queue(2);
    ASSERT_TRUE(queue.enqueue(1));
    ASSERT_TRUE(queue.dequeue(val));
    ASSERT_EQ(val, 1);
    ASSERT_FALSE(queue.dequeue(val));

    ASSERT_TRUE(queue.enqueue(2));
    ASSERT_TRUE(queue.enqueue(3));
    ASSERT_FALSE(queue.enqueue(4));

    ASSERT_TRUE(queue.dequeue(val));
    ASSERT_EQ(val, 2);
    ASSERT_TRUE(queue.dequeue(val));
    ASSERT_EQ(val, 3);

    ASSERT_FALSE(queue.dequeue(val));
}

TEST(Correctness, NoSpuriousFails) {
    const int N = 1024 * 1024;
    const int nThreads = 4;
    MPMCBoundedQueue<int> queue(N * nThreads);

    std::vector<std::thread> writers;
    for (int i = 0; i < nThreads; i++) {
        writers.emplace_back([&] {
            for (int j = 0; j < N; ++j) {
                ASSERT_TRUE(queue.enqueue(0));
            }
        });
    }

    for (auto& t : writers) t.join();

    std::vector<std::thread> readers;
    for (int i = 0; i < nThreads; i++) {
        readers.emplace_back([&] {
            for (int j = 0; j < N; ++j) {
                int k;
                ASSERT_TRUE(queue.dequeue(k));
            }
        });
    }

    for (auto& t : readers) t.join();    
}
