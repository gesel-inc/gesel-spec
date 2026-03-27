#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_indices.hpp"

#include "utils.h"

class TestCheckIndices : public ::testing::Test {
protected:
    template<bool has_gzip_>
    static void check_indices(const std::string& path, uint64_t limit, const std::vector<uint64_t>& ranges) {
        gesel::internal::check_indices<has_gzip_>(
            path,
            limit,
            ranges,
            [&](uint64_t, const std::vector<uint64_t>&) {}
        );
    }
};

TEST_F(TestCheckIndices, Success) {
    auto path = temp_file_path("check_indices");

    std::string payload = "0\t123\t45\n6\n780\t1\t234\t45\n\n67\t890\n";
    quick_text_write(path, payload);
    quick_gzip_write(path + ".gz", payload);

    std::vector<uint64_t> ranges{ 8, 1, 12, 0, 6 };
    check_indices<false>(path, 2000, ranges);
    check_indices<true>(path, 2000, ranges);

    // Checking that the extra code is run.
    gesel::internal::check_indices<false>(
        path,
        2000, 
        ranges,
        [&](uint64_t, const std::vector<uint64_t>& indices) {
            for (size_t i = 1, end = indices.size(); i < end; ++i) {
                if (indices[i] <= indices[i-1]) {
                    throw std::runtime_error("indices should be strictly increasing");
                }
            }
        }
    );

    // Check that we correctly handle empty lines at the end.
    std::string payload2 = "0\t123\t45\n6\n780\t1\t234\t45\n67\t890\n\n";
    quick_text_write(path, payload2);
    quick_gzip_write(path + ".gz", payload2);

    ranges = std::vector<uint64_t>{ 8, 1, 12, 6, 0 };
    check_indices<true>(path, 2000, ranges);
}

TEST_F(TestCheckIndices, RawFailure) {
    auto path = temp_file_path("check_indices");

    // Basic checks for correct integer parsing.
    quick_text_write(path, "0\ta\tb\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 2 }); }, "non-digit");

    quick_text_write(path, "0\t\t\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 2 }); }, "empty field");

    quick_text_write(path, "0\n4");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 1, 1 }); }, "terminating newline");

    // Alright, moving onto some more interesting failures.
    quick_text_write(path, "12\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 3 }); }, "out-of-range");

    quick_text_write(path, "3\t0\t5\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 5 }); }, "duplicate index");

    quick_text_write(path, "3\t4\t5\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 5 }); }, "out-of-range");

    quick_text_write(path, "3\t4\t2\n1\t4\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 5 }); }, "number of lines");

    quick_text_write(path, "3\t4\t2\n1\t4\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 5, 4 }); }, "number of bytes per line");

    quick_text_write(path, "3\t4\t2\n1\t4\n");
    expect_error([&]() { check_indices<false>(path, 10, std::vector<uint64_t>{ 5, 3, 1 }); }, "number of lines");

    // Checking that the extra code is run.
    quick_text_write(path, "3\t4\t2\n1\t4\n");
    expect_error([&]() { 
        gesel::internal::check_indices<false>(
            path,
            10, 
            std::vector<uint64_t>{ 5, 3 },
            [&](uint64_t, const std::vector<uint64_t>&) {
                throw std::runtime_error("foo failed");
            }
        );
    }, "foo failed");
}

TEST_F(TestCheckIndices, GzipFailure) {
    auto path = temp_file_path("check_indices");

    quick_text_write(path, "0\n2\n3\n");
    quick_gzip_write(path + ".gz", "0\n2\n");
    expect_error([&]() { check_indices<true>(path, 10, std::vector<uint64_t>{ 1, 1, 1 }); }, "early termination");

    quick_text_write(path, "0\n2\n3\n");
    quick_gzip_write(path + ".gz", "0\n2\t3\n3\n");
    expect_error([&]() { check_indices<true>(path, 10, std::vector<uint64_t>{ 1, 1, 1 }); }, "different indices");

    quick_text_write(path, "0\n2\t2\n3\n");
    quick_gzip_write(path + ".gz", "0\n2\t3\n3\n");
    expect_error([&]() { check_indices<true>(path, 10, std::vector<uint64_t>{ 1, 3, 1 }); }, "different indices");
}
