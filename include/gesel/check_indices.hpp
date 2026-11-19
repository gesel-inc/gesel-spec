#ifndef GESEL_CHECK_INDICES_HPP
#define GESEL_CHECK_INDICES_HPP

#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

#include "byteme/byteme.hpp"

#include "parse_field.hpp"

namespace gesel {

namespace internal {

template<bool has_gzip_, class Extra_>
void check_indices(const std::string& path, std::uint64_t index_limit, const std::vector<std::uint64_t>& ranges, Extra_ extra) {
    byteme::RawFileReader raw_r(path.c_str(), {});
    const auto gzpath = path + ".gz";
    auto gzip_r = [&]{
        if constexpr(has_gzip_) {
            return byteme::GzipFileReader(gzpath.c_str(), {});
        } else {
            return false;
        }
    }();

    const auto buffer_size = sanisizer::cap<std::size_t>(65536);
    byteme::SerialBufferedReader<char, decltype(&raw_r)> raw_p(&raw_r, buffer_size);
    auto gzip_p = [&]{
        if constexpr(has_gzip_) {
            return byteme::SerialBufferedReader<char, decltype(&gzip_r)>(&gzip_r, buffer_size);
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

    std::vector<std::uint64_t> raw_indices;
    typename std::conditional<has_gzip_, std::vector<std::uint64_t>, bool>::type gzip_indices;
    std::uint64_t line = 0;
    const auto num_ranges = ranges.size();

    while (raw_valid) {
        raw_indices.clear();
        const auto raw_pos = raw_p.position();

        if (raw_p.get() != '\n') {
            do {
                const auto status = parse_integer_field<FieldType::UNKNOWN>(raw_p, raw_valid, path, line);
                raw_indices.push_back(status.first);
                if (status.second) {
                    break;
                }
            } while (true);

            auto cumulative = raw_indices.front();
            if (cumulative >= index_limit) {
                throw std::runtime_error("out-of-range index in '" + path + "' (line " + std::to_string(line + 1) + ")");
            }

            const auto num_indices = raw_indices.size();
            for (I<decltype(num_indices)> i = 1; i < num_indices; ++i) {
                const auto delta = raw_indices[i];
                if (delta == 0) {
                    throw std::runtime_error("duplicate index in '" + path + "' (line " + std::to_string(line + 1) + ")");
                }
                if (sanisizer::is_greater_than_or_equal(delta, index_limit - cumulative)) {
                    throw std::runtime_error("out-of-range index in '" + path + "' (line " + std::to_string(line + 1) + ")");
                }
                cumulative += delta;
            }
        } else {
            raw_valid = raw_p.advance();
        }

        if (sanisizer::is_greater_than_or_equal(line, num_ranges)) {
            throw std::runtime_error("number of lines in '" + path + "' exceeds that expected from its '*.ranges.gz' file (line " + std::to_string(line + 1) + ")");
        }
        if (!sanisizer::is_equal(raw_p.position() - raw_pos - 1, ranges[line])) {
            throw std::runtime_error("number of bytes per line in '" + path + "' is not the same as that expected from the '*.ranges.gz' file (line " + std::to_string(line + 1) + ")");
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
                gzip_valid = gzip_p.advance();
            }

            if (raw_indices != gzip_indices) {
                throw std::runtime_error("different indices between '" + path + "' and its Gzipped version (line " + std::to_string(line + 1) + ")");
            }
        }

        const auto num_indices = raw_indices.size();
        for (I<decltype(num_indices)> i = 1; i < num_indices; ++i) {
            raw_indices[i] += raw_indices[i - 1];
        }
        extra(line, raw_indices);

        sanisizer::increment(line);
    }

    if (!sanisizer::is_equal(line, num_ranges)) {
        throw std::runtime_error("number of lines in '" + path + "' is less than that expected from its '*.ranges.gz' file (line " + std::to_string(line + 1) + ")");
    }
}

}

}

#endif
