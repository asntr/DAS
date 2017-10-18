#pragma once

#include <utility>
#include <mutex>
#include <condition_variable>
#include <memory>

template<class T>
class UnbufferedChannel {
public:
    void send(const T& value) {
        std::unique_lock<std::mutex> lock(send_mutex_);
        std::unique_lock<std::mutex> glock(global_mutex_);
        if (closed_) throw std::runtime_error("channel closed");
        p_.store(value);
        rcv_.notify_one();
        scv_.wait(glock, [this] {
            return p_.received();
        });
        p_.check();
    }
    std::pair<T, bool> recv() {
        std::unique_lock<std::mutex> lock(recv_mutex_);
        std::unique_lock<std::mutex> glock(global_mutex_);
        if (closed_) return std::make_pair(T(), false);
        rcv_.wait(glock, [this] {
            return p_.sent() || closed_;
        });
        if (closed_ && !p_.sent()) return std::make_pair(T(), false);
        T res = p_.load();
        scv_.notify_one();
        return std::make_pair(res, true);
    }
    void close() {
        std::lock_guard<std::mutex> lock(global_mutex_);
        closed_ = true;
        rcv_.notify_all();
        scv_.notify_all();
    }

private:
    class Parcel {
    public:
        void store(T val) {
            value_ = val;
            state_ = SENT;
        }
        T load() {
            state_ = RECEIVED;
            return value_;
        }
        void check() {
            state_ = IDLE;
        }
        bool sent() {
            return state_ == SENT;
        }
        bool received() {
            return state_ == RECEIVED;
        }
        bool idle() {
            return state_ == IDLE;
        }
    private:
        enum ParcelState {
            IDLE, SENT, RECEIVED
        };
        T value_;
        ParcelState state_ = IDLE;
    };
    std::mutex send_mutex_;
    std::mutex recv_mutex_;
    std::mutex global_mutex_;
    std::condition_variable scv_;
    std::condition_variable rcv_;
    bool closed_ = false;
    Parcel p_;
};