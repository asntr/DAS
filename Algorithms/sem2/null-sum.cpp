#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <ctime>
#include <cassert>

class UniversalHashFunctionsFamily {
public:
    UniversalHashFunctionsFamily(int64_t p):p_(p) {}
    std::pair<std::function<size_t(int64_t)>, std::function<size_t(int64_t)>> GetPair() {
        using namespace std::placeholders;

        int64_t a1 = (std::rand() % (p_ - 1) + 1),
                b1 = std::rand() % p_,
                a2 = (std::rand() % (p_ - 1) + 1),
                b2 = std::rand() % p_;

        while (a1 == a2 && b1 == b2) {
            a1 = std::rand() % (p_ - 1) + 1;
            b1 = std::rand() % p_;
        }
        return std::make_pair(std::bind(Hash, a1, b1, _1, p_), std::bind(Hash, a2, b2, _1, p_));
    }
private:
    int64_t p_;
    static size_t Hash(int64_t a, int64_t b, int64_t key, int64_t p) {
        return (a * key + b) % p;
    }
};

template<class V = int, int64_t prime = 32452843>
class CuckooHashTable {
public:
    CuckooHashTable() = delete;
    CuckooHashTable(size_t capacity):family_(prime) {
        capacity_ = std::max<int64_t>(std::min<int64_t>(capacity * 2, MAX_ELEMENTS), capacity * 1.2);
        size_ = 0;
        table_.resize(capacity_, {-1, -1});
        ReloadHashPair();
    }
    void Add(int64_t key, V value) {
        Insert(key, value);
        ++size_;
    }
    void Remove(int64_t key) {
        if (table_[h1_(key) % capacity_].first == key) {
            table_[h1_(key) % capacity_] = { -1, -1 };
            size_--;
        }
        if (table_[h2_(key) % capacity_].first == key) {
            table_[h2_(key) % capacity_] = { -1, -1 };
            size_--;
        }
    }
    bool Contains(int64_t key) {
        if (table_[h1_(key) % capacity_].first == key) {
            return true;
        }
        if (table_[h2_(key) % capacity_].first == key) {
            return true;
        }
        return false;
    }
    V GetValue(int64_t key) {
        if (table_[h1_(key) % capacity_].first == key) {
            return table_[h1_(key) % capacity_].second;
        }
        if (table_[h2_(key) % capacity_].first == key) {
            return table_[h2_(key) % capacity_].second;
        }
        throw std::out_of_range("no such element in table");
    }
private:
    size_t capacity_;
    size_t size_;
    std::vector<std::pair<int64_t, V>> table_;
    UniversalHashFunctionsFamily family_;
    std::function<size_t(int64_t)> h1_;
    std::function<size_t(int64_t)> h2_;
    static const int64_t MAX_ELEMENTS = 4500000;
    void ReloadHashPair() {
        auto pair = family_.GetPair();
        h1_ = pair.first;
        h2_ = pair.second;
    }
    void Rehash(bool resize, bool change_hash) {
        if (resize) capacity_ = std::max<int64_t>(std::min<int64_t>(capacity_ * 2, MAX_ELEMENTS), capacity_);
        if (change_hash) ReloadHashPair();
        std::vector<std::pair<int64_t, V>> old_table_(std::move(table_));
        table_.resize(capacity_, { -1, -1 });
        for (size_t i = 0; i < old_table_.size(); ++i) {
            if (old_table_[i].first == -1) continue;
            Insert(old_table_[i].first, old_table_[i].second);
        }
    }
    bool TrySimpleInsertion(int64_t key, V value) {
        if (Contains(key)) {
            return true;
        }
        if (table_[h1_(key) % capacity_].first == -1) {
            table_[h1_(key) % capacity_] = std::make_pair(key, value);
            return true;
        }
        if (table_[h2_(key) % capacity_].first == -1) {
            table_[h2_(key) % capacity_] = std::make_pair(key, value);
            return true;
        }
        return false;
    }
    void Insert(int64_t key, V value) {
        for (size_t i = 0; i < 9 * log(capacity_); ++i) {
            if (TrySimpleInsertion(key, value)) {
                return;
            }
            std::swap(table_[h1_(key) % capacity_].first, key);
            std::swap(table_[h1_(key) % capacity_].second, value);
        }
        Rehash(true, true);
        Insert(key, value);
    }
};


int NullSumSubarrayLength(const std::vector<int64_t>& arr) {
    size_t max_length = 0;
    int64_t cur_range_sum = 0;
    CuckooHashTable<size_t> sum_store(arr.size());
    sum_store.Add(0, 0);
    for (size_t i = 0, n = arr.size(); i < n; ++i) {
        cur_range_sum += arr[i];
        if (sum_store.Contains(cur_range_sum)) {
            max_length = std::max(max_length, i - sum_store.GetValue(cur_range_sum) + 1);
        } else {
            sum_store.Add(cur_range_sum, i + 1);
        }
    }
    return max_length;
}

std::vector<int64_t> ReadInput(std::istream& input) {
    size_t n;
    std::vector<int64_t> arr;
    std::cin >> n;
    arr.resize(n);
    for (size_t i = 0; i < n; ++i) {
        std::cin >> arr[i];
    }
    return arr;
}

int main() {
    std::srand(unsigned(std::time(0)));
    auto arr = ReadInput(std::cin);
    std::cout << NullSumSubarrayLength(arr) << std::endl;
    return 0;
}
