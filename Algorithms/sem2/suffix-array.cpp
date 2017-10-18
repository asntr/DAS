#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

class StringHasher {
public:
    StringHasher() = delete;
    StringHasher(const std::string& s):s_(s) {
        CalculatePowers();
        CalculateHash();
    }

    std::pair<int64_t, int64_t> MultipliedSubstringHash(size_t l, size_t r) {
        return std::make_pair(l != 0 ? hash_[r] - hash_[l - 1] : hash_[r], powers_[l]);
    }

    bool AreEqualSubstrings(size_t l1, size_t r1, size_t l2, size_t r2) {
        auto first = MultipliedSubstringHash(l1, r1);
        auto second = MultipliedSubstringHash(l2, r2);
        return first.first * second.second == first.second * second.first;
    }

    size_t CommonSuffixPrefixLength(size_t i, size_t j) {
        size_t upper_bound = std::min(s_.length() - i, s_.length() - j);
        size_t low_bound = 0;
        size_t result = 0;
        while (low_bound <= upper_bound) {
            size_t middle = (upper_bound + low_bound) / 2;
            if (middle == 0 || AreEqualSubstrings(i, i + middle - 1, j, j + middle - 1)) {
                low_bound = middle + 1;
                result = middle;
            } else {
                upper_bound = middle - 1;
            }
        }
        return result;
    }

    bool IsSuffixLexicallyLess(size_t i, size_t j) {
        size_t common_length = CommonSuffixPrefixLength(i, j);
        char i_symbol = i + common_length >= s_.length() ? 0 : s_[i + common_length];
        char j_symbol = j + common_length >= s_.length() ? 0 : s_[j + common_length];
        return i_symbol < j_symbol;
    }

    std::vector<size_t> GetSuffixArray() {
        std::vector<size_t> suffix_array(s_.length());
        for (size_t i = 0; i < s_.length(); ++i) {
            suffix_array[i] = i + 1;
        }
        std::sort(suffix_array.begin(), suffix_array.end(), [&](const size_t& a, const size_t& b) {
            return IsSuffixLexicallyLess(a - 1, b - 1);
        });
        return suffix_array;
    }
private:
    void CalculateHash() {
        hash_.resize(s_.length());
        hash_[0] = (s_[0] - 'a' + 1);
        for (size_t i = 1; i < s_.length(); ++i) {
            hash_[i] = hash_[i - 1] + powers_[i] * (s_[i] - 'a' + 1);
        }
    }
    void CalculatePowers() {
        powers_.resize(s_.length());
        powers_[0] = 1;
        for (size_t i = 1; i < s_.length(); ++i) {
            powers_[i] = powers_[i - 1] * MUL;
        }
    }
    std::vector<int64_t> powers_;
    std::string s_;
    std::vector<int64_t> hash_;
    static const int64_t MUL = 31;
};

int main() {
    std::string s;
    std::cin >> s;
    StringHasher sh(s);
    auto suffix_array = sh.GetSuffixArray();
    for (auto i : suffix_array) {
        std::cout << i << " ";
    }
    return 0;
}