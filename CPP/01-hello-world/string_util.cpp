#include "string_util.h"

#include <stdexcept>


bool Contains(const std::string& haystack, const std::string& needle) {
    return (haystack.find(needle) != std::string::npos);
}
