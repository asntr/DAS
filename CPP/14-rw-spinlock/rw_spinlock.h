#pragma once

#include <atomic>
#include <thread>

struct RWSpinLock {
    void lockRead() {
        short test_no_writers = (counter_.load(std::memory_order_acq_rel) >> 1) << 1;
        while (!counter_.compare_exchange_weak(test_no_writers, test_no_writers + 0b10, std::memory_order_acquire)) {
            std::this_thread::yield();
            test_no_writers = (counter_.load(std::memory_order_relaxed) >> 1) << 1;
        }
    }

    void unlockRead() {
        counter_.fetch_sub(0b10, std::memory_order_release);
    }

    void lockWrite() {
        short test_empty = 0b0;
        while (!counter_.compare_exchange_weak(test_empty, 0b1, std::memory_order_acquire)) {
            std::this_thread::yield();
            test_empty = 0b0;
        };
    }

    void unlockWrite() {
        counter_.fetch_and(0b0, std::memory_order_release);
    }

private:
    std::atomic_short counter_ = {0b0};
};
