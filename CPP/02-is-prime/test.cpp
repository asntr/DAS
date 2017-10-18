#include <iostream>
#include <gtest/gtest.h>
#include "is_prime.h"

TEST(basic_tests, just_test) {
    ASSERT_TRUE(is_prime(2));
    ASSERT_FALSE(is_prime(1));
    ASSERT_FALSE(is_prime(0));
    ASSERT_TRUE(is_prime(17239));
    ASSERT_FALSE(is_prime(9));

    int mul = 100 * 100 * 100 + 3;
    ASSERT_TRUE(is_prime(mul));
    ASSERT_FALSE(is_prime(static_cast<uint64_t> (mul) * mul));
}
