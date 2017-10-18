#include <iostream>
#include <vector>
#include <algorithm>

std::vector<int> convert_prefix_to_z(const std::vector<int>& p, int n) {
    std::vector<int> z(n);
    for (int i = 1; i < n; ++i) {
        if (p[i] > 0) z[i - p[i] + 1] = p[i];
    }
    z[0] = n;
    for (int i = 1; i < n;) {
        int t = i;
        if (z[i] > 0) {
            for (int j = 0; j < z[i]; ++j) {
                if (z[i + j] > z[j]) break;
                z[i + j] = std::min(z[j], z[i] - j);
                t = i + j;
            }
        }
        i = t + 1;
    }
    z[0] = 0;
    return z;
}

int main() {
    int n;
    std::vector<int> p;
    std::cin >> n;
    p.resize(n);
    for (int i = 0; i < n; ++i) {
        std::cin >> p[i];
    }
    auto z = convert_prefix_to_z(p, n);
    for (int i : z) {
        std::cout << i << " ";
    }
    return 0;
}