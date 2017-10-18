#pragma once

#include <thread>
#include <vector>
#include <mutex>

template<class RandomAccessIterator, class T, class Func>
T reduce(RandomAccessIterator first, RandomAccessIterator last, const T& initial_value, Func func) {
    auto cores_number = std::thread::hardware_concurrency();
    std::vector<T> part_values(cores_number);
    std::vector<bool> skip_flags(cores_number, false);
    std::vector<std::thread> threads(cores_number);
    std::mutex mutex;
    for (int i = 0; i < cores_number; i++) {
        threads[i] = std::thread([&func, first, last, i, &part_values, &skip_flags, cores_number, &mutex] {
            if (first + i >= last) {
                mutex.lock();
                skip_flags[i] = true;
                mutex.unlock();
                return;
            }
            T value(*(first + i));
            auto iterator = (first + i);
            while ((iterator + cores_number) < last) {
                value = func(value, *(iterator + cores_number));
                iterator += cores_number;
            }
            mutex.lock();
            part_values[i] = value;
            mutex.unlock();
        });
    }
    for (int i = 0; i < cores_number; i++) {
        threads[i].join();
    }
    T eventual_value(initial_value);
    for (int i = 0; i < cores_number; i++) {
        if (skip_flags[i]) {
            continue;
        }
        eventual_value = func(eventual_value, part_values[i]);
    }
    return eventual_value;
}

