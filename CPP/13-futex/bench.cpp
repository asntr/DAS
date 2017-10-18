#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <mutex.h>
#include <memory>

int counter;
std::shared_ptr<Mutex> mutex;

void run(benchmark::State& state) {
    if (state.thread_index == 0) {
        counter = 0;
        mutex = std::make_shared<Mutex>();
    }
    while (state.KeepRunning()) {
        mutex->lock();
        ++counter;
        mutex->unlock();
    }
}

BENCHMARK(run)->UseRealTime()->Threads(1)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK_MAIN();
