#include <unbuffered_channel.h>
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include <chrono>
#include <algorithm>


std::chrono::high_resolution_clock::time_point now() {
    return std::chrono::high_resolution_clock::now();
}

double elapsed_time(std::chrono::high_resolution_clock::time_point start) {
    using std::chrono::duration;
    duration<double> time_span = std::chrono::duration_cast<duration<double>>(now() - start);
    return time_span.count();
}

std::vector<int> unpack(const std::vector<std::vector<int>>& values) {
    std::vector<int> all;
    for (const auto& vector : values)
        all.insert(all.end(), vector.begin(), vector.end());
    return all;
}

void check_values(const std::vector<std::vector<int>>& send_values,
                  const std::vector<std::vector<int>>& recv_values) {
    auto all_send = unpack(send_values);
    auto all_recv = unpack(recv_values);
    std::sort(all_send.begin(), all_send.end());
    std::sort(all_recv.begin(), all_recv.end());
    ASSERT_EQ(all_send, all_recv);
}

void run(int senders_count, int receivers_count, int close_limit) {
    std::vector<std::thread> threads;
    threads.reserve(senders_count + receivers_count);
    UnbufferedChannel<int> channel;
    std::atomic<int> counter(0);
    std::vector<std::vector<int>> send_values(senders_count);
    std::atomic<bool> was_closed(false);
    for (int i = 0; i < senders_count; ++i)
        threads.emplace_back([&channel, &counter, &send_values, &was_closed, i]() {
            for (;;) {
                int value = std::atomic_fetch_add(&counter, 1);
                try {
                    channel.send(value);
                    send_values[i].push_back(value);
                } catch (std::runtime_error&) {
                    ASSERT_TRUE(was_closed.load());
                    break;
                }
            }
        });
    std::vector<std::vector<int>> recv_values(receivers_count);
    for (int i = 0; i < receivers_count; ++i)
        threads.emplace_back([&channel, &recv_values, &was_closed, i]() {
            for (;;) {
                auto value = channel.recv();
                if (!value.second) {
                    ASSERT_TRUE(was_closed.load());
                    break;
                }
                recv_values[i].push_back(value.first);
            }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(close_limit));
    was_closed = true;
    channel.close();

    for (auto& cur : threads)
        cur.join();

    check_values(send_values, recv_values);
    if (senders_count == 1 && receivers_count == 1)
        ASSERT_EQ(send_values[0], recv_values[0]);
}

TEST(Correctness, Simple) {
    run(1, 1, 200);
}

TEST(Correctness, Senders) {
    run(4, 1, 200);
}

TEST(Correctness, Receivers) {
    run(1, 6, 200);
}

TEST(Correctness, BigBuf) {
    run(3, 3, 200);
}

enum class BlockType {SENDER, RECEIVER};

void block_run(BlockType block_type) {
    UnbufferedChannel<int> channel;
    int time_limit = 400;
    int iterations = 3;
    std::thread sender([&channel, time_limit, iterations, block_type]() {
        for (int i = 0; i < iterations; ++i) {
            if (block_type == BlockType::RECEIVER)
                std::this_thread::sleep_for(std::chrono::milliseconds(time_limit));
            auto start = now();
            channel.send(i);
            int elapsed = static_cast<int> (elapsed_time(start) * 1000);
            if (block_type == BlockType::SENDER) {
                ASSERT_LT(time_limit - 100, elapsed);
                ASSERT_LT(elapsed, time_limit + 100);
            }
        }
        channel.close();
    });
    std::thread receiver([&channel, time_limit, iterations, block_type]() {
        for (int i = 0; i < iterations; ++i) {
            if (block_type == BlockType::SENDER)
                std::this_thread::sleep_for(std::chrono::milliseconds(time_limit));
            auto start = now();
            auto value = channel.recv();
            ASSERT_EQ(i, value.first);
            int elapsed = static_cast<int> (elapsed_time(start) * 1000);
            if (block_type == BlockType::RECEIVER) {
                ASSERT_LT(time_limit - 100, elapsed);
                ASSERT_LT(elapsed, time_limit + 100);
            }
        }
        ASSERT_FALSE(channel.recv().second);
    });

    sender.join();
    receiver.join();
}

TEST(Block, Sender) {
    block_run(BlockType::SENDER);
}

TEST(Block, Receiver) {
    block_run(BlockType::RECEIVER);
}
