#pragma once

#include <vector>
#include <random>

template<class T>
class Summator {
public:
    T operator()(const T& sum, const T& cur_value) const {
        return sum + cur_value;
    }
};

template<class T>
class Multiplier {
public:
    T operator()(const T& mul, const T& cur_value) const {
        return mul * cur_value;
    }
};

std::vector<uint32_t> gen_test(int size) {
    std::vector<uint32_t> result(size);
    static std::mt19937 gen(7347475);
    for(auto& cur : result) {
        cur = gen();
        if (!cur)
            ++cur;
    }
    return result;
}
