#include <cstdint>
#include <array>
#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include "is_prime.h"

const std::array<uint64_t, 4> numbers = {1000000000000000177ull, 1000000000375847551ull,
                                         3778118040573702001ull,
                                         1000ull * 1000 * 1000 * 1000 * 1000 * 1000};

const std::array<bool, 4> results = {true, true, false, false};

void check_numbers(benchmark::State& state) {
    while (state.KeepRunning()) {
        uint64_t num = numbers[state.range(0)];
        if (is_prime(num) != results[state.range(0)]) {
            state.SkipWithError("Invalid prime check");
        }
    }
}

BENCHMARK(check_numbers)
    ->DenseRange(0, static_cast<int> (numbers.size()) - 1)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

BENCHMARK_MAIN();
