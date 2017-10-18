#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_set>

std::vector<int> get_lcp(const std::vector<int>& suffix_array, const std::string& s) {
    int n = s.size();
    std::vector<int> lcp(n);
    std::vector<int> inversed_suffix_array(n);
    for (int i = 0; i < n; ++i) {
        inversed_suffix_array[suffix_array[i]] = i;
    }
    int k = 0;
    for (int i = 0; i < n; ++i) {
        if (k > 0) --k;
        if (inversed_suffix_array[i] == n - 1) {
            lcp[n - 1] = -1;
            k = 0;
        } else {
            int j = suffix_array[inversed_suffix_array[i] + 1];
            while (std::max(i + k, j + k) < n && s[i + k] == s[j + k]) {
                ++k;
            }
            lcp[inversed_suffix_array[i]] = k;
        }
    }
    return lcp;
}

void compute(const std::string& s, const std::vector<int> suffix_array) {
    int n = s.size();
    auto lcp = get_lcp(suffix_array, s);

    uint64_t distinct_substrings = n - suffix_array[0];
    int max_lcp = -1;
    std::unordered_set<int> unique_counter;

    for (int i = 1; i < lcp.size(); i++) {
        distinct_substrings += (n - suffix_array[i]) - lcp[i - 1];
        max_lcp = std::max(max_lcp, lcp[i - 1]);
        if (i != 1) unique_counter.insert(lcp[i - 1]);
    }
    ++distinct_substrings;
    distinct_substrings -= s.size();
    std::cout << distinct_substrings << " " << max_lcp << " " << unique_counter.size() << std::endl;
}

int main() {
    std::string s;
    std::vector<int> suffix_array;
    std::getline(std::cin, s);
    suffix_array.resize(s.size() + 1);
    suffix_array[0] = s.size();
    for (int i = 0; i < s.size(); ++i) {
        std::cin >> suffix_array[i + 1];
    }
    s.append(1, '$');
    compute(s, suffix_array);
    return 0;
}