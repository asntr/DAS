#include <gtest/gtest.h>

#include <random>

#include <psnappy.h>

void TestCompression(const std::vector<char>& data) {
  auto compressed = Compress(data);
  auto uncompressed = Uncompress(compressed);
  ASSERT_EQ(data.size(), uncompressed.size());
  ASSERT_EQ(data, uncompressed);
}

TEST(PSnappy, Empty) { TestCompression({}); }

TEST(PSnappy, Small) {
  std::vector<char> small = {'a', 'b', 'c', 'd', 'e'};
  TestCompression(small);
}

TEST(PSnappy, ManyZeroes) {
  std::vector<char> zeroes(1024 * 1024 * 64, '\0');
  TestCompression(zeroes);
}

std::vector<char> MakeRandom(int size) {
  std::vector<char> random_symbols;

  std::default_random_engine engine(42 + size);
  std::uniform_int_distribution<char> dist('a', 'z');
  for (int i = 0; i < size; ++i) {
    random_symbols.push_back(dist(engine));
  }
  return random_symbols;
}

TEST(PSnappy, ManyRandom) { TestCompression(MakeRandom(4 * 1024 * 1024)); }

TEST(PSnappy, TrickySizes) {
  for (size_t size : {1, 4, 7, 15, 31, 123, 1023, 1025, 6553, 1234}) {
    TestCompression(MakeRandom(size));
    TestCompression(MakeRandom(size * 1023 + size));
  }
}
