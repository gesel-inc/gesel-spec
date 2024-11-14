#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/load_ranges.hpp"
#include "byteme/temp_file_path.hpp"

#include "utils.h"

TEST(LoadRanges, Success) {
    auto path = byteme::temp_file_path("load_ranges");

    {
        {
            byteme::GzipFileWriter writer(path);
            writer.write("0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n");
        }

        auto output = gesel::internal::load_ranges(path);
        std::vector<int32_t> expected{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        EXPECT_EQ(output, expected);
    }

    {
        {
            byteme::GzipFileWriter writer(path);
            writer.write("123\n456\n789\n0\n");
        }

        auto output = gesel::internal::load_ranges(path);
        std::vector<int32_t> expected{ 123, 456, 789, 0 };
        EXPECT_EQ(output, expected);
    }
}

TEST(LoadRanges, Failure) {
    auto path = byteme::temp_file_path("load_ranges");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\n1\n2\n3\n4\n5\n6\n7\n8\n9");
    }
    expect_error([&]() { gesel::internal::load_ranges(path); }, "terminating newline");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\nabc\n2\n3\n4\n5\n6\n7\n8\n9\n");
    }
    expect_error([&]() { gesel::internal::load_ranges(path); }, "non-digit");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\t1\n2\n3\n4\n5\n6\n7\n8\n9\n");
    }
    expect_error([&]() { gesel::internal::load_ranges(path); }, "non-digit");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("00\n1\n2\n3\n4\n5\n6\n7\n8\n9\n");
    }
    expect_error([&]() { gesel::internal::load_ranges(path); }, "leading zero");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\n01\n2\n3\n4\n5\n6\n7\n8\n9\n");
    }
    expect_error([&]() { gesel::internal::load_ranges(path); }, "leading zero");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\n1\n2147483648\n3\n4\n5\n6\n7\n8\n9\n");
    }
    expect_error([&]() { gesel::internal::load_ranges(path); }, "overflow");
}

TEST(LoadRangesWithSizes, Success) {
    auto path = byteme::temp_file_path("load_ranges_with_sizes");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\t0\n12\t23\n234\t5\n34\t456\n4\t56789\n");
    }

    auto output = gesel::internal::load_ranges_with_sizes(path);
    std::vector<int32_t> expected_bytes { 0, 12, 234, 34, 4 };
    EXPECT_EQ(output.first, expected_bytes);
    std::vector<int32_t> expected_sizes { 0, 23, 5, 456, 56789 };
    EXPECT_EQ(output.second, expected_sizes);
}

TEST(LoadRangesWithSizes, Failure) {
    auto path = byteme::temp_file_path("load_ranges_with_sizes");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\n12\t23\n234\t5\n34\t456\n4\t56789\n");
    }
    expect_error([&]() { gesel::internal::load_ranges_with_sizes(path); }, "non-digit");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\t0\t0\n12\t23\n234\t5\n34\t456\n4\t56789\n");
    }
    expect_error([&]() { gesel::internal::load_ranges_with_sizes(path); }, "non-digit");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("0\t0\n12\t23\n234\t5\n34\t456\n4");
    }
    expect_error([&]() { gesel::internal::load_ranges_with_sizes(path); }, "no terminating newline");

    // All the other checks are the same as those in load_ranges.
}

TEST(LoadNamedRanges, Success) {
    auto path = byteme::temp_file_path("load_named_ranges");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("alpha\t1\nbravo\t23\ncharlie delta\t5\necho0foxtrot0\t456\ngolf_hotel\t56789\n");
    }

    auto output = gesel::internal::load_named_ranges(path);
    std::vector<std::string> expected_names { "alpha", "bravo", "charlie delta", "echo0foxtrot0", "golf_hotel" };
    EXPECT_EQ(output.first, expected_names);
    std::vector<int32_t> expected_bytes { 1, 23, 5, 456, 56789 };
    EXPECT_EQ(output.second, expected_bytes);
}

TEST(LoadNamedRanges, Failure) {
    auto path = byteme::temp_file_path("load_named_ranges");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("alpha\nbravo\t23\ncharlie delta\t5\necho0foxtrot0\t456\ngolf_hotel\t56789\n");
    }
    expect_error([&]() { gesel::internal::load_named_ranges(path); }, "newline");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("alpha\t0\t0\nbravo\t23\ncharlie delta\t5\necho0foxtrot0\t456\ngolf_hotel\t56789\n");
    }
    expect_error([&]() { gesel::internal::load_named_ranges(path); }, "non-digit");

    {
        byteme::GzipFileWriter writer(path);
        writer.write("alpha\t0\nbravo\t23\ncharlie delta\t5\necho0foxtrot0\t456\ngolf_hotel");
    }
    expect_error([&]() { gesel::internal::load_named_ranges(path); }, "terminating newline");

    // All the other checks are the same as those in load_ranges.
}

