#ifndef GESEL_UTILS_HPP
#define GESEL_UTILS_HPP

#include <string>
#include <limits>
#include <cstdint>

namespace gesel {

namespace internal {

inline std::string append_line_number(uint64_t line) {
    if (line == std::numeric_limits<uint64_t>::max()) {
        return " (line 18446744073709551616)";
    } else {
        return " (line " + std::to_string(line + 1) + ")";
    }
}
    
}

}

#endif
