#include <string>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>
#include <functional>
#include <stdexcept>
#include <queue>
#include <algorithm>
#include <thread>
#include <gtest/gtest.h>
#include <concurrent_hash_map.h>

#include "commons.h"

using std::string;

const int TEST_MAX_ITERATIONS = 10000;
const int SEED = 723834;

size_t pair_hash(const std::pair<int, int>& x) {
    return 17239u * x.first + x.second;
}

TEST(Correctness, Constructors) {
    ConcurrentHashMap<int, int> first_table;
    ConcurrentHashMap<string, string> second_table(5, 16);
    ConcurrentHashMap<int, long long> third_table(17);

    ConcurrentHashMap<
        std::pair<int, int>, int, size_t(*)(const std::pair<int, int>&)
    > pair_table(pair_hash);
    ASSERT_TRUE(pair_table.insert(std::make_pair(1, 1), 2));
    ASSERT_EQ(pair_table.size(), 1u);

    auto lambda = [](const std::pair<int, int>& x) {
        return pair_hash(x);
    };
    ConcurrentHashMap<
        std::pair<int, int>, string, decltype(lambda)
    > new_pair_table(100, 1, lambda);
    ASSERT_TRUE(new_pair_table.insert(std::make_pair(1, 2), "string"));
    ASSERT_EQ(new_pair_table.size(), 1u);
}

TEST(Correctness, Operations) {
    ConcurrentHashMap<int, int> table;
    ASSERT_TRUE(table.insert(3, 1));
    ASSERT_TRUE(table.insert(2, 2));
    ASSERT_FALSE(table.insert(2, 1));
    ASSERT_EQ(2, table.find(2).second);
    ASSERT_FALSE(table.find(5).first);
    ASSERT_EQ(2u, table.size());
    table.insert(5, 5);
    ASSERT_EQ(5, table.find(5).second);

    ASSERT_TRUE(table.erase(5));
    ASSERT_EQ(2u, table.size());
    auto tmp = table.find(2);
    ASSERT_TRUE(tmp.first);
    auto some_search = table.find(5);
    ASSERT_FALSE(some_search.first);
    table.clear();
    ASSERT_EQ(0u, table.size());
    table.insert(7, 3);
    ASSERT_EQ(std::make_pair(true, 3), table.find(7));
}

TEST(Correctness, Constness) {
    ConcurrentHashMap<int, int> table;
    table.insert(1, 1);
    table.insert(2, 2);
    table.insert(3, 3);
    const auto& ref = table;
    ASSERT_EQ(std::make_pair(true, 3), ref.find(3));
    ASSERT_FALSE(ref.find(4).first);
    ASSERT_EQ(1, ref.at(1));
    ASSERT_THROW(ref.at(0), std::out_of_range);
}

void check_output(const ConcurrentHashMap<int, int>& table,
              std::vector<std::vector<int>> queries) {
    struct Item {
        int value;
        int thread_num;

        bool operator <(const Item& item) const {
            return value < item.value;
        }
    };
    std::priority_queue<Item> queue;
    int it = 0;
    for (auto& cur_queries : queries) {
        std::sort(cur_queries.begin(), cur_queries.end());
        queue.push(Item{cur_queries.back(), it++});
    }
    int unique_size = 0;
    int previous_value;
    bool is_first = true;
    while (!queue.empty()) {
        auto top = queue.top();
        queue.pop();
        if (is_first || top.value != previous_value)
            ++unique_size;
        is_first = false;
        previous_value = top.value;
        ASSERT_TRUE(table.find(top.value).first);
        queries[top.thread_num].pop_back();
        if (!queries[top.thread_num].empty())
            queue.push(Item{queries[top.thread_num].back(), top.thread_num});
    }
    ASSERT_EQ(unique_size, table.size());
}

TEST(Concurrency, Insertions) {
    const int threads_count = 4;
    const int query_count = TEST_MAX_ITERATIONS;
    ConcurrentHashMap<int, int> table;
    std::vector<SimpleLogger> queries(threads_count);
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    for (int i = 0; i < threads_count; ++i)
        threads.emplace_back(make_queries<Random, SimpleLogger>, std::ref(table),
                             std::ref(queries[i]),
                             query_count, QueryType::INSERT, Random(SEED + i));
    for (int i = 0; i < threads_count; ++i)
        threads[i].join();

    check_output(table, move_to_vectors(std::move(queries)));
}

