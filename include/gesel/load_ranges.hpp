#ifndef GESEL_LOAD_RANGES_HPP
#define GESEL_LOAD_RANGES_HPP

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>

#include "byteme/byteme.hpp"

#include "parse_field.hpp"

namespace gesel {

namespace internal {
inline std::vector<int32_t> load_ranges(const std::string& path) {
    byteme::GzipFileReader reader(path);
    byteme::PerByte pb(&reader);
    std::vector<int32_t> output;

    bool valid = pb.valid();
    int32_t line = 1;
    while (valid) {
        int32_t number = parse_integer_field<FieldType::LAST>(pb, valid, path, line);
        output.push_back(number);
        ++line;
    }

    return output;
}

inline std::pair<std::vector<int32_t>, std::vector<int32_t> > load_ranges_with_sizes(const std::string& path) {
    byteme::GzipFileReader reader(path);
    byteme::PerByte pb(&reader);
    std::vector<int32_t> output_byte, output_size;

    bool valid = pb.valid();
    int32_t line = 1;
    while (valid) {
        int32_t byte_size = parse_integer_field<FieldType::MIDDLE>(pb, valid, path, line);
        int32_t other_size = parse_integer_field<FieldType::LAST>(pb, valid, path, line);
        ++line;
        output_byte.push_back(byte_size);
        output_size.push_back(other_size);
    }

    return std::make_pair(std::move(output_byte), std::move(output_size));
}

inline std::pair<std::vector<std::string>, std::vector<int32_t> > load_named_ranges(const std::string& path) {
    byteme::GzipFileReader reader(path);
    byteme::PerByte pb(&reader);
    std::vector<std::string> output_name;
    std::vector<int32_t> output_byte;

    bool valid = pb.valid();
    int32_t line = 1;
    while (valid) {
        auto name = parse_string_field<FieldType::MIDDLE>(pb, valid, path, line);
        int32_t byte_size = parse_integer_field<FieldType::LAST>(pb, valid, path, line);
        ++line;
        output_name.push_back(name);
        output_byte.push_back(byte_size);
    }

    return std::make_pair(std::move(output_name), std::move(output_byte));
}


}

}

#endif
