#include <iostream>
#include <vector>
#include <unordered_set>

std::vector<int> reverse_kmp(const std::vector<int>& prefix_function) {
    int n = prefix_function.size();
    std::vector<int> result(n, 0);
    int next_new = 1;
    if (prefix_function[0] != 0) {
        return { -1 };
    }
    for (int i = 1; i < n; ++i) {
        if (prefix_function[i] == 0) {
            result[i] = next_new++;
            continue;
        }
        std::unordered_set<int> abandoned_numbers;
        int prev = prefix_function[i - 1];
        while (prefix_function[i] < prev + 1) {
            abandoned_numbers.insert(result[prev]);
            prev = prefix_function[prev - 1];
        }
        if (prefix_function[i] > prev + 1) {
            return { -1 };
        } else {
            if (abandoned_numbers.find(result[prefix_function[i] - 1]) != abandoned_numbers.end()) {
                return{ -1 };
            }
            result[i] = result[prefix_function[i] - 1];
        }
    }
    return result;
}

int main() {
    std::vector<int> pf;
    int n;
    std::cin >> n;
    pf.resize(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> pf[i];
    }
    auto res = reverse_kmp(pf);
    for (auto i : res) {
        std::cout << i << " ";
    }
    return 0;
}