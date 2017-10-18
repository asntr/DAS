#include "is_prime.h"
#include <cmath>
#include <algorithm>
#include <thread>
#include <atomic>
#include <vector>

void CheckCycle(std::atomic_bool& verdict, uint64_t x, uint64_t bound, int cores_number, int order) {
    for (auto i = 2ull + order; i < bound && verdict.load(); i+= cores_number) {
        if (x % i == 0) {
            verdict.store(false);
            return;
        }
    }
}

bool is_prime(uint64_t x) {
    if (x <= 1)
        return false;
    uint64_t root = sqrt(x);
    auto bound = std::min(root + 6, x);
    auto cores_number = std::thread::hardware_concurrency();
    std::atomic_bool verdict(true);
    std::vector<std::thread> checkers(cores_number);
    for (int i = 0; i < cores_number; i++) {
        checkers[i] = std::thread(CheckCycle, std::ref(verdict), x, bound, cores_number, i);
    }
    for (int i = 0; i < cores_number; i++) {
        checkers[i].join();
    }
    return verdict.load();
}
