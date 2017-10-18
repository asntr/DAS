#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <reduce.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "commons.h"

const int MAX_SIZE = 1000 * 1000 * 100;

uint32_t gcd(uint32_t a, uint32_t b) {
    return !b ? a : gcd(b, a % b);
}

const std::vector<uint32_t> test(gen_test(MAX_SIZE));
const uint32_t ok_result = std::accumulate(test.begin(), test.end(), 0u, gcd);

void run(benchmark::State& state) {
    while (state.KeepRunning()) {
        auto result = reduce(test.begin(), test.end(), 0u, gcd);
        if (result != ok_result)
            state.SkipWithError("Incorrect reduce result");
    }
}

BENCHMARK(run)->Unit(benchmark::kMillisecond)->UseRealTime();

BENCHMARK_MAIN();
