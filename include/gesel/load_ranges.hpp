#ifndef GESEL_LOAD_RANGES_HPP
#define GESEL_LOAD_RANGES_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include <string>

#include "byteme/byteme.hpp"

#include "parse_field.hpp"

namespace gesel {

namespace internal {

inline void check_bytes(const std::vector<uint64_t>& bytes) {
    uint64_t cumulative = 0;
    constexpr uint64_t max_value = std::numeric_limits<uint64_t>::max();
    for (auto by : bytes) {
        if (by >= max_value - cumulative) {
            throw std::runtime_error("cumulative sum of bytes should fit in a 64-bit integer"); 
        }
        cumulative += by;
        ++cumulative;
    }
}

inline std::vector<uint64_t> load_ranges(const std::string& path) {
    byteme::GzipFileReader reader(path);
    byteme::PerByte pb(&reader);
    std::vector<uint64_t> output;

    bool valid = pb.valid();
    uint64_t line = 0;
    constexpr uint64_t max_line = std::numeric_limits<uint64_t>::max();

    while (valid) {
        uint64_t number = parse_integer_field<FieldType::LAST>(pb, valid, path, line);
        output.push_back(number);

        if (line == max_line) {
            throw std::runtime_error("number of lines should fit in a 64-bit integer"); 
        }
        ++line;
    }

    check_bytes(output);
    return output;
}

inline std::pair<std::vector<uint64_t>, std::vector<uint64_t> > load_ranges_with_sizes(const std::string& path) {
    byteme::GzipFileReader reader(path);
    byteme::PerByte pb(&reader);
    std::vector<uint64_t> output_byte, output_size;

    bool valid = pb.valid();
    uint64_t line = 0;
    constexpr uint64_t max_line = std::numeric_limits<uint64_t>::max();

    while (valid) {
        uint64_t byte_size = parse_integer_field<FieldType::MIDDLE>(pb, valid, path, line);
        output_byte.push_back(byte_size);

        uint64_t other_size = parse_integer_field<FieldType::LAST>(pb, valid, path, line);
        output_size.push_back(other_size);

        if (line == max_line) {
            throw std::runtime_error("number of lines should fit in a 32-bit integer"); 
        }
        ++line;
    }

    check_bytes(output_byte);
    return std::make_pair(std::move(output_byte), std::move(output_size));
}

inline std::vector<uint64_t> load_named_ranges(const std::string& path) {
    byteme::GzipFileReader reader(path);
    byteme::PerByte pb(&reader);
    std::vector<uint64_t> output_byte;

    bool valid = pb.valid();
    uint64_t line = 0;
    std::string last; 
    constexpr uint64_t max_line = std::numeric_limits<uint64_t>::max();

    while (valid) {
        auto name = parse_string_field<FieldType::MIDDLE>(pb, valid, path, line);
        uint64_t byte_size = parse_integer_field<FieldType::LAST>(pb, valid, path, line);
        output_byte.push_back(byte_size);

        for (auto x : name) {
            if ((x < 'a' || x > 'z') && (x < '0' || x > '9') && (x != '-')) {
                throw std::runtime_error("tokens should only contain lower-case alphabetical characters, digits or a dash in '" + path + "' " + append_line_number(line));
            }
        }
        if (line && name <= last) {
            throw std::runtime_error("tokens should be unique and lexicographically sorted in '" + path + "' " + append_line_number(line));
        } 
        last.swap(name);

        if (line == max_line) {
            throw std::runtime_error("number of lines should fit in a 32-bit integer in '" + path + "'"); 
        }
        ++line;
    }

    check_bytes(output_byte);
    return output_byte;
}

}

}

#endif
