#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <algorithm>

#include "byteme/temp_file_path.hpp"
#include "gesel/validate_database.hpp"
#include "utils.h"

TEST(Tokenization, Generator) {
    std::unordered_map<std::string, std::vector<uint64_t> > tokens_to_sets;
    gesel::internal::tokenize(1, "aaron is awesome", tokens_to_sets);
    gesel::internal::tokenize(2, "Aaron and   Aaron", tokens_to_sets); // throwing in some empty space to check that it doesn't get picked up.
    gesel::internal::tokenize(5, "12345.4567890 is aaron", tokens_to_sets);

    EXPECT_EQ(tokens_to_sets.size(), 6);
    std::vector<uint64_t> expected{ 1, 2, 5 };
    EXPECT_EQ(tokens_to_sets["aaron"], expected);
    expected = std::vector<uint64_t>{ 2 };
    EXPECT_EQ(tokens_to_sets["and"], expected);
    expected = std::vector<uint64_t>{ 1 };
    EXPECT_EQ(tokens_to_sets["awesome"], expected);
    expected = std::vector<uint64_t>{ 1, 5 };
    EXPECT_EQ(tokens_to_sets["is"], expected);
    expected = std::vector<uint64_t>{ 5 };
    EXPECT_EQ(tokens_to_sets["12345"], expected);
    EXPECT_EQ(tokens_to_sets["4567890"], expected);
}

TEST(Tokenization, Checker) {
    gesel::internal::check_tokens(std::vector<std::string>{ "alpha", "bravo", "charlie" }, "foobar.tsv");
    gesel::internal::check_tokens(std::vector<std::string>{ "12", "345", "6-7" }, "foobar.tsv");
    expect_error([&]() { gesel::internal::check_tokens(std::vector<std::string>{ "bravo", "alpha", "charlie" }, "foobar.tsv"); }, "sorted");
    expect_error([&]() { gesel::internal::check_tokens(std::vector<std::string>{ "Alpha", "charlie" }, "foobar.tsv"); }, "alphabetical");
    expect_error([&]() { gesel::internal::check_tokens(std::vector<std::string>{ "alpha bravo", "charlie" }, "foobar.tsv"); }, "alphabetical");
    expect_error([&]() { gesel::internal::check_tokens(std::vector<std::string>{ "" }, "foobar.tsv"); }, "empty");
}

class TestValidateDatabase : public ::testing::Test {
protected:
    static constexpr int max_genes = 20;

    template<typename Type_>
    static std::string delta_encode(const std::vector<Type_>& values) {
        std::string output;
        for (size_t j = 0, jend = values.size(); j < jend; ++j) {
            if (j != 0) {
                output += "\t";
                output += std::to_string(values[j] - values[j - 1]);
            } else {
                output += std::to_string(values[j]);
            }
        }
        return output;
    }

    static void save_collections(const std::string& path, const std::vector<std::string>& payloads, const std::vector<uint64_t>& sizes) {
        byteme::RawFileWriter rwriter(path);
        byteme::GzipFileWriter gwriter(path + ".gz");
        byteme::GzipFileWriter rrwriter(path + ".ranges.gz");
        for (size_t i = 0, end = sizes.size(); i < end; ++i) {
            const auto& current = payloads[i];
            rwriter.write(current + "\n");
            auto as_str = std::to_string(sizes[i]);
            gwriter.write(current + "\t" + as_str + "\n");
            rrwriter.write(std::to_string(current.size()) + "\t" + as_str + "\n");
        }
    }

    static void save_sets(const std::string& path, const std::vector<std::pair<std::string, std::string> >& payloads, const std::vector<uint64_t>& sizes) {
        byteme::RawFileWriter rwriter(path);
        byteme::GzipFileWriter gwriter(path + ".gz");
        byteme::GzipFileWriter rrwriter(path + ".ranges.gz");
        for (size_t i = 0, end = sizes.size(); i < end; ++i) {
            const auto& current = payloads[i];
            auto combined = current.first + "\t" + current.second;
            rwriter.write(combined + "\n");
            auto as_str = std::to_string(sizes[i]);
            gwriter.write(combined + "\t" + as_str + "\n");
            rrwriter.write(std::to_string(combined.size()) + "\t" + as_str + "\n");
        }
    }

    static void save_indices(const std::string& path, const std::vector<std::vector<int > >& mapping) {
        byteme::RawFileWriter rwriter(path);
        byteme::GzipFileWriter gwriter(path + ".gz");
        byteme::GzipFileWriter rrwriter(path + ".ranges.gz");
        for (const auto& x : mapping) {
            auto as_str = delta_encode(x);
            rwriter.write(as_str + "\n");
            gwriter.write(as_str + "\n");
            rrwriter.write(std::to_string(as_str.size()) + "\n");
        }
    }

