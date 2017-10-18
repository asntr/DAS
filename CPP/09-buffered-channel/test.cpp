#include <buffered_channel.h>
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <chrono>
#include <algorithm>

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

void run(int senders_count, int receivers_count, int buff_size, int close_limit) {
    std::vector<std::thread> threads;
    threads.reserve(senders_count + receivers_count);
    BufferedChannel<int> channel(buff_size);
    std::atomic<int> counter(0);
    std::vector<std::vector<int>> send_values(senders_count);
    bool was_closed = false;
    for (int i = 0; i < senders_count; ++i)
        threads.emplace_back([&channel, &counter, &send_values, &was_closed, i]() {
            for (;;) {
                int value = std::atomic_fetch_add(&counter, 1);
                try {
                    channel.send(value);
                    send_values[i].push_back(value);
                } catch (std::runtime_error&) {
                    ASSERT_TRUE(was_closed);
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
                    ASSERT_TRUE(was_closed);
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
    run(1, 1, 1, 300);
}

TEST(Correctness, Senders) {
    run(4, 1, 3, 300);
}

TEST(Correctness, Receivers) {
    run(1, 6, 4, 300);
}

TEST(Correctness, SmallBuf) {
    run(8, 2, 2, 300);
}

TEST(Correctness, BigBuf) {
    run(3, 3, 100, 300);
}

TEST(Correctness, Random) {
    run(8, 8, 8, 300);
}
