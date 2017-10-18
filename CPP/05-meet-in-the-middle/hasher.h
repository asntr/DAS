#pragma once

#include <string>
#include <cctype>

class Hasher {
public:
    explicit Hasher(int64_t modulo = kDefaultModulo, int multiplier = kDefaultMultiplier) {
        modulo_ = modulo;
        multiplier_ = multiplier;
    }

    int64_t operator()(const std::string& s) const {
        int64_t sum = 0;
        for (char c : s)
            sum = (sum * multiplier_ + c) % modulo_;
        return sum;
    }

    int64_t get_modulo() const {
        return modulo_;
    }

    int get_multiplier() const {
        return multiplier_;
    }
private:
    int64_t modulo_;
    int multiplier_;

    static const int64_t kDefaultModulo = 1000000000000037ll;
    static const int kDefaultMultiplier = 259;
};
