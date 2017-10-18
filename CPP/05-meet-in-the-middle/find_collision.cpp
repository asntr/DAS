#include "find_collision.h"
#include "hasher.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include "commons.h"

std::atomic_bool found(false);
std::mutex string_warden;
std::string res;

std::pair<std::vector<std::unordered_map<std::string, int64_t>>, std::string> generate_suffix_map(const Hasher& hasher, int64_t tester) {
    std::vector<std::unordered_map<std::string, int64_t>> order_suffix_hash(7);
    for (char c : ALPHABET) {
        std::string next(1, c);
        int64_t hash = hasher(next);
        if (hash == tester) {
            found.store(true);
            return {order_suffix_hash, next};
        }
        order_suffix_hash[0].insert(std::make_pair(next, hash));
    }
    for (int i = 1; i < 7; i++) {
        for (auto el : order_suffix_hash[i-1]) {
            for (char c : ALPHABET) {
                std::string next = el.first + c;
                int64_t hash = hasher(next);
                if (hash == tester) {
                    found.store(true);
                    return {order_suffix_hash, next};
                }
                order_suffix_hash[i].insert(std::make_pair(next, hash));
            }
        }
    }
    return {order_suffix_hash, "?"};
}

int64_t hash_mod_pow(int64_t hash, int64_t number, int64_t modulo, int power) {
    int64_t res = hash % modulo;
    int64_t number_modulo = number % modulo;
    for (int i = 0; i < power; ++i) {
        res = (res * number_modulo) % modulo;
    }
    return res;
}

void searching(const std::unordered_map<std::string, int64_t>& prefix_map, const std::unordered_map<std::string, int64_t>& check_map, const Hasher& hasher, int64_t tester, int length) {
    for (auto& p : prefix_map) {
        int64_t pre_hash = hash_mod_pow(p.second, hasher.get_multiplier(), hasher.get_modulo(), length);
        if (found) break;
        for (auto& c : check_map) {
            if (found) break;
            int64_t hash = (tester + hasher.get_modulo() - pre_hash) % hasher.get_modulo();
            if (hash == tester && !found) {
                string_warden.lock();
                found.store(true);
                res = c.first;
                string_warden.unlock();
                break;
            }
        }
    }
}

std::string find_collision(const std::string& s, const Hasher& hasher) {
    int64_t tester = hasher(s);
    auto preprocessing = generate_suffix_map(hasher, tester);
    if (preprocessing.second != "?") {
        return preprocessing.second;
    }
    std::vector<std::thread> workers;
    workers.reserve(7);
    for (int i = 0; i < 7; i++) {
        workers.emplace_back(searching, std::ref(preprocessing.first[6]), std::ref(preprocessing.first[i]), std::ref(hasher), tester, i+1);
    }
    for (auto& t : workers) {
        t.join();
    }
    return res;
}
