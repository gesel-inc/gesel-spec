#ifndef GESEL_CHECK_COLLECTION_DETAILS_HPP
#define GESEL_CHECK_COLLECTION_DETAILS_HPP

#include <string>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <exception>

#include "byteme/byteme.hpp"
#include "sanisizer/sanisizer.hpp"

#include "parse_field.hpp"

namespace gesel {

namespace internal {

inline void check_collection_details(const std::string& path, const std::vector<std::uint64_t>& ranges, const std::vector<std::uint64_t>& numbers) {
    byteme::RawFileReader raw_r(path.c_str(), {});
    auto gzpath = path + ".gz";
    byteme::GzipFileReader gzip_r(gzpath.c_str(), {});

    auto buffer_size = sanisizer::cap<std::size_t>(65536);
    byteme::SerialBufferedReader<char, decltype(&raw_r)> raw_p(&raw_r, buffer_size);
    byteme::SerialBufferedReader<char, decltype(&gzip_r)> gzip_p(&gzip_r, buffer_size);

    bool raw_valid = raw_p.valid();
    bool gzip_valid = gzip_p.valid();
    std::uint64_t line = 0;
    const auto num_ranges = ranges.size();

    while (raw_valid) {
        if (line >= num_ranges) {
            throw std::runtime_error("number of lines is greater than that of the '*.ranges.gz' file");
        }

        try {
            const auto raw_pos = raw_p.position();

            auto title = [&]{
                try {
                    return parse_string_field<FieldType::MIDDLE>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse title"));
                }
            }();

            auto description = [&]{
                try {
                    return parse_string_field<FieldType::MIDDLE>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse description"));
                }
            }();

            auto species = [&]{
                try {
                    return parse_integer_field<FieldType::MIDDLE>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse species"));
                }
            }();

            auto maintainer = [&]{
                try {
                    return parse_string_field<FieldType::MIDDLE>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse maintainer"));
                }
            }();

            auto source = [&]{
                try {
                    return parse_string_field<FieldType::LAST>(raw_p, raw_valid);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to parse source"));
                }
            }();

            // We know that line < num_ranges at this point.
            if (!sanisizer::is_equal(raw_p.position() - raw_pos - 1, ranges[line])) {
                throw std::runtime_error("number of bytes is not the same as that expected from the '*.ranges.gz' file");
            }

            try {
                if (!gzip_valid) {
                    throw std::runtime_error("early termination of the Gzipped file");
                }

                auto gz_title = [&]{
                    try {
                        return parse_string_field<FieldType::MIDDLE>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse title")); 
                    }
                }();
                if (gz_title != title) {
                    throw std::runtime_error("title is different");
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

                auto gz_species = [&]{
                    try {
                        return parse_integer_field<FieldType::MIDDLE>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse species"));
                    }
                }();
                if (gz_species != species) {
                    throw std::runtime_error("species is different");
                }

                auto gz_maintainer = [&]{
                    try {
                        return parse_string_field<FieldType::MIDDLE>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse maintainer"));
                    }
                }();
                if (gz_maintainer != maintainer) {
                    throw std::runtime_error("maintainer is different");
                }

                auto gz_source = [&]{
                    try {
                        return parse_string_field<FieldType::MIDDLE>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse source"));
                    }
                }();
                if (gz_source != source) {
                    throw std::runtime_error("source is different");
                }

                auto gz_number = [&]{
                    try {
                        return parse_integer_field<FieldType::LAST>(gzip_p, gzip_valid);
                    } catch (...) {
                        std::throw_with_nested(std::runtime_error("failed to parse number"));
                    }
                }();
                if (gz_number != numbers[line]) {
                    throw std::runtime_error("number of sets is not consistent with that in the '*.ranges.gz' file");
                }
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to compare the corresponding line in the Gzipped version of the file"));
            }

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
