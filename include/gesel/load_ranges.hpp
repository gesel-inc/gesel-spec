#ifndef GESEL_LOAD_RANGES_HPP
#define GESEL_LOAD_RANGES_HPP

#include <cstdint>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <exception>
#include <vector>
#include <string>

#include "byteme/byteme.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils.hpp"
#include "parse_field.hpp"

namespace gesel {

namespace internal {

inline void check_bytes(const std::vector<std::uint64_t>& bytes) {
    std::uint64_t cumulative = 0;
    for (auto by : bytes) {
        auto added = sanisizer::sum_maybe<std::uint64_t>(cumulative, by);
        if (!added.has_value()) {
            throw std::overflow_error("sum of bytes should fit in a 64-bit integer"); 
        }
        cumulative = *added;
    }
}

inline std::vector<std::uint64_t> load_ranges(const std::string& path) {
    byteme::GzipFileReader reader(path.c_str(), {});
    byteme::SerialBufferedReader<char, decltype(&reader)> pb(&reader, sanisizer::cap<std::size_t>(65536));
    std::vector<std::uint64_t> output;

    bool valid = pb.valid();
    std::uint64_t line = 0;
    while (valid) {
        try {
            const auto number = [&]{
                try {
                    return parse_integer_field<FieldType::LAST>(pb, valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse byte count"));
                }
            }();
            output.push_back(number);

            sanisizer::increment(line);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to load line " + to_string(line)));
        }
    }

    check_bytes(output);
    return output;
}

inline std::pair<std::vector<std::uint64_t>, std::vector<std::uint64_t> > load_ranges_with_sizes(const std::string& path) {
    byteme::GzipFileReader reader(path.c_str(), {});
    byteme::SerialBufferedReader<char, decltype(&reader)> pb(&reader, sanisizer::cap<std::size_t>(65536));
    std::vector<std::uint64_t> output_byte, output_size;

    bool valid = pb.valid();
    std::uint64_t line = 0;
    while (valid) {
        try {
            const auto byte_size = [&]{
                try {
                    return parse_integer_field<FieldType::MIDDLE>(pb, valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse byte count"));
                }
            }();
            output_byte.push_back(byte_size);

            const auto other_size = [&]{
                try {
                    return parse_integer_field<FieldType::LAST>(pb, valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse size"));
                }
            }();
            output_size.push_back(other_size);

            sanisizer::increment(line);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to load line " + to_string(line)));
        }
    }

    check_bytes(output_byte);
    return std::make_pair(std::move(output_byte), std::move(output_size));
}

inline std::pair<std::vector<std::string>, std::vector<std::uint64_t> > load_named_ranges(const std::string& path) {
    byteme::GzipFileReader reader(path.c_str(), {});
    byteme::SerialBufferedReader<char, decltype(&reader)> pb(&reader, sanisizer::cap<std::size_t>(65536));
    std::vector<std::string> output_name; 
    std::vector<std::uint64_t> output_byte;

    bool valid = pb.valid();
    std::uint64_t line = 0;
    while (valid) {
        const auto name = [&]{
            try {
                return parse_string_field<FieldType::MIDDLE>(pb, valid);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to parse name"));
            }
        }();
        output_name.push_back(std::move(name));

        const auto byte_size = [&]{
            try {
                return parse_integer_field<FieldType::LAST>(pb, valid);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to parse byte count"));
            }
        }();
        output_byte.push_back(byte_size);

        sanisizer::increment(line);
    }

    check_bytes(output_byte);
    return std::make_pair(std::move(output_name), std::move(output_byte));
}

}

}

#endif
