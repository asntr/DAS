#include <iostream>
#include <vector>
#include <cmath>

template<typename T>
void LongSwap(std::vector<T> &v, int start_1, int start_2, int len) {
    for (int i = 0; i < len; i++) {
        std::swap(v[start_1++], v[start_2++]);
    }
}

template<typename T>
void MergeTo(std::vector<T> &v, int left_1, int right_1, int left_2, int right_2, int start_to_write) {
    if(left_1 == left_2 && right_1 == right_2) {
        std::swap(v[left_1], v[start_to_write]);
        return;
    }
    int it1 = left_1, it2 = left_2, it3 = start_to_write;
    while (it1 <= right_1 || it2 <= right_2) {
        if (it1 > right_1) {
            std::swap(v[it3++], v[it2++]);
        } else if (it2 > right_2) {
            std::swap(v[it3++], v[it1++]);
        } else {
            if (v[it1] < v[it2]) {
                std::swap(v[it3++], v[it1++]);
            } else {
                std::swap(v[it3++], v[it2++]);
            }
        }
    }
}

template<typename T>
void SortHalf(std::vector<T> &v, int left, int right) {
    int half_length = (right - left + 1) / 2;
    for (int i = right - half_length + 1, pow = 1; i <= right; i += pow, pow *= 2) {
        for (int j = right - half_length + 1; j <= right; j += 2 * pow) {
            MergeTo(v, j, std::min(j + pow - 1, right), std::min(j + pow, right), std::min(j + 2 * pow - 1, right), left + j - (right - half_length + 1));
        }
        LongSwap(v, left, right - half_length + 1, half_length);
    }
}

template<typename T>
void Sort(std::vector<T> &v, int left, int right) {
    if (left == right) {
        return;
    }
    SortHalf(v, left, right);
    int right_unsorted = (right + left) / 2;
    int half_length = (right_unsorted - left + 1) / 2;
    while (right_unsorted > 0) {
        SortHalf(v, left, right_unsorted);
        LongSwap(v, right_unsorted - half_length + 1, left, half_length);
        MergeTo(v, right_unsorted + 1, right, left, left + half_length - 1, right_unsorted - half_length + 1);
        right_unsorted /= 2;
        half_length = (right_unsorted - left + 1) / 2;
    }
    for (int i = left; v[i] > v[i + 1] && i < right; i++) {
        std::swap(v[i], v[i + 1]);
    }
}

int main() {
    int n, next;
    long long waiting_time = 0;
    std::vector<long long> v;
    std::cin >> n;
    for (int i = 0; i < n; i++) {
        std::cin >> next;
        v.push_back(next);
    }
    Sort<long long>(v, 0, v.size() - 1);
    for (int n = v.size(), i = n - 1; i >= 0; i--) {
        waiting_time += i * v[n - 1 - i];
    }
    std::cout << waiting_time << std::endl;
}