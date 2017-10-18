#pragma once

#include <atomic>
#include <memory>

template<class T>
class MPMCBoundedQueue {
public:
    explicit MPMCBoundedQueue(size_t size):size_(size), mask_(size - 1), buffer_(new QueueCell[size]) {
        for (size_t i = 0; i < size; i++) {
            buffer_.get()[i].generation.store(i, std::memory_order_relaxed);
        }
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    bool enqueue(const T& value) {
        QueueCell* cell;
        size_t enqueue_pos = tail_.load(std::memory_order_relaxed);
        while(true) {
            cell = &buffer_.get()[enqueue_pos & mask_];
            size_t generation = cell->generation.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)generation - (intptr_t)enqueue_pos;
            if (dif == 0) {
                if (tail_.compare_exchange_weak(enqueue_pos, enqueue_pos + 1, std::memory_order_relaxed)) break;
            }
            else if (dif < 0) return false;
            else enqueue_pos = tail_.load(std::memory_order_relaxed);
        }
        cell->val = value;
        cell->generation.store(enqueue_pos + 1, std::memory_order_release);
        return true;
    }

    bool dequeue(T& data) {
        QueueCell* cell;
        size_t dequeue_pos = head_.load(std::memory_order_relaxed);
        while(true) {
            cell = &buffer_.get()[dequeue_pos & mask_];
            size_t generation = cell->generation.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)generation - (intptr_t)(dequeue_pos + 1);
            if (dif == 0) {
                if (head_.compare_exchange_weak(dequeue_pos, dequeue_pos + 1, std::memory_order_relaxed)) break;
            }
            else if (dif < 0) return false;
            else dequeue_pos = head_.load(std::memory_order_relaxed);
        }
        data = cell->val;
        cell->generation.store(dequeue_pos + mask_ + 1, std::memory_order_release);
        return true;
    }

private:
    struct QueueCell {
        T val;
        std::atomic_size_t generation;
    };
    std::unique_ptr<QueueCell> buffer_;
    size_t size_ = 0;
    size_t mask_;
    alignas(64) std::atomic_size_t head_ = {0};
    alignas(64) std::atomic_size_t tail_ = {0};
};
