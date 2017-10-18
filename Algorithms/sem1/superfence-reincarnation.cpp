#include <iostream>
#include <tuple>
#include <ctime>
#include <vector>
#include <queue>
#include <memory>
#include <cassert>
#include <fstream>

class Treap {
public:
    long long int x;
    long long int y;
    long long int sum;
    size_t size;
    std::shared_ptr<Treap> left;
    std::shared_ptr<Treap> right;
    static std::shared_ptr<Treap> Merge(std::shared_ptr<Treap> l, std::shared_ptr<Treap> r) {
        std::shared_ptr<Treap> res;
        if (l == nullptr) {
            return r;
        }
        if (r == nullptr) {
            return l;
        }
        if (l->y > r->y) {
            std::shared_ptr<Treap> new_r = Merge(l->right, r);
            res.reset(new Treap(l->x, l->y, l->left, new_r));
        } else {
            std::shared_ptr<Treap> new_l = Merge(l, r->left);
            res.reset(new Treap(r->x, r->y, new_l, r->right));
        }
        res->Recalculate();
        return res;
    }
    std::tuple<std::shared_ptr<Treap>, std::shared_ptr<Treap>> Split(long long int x) {
        std::shared_ptr<Treap> l = nullptr;
        std::shared_ptr<Treap> r = nullptr;
        if (this == nullptr) {
            return std::make_tuple(l, r);
        }
        std::shared_ptr<Treap> temp = nullptr;
        if (this->x <= x) {
            if (this->right == nullptr) {
                r = nullptr;
            } else {
                auto res = right->Split(x);
                temp = std::get<0>(res);
                r = std::get<1>(res);
            }
            l.reset(new Treap(this->x, this->y, this->left, temp));
            l->Recalculate();
        } else {
            if (this->left == nullptr) {
                l = nullptr;
            } else {
                auto res = this->left->Split(x);
                temp = std::get<1>(res);
                l = std::get<0>(res);
            }
            r.reset(new Treap(this->x, this->y, temp, this->right));
            r->Recalculate();
        }
        return std::make_tuple(l, r);
    }
    static void Add(std::shared_ptr<Treap>& t, long long int x) {
        std::shared_ptr<Treap> l;
        std::shared_ptr<Treap> r;
        auto res = t->Split(x);
        l = std::get<0>(res);
        r = std::get<1>(res);
        std::shared_ptr<Treap> m(new Treap(x, rand()));
        t = (Merge(Merge(l, m), r));
    }
    static void Delete(std::shared_ptr<Treap>& t, long long int x) {
        if (t == nullptr) {
            return;
        }
        if (t->x == x) {
            t = Merge(t->left, t->right);
            return;
        } else if (t->x > x) {
            t->sum -= x;
            t->size -= 1;
            Delete(t->left, x);
        } else {
            t->sum -= x;
            t->size -= 1;
            Delete(t->right, x);
        }
    }
    static size_t Size(std::shared_ptr<Treap> t) {
        return t == nullptr ? 0 : t->size;
    }
    static long long int KthStatistics(std::shared_ptr<Treap> t, long long int k) {
        std::shared_ptr<Treap> cur(t);
        while (cur != nullptr) {
            long long int left_size = Size(cur->left);
            if (left_size == k) {
                return cur->x;
            }
            cur = left_size > k ? cur->left : cur->right;
            if (left_size < k) {
                k -= left_size + 1;
            }
        }
        throw std::runtime_error(":(");
    }
    Treap(long long int _x, long long int _y, std::shared_ptr<Treap> _left = nullptr, std::shared_ptr<Treap> _right = nullptr) :
        x(_x), y(_y), left(_left), right(_right) {
        Recalculate();
    };
    void Recalculate() {
        long long int left_sum = left ? left->sum : 0;
        long long int right_sum = right ? right->sum : 0;
        sum = x + left_sum + right_sum;
        size = Size(left) + Size(right) + 1;
    }
};

long long int CalculateCost(std::shared_ptr<Treap> t, long long int w) {
    long long int median = Treap::KthStatistics(t, (t->size - 1) / 2);
    auto res = t->Split(median);
    long long int sum_left = (std::get<0>(res) == nullptr) ? 0 : std::get<0>(res)->sum;
    long long int sum_right = (std::get<1>(res) == nullptr) ? 0 : std::get<1>(res)->sum;
    long long int size_left = (std::get<0>(res) == nullptr) ? 0 : std::get<0>(res)->size;
    long long int size_right = (std::get<1>(res) == nullptr) ? 0 : std::get<1>(res)->size;
    return (median * size_left - sum_left) + (sum_right - median * size_right);
}

int main() {
    srand(time(0));
    long long int n, w, next, min_cost;
    std::cin >> n >> w;
    std::shared_ptr<Treap> treap = nullptr;
    std::queue<long long int> fence;
    for (long long int i = 0; i < w; i++) {
        std::cin >> next;
        Treap::Add(treap, next);
        fence.push(next);
    }
    min_cost = CalculateCost(treap, w);
    for (long long int i = w; i < n; i++) {
        Treap::Delete(treap, fence.front());
        fence.pop();
        std::cin >> next;
        Treap::Add(treap, next);
        fence.push(next);
        long long int cost = CalculateCost(treap, w);
        if (cost < min_cost) {
            min_cost = cost;
        }
    }
    std::cout << min_cost;
    return 0;
}