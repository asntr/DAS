#include <benchmark/benchmark.h>
#include <benchmark/benchmark_api.h>
#include <buffered_channel.h>

#include <thread>
#include <atomic>
#include <vector>

const int count = 5e5;

void send(int threads_count, BufferedChannel<int>& channel) {
    auto func = [&channel, threads_count](int start) {
        for (int i = start; i < count; i += threads_count)
            channel.send(i);
    };
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    for (int i = 0; i < threads_count; ++i)
        threads.emplace_back(func, i);
    for (auto& thread : threads)
        thread.join();
    channel.close();
}

int64_t calc_sum(int senders_count, int readers_count, int buff_size) {
    BufferedChannel<int> channel(buff_size);
    std::thread sender_thread(send, senders_count, std::ref(channel));
    std::vector<std::thread> threads;
    threads.reserve(readers_count);
    std::atomic<int64_t> sum(0);
    for (int i = 0; i < readers_count; ++i)
        threads.emplace_back([&channel, &sum]() {
            for (;;) {
                auto value = channel.recv();
                if (!value.second)
                    break;
                sum += value.first;
            }
        });

    for (auto& cur : threads)
        cur.join();

    sender_thread.join();

    return sum;
}

void run(benchmark::State& state) {
    int64_t ok_ans = static_cast<int64_t> (count) * (count - 1) / 2;
    while (state.KeepRunning()) {
        int buff_size = state.range(0);
        int senders_count = state.range(1);
        int readers_count = state.range(2);
        if (ok_ans != calc_sum(senders_count, readers_count, buff_size))
            state.SkipWithError("Wrong sum");
    }
}

const int threads = std::thread::hardware_concurrency();
const int half = std::max(1, threads / 2);
const int another_half = std::max(1, threads - half);

BENCHMARK(run)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Args({2, 2, std::max(1, threads - 2)})
    ->Args({10, half, another_half})
    ->Args({100000, half, another_half})
    ->MinTime(-1.0);

BENCHMARK_MAIN();