    static void mock_database(const std::string& dir, const std::string& prefix) {
        if (std::filesystem::exists(dir)) {
            std::filesystem::remove_all(dir);
        }
        std::filesystem::create_directory(dir);

        // Saving the collection.
        {
            std::vector<std::string> payloads {
                "aaron's collection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net",
                "yet another collection\tsomeone else's collection\t9999\tSomeone else\thttps://someone.else.com"
            };
            std::vector<uint64_t> sizes { 3, 4 };
            auto path = dir + "/" + prefix + "collections.tsv";
            save_collections(path, payloads, sizes);
        }

        // Saving the sets and the token information.
        {
            std::vector<std::pair<std::string, std::string> > payloads = {
                { "Akira's set", "this is akira's set" },
                { "Alicia's set", "but this is alicia's set" },
                { "Athena's set", "can't forget about athena, of course" },
                { "Ai's set", "and there's also ai" },
                { "Alice's set", "and alice" },
                { "Aika's set", "I-I should also mention aika, b-baka" }, // throw in some dashes for the tsundere
                { "Akari's set", "But the best girl is still akari" }
            };
            std::vector<uint64_t> sizes{ 1, 3, 5, 7, 6, 4, 2 };
            auto path = dir + "/" + prefix + "sets.tsv";
            save_sets(path, payloads, sizes);

            std::unordered_map<std::string, std::vector<uint64_t> > token_n, token_d;
            for (size_t i = 0, end = payloads.size(); i < end; ++i) {
                const auto& p = payloads[i];
                gesel::internal::tokenize(i, p.first, token_n);
                gesel::internal::tokenize(i, p.second, token_d);
            }

            auto deposit_token_text = [&](const std::string& path, const std::unordered_map<std::string, std::vector<uint64_t> >& tokens_to_sets) {
                std::vector<std::string> all_tokens;
                all_tokens.reserve(tokens_to_sets.size());
                for (const auto& pp : tokens_to_sets) {
                    all_tokens.push_back(pp.first);
                }
                std::sort(all_tokens.begin(), all_tokens.end());

                byteme::RawFileWriter rwriter(path);
                byteme::GzipFileWriter rrwriter(path + ".ranges.gz");
                for (const auto& tok : all_tokens) {
                    auto encoded = delta_encode(tokens_to_sets.find(tok)->second);
                    rwriter.write(encoded + "\n");
                    rrwriter.write(tok + "\t" + std::to_string(encoded.size()) + "\n");
                }
            };
            deposit_token_text(dir + "/" + prefix + "tokens-names.tsv", token_n);
            deposit_token_text(dir + "/" + prefix + "tokens-descriptions.tsv", token_d);
        }

        // Saving the set->gene mappings, and vice versa.
        {
            std::vector<std::vector<int> > map_to = {
                { 0 },
                { 1, 3, 4 },
                { 2, 3, 7, 9, 13 },
                { 0, 5, 7, 10, 11, 12, 17 },
                { 8, 10, 14, 17, 18, 19 },
                { 2, 8, 9, 13 },
                { 6, 16 }
            };
            save_indices(dir + "/" + prefix + "set2gene.tsv", map_to);

            std::vector<std::vector<int> > map_from(max_genes);
            for (size_t s = 0, end = map_to.size(); s < end; ++ s) {
                for (auto i : map_to[s]) {
                    map_from[i].push_back(s);
                }
            }
            save_indices(dir + "/" + prefix + "gene2set.tsv", map_from);
        }
    }
};

TEST_F(TestValidateDatabase, Basic) {
    auto path = byteme::temp_file_path("validation");
    mock_database(path, "9606_");
    gesel::validate_database(path + "/9606_", max_genes);
}

TEST_F(TestValidateDatabase, CollectionFailures) {
    auto path = byteme::temp_file_path("validation");
    mock_database(path, "9606_");

    // Verify that the collections are indeed checked.
    {
        std::vector<std::string> payloads {
            "aaron's\tcollection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net", // extra tab pushes things out of the way.
            "yet another collection\tsomeone else's collection\t9999\tSomeone else\thttps://someone.else.com"
        };
        std::vector<uint64_t> sizes { 3, 4 };
        save_collections(path + "/9606_collections.tsv", payloads, sizes);
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "non-digit");
    }

    // Checking for overflow.
    {
        std::vector<std::string> payloads {
            "aaron's collection\tthis is aaron's collection\t12345\tAaron Lun\thttps://aaron.net",
            "yet another collection\tsomeone else's collection\t9999\tSomeone else\thttps://someone.else.com"
        };
        std::vector<uint64_t> sizes { std::numeric_limits<uint64_t>::max(), 4 };
        save_collections(path + "/9606_collections.tsv", payloads, sizes);
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "overflow");
    }
}

