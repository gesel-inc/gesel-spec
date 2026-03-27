#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_set_details.hpp"

#include "utils.h"

class TestCheckSetDetails : public ::testing::Test {
protected:
    static void check_set_details(const std::string& path, const std::vector<uint64_t>& ranges, const std::vector<uint64_t>& sizes) {
        gesel::internal::check_set_details(path, ranges, sizes, [&](uint64_t, const std::string&, const std::string&) {});
    }
};

TEST_F(TestCheckSetDetails, Success) {
    auto path = temp_file_path("check_sets");

    std::string payload1 = "aaron's set\tthis is aaron's set";
    std::string payload2 = "another set\tyet another set";
    quick_text_write(path, payload1 + "\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\n" + payload2 + "\t82\n");

    std::vector<uint64_t> ranges { static_cast<uint64_t>(payload1.size()), static_cast<uint64_t>(payload2.size()) };
    std::vector<uint64_t> sizes { 51, 82 };
    check_set_details(path, ranges, sizes);
}

TEST_F(TestCheckSetDetails, Failure) {
    auto path = temp_file_path("check_sets");

    std::string payload1 = "aaron's set\tthis is aaron's set";
    std::string payload2 = "another set\tyet another set";
    uint64_t r1 = payload1.size(), r2 = payload2.size();

    // Basic string field parsing checks.
    quick_text_write(path, "aaron's\tset\tthis is aaron's set\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\n" + payload2 + "\t82\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "tab");

    quick_text_write(path, "aaron's\nset\tthis is aaron's set\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\n" + payload2 + "\t82\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "newline");

    // Now for some more interesting checks.
    quick_text_write(path, payload1 + "\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\n" + payload2 + "\t82\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1 }, std::vector<uint64_t>{ 51 }); }, "number of lines");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1 + 1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "bytes per line");

    quick_text_write(path, payload1 + "\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "early termination");

    quick_text_write(path, payload1 + "\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\nfoo bar\tyet another set\t82\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "different name");

    quick_text_write(path, payload1 + "\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\nanother set\tfoobar\t82\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 51, 82 }); }, "different description");

    quick_text_write(path, payload1 + "\n" + payload2 + "\n");
    quick_gzip_write(path + ".gz", payload1 + "\t51\n" + payload2 + "\t82\n");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2 }, std::vector<uint64_t>{ 52, 82 }); }, "different size");
    expect_error([&]() { check_set_details(path, std::vector<uint64_t>{ r1, r2, 42 }, std::vector<uint64_t>{ 51, 82, 93 }); }, "number of lines");

    // Checking if the extra code is actually run.
    expect_error([&]() {
        gesel::internal::check_set_details(
            path,
            std::vector<uint64_t>{ r1, r2 },
            std::vector<uint64_t>{ 51, 82 },
            [&](uint64_t, const std::string& n, const std::string& d) {
                throw std::runtime_error(n + "\n" + d);
            }
        );
    }, "aaron");
}
