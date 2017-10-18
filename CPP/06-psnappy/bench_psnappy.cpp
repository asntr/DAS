#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "psnappy.h"

const std::vector<std::string> kTestDictionary = {
    "the",  "be",    "to",    "of",   "and",    "a",       "in",    "that",
    "have", "I",     "it",    "for",  "not",    "on",      "with",  "he",
    "as",   "you",   "do",    "at",   "this",   "but",     "his",   "by",
    "from", "they",  "we",    "say",  "her",    "she",     "or",    "an",
    "will", "my",    "one",   "all",  "would",  "there",   "their", "what",
    "so",   "up",    "out",   "if",   "about",  "who",     "get",   "which",
    "go",   "me",    "when",  "make", "can",    "like",    "time",  "no",
    "just", "him",   "know",  "take", "person", "into",    "year",  "your",
    "good", "some",  "could", "them", "see",    "other",   "than",  "then",
    "now",  "look",  "only",  "come", "its",    "over",    "think", "also",
    "back", "after", "use",   "two",  "how",    "our",     "work",  "first",
    "well", "way",   "even",  "new",  "want",   "because", "any",   "these",
    "give", "day",   "most",  "us",
};

std::vector<char> MakeRandomWords(size_t size) {
  std::vector<char> random_words;

  std::default_random_engine engine(42 + size);
  std::uniform_int_distribution<int> dist(0, kTestDictionary.size() - 1);
  while (random_words.size() < size) {
    auto word = kTestDictionary[dist(engine)];

    for (size_t i = 0; i < word.size(); ++i) {
      random_words.push_back(word[i]);
      if (random_words.size() >= size) break;
    }

    if (random_words.size() < size) {
      random_words.push_back(' ');
    }
  }
  return random_words;
}

std::vector<char> MakeZeroes(size_t size) {
  return std::vector<char>(size, '\0');
}

static void BM_CompressionSpeed(
    benchmark::State& state,
    std::function<std::vector<char>(size_t)> gen_input) {
  std::vector<char> data = gen_input(state.range(0));
  std::vector<char> compressed;

  while (state.KeepRunning()) {
    compressed = Compress(data);
  }

  state.SetBytesProcessed(state.iterations() * state.range(0));
  state.SetLabel("compression:" +
                 std::to_string(100 * compressed.size() / data.size()) + "%");
}

static void BM_UncompressionSpeed(
    benchmark::State& state,
    std::function<std::vector<char>(size_t)> gen_input) {
  std::vector<char> data = gen_input(state.range(0));
  std::vector<char> compressed = Compress(data);

  while (state.KeepRunning()) {
    Uncompress(compressed);
  }

  state.SetBytesProcessed(state.iterations() * state.range(0));
}

BENCHMARK_CAPTURE(BM_CompressionSpeed, RandomWords, MakeRandomWords)
    ->Range(1024, 16 * 1024 * 1024);

BENCHMARK_CAPTURE(BM_CompressionSpeed, Zeroes, MakeZeroes)
    ->Range(1024, 16 * 1024 * 1024);

BENCHMARK_CAPTURE(BM_UncompressionSpeed, RandomWords, MakeRandomWords)
    ->Range(1024, 16 * 1024 * 1024);

BENCHMARK_CAPTURE(BM_UncompressionSpeed, Zeroes, MakeZeroes)
    ->Range(1024, 16 * 1024 * 1024);

BENCHMARK_MAIN();
