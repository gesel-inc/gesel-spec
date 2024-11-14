#ifndef GESEL_UTILS_HPP
#define GESEL_UTILS_HPP

#include <string>
#include <limits>
#include <cstdint>

namespace gesel {

namespace internal {

inline std::string append_line_number(int32_t line) {
    if (line == std::numeric_limits<int32_t>::max()) {
        return " (line 2147483648)";
    } else {
        return " (line " + std::to_string(line + 1) + ")";
    }
}
    
}

}

#endif
