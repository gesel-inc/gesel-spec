#ifndef GESEL_CHECK_SET_DETAILS_HPP
#define GESEL_CHECK_SET_DETAILS_HPP

#include <string>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <exception>
#include <stdexcept>

#include "sanisizer/sanisizer.hpp"
#include "byteme/byteme.hpp"

#include "parse_field.hpp"

namespace gesel {

namespace internal {

template<class Extra_>
void check_set_details(const std::string& path, const std::vector<std::uint64_t>& ranges, const std::vector<std::uint64_t>& sizes, Extra_ extra) {
    byteme::RawFileReader raw_r(path.c_str(), {});
    auto gzpath = path + ".gz";
    byteme::GzipFileReader gzip_r(gzpath.c_str(), {});

    const auto buffer_size = sanisizer::cap<std::size_t>(65536);
    byteme::SerialBufferedReader<char, decltype(&raw_r)> raw_p(&raw_r, buffer_size);
    byteme::SerialBufferedReader<char, decltype(&gzip_r)> gzip_p(&gzip_r, buffer_size);

    bool raw_valid = raw_p.valid();
    bool gzip_valid = gzip_p.valid();
    std::uint64_t line = 0;
    const auto num_ranges = ranges.size();

    while (raw_valid) {
        if (line >= num_ranges) {
            throw std::runtime_error("number of lines is greater than that of its '*.ranges.gz' file");
        }

        try {
            auto raw_pos = raw_p.position();

            auto name = [&]{
                try {
                    return parse_string_field<FieldType::MIDDLE>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse name"));
                }
            }();

            auto description = [&]{
                try {
                    return parse_string_field<FieldType::LAST>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse description"));
                }
            }();

            if (!sanisizer::is_equal(raw_p.position() - raw_pos - 1, ranges[line])) {
                throw std::runtime_error("number of bytes is not the same as that expected from the '*.ranges.gz' file");
            }

            try {
                if (!gzip_valid) {
                    throw std::runtime_error("early termination of the Gzipped file");
                }

                auto gz_name = [&]{
                    try {
                        return parse_string_field<FieldType::MIDDLE>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse name"));
                    }
                }();
                if (gz_name != name) {
                    throw std::runtime_error("name is different");
                }

                auto gz_description = [&]{
                    try {
                        return parse_string_field<FieldType::MIDDLE>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse description"));
                    }
                }();
                if (gz_description != description) {
                    throw std::runtime_error("description is different");
                }

                auto gz_size = [&]{
                    try {
                        return parse_integer_field<FieldType::LAST>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse size"));
                    }
                }();
                if (gz_size != sizes[line]) {
                    throw std::runtime_error("set size is not consistent with that in the '*.ranges.gz' file");
                }
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to compare the corresponding line in the Gzipped version of the file"));
            }

            extra(line, name, description);

            sanisizer::increment(line);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate line " + to_string(line)));
        }
    }

    if (line != num_ranges) {
        throw std::runtime_error("number of lines is less than that expected from its '*.ranges.gz' file");
    }
}

}

}

#endif
