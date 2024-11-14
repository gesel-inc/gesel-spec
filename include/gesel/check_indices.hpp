#ifndef GESEL_CHECK_INDICES_HPP
#define GESEL_CHECK_INDICES_HPP

#include <string>
#include <cstdint>
#include <vector>

#include "byteme/byteme.hpp"

#include "parse_field.hpp"

namespace gesel {

namespace internal {

template<bool has_gzip_>
void check_indices(const std::string& path, int32_t index_limit, const std::vector<int32_t>& ranges) {
    byteme::RawFileReader raw_r(path);
    auto gzpath = path + ".gz";
    auto gzip_r = [&]{
        if constexpr(has_gzip_) {
            return byteme::GzipFileReader(gzpath);
        } else {
            return false;
        }
    }();

    byteme::PerByte raw_p(&raw_r);
    auto gzip_p = [&]{
        if constexpr(has_gzip_) {
            return byteme::PerByte(&gzip_r);
        } else {
            return false;
        }
    }();

    bool raw_valid = raw_p.valid();
    bool gzip_valid = [&]{
        if constexpr(has_gzip_) {
            return gzip_p.valid();
        } else {
            return false;
        }
    }();

    std::vector<int32_t> raw_indices;
    typename std::conditional<has_gzip_, std::vector<int32_t>, bool>::type gzip_indices;
    size_t line = 1;
    const size_t num_ranges = ranges.size();

    while (raw_valid) {
        raw_indices.clear();
        auto raw_pos = raw_p.position();

        if (raw_p.get() != '\n') {
            do {
                auto status = parse_integer_field<FieldType::UNKNOWN>(raw_p, raw_valid, path, line);
                raw_indices.push_back(status.first);
                if (status.second) {
                    break;
                }
            } while (true);

            int32_t cumulative = raw_indices.front();
            if (cumulative >= index_limit) {
                throw std::runtime_error("out-of-range index in '" + path + "' (line " + std::to_string(line) + ")");
            }

            for (size_t i = 1, end = raw_indices.size(); i < end; ++i) {
                auto delta = raw_indices[i];
                if (delta == 0) {
                    throw std::runtime_error("duplicate index in '" + path + "' (line " + std::to_string(line) + ")");
                }
                if (delta >= index_limit - cumulative) {
                    throw std::runtime_error("out-of-range index in '" + path + "' (line " + std::to_string(line) + ")");
                }
                cumulative += delta;
            }
        } else {
            raw_p.advance();
        }

        if (line > num_ranges) {
            throw std::runtime_error("number of lines in '" + path + "' exceeds that expected from its '*.ranges.gz' file (line " + std::to_string(line) + ")");
        }
        if (raw_p.position() - raw_pos - 1 != static_cast<size_t>(ranges[line - 1])) {
            throw std::runtime_error("number of bytes per line in '" + path + "' is not the same as that expected from the '*.ranges.gz' file (line " + std::to_string(line) + ")");
        }

        if constexpr(has_gzip_) {
            if (!gzip_valid) {
                throw std::runtime_error("early termination of the Gzipped version of '" + path + "'");
            }

            gzip_indices.clear();
            if (gzip_p.get() != '\n') {
                do {
                    auto status = parse_integer_field<FieldType::UNKNOWN>(gzip_p, gzip_valid, gzpath, line);
                    gzip_indices.push_back(status.first);
                    if (status.second) {
                        break;
                    }
                } while (true);
            } else {
                gzip_p.advance();
            }

            size_t num_indices = raw_indices.size();
            if (num_indices != gzip_indices.size()) {
                throw std::runtime_error("different indices between '" + path + "' and its Gzipped version (line " + std::to_string(line) + ")");
            }
            for (size_t i = 0; i < num_indices; ++i) {
                if (raw_indices[i] != gzip_indices[i]) {
                    throw std::runtime_error("different indices between '" + path + "' and its Gzipped version (line " + std::to_string(line) + ")");
                }
            }
        }

        ++line;
    }

    if (line - 1 != num_ranges) {
        throw std::runtime_error("number of lines in '" + path + "' is less than that expected from its '*.ranges.gz' file (line " + std::to_string(line) + ")");
    }
}

}

}

#endif
