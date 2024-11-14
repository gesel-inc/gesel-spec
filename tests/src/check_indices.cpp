#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_indices.hpp"
#include "byteme/temp_file_path.hpp"

#include "utils.h"

TEST(CheckIndices, Success) {
    auto path = byteme::temp_file_path("check_indices");

    std::string payload = "0\t123\t45\n6\n780\t1\t234\t45\n\n67\t890\n";
    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload);
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload);
    }

    std::vector<uint64_t> ranges{ 8, 1, 12, 0, 6 };
    gesel::internal::check_indices<false>(path, 2000, ranges);
    gesel::internal::check_indices<true>(path, 2000, ranges);
}

TEST(CheckIndices, RawFailure) {
    auto path = byteme::temp_file_path("check_indices");

    // Basic checks for correct integer parsing.
    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("0\ta\tb\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 2 }); }, "non-digit");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("0\t\t\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 2 }); }, "empty field");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("0\n4");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 1, 1 }); }, "terminating newline");

    // Alright, moving onto some more interesting failures.
    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("12\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 3 }); }, "out-of-range");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("3\t0\t5\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 5 }); }, "duplicate index");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("3\t4\t5\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 5 }); }, "out-of-range");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("3\t4\t2\n1\t4\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 5 }); }, "number of lines");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("3\t4\t2\n1\t4\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 5, 4 }); }, "number of bytes per line");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("3\t4\t2\n1\t4\n");
    }
    expect_error([&]() { gesel::internal::check_indices<false>(path, 10, std::vector<uint64_t>{ 5, 3, 1 }); }, "number of lines");
}

TEST(CheckIndices, GzipFailure) {
    auto path = byteme::temp_file_path("check_indices");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("0\n2\n3\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("0\n2\n");
    }
    expect_error([&]() { gesel::internal::check_indices<true>(path, 10, std::vector<uint64_t>{ 1, 1, 1 }); }, "early termination");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("0\n2\n3\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("0\n2\t3\n3\n");
    }
    expect_error([&]() { gesel::internal::check_indices<true>(path, 10, std::vector<uint64_t>{ 1, 1, 1 }); }, "different indices");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("0\n2\t2\n3\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("0\n2\t3\n3\n");
    }
    expect_error([&]() { gesel::internal::check_indices<true>(path, 10, std::vector<uint64_t>{ 1, 3, 1 }); }, "different indices");
}
