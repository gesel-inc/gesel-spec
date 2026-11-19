#ifndef GESEL_VALIDATE_DATABASE_HPP
#define GESEL_VALIDATE_DATABASE_HPP

#include "check_collection_details.hpp"
#include "check_indices.hpp"
#include "check_set_details.hpp"
#include "load_ranges.hpp"

#include <string>
#include <cstdint>
#include <stdexcept> 
#include <exception>
#include <vector>
#include <unordered_map>

#include "sanisizer/sanisizer.hpp"

/**
 * @file validate_database.hpp
 * @brief Validate database files.
 */

namespace gesel {

/**
 * @cond
 */
namespace internal {

inline void tokenize(std::uint64_t index, const std::string& text, std::unordered_map<std::string, std::vector<std::uint64_t> >& tokens_to_sets) {
    std::string latest;
    auto add = [&]() -> void {
        if (latest.size()) {
            auto& vec = tokens_to_sets[latest];
            if (vec.empty() || vec.back() != index) {
                vec.push_back(index);
            }
            latest.clear();
        }
    };

    for (auto x : text) {
        // Avoid using std::tolower() as this is locale-dependent.
        if (x >= 'A' && x <= 'Z') {
            x += 32;
        }
        if (invalid_token_character(x)) {
            add();
        } else {
            latest += x;
        }
    }

    add();
}

inline void check_tokens(const std::vector<std::string>& tokens) {
    const auto num_tokens = tokens.size();
    for (I<decltype(num_tokens)> t = 0; t < num_tokens; ++t) {
        const auto& token = tokens[t];
        try {
            if (token.empty()) {
                throw std::runtime_error("token should not be an empty string");
            }

            for (auto x : token) {
                if (invalid_token_character(x)) {
                    throw std::runtime_error("token should only contain lower-case alphabetical characters, digits or a dash");
                }
            }

            if (t && token <= tokens[t - 1]) {
                throw std::runtime_error("tokens should be unique and lexicographically sorted");
            } 
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate token '" + token + "' on line " + to_string(t))); 
        }
    }
}

}
/**
 * @endcond
 */

/**
 * Validate Gesel database files for a particular species.
 * Any invalid formatting or inconsistency between files will result in an error.
 *
 * @param prefix Prefix for the Gesel database files.
 * @param num_genes Total number of genes for this species.
 * This should be obtained from the accompanying gene annotation files, validated by `validate_genes()`.
 */
inline void validate_database(const std::string& prefix, std::uint64_t num_genes) {
    // Validating the collection details first.
    std::uint64_t total_sets = 0;
    {
        auto coll_info = [&]{
            try {
                auto info = internal::load_ranges_with_sizes(prefix + "collections.tsv.ranges.gz");
                for (auto x : info.second) {
                    auto added = sanisizer::sum_maybe<std::uint64_t>(total_sets, x);
                    if (!added.has_value()) {
                        throw std::overflow_error("integer overflow for the sum of the number of sets");
                    }
                    total_sets = *added;
                }
                return info;
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate 'collections.tsv.ranges.gz'"));
            }
        }();

        try {
            internal::check_collection_details(prefix + "collections.tsv", coll_info.first, coll_info.second);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'collections.tsv'"));
        }
    }

    // Now validating the set details.
    std::vector<std::uint64_t> set_sizes;
    {
        auto set_info = [&]{
            try {
                auto info = internal::load_ranges_with_sizes(prefix + "sets.tsv.ranges.gz");
                if (!sanisizer::is_equal(info.first.size(), total_sets)) {
                    throw std::runtime_error("number of sets does not match the sum of set sizes from 'collections.tsv.ranges.gz'");
                }
                return info;
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate 'sets.tsv.ranges.gz'"));
            }
        }();
        set_sizes.swap(set_info.second);

        std::unordered_map<std::string, std::vector<std::uint64_t> > token_n, token_d;
        try {
            internal::check_set_details(
                prefix + "sets.tsv",
                set_info.first,
                set_sizes,
                [&](std::uint64_t line, const std::string& name, const std::string& description) {
                    internal::tokenize(line, name, token_n);
                    internal::tokenize(line, description, token_d);
                }
            );
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'sets.tsv'"));
        }

        // Check for correct tokenization.
        for (int tt = 0; tt < 2; ++tt) {
            std::string type = (tt == 0 ? "names" : "descriptions");
            const auto& tokens = (tt == 0 ? token_n : token_d);

            auto path = "tokens-" + type + ".tsv";
            auto ranges_path = path + ".ranges.gz";
            auto tok_info = [&]{
                try {
                    auto info = internal::load_named_ranges(prefix + ranges_path);
                    internal::check_tokens(info.first);
                    if (info.first.size() != tokens.size()) {
                        throw std::runtime_error("number of tokens is different to that derived from the " + type + " of 'sets.tsv'");
                    }
                    return info;
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate '" + ranges_path + "'"));
                }
            }();

            bool skip_err_wrap = false;
            try {
                internal::check_indices<false>(
                    prefix + path,
                    total_sets,
                    tok_info.second,
                    [&](std::uint64_t line, const std::vector<std::uint64_t>& indices) {
                        const auto& tok = tok_info.first[line];
                        auto tIt = tokens.find(tok);
                        if (tIt == tokens.end()) {
                            skip_err_wrap = true;
                            throw std::runtime_error("token '" + tok + "' in '" + ranges_path + "' is not present in the " + type + " of 'sets.tsv'");
                        } else if (tIt->second != indices) {
                            throw std::runtime_error("sets for token '" + tok + "' are inconsistent with the " + type + " of 'sets.tsv'");
                        }
                    }
                );
            } catch (...) {
                if (skip_err_wrap) {
                    throw; // error message isn't really related to the token->set mappings.
                } else {
                    std::throw_with_nested(std::runtime_error("failed to validate '" + path + "'"));
                }
            }
        }
    }

    // Check for correct mapping of sets to genes.
    std::vector<std::vector<std::uint64_t> > reverse_map(num_genes);
    {
        auto s2g_info = [&]{
            try {
                auto info = internal::load_ranges(prefix + "set2gene.tsv.ranges.gz");
                if (!sanisizer::is_equal(info.size(), total_sets)) {
                    throw std::runtime_error("number of lines does not match the number of sets from 'sets.tsv'");
                }
                return info;
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate 'set2gene.tsv.ranges.gz'"));
            }
        }();

        try {
            internal::check_indices<true>(
                prefix + "set2gene.tsv",
                num_genes,
                s2g_info,
                [&](std::uint64_t line, const std::vector<std::uint64_t>& indices) {
                    if (!sanisizer::is_equal(indices.size(), set_sizes[line])) {
                        throw std::runtime_error("size of set " + std::to_string(line) + " does not match the size from 'sets.tsv.ranges.gz'");
                    }
                    for (auto i : indices) {
                        reverse_map[i].push_back(line);
                    }
                }
            );
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'set2gene.tsv'"));
        }
    }

    // And making sure that the reverse mapping is consistent.
    {
        auto g2s_info = [&]{
            try {
                auto info = internal::load_ranges(prefix + "gene2set.tsv.ranges.gz");
                if (!sanisizer::is_equal(info.size(), num_genes)) {
                    throw std::runtime_error("number of lines does not match the total number of genes");
                }
                return info;
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate 'gene2set.tsv.ranges.gz'"));
            }
        }();

        try {
            internal::check_indices<true>(
                prefix + "gene2set.tsv",
                total_sets,
                g2s_info,
                [&](std::uint64_t line, const std::vector<std::uint64_t>& indices) {
                    if (reverse_map[line] != indices) {
                        throw std::runtime_error("sets for gene " + std::to_string(line) + " are inconsistent with their memberships in 'set2gene.tsv'");
                    }
                }
            );
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'gene2set.tsv'"));
        }
    }
} 

}

#endif