TEST_F(TestValidateDatabase, SetFailures) {
    auto path = byteme::temp_file_path("validation");
    mock_database(path, "9606_");

    {
        auto spath = path + "/9606_sets.tsv.ranges.gz";
        // Check the number of sets.
        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("5\t5\n6\t6\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "total number of sets");

        // Verify that the sets are indeed checked.
        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("1\t1\n2\t2\n3\t3\n4\t4\n5\t5\n6\t6\n7\t7\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "number of bytes");
    }

    // Checking tokens for the names.
    mock_database(path, "9606_");

    {
        auto spath = path + "/9606_tokens-names.tsv.ranges.gz";
        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("a\t1\nB C\t2\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "lower-case");

        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("a\t1\nb\t2\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "different number of tokens");

        std::vector<std::pair<std::string, std::string> > payloads = {
            { "alpha", "1" },
            { "bravo", "2" },
            { "charlie", "3" },
            { "delta", "4" },
            { "echo", "5" },
            { "foxtrot", "6" },
            { "golf", "7" }
        };
        std::vector<uint64_t> sizes{ 1, 2, 3, 4, 5, 6, 7 };
        save_sets(path + "/9606_sets.tsv", payloads, sizes);
        {
            byteme::GzipFileWriter rrwriter(spath);
            // No one should have zero bytes.
            rrwriter.write("alpha\t0\nbravo\t0\ncharlie\t0\ndelta\t0\necho\t0\nfoxtrot\t0\ngolf\t0\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "number of bytes");

        {
            byteme::GzipFileWriter rrwriter(spath);
            // Just get the correct number of bytes to get through the first tranche of index checks.
            // The first few tokens in 'tokens-names.tsv' should have exactly one (one-digit) set, because all of the
            // first names of our waifus start with A and are unique; so the number of bytes is 1.
            rrwriter.write("aaron\t1\nbravo\t1\ncharlie\t1\ndelta\t1\necho\t1\nfoxtrot\t1\ngolf\t1\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "not present");

        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("alpha\t1\nbravo\t1\ncharlie\t1\ndelta\t1\necho\t1\nfoxtrot\t1\ngolf\t1\n"); // again, just get the correct number of bytes. 
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "inconsistent");
    }

    // Cursory check for the description tokens.
    mock_database(path, "9606_");

    {
        auto spath = path + "/9606_tokens-descriptions.tsv.ranges.gz";
        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("a\t1\nB C\t2\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "lower-case");
    }
}

TEST_F(TestValidateDatabase, SetToGeneFailures) {
    auto path = byteme::temp_file_path("validation");
    mock_database(path, "9606_");

    {
        auto spath = path + "/9606_set2gene.tsv.ranges.gz";
        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("1\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "number of lines in 'set2gene");
    }

    mock_database(path, "9606_");
    {
        std::vector<std::vector<int> > map_to = {
            { 0 },
            { 1, 3, 4 },
            { 2, 3, 7, 9, 13 },
            { 0, 5, 7, 10, 11, 12, 17, 18 },
            { 8, 10, 14, 17, 18, 19 },
            { 2, 8, 9, 13 },
            { 6, 16 }
        };
        save_indices(path + "/9606_set2gene.tsv", map_to);
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "size of set");
    }

    mock_database(path, "9606_");
    {
        auto spath = path + "/9606_gene2set.tsv.ranges.gz";
        {
            byteme::GzipFileWriter rrwriter(spath);
            rrwriter.write("1\n");
        }
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "number of lines in 'gene2set");
    }

    mock_database(path, "9606_");
    {
        std::vector<std::vector<int> > map_to = {
            { 0 },
            { 1, 3, 4 },
            { 2, 3, 7, 9, 13 },
            { 0, 5, 7, 10, 11, 12, 18 },
            { 8, 10, 14, 17, 18, 19 },
            { 2, 8, 9, 13 },
            { 6, 16 }
        };
        save_indices(path + "/9606_set2gene.tsv", map_to);
        expect_error([&]() { gesel::validate_database(path + "/9606_", max_genes); }, "inconsistent");
    }
}
