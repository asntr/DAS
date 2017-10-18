#include <gtest/gtest.h>

#include "string_util.h"

TEST(Contains, Simple) {
    ASSERT_TRUE(Contains("abc", "a"));
    ASSERT_TRUE(Contains("", ""));

    ASSERT_TRUE(Contains("abc", "abc"));

    ASSERT_FALSE(Contains("", "a"));
    ASSERT_FALSE(Contains("abacaba", "abc"));
}

TEST(Contains, Big) {
    std::string haystack(1024 * 1024, 'a');
    haystack = haystack + "test" + haystack;

    ASSERT_TRUE(Contains(haystack, "test"));
}