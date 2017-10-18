#include <thread>
#include <vector>

#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <concurrent_hash_map.h>

#include "commons.h"

std::unique_ptr<ConcurrentHashMap<int, int>> TEST_TABLE;

const int SEED = 82944584;
auto undefined_size = ConcurrentHashMap<int, int>::kUndefinedSize;

void random_insertions(benchmark::State& state) {
    if (state.thread_index == 0) {
        TEST_TABLE.reset(new ConcurrentHashMap<int, int>(undefined_size, state.threads));
    }

    Random random(SEED + state.thread_index * 10);
    while (state.KeepRunning()) {
        TEST_TABLE->insert(random(), 1);
    }

    if (state.thread_index == 0) {
        TEST_TABLE.reset();
    }
}


void special_insertions(benchmark::State& state) {
    if (state.thread_index == 0) {
        TEST_TABLE.reset(new ConcurrentHashMap<int, int>(undefined_size, state.threads));
    }

    uint32_t i = state.thread_index * 100000;
    EqualLowBits low_bits(16);
    while (state.KeepRunning()) {
        if (state.thread_index == 0) {
            TEST_TABLE->insert(low_bits(), 1);
        } else {
            TEST_TABLE->insert(i++, 1);
        }
    }

    if (state.thread_index == 0) {
        TEST_TABLE.reset();
    }
}

void many_searches(benchmark::State& state) {
    if (state.thread_index == 0) {
        TEST_TABLE.reset(new ConcurrentHashMap<int, int>(undefined_size, state.threads));
        DummyLogger logger;
        make_queries(*TEST_TABLE, logger, 100000, QueryType::INSERT, Increment(0));
    }

    Random rnd(SEED - state.thread_index - 1, 0, 1000000);
    while (state.KeepRunning()) {
        if (state.thread_index == 0) {
            TEST_TABLE->insert(rnd(), 1);
        } else {
            TEST_TABLE->find(rnd());
        }
    }

    if (state.thread_index == 0) {
        TEST_TABLE.reset();
    }
}

void deletions(benchmark::State& state) {
    if (state.thread_index == 0) {
        TEST_TABLE.reset(new ConcurrentHashMap<int, int>(undefined_size, state.threads));
    }

    Random rnd(SEED + state.thread_index / 2);
    while (state.KeepRunning()) {
        if (state.thread_index % 2 == 0) {
            TEST_TABLE->insert(rnd(), 1);
        } else {
            TEST_TABLE->erase(rnd());
        }
    }

    if (state.thread_index == 0) {
        TEST_TABLE.reset();
    }

}

BENCHMARK(random_insertions)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->UseRealTime();

BENCHMARK(special_insertions)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->UseRealTime();

BENCHMARK(many_searches)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->UseRealTime();

BENCHMARK(deletions)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->UseRealTime();

BENCHMARK_MAIN();
