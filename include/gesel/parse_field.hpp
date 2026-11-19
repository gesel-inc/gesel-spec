#ifndef GESEL_PARSE_INTEGER_FIELD_HPP
#define GESEL_PARSE_INTEGER_FIELD_HPP

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>

#include "utils.hpp"

namespace gesel {

namespace internal {

enum class FieldType : char { LAST, MIDDLE, UNKNOWN }; 

template<FieldType type_, class ByteSource_>
typename std::conditional<type_ == FieldType::UNKNOWN, std::pair<std::uint64_t, bool>, std::uint64_t>::type
parse_integer_field(ByteSource_& pb, bool& valid) {
    std::uint64_t number = 0;
    std::uint64_t ndigits = 0;
    std::uint64_t has_leading_zero = false;
    bool terminated = false;

    do {
        char c = pb.get();
        valid = pb.advance();

        if constexpr(type_ == FieldType::UNKNOWN || type_ == FieldType::LAST) {
            if (c == '\n') {
                terminated = true;
                break;
            }
        }
        if constexpr(type_ == FieldType::UNKNOWN || type_ == FieldType::MIDDLE) {
            if (c == '\t') {
                break;
            }
        }

        if (c < '0' || c > '9') {
            throw std::runtime_error("detected non-digit character");
        } else if (!valid) {
            throw std::runtime_error("no terminating newline");
        }

        const std::uint64_t delta = c - '0';
        add_next_digit(number, delta);
        has_leading_zero += (ndigits == 0 && delta == 0);
        ++ndigits;
    } while (valid);

    if (number == 0) {
        if (ndigits > 1) {
            throw std::runtime_error("detected leading zero");
        } else if (ndigits == 0) {
            throw std::runtime_error("detected empty field");
        }
    } else if (has_leading_zero) {
        throw std::runtime_error("detected leading zero");
    }

    if constexpr(type_ == FieldType::UNKNOWN) {
        return std::make_pair(number, terminated);
    } else {
        return number;
    }
}

template<FieldType type_, class ByteSource_>
typename std::conditional<type_ == FieldType::UNKNOWN, std::pair<std::string, bool>, std::string>::type
parse_string_field(ByteSource_& pb, bool& valid) {
    std::string value;
    bool terminated = false;

    do {
        char c = pb.get();
        valid = pb.advance();

        if constexpr(type_ == FieldType::UNKNOWN || type_ == FieldType::LAST) {
            if (c == '\n') {
                terminated = true;
                break;
            }
        }
        if constexpr(type_ == FieldType::UNKNOWN || type_ == FieldType::MIDDLE) {
            if (c == '\t') {
                break;
            }
        }

        if (c == '\t' || c == '\n') {
            throw std::runtime_error("string containing a newline or tab");
        } else if (!valid) {
            throw std::runtime_error("no terminating newline");
        }

        value += c;
    } while (valid);

    if constexpr(type_ == FieldType::UNKNOWN) {
        return std::make_pair(value, terminated);
    } else {
        return value;
    }
}

}

}

#endif
