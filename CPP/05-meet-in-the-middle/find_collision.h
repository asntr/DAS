#pragma once

#include <string>
#include "hasher.h"

std::string find_collision(const std::string&, const Hasher& hasher);
