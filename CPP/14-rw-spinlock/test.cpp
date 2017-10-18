#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>

#include <rw_spinlock.h>

using namespace std::chrono;

TEST(Correctness, Read) {
    std::atomic<int> counter = {0};

    RWSpinLock lock;
    lock.lockRead();
    std::thread t1([&] {
        lock.lockRead();
        counter++;
    });
    t1.join();
    ASSERT_EQ(counter.load(), 1);
}

TEST(Correctness, Write) {
    std::atomic<int> counter = {0};

    RWSpinLock lock;
    lock.lockWrite();
    std::thread t1([&] {
        lock.lockWrite();
        counter++;
    });
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(counter.load(), 0);
    lock.unlockWrite();
    t1.join();
    ASSERT_EQ(counter.load(), 1);
}

TEST(Correctness, ReadWrite) {
    std::atomic<int> counter = {0};

    RWSpinLock lock;
    lock.lockRead();
    std::thread t1([&] {
        lock.lockWrite();
        counter++;
    });
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(counter.load(), 0);
    lock.unlockRead();
    t1.join();
    ASSERT_EQ(counter.load(), 1);
}

TEST(Correctness, WriteRead) {
    std::atomic<int> counter = {0};

    RWSpinLock lock;
    lock.lockWrite();
    std::thread t1([&] {
        lock.lockRead();
        counter++;
    });
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(counter.load(), 0);
    lock.unlockWrite();
    t1.join();
    ASSERT_EQ(counter.load(), 1);
}
