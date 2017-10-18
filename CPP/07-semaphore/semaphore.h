#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

class DefaultCallback {
public:
    void operator()(int& value) {
        --value;
    }
};

class Semaphore {
public:
    Semaphore(int count): count_(count) {}

    void leave() {
        std::unique_lock<std::mutex> lock(main_mutex_);
        ++count_;
        cv_.notify_all();
    }

    template<class Func>
    void enter(Func callback) {
        std::unique_lock<std::mutex> lock(main_mutex_);
        thread_queue_.push(std::this_thread::get_id());
        cv_.wait(lock, [this] {
            return count_ != 0 && thread_queue_.front() == std::this_thread::get_id();
        });
        thread_queue_.pop();
        callback(count_);
    }

    void enter() {
        DefaultCallback callback;
        enter(callback);
    }

private:
    std::mutex main_mutex_;
    std::queue<std::thread::id> thread_queue_;
    std::condition_variable cv_;
    int count_ = 0;
};

