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
    gesel::internal::tokenize(2, "Aaron and Aaron", tokens_to_sets);
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
            std::vector<int> sizes { 3, 4 };

            auto path = dir + "/" + prefix + "collections.tsv";
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
            std::vector<int> sizes { 1, 3, 5, 7, 6, 4, 2 };

            {
                auto path = dir + "/" + prefix + "sets.tsv";
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

            {
                auto path = dir + "/" + prefix + "set2gene.tsv";
                byteme::RawFileWriter rwriter(path);
                byteme::GzipFileWriter gwriter(path + ".gz");
                byteme::GzipFileWriter rrwriter(path + ".ranges.gz");
                for (const auto& x : map_to) {
                    auto as_str = delta_encode(x);
                    rwriter.write(as_str + "\n");
                    gwriter.write(as_str + "\n");
                    rrwriter.write(std::to_string(as_str.size()) + "\n");
                }
            }

            std::vector<std::vector<int> > map_from(max_genes);
            for (size_t s = 0, end = map_to.size(); s < end; ++ s) {
                for (auto i : map_to[s]) {
                    map_from[i].push_back(s);
                }
            }

            {
                auto path = dir + "/" + prefix + "gene2set.tsv";
                byteme::RawFileWriter rwriter(path);
                byteme::GzipFileWriter gwriter(path + ".gz");
                byteme::GzipFileWriter rrwriter(path + ".ranges.gz");
                for (const auto& x : map_from) {
                    auto as_str = delta_encode(x);
                    rwriter.write(as_str + "\n");
                    gwriter.write(as_str + "\n");
                    rrwriter.write(std::to_string(as_str.size()) + "\n");
                }
            }
        }
    }
};

TEST_F(TestValidateDatabase, Basic) {
    auto path = byteme::temp_file_path("validation");
    mock_database(path, "9606_");
    gesel::validate_database(path + "/9606_", max_genes);
}
