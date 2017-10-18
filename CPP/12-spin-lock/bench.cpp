#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <spin_lock.h>
#include <memory>

int counter;
std::shared_ptr<SpinLock> spin_lock;

void run(benchmark::State& state) {
    if (state.thread_index == 0) {
        counter = 0;
        spin_lock = std::make_shared<SpinLock>();
    }
    while (state.KeepRunning()) {
        spin_lock->lock();
        ++counter;
        spin_lock->unlock();
    }
}

BENCHMARK(run)->UseRealTime()->Threads(1)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK_MAIN();
