#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_set_details.hpp"
#include "byteme/temp_file_path.hpp"

#include "utils.h"

TEST(CheckSetDetails, Success) {
    auto path = byteme::temp_file_path("check_sets");

    std::string payload1 = "aaron's set\tthis is aaron's set";
    std::string payload2 = "another set\tyet another set";
    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\n" + payload2 + "\t82\n");
    }

    std::vector<uint64_t> ranges { static_cast<uint64_t>(payload1.size()), static_cast<uint64_t>(payload2.size()) };
    std::vector<uint64_t> sizes { 51, 82 };
    gesel::internal::check_set_details(path, ranges, sizes);
}

TEST(CheckSetDetails, Failure) {
    auto path = byteme::temp_file_path("check_sets");

    std::string payload1 = "aaron's set\tthis is aaron's set";
    std::string payload2 = "another set\tyet another set";
    uint64_t r1 = payload1.size(), r2 = payload2.size();

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's\tset\tthis is aaron's set\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\n" + payload2 + "\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "tab");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's\nset\tthis is aaron's set\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\n" + payload2 + "\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "newline");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\n" + payload2 + "\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1 }, std::vector<uint64_t>{ 51 }); }, "number of lines");
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1 + 1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "bytes per line");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "early termination");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\nfoo bar\tyet another set\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "different name");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\nanother set\tfoobar\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "different description");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t51\n" + payload2 + "\t82\n");
    }
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 52, 82 }); }, "different size");
    expect_error([&]() { gesel::internal::check_set_details(path, std::vector<uint64_t>{ r1, r2, 42 }, std::vector<uint64_t>{ 51, 82, 93 }); }, "number of lines");
}
