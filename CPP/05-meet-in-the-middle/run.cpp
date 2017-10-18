#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <random>
#include <string>
#include "params.h"
#include "commons.h"
#include "hasher.h"
#include "find_collision.h"

void run(benchmark::State& state) {
    std::mt19937 gen(SEED + 17239);
    while (state.KeepRunning()) {
        int64_t modulo = get_random_modulo(BENCH_MODULES, gen);
        int multiplier = get_random_multiplier(gen);
        Hasher hasher(modulo, multiplier);
        std::string first_string = get_random_string(MAX_LEN, gen);
        std::string second_string = find_collision(first_string, hasher);
        if (first_string == second_string || second_string.size() > (size_t)MAX_LEN ||
            hasher(second_string) != hasher(first_string) || !is_correct_string(second_string)) {
            state.SkipWithError("Incorrect string has been found");
        }
    }
}

BENCHMARK(run)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
