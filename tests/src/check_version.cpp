#include "gtest/gtest.h"
#include "gesel/utils.hpp"
#include "gesel/check_version.hpp"

#include <fstream>
#include <string>

#include "utils.h"

TEST(CheckVersion, Basic) {
    auto path = temp_file_path("version");

    {
        quick_text_write(path, "0.0.0\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 0);
        EXPECT_EQ(output.minor, 0);
        EXPECT_EQ(output.patch, 0);
    }

    {
        quick_text_write(path, "123.456.7890\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 123);
        EXPECT_EQ(output.minor, 456);
        EXPECT_EQ(output.patch, 7890);
    }

    // Making some longer strings to check that the read from file is correct.
    {
        quick_text_write(path, "1221.34443.566665\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 1221);
        EXPECT_EQ(output.minor, 34443);
        EXPECT_EQ(output.patch, 566665);
    }

    {
        quick_text_write(path, "1234567.8901234.5678901\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 1234567);
        EXPECT_EQ(output.minor, 8901234);
        EXPECT_EQ(output.patch, 5678901);
    }

    // Alright, back to something simpler with mixed non-zeros and zeros.
    {
        quick_text_write(path, "1.0.0\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 1);
        EXPECT_EQ(output.minor, 0);
        EXPECT_EQ(output.patch, 0);
    }

    {
        quick_text_write(path, "0.1.0\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 0);
        EXPECT_EQ(output.minor, 1);
        EXPECT_EQ(output.patch, 0);
    }

    {
        quick_text_write(path, "0.0.1\n");
        auto output = gesel::internal::check_version(path);
        EXPECT_EQ(output.major, 0);
        EXPECT_EQ(output.minor, 0);
        EXPECT_EQ(output.patch, 1);
    }
}

TEST(CheckVersion, Comparisons) {
    EXPECT_EQ(gesel::internal::Version(0, 0, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_NE(gesel::internal::Version(1, 0, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_NE(gesel::internal::Version(0, 1, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_NE(gesel::internal::Version(0, 0, 1), gesel::internal::Version(0, 0, 0));

    EXPECT_GT(gesel::internal::Version(1, 0, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_GT(gesel::internal::Version(0, 1, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_GT(gesel::internal::Version(0, 0, 1), gesel::internal::Version(0, 0, 0));

    EXPECT_GE(gesel::internal::Version(0, 0, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_GE(gesel::internal::Version(1, 0, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_GE(gesel::internal::Version(0, 1, 0), gesel::internal::Version(0, 0, 0));
    EXPECT_GE(gesel::internal::Version(0, 0, 1), gesel::internal::Version(0, 0, 0));

    EXPECT_LT(gesel::internal::Version(1, 0, 0), gesel::internal::Version(10, 0, 0));
    EXPECT_LT(gesel::internal::Version(0, 1, 0), gesel::internal::Version(10, 0, 0));
    EXPECT_LT(gesel::internal::Version(0, 0, 1), gesel::internal::Version(10, 0, 0));

    EXPECT_LE(gesel::internal::Version(10, 0, 0), gesel::internal::Version(10, 0, 0));
    EXPECT_LE(gesel::internal::Version(1, 0, 0), gesel::internal::Version(10, 0, 0));
    EXPECT_LE(gesel::internal::Version(0, 1, 0), gesel::internal::Version(10, 0, 0));
    EXPECT_LE(gesel::internal::Version(0, 0, 1), gesel::internal::Version(10, 0, 0));
}

TEST(CheckVersion, Errors) {
    auto path = temp_file_path("version");

    // Check for an empty field.
    quick_text_write(path, ".0.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "empty field");

    quick_text_write(path, "0..0");
    expect_error([&]() { gesel::internal::check_version(path); }, "empty field");

    // non-digit tests.
    quick_text_write(path, "a.0.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "non-digit");

    quick_text_write(path, "1a.0.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "non-digit");

    quick_text_write(path, "0.a.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "non-digit");

    quick_text_write(path, "0.2a.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "non-digit");

    quick_text_write(path, "0.0.a");
    expect_error([&]() { gesel::internal::check_version(path); }, "non-digit");

    quick_text_write(path, "0.0.3a");
    expect_error([&]() { gesel::internal::check_version(path); }, "non-digit");

    // Leading zero checks.
    quick_text_write(path, "02.0.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "leading zero");

    quick_text_write(path, "2.01.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "leading zero");

    quick_text_write(path, "2.1.02");
    expect_error([&]() { gesel::internal::check_version(path); }, "leading zero");

    // Overflow.
    quick_text_write(path, "2.10000000000.2");
    expect_error([&]() { gesel::internal::check_version(path); }, "overflow");

    quick_text_write(path, "2.2147483648.2");
    expect_error([&]() { gesel::internal::check_version(path); }, "overflow");

    // Premature termination.
    quick_text_write(path, "0");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    quick_text_write(path, "1");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    quick_text_write(path, "0.");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    quick_text_write(path, "0.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    quick_text_write(path, "0.1");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    quick_text_write(path, "0.0.");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    quick_text_write(path, "0.0.0");
    expect_error([&]() { gesel::internal::check_version(path); }, "premature termination");

    // Premature termination.
    quick_text_write(path, "0.0.0\nfoo");
    expect_error([&]() { gesel::internal::check_version(path); }, "additional characters");
}
