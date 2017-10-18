#pragma once

#include <vector>
#include <list>
#include <map>
#include <mutex>
#include <functional>
#include <algorithm>
#include <atomic>
#include <iostream>

template<class K, class V, class Hash = std::hash<K>>
class ConcurrentHashMap {
public:
    ConcurrentHashMap(const Hash& hasher = Hash()):
        ConcurrentHashMap(kUndefinedSize, hasher) {}

    explicit ConcurrentHashMap(int expected_size, const Hash& hasher = Hash()):
        ConcurrentHashMap(expected_size, kDefaultConcurrencyLevel, hasher) {}

    ConcurrentHashMap(int expected_size, int expected_threads_count, const Hash& hasher = Hash()):
            hasher_(hasher),
            locks_(expected_threads_count <= 0 ? kDefaultConcurrencyLevel : expected_threads_count),
            size_(0) {
        if (expected_threads_count <= 0) {
            expected_threads_count = kDefaultConcurrencyLevel;
        }
        if (expected_size != kUndefinedSize) {
            capacity_.store(std::max(expected_size / expected_threads_count * expected_threads_count, expected_threads_count));
            table_.resize(capacity_);
        } else {
            capacity_.store(expected_threads_count);
            table_.resize(expected_threads_count);
        }
    }

    bool insert(const K& key, const V& value) {
        std::unique_lock<std::mutex> u_lock(locks_[hasher_(key) % locks_.size()]);
        int cell = hasher_(key) % capacity_;
        auto res = table_[cell].insert({key, value});
        if (res.second) {
            ++size_;
            if (size_ * 1. / capacity_ > 1.5) {
                u_lock.unlock();
                Rehash();
            }
        }
        return res.second;
    }

    bool erase(const K& key) {
        std::lock_guard<std::mutex> lock(locks_[hasher_(key) % locks_.size()]);
        int cell = hasher_(key) % capacity_;
        auto deleted_count = table_[cell].erase(key);
        if (deleted_count == 0) {
            return false;
        }
        --size_;
        return true;
    }

    void clear() {
        for (auto &m : locks_) {
            m.lock();
        }
        for (auto &l : table_) {
            l.clear();
        }
        size_ = 0;
        for (auto &m : locks_) {
            m.unlock();
        }
    }

    std::pair<bool, V> find(const K& key) const {
        std::lock_guard<std::mutex> lock(locks_[hasher_(key) % locks_.size()]);
        int cell = hasher_(key) % capacity_;
        auto res = table_[cell].find(key);
        return res == table_[cell].end() ? std::make_pair(false, V()) : std::make_pair(true, res->second);
    }

    const V at(const K& key) const {
        std::lock_guard<std::mutex> lock(locks_[hasher_(key) % locks_.size()]);
        int cell = hasher_(key) % capacity_;
        return table_[cell].at(key);
    }

    size_t size() const {
        return size_;
    }

    static const int kDefaultConcurrencyLevel;
    static const int kUndefinedSize;
private:
    void Rehash() {
        for (auto &m : locks_) {
            m.lock();
        }
        std::vector<std::map<K, V>> new_table;
        size_t new_capacity = capacity_ * 2;
        new_table.resize(new_capacity);
        for (auto &l : table_) {
            for (auto &p : l) {
                int cell = hasher_(p.first) % new_capacity;
                new_table[cell].insert(std::move(p));
            }
        }
        table_.clear();
        table_ = std::move(new_table);
        capacity_ = new_capacity;
        for (auto &m : locks_) {
            m.unlock();
        }
    }
    std::vector<std::map<K, V>> table_;
    mutable std::vector<std::mutex> locks_;
    std::atomic_size_t size_;
    std::atomic_size_t capacity_;
    Hash hasher_;
};

template<class K, class V, class Hash>
const int ConcurrentHashMap<K, V, Hash>::kDefaultConcurrencyLevel = 8;

template<class K, class V, class Hash>
const int ConcurrentHashMap<K, V, Hash>::kUndefinedSize = -1;
