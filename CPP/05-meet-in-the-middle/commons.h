#pragma once

#include <random>
#include "params.h"

const std::string ALPHABET = "_0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

inline int get_random_multiplier(std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(257, 1000);
    int result = dist(gen);
    if (result % 2 == 0)
        ++result;
    return result;
}

inline int64_t get_random_modulo(const std::vector<int64_t>& modules, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, modules.size() - 1);
    return modules[dist(gen)];
}

inline char get_random_char(std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, ALPHABET.size() - 1);
    return ALPHABET[dist(gen)];
}

inline std::string get_random_string(int len, std::mt19937& gen) {
    std::string s(len, 'a');
    for (int i = 0; i < len; ++i)
        s[i] = get_random_char(gen);
    return s;
}

inline bool is_correct_string(const std::string& s) {
    for (char c : s)
        if (ALPHABET.find(c) == std::string::npos)
            return false;
    return true;
}
