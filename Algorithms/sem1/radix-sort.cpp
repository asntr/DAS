#include <vector>
#include <iostream>

void FillMatrix(int n, int a, int b, int c, int d0, std::vector<std::vector<int>>& matrix) {
    matrix[0][0] = d0;
    int prev = d0;
    for (int i = 1; i < n * n; i++) {
        int row = i / n;
        int column = i - n * row;
        prev = (prev * a + b) % c;
        matrix[row][column] = prev;
    }
}

std::vector<int> CountingSort(int n, int c, int digit, const std::vector<std::vector<int>>& matrix, const std::vector<int> old_permutation) {
    std::vector<int> permutation(n);
    std::vector<int> counter(c, 0);
    std::vector<int> start_positions(c, 0);
    for (int i : old_permutation) {
        counter[matrix[i][digit]]++;
    }
    for (int i = 1; i < c; i++) {
        start_positions[i] = start_positions[i - 1] + counter[i - 1];
    }
    for (int i : old_permutation) {
        int digit_value = matrix[i][digit];
        permutation[start_positions[digit_value]++] = i;
    }
    return permutation;
}

std::vector<int> RadixSort(int n, int c, std::vector<std::vector<int>>& matrix) {
    std::vector<int> permutation(n);
    for (int i = 0; i < n; i++) {
        permutation[i] = i;
    }

    for (int i = n - 1; i >= 0; i--) {
        permutation = CountingSort(n, c, i, matrix, permutation);
    }

    return permutation;
}

int main() {
    int n, a, b, c, d0;
    std::cin >> n >> a >> b >> c >> d0;
    std::vector<std::vector<int>> matrix(n);
    for (int i = 0; i < n; i++) {
        matrix[i].resize(n);
    }
    FillMatrix(n, a, b, c, d0, matrix);
    auto final_permutation = RadixSort(n, c, matrix);
    for (int i : final_permutation) {
        std::cout << i << ' ';
    }
    return 0;
}