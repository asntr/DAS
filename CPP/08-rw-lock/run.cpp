#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <rw_lock.h>

#include <vector>
#include <thread>

int64_t mul_mod(int64_t a, int64_t b, int64_t M) {
    int64_t res = 0;
    while (a > 0) {
        if (a & 1) {
            res += b;
            if (res >= M)
                res -= M;
        }
        a >>= 1;
        b <<= 1;
        if (b >= M)
            b -= M;
    }
    return res;
}

int64_t bin_pow(int64_t a, int64_t b, int64_t c) {
    if (!b)
        return 1;
    else if (b % 2 == 0) {
        int64_t result = bin_pow(a, b / 2, c);
        return mul_mod(result, result, c);
    } else
        return mul_mod(a, bin_pow(a, b - 1, c), c);
}

std::unique_ptr<RWLock> rw_lock;
std::unique_ptr<int64_t> counter;

void run_bench(benchmark::State& state, int read_count) {
    if (state.thread_index == 0) {
        rw_lock.reset(new RWLock());
        counter.reset(new int64_t(0));
    }
    const int64_t prime = 100000000000000003ll;
    while (state.KeepRunning()) {
        if (state.thread_index < read_count)
            rw_lock->read([&state, prime]() {
                if (*counter > 0 && bin_pow(*counter, prime - 1, prime) != 1)
                    state.SkipWithError("Fermat's little theorem is wrong?");
            });
        else
            rw_lock->write([]() {
                ++(*counter);
            });
    }
}

void half(benchmark::State& state) {
    run_bench(state, state.threads / 2);
}

void read_only(benchmark::State& state) {
    run_bench(state, state.threads);
}

void reads(benchmark::State& state) {
    run_bench(state, std::max(2, state.threads - 2));
}

const int threads = std::thread::hardware_concurrency();

BENCHMARK(half)->Threads(threads)->UseRealTime();
BENCHMARK(read_only)->Threads(threads)->UseRealTime();
BENCHMARK(reads)->Threads(threads)->UseRealTime();

BENCHMARK_MAIN();
