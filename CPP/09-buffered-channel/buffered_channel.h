#pragma once

#include <queue>
#include <utility>
#include <condition_variable>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size):queue_max_size_(size) {}
    void send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        scv_.wait(lock, [this]{
            return channel_.size() < queue_max_size_ || closed_;
        });
        if (closed_) {
            throw std::runtime_error("channel is closed, bye");
        }
        channel_.push(value);
        rcv_.notify_one();
    }
    std::pair<T, bool> recv() {
        std::unique_lock<std::mutex> lock(mutex_);
        rcv_.wait(lock, [this] {
            return !channel_.empty() || closed_;
        });
        if (closed_) {
            if (channel_.empty()) {
                return std::make_pair(T(), false);
            }
            T res = channel_.front();
            channel_.pop();
            return std::make_pair(res, true);
        }
        T res = channel_.front();
        channel_.pop();
        scv_.notify_one();
        return std::make_pair(res, true);
    }
    void close() {
        std::unique_lock<std::mutex> lock(mutex_);
        closed_ = true;
        scv_.notify_all();
        rcv_.notify_all();
    }

private:
    size_t queue_max_size_;
    std::condition_variable scv_;
    std::condition_variable rcv_;
    bool closed_ = false;
    std::mutex mutex_;
    std::queue<T> channel_;
};
