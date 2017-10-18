#pragma once
#include <mutex>
#include <condition_variable>

class RWLock {
public:
    template<class Func>
    void read(Func func) {
        std::unique_lock<std::mutex> lock(global_);
        rcv_.wait(lock, [this] {
            return waiting_writers_ == 0;
        });
        ++blocked_readers_;
        lock.unlock();
        try {
            func();
        } catch(...) {
            end_read();
            throw;
        }
        end_read();
    }

    template<class Func>
    void write(Func func) {
        std::unique_lock<std::mutex> lock(global_);
        ++waiting_writers_;
        wcv_.wait(lock, [this] {
            return blocked_readers_ == 0;
        });
        --waiting_writers_;
        func();
        if (waiting_writers_) wcv_.notify_one();
        else rcv_.notify_all();
    }

private:
    std::mutex global_;
    std::condition_variable wcv_;
    std::condition_variable rcv_;
    int blocked_readers_ = 0;
    int waiting_writers_ = 0;

    void end_read() {
        global_.lock();
        --blocked_readers_;
        if (!blocked_readers_ && waiting_writers_) wcv_.notify_one();
        global_.unlock();
    }
};
