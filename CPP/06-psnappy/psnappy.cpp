#include <psnappy.h>

#include <stdexcept>

std::vector<char> Compress(const std::vector<char>& input) {
  std::vector<char> output(snappy::MaxCompressedLength(input.size()));

  size_t compressed_length;
  snappy::RawCompress(input.data(), input.size(), output.data(),
                      &compressed_length);
  output.resize(compressed_length);
  return output;
}

std::vector<char> Uncompress(const std::vector<char>& input) {
  size_t uncompressed_length;
  snappy::GetUncompressedLength(input.data(), input.size(),
                                &uncompressed_length);

  std::vector<char> output(uncompressed_length);
  snappy::RawUncompress(input.data(), input.size(), output.data());
  return output;
}
