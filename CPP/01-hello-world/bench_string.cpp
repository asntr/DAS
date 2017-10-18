#include <benchmark/benchmark.h>

#include "string_util.h"

static void BM_StringContains_SmallTest(benchmark::State& state) {
    std::string str1 = "abc", str2 = "b";

    while (state.KeepRunning())
        benchmark::DoNotOptimize(Contains(str1, str2));
}

BENCHMARK(BM_StringContains_SmallTest);

BENCHMARK_MAIN();