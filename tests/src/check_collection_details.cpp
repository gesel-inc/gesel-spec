#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_collection_details.hpp"
#include "byteme/temp_file_path.hpp"

#include "utils.h"

TEST(CheckCollectionDetails, Success) {
    auto path = byteme::temp_file_path("check_collections");

    std::string payload1 = "aaron's collection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net";
    std::string payload2 = "yet another collection\tsomeone else's collection\t9999\tSomeone else\thttps://someone.else.com";
    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }

    std::vector<int32_t> ranges { static_cast<int32_t>(payload1.size()), static_cast<int32_t>(payload2.size()) };
    std::vector<int32_t> sizes { 12, 55 };
    gesel::internal::check_collection_details(path, ranges, sizes);
}

TEST(CheckCollectionDetails, Failure) {
    auto path = byteme::temp_file_path("check_collections");

    std::string payload1 = "aaron's collection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net";
    std::string payload2 = "yet another collection\tsomeone else's collection\t9999\tSomeone else\thttps://someone.else.com";
    std::vector<int32_t> ranges { static_cast<int32_t>(payload1.size()), static_cast<int32_t>(payload2.size()) };

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's\tcollection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "non-digit");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's collection\tthis is aaron's collection\tAARON\tAaron Lun\thttps://aaron.net\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "non-digit");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write("aaron's collection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net\tfoo\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "tab");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "number of lines");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }
    expect_error([&]() { 
        std::vector<int32_t> ranges2 = ranges;
        ++(ranges2.front());
        gesel::internal::check_collection_details(path, ranges2, std::vector<int32_t>{ 12, 55 });
    }, "number of bytes");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "early termination");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\nfoobar\tsomeone else's collection\t9999\tSomeone else\thttps://someone.else.com\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "different title");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\nyet another collection\tfoo bar\t9999\tSomeone else\thttps://someone.else.com\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "different description");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\nyet another collection\tsomeone else's collection\t9998\tSomeone else\thttps://someone.else.com\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "different species");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\nyet another collection\tsomeone else's collection\t9999\tStill Aaron\thttps://someone.else.com\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "different maintainer");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\nyet another collection\tsomeone else's collection\t9999\tSomeone else\thttps://aaron.com\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 55 }); }, "different source");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }
    expect_error([&]() { gesel::internal::check_collection_details(path, ranges, std::vector<int32_t>{ 12, 54 }); }, "different number");

    {
        byteme::RawFileWriter rwriter(path);
        rwriter.write(payload1 + "\n" + payload2 + "\n");
        byteme::GzipFileWriter gwriter(path + ".gz");
        gwriter.write(payload1 + "\t12\n" + payload2 + "\t55\n");
    }
    expect_error([&]() { 
        auto copy = ranges;
        copy.push_back(0);
        gesel::internal::check_collection_details(path, copy, std::vector<int32_t>{ 12, 55, 0 }); 
    }, "number of lines");
}
