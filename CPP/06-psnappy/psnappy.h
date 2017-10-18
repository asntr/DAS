#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <snappy.h>

std::vector<char> Compress(const std::vector<char>& input);
std::vector<char> Uncompress(const std::vector<char>& input);
