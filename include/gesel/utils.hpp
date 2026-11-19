#ifndef GESEL_UTILS_HPP
#define GESEL_UTILS_HPP

#include <string>
#include <limits>
#include <tuple>
#include <cstdint>
#include <type_traits>
#include <algorithm>

#include "byteme/byteme.hpp"
#include "sanisizer/sanisizer.hpp"

namespace gesel {

namespace internal {

inline std::string to_string(std::uint64_t line) {
    // Custom-rolled version of std::to_string() that avoids locale changes.
    // It also works if uint64_t isn't an alias for the standard integer types.
    if (line == 0) {
        return "0";
    }
    std::string output;
    while (line) {
        output += static_cast<char>((line % 10) + '0');
        line /= 10;
    }
    std::reverse(output.begin(), output.end());
    return output;
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
        throw std::overflow_error("integer overflow");
    }
    
    current *= 10;
    current += delta;
    return true;
}

}

}

#endif
