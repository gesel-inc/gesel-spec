#ifndef GESEL_PARSE_INTEGER_FIELD_HPP
#define GESEL_PARSE_INTEGER_FIELD_HPP

#include <type_traits>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace gesel {

namespace internal {

enum class FieldType : char { LAST, MIDDLE, UNKNOWN }; 

template<FieldType type_, class ByteSource_>
typename std::conditional<type_ == FieldType::UNKNOWN, std::pair<int32_t, bool>, int32_t>::type
parse_integer_field(ByteSource_& pb, bool& valid, const std::string& path, int32_t line) {
    constexpr int32_t threshold = std::numeric_limits<int32_t>::max() / 10;
    constexpr int32_t max_remainder = std::numeric_limits<int32_t>::max() % 10;

    int32_t number = 0;
    int32_t ndigits = 0;
    int32_t has_leading_zero = false;
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
            throw std::runtime_error("non-digit character detected in '" + path + "' (line " + std::to_string(line) + ")");
        } else if (!valid) {
            throw std::runtime_error("no terminating newline in '" + path + "' (line " + std::to_string(line) + ")");
        }

        int32_t delta = c - '0';
        if (number == threshold && delta > max_remainder) {
            throw std::runtime_error("32-bit integer overflow in '" + path + "' (line " + std::to_string(line) + ")");
        }

        has_leading_zero += (ndigits == 0 && delta == 0);
        number *= 10;
        number += delta;
        ++ndigits;
    } while (valid);

    if (number == 0) {
        if (ndigits > 1) {
            throw std::runtime_error("leading zero detected in '" + path + "' (line " + std::to_string(line) + ")");
        }
    } else if (has_leading_zero) {
        throw std::runtime_error("leading zero detected in '" + path + "' (line " + std::to_string(line) + ")");
    }

    if constexpr(type_ == FieldType::UNKNOWN) {
        return std::make_pair(number, terminated);
    } else {
        return number;
    }
}

template<FieldType type_, class ByteSource_>
typename std::conditional<type_ == FieldType::UNKNOWN, std::pair<std::string, bool>, std::string>::type
parse_string_field(ByteSource_& pb, bool& valid, const std::string& path, int32_t line) {
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
            throw std::runtime_error("string containing a newline or tab in '" + path + "' (line " + std::to_string(line) + ")");
        } else if (!valid) {
            throw std::runtime_error("no terminating newline in '" + path + "' (line " + std::to_string(line) + ")");
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
