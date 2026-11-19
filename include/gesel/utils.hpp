#ifndef GESEL_UTILS_HPP
#define GESEL_UTILS_HPP

#include <string>
#include <limits>
#include <tuple>
#include <cstdint>
#include <type_traits>

#include "byteme/byteme.hpp"
#include "sanisizer/sanisizer.hpp"

namespace gesel {

namespace internal {

inline std::string append_line_number(std::uint64_t line) {
    if (line == std::numeric_limits<std::uint64_t>::max()) {
        return " (line 18446744073709551616)";
    } else {
        return " (line " + std::to_string(line + 1) + ")";
    }
}

inline bool invalid_token_character(char x) {
    return (x < 'a' || x > 'z') && (x < '0' || x > '9') && (x != '-');
}

template<typename Input_>
using I = std::remove_cv_t<std::remove_reference_t<Input_> >;

template<typename Integer_>
bool add_next_digit(Integer_& current, Integer_ delta) {
    constexpr Integer_ div_limit = std::numeric_limits<Integer_>::max() / 10;
    constexpr Integer_ add_limit = std::numeric_limits<Integer_>::max() % 10;

    if (current >= div_limit && (current > div_limit || delta > add_limit)) { // most of the time we should fail the first condition.
        return false;
    }
    
    current *= 10;
    current += delta;
    return true;
}

}

}

#endif