TEST(Concurrency, Searching) {
    const int threads_count = 4;
    const int query_count = TEST_MAX_ITERATIONS;
    ConcurrentHashMap<int, int> table;
    std::vector<SimpleLogger> queries(threads_count / 2);
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    DummyLogger dummy;
    threads.emplace_back(make_queries<Random, SimpleLogger>, std::ref(table), std::ref(queries[0]),
                         query_count, QueryType::INSERT, Random(SEED - 1));
    threads.emplace_back(make_queries<Random, DummyLogger>, std::ref(table), std::ref(dummy),
                         query_count, QueryType::FIND, Random(SEED - 2));
    threads.emplace_back(make_queries<Increment, SimpleLogger>, std::ref(table),
                         std::ref(queries[1]),
                         query_count, QueryType::INSERT, Increment(-1));
    threads.emplace_back(make_queries<Increment, DummyLogger>, std::ref(table), std::ref(dummy),
                         query_count, QueryType::FIND, Increment(0));

    for (int i = 0; i < threads_count; ++i)
        threads[i].join();

    check_output(table, move_to_vectors(std::move(queries)));
}

TEST(Concurrency, Erasing) {
    const int threads_count = 8;
    const int query_count = 1000;
    ConcurrentHashMap<int, int> table;
    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    DummyLogger dummy;
    for (int i = 0; i < 2; ++i)
        threads.emplace_back(make_queries<Random, DummyLogger>, std::ref(table), std::ref(dummy),
                             query_count, QueryType::INSERT, Random(SEED + 100 * (i + 1)));
    for (int i = 0; i < 2; ++i)
        threads.emplace_back(make_queries<Random, DummyLogger>, std::ref(table), std::ref(dummy),
                             query_count, QueryType::ERASE, Random(SEED + 100 * (i + 1)));
    threads.emplace_back(make_queries<Increment, DummyLogger>, std::ref(table), std::ref(dummy),
                         query_count, QueryType::INSERT, Increment(100));
    threads.emplace_back(make_queries<Increment, DummyLogger>, std::ref(table), std::ref(dummy),
                         query_count, QueryType::ERASE, Increment(-100));
    for (int i = 0; i < 2; ++i)
        threads.emplace_back(make_queries<Random, DummyLogger>, std::ref(table), std::ref(dummy),
                             query_count, QueryType::FIND, Random(19, 100, 200));

    for (int i = 0; i < threads_count; ++i)
        threads[i].join();
}

TEST(Concurrency, Clear) {
    const int query_count = TEST_MAX_ITERATIONS / 2;
    DummyLogger dummy;
    ConcurrentHashMap<int, int> table;
    std::thread insert(make_queries<Random, DummyLogger>, std::ref(table), std::ref(dummy),
                       query_count, QueryType::INSERT, Random(SEED + 17239));
    std::thread clear(make_queries<Increment, DummyLogger>, std::ref(table), std::ref(dummy),
                      query_count, QueryType::CLEAR, Increment(0));
    insert.join();
    clear.join();
}

template<class Func>
void make_queries_and_check(ConcurrentHashMap<int, int>& table, int query_count, int seed,
                            Func func, std::unordered_set<int>& result_set) {
    result_set.reserve(query_count);
    std::vector<int> inserted;
    inserted.reserve(query_count);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist;
    for (int i = 0; i < query_count; ++i) {
        double prob = dist(gen);
        if (prob < 0.6) {
            int value = func();
            inserted.push_back(value);
            table.insert(value, 1);
            result_set.insert(value);
        } else if (prob < 0.8 && !inserted.empty()) {
            std::uniform_int_distribution<size_t> index(0, inserted.size() - 1);
            int value = inserted[index(gen)];
            table.erase(value);
            result_set.erase(value);
        } else if (!inserted.empty()) {
            std::uniform_int_distribution<size_t> index(0, inserted.size() - 1);
            int value = inserted[index(gen)];
            ASSERT_EQ(result_set.count(value), table.find(value).first);
        }
    }
}

TEST(Concurrency, OneThreadInteraction) {
    const int query_count = TEST_MAX_ITERATIONS;
    const int threads_count = 4;
    const int window = query_count * 100;

    std::vector<std::thread> threads;
    threads.reserve(threads_count);
    ConcurrentHashMap<int, int> table;
    std::vector<std::unordered_set<int>> results(threads_count);
    for (int i = 0; i < threads_count; ++i)
        threads.emplace_back(make_queries_and_check<Random>, std::ref(table), query_count,
                             SEED + (i + 1) * 10,
                             Random(SEED - (i + 1) * 100, i * window + 1, (i + 1) * window),
                             std::ref(results[i]));

    for (int i = 0; i < threads_count; ++i)
        threads[i].join();

    size_t total_size = 0;
    for (const auto& set : results) {
        total_size += set.size();
        for (int x : set)
            ASSERT_TRUE(table.find(x).first);
    }
    ASSERT_EQ(total_size, table.size());
}
