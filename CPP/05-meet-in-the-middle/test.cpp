#include <gtest/gtest.h>
#include <random>
#include "params.h"
#include "commons.h"
#include "find_collision.h"

TEST(Correctness, Simple) {
    std::mt19937 gen(SEED);
    for (int i = 0; i < 10; ++i) {
        int64_t modulo = get_random_modulo(TEST_MODULES, gen);
        int multiplier = get_random_multiplier(gen);
        Hasher hasher(modulo, multiplier);
        auto first_string = get_random_string(MAX_LEN, gen);
        auto second_string = find_collision(first_string, hasher);
        ASSERT_TRUE(is_correct_string(second_string));
        ASSERT_NE(first_string, second_string);
        ASSERT_EQ(hasher(first_string), hasher(second_string));
        ASSERT_LE(second_string.size(), MAX_LEN);
    }
}
