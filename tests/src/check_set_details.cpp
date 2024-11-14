#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_set_details.hpp"
#include "byteme/temp_file_path.hpp"

#include "utils.h"

TEST(CheckSetDetails, Success) {
    auto path = byteme::temp_file_path("check_sets");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }

    std::vector<uint64_t> ranges { 31, 27 };
    std::vector<uint64_t> sizes { 51, 82 };
    gesel::internal::check_set_details(path, ranges, sizes);
}

TEST(CheckSetDetails, Failure) {
    auto path = byteme::temp_file_path("check_sets");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's\tset\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27 }, std::vector<uint64_t>{ 51, 82 }); }, "tab");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's\nset\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27 }, std::vector<uint64_t>{ 51, 82 }); }, "newline");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31 }, std::vector<uint64_t>{ 51 }); }, "number of lines");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 28 }, std::vector<uint64_t>{ 51, 82 }); }, "bytes per line");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27 }, std::vector<uint64_t>{ 51, 82 }); }, "early termination");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nfoo bar\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27 }, std::vector<uint64_t>{ 51, 82 }); }, "different name");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tfoobar\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27 }, std::vector<uint64_t>{ 51, 82 }); }, "different description");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27 }, std::vector<uint64_t>{ 52, 82 }); }, "different size");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's set\tthis is aaron's set\nanother set\tyet another set\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write("aaron's set\tthis is aaron's set\t51\nanother set\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ 31, 27, 42 }, std::vector<uint64_t>{ 51, 82, 93 }); }, "number of lines");
}
