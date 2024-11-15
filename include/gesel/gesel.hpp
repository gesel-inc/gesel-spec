#ifndef GESEL_GESEL_HPP
#define GESEL_GESEL_HPP

#include "check_collection_details.hpp"
#include "check_genes.hpp"
#include "check_indices.hpp"
#include "check_set_details.hpp"
#include "load_ranges.hpp"

#include <string>
#include <cstdint>
#include <stdexcept> 
#include <filesystem>

/**
 * @file gesel.hpp
 * @brief Validate Gesel database and gene files
 */

/**
 * @namespace gesel
 * @brief Validate Gesel database and gene files
 */
namespace gesel {

/**
 * Validate Gesel database files for a particular species.
 *
 * @param prefix Prefix for the Gesel database files.
 * This should be of the form `<DIRECTORY>/<SPECIES>_`, where `<SPECIES>` is an NCBI taxonomy ID.
 * @param num_genes Total number of genes for this species.
 */
inline void validate_database(const std::string& prefix, uint64_t num_genes) {
    uint64_t total_sets = 0;
    {
        auto coll_info = internal::load_ranges_with_sizes(prefix + "collections.tsv.ranges.gz");
        internal::check_collection_details(prefix + "collections.tsv", coll_info.first, coll_info.second);
        constexpr uint64_t limit = std::numeric_limits<uint64_t>::max();
        for (auto x : coll_info.second) {
            if (limit - total_sets < x) {
                throw std::runtime_error("64-bit unsigned integer overflow for the sum of the number of sets in 'collections.tsv.ranges.gz'");
            }
            total_sets += x;
        }
    }

    std::vector<uint64_t> set_sizes;
    {
        auto set_info = internal::load_ranges_with_sizes(prefix + "sets.tsv.ranges.gz");
        internal::check_set_details(prefix + "sets.tsv", set_info.first, set_info.second);
        if (static_cast<uint64_t>(set_info.first.size()) != total_sets) {
            throw std::runtime_error("total number of sets in 'sets.tsv' does not match with the reported number from 'collections.tsv.ranges.gz'");
        }
        set_sizes.swap(set_info.second);
    }

    std::vector<std::vector<uint64_t> > reverse_map(num_genes);
    {
        auto s2g_info = internal::load_ranges(prefix + "set2gene.tsv.ranges.gz");
        if (s2g_info.size() != static_cast<size_t>(total_sets)) {
            throw std::runtime_error("number of lines in 'set2gene.tsv.ranges.gz' does not match the total number of sets");
        }

        internal::check_indices<true>(
            prefix + "set2gene.tsv",
            num_genes,
            s2g_info,
            [&](uint64_t line, const std::vector<uint64_t>& indices) {
                if (static_cast<uint64_t>(indices.size()) != set_sizes[line]) {
                    throw std::runtime_error("size of set " + std::to_string(line) + " from 'sets.tsv.ranges.gz' does not match with that in 'set2gene.tsv'");
                }
                for (auto i : indices) {
                    reverse_map[i].push_back(line);
                }
            }
        );
    }

    {
        auto g2s_info = internal::load_ranges(prefix + "gene2set.tsv.ranges.gz");
        if (g2s_info.size() != static_cast<size_t>(num_genes)) {
            throw std::runtime_error("number of lines in 'gene2set.tsv.ranges.gz' does not match the total number of genes");
        }

        internal::check_indices<true>(
            prefix + "gene2set.tsv",
            total_sets,
            g2s_info,
            [&](uint64_t line, const std::vector<uint64_t>& indices) {
                const auto& expected = reverse_map[line];
                size_t num_expected = expected.size();
                if (num_expected != indices.size()) {
                    throw std::runtime_error("number of sets for gene " + std::to_string(line) + " in 'gene2set.tsv' is inconsistent with 'set2gene.tsv'");
                }
                for (size_t i = 0; i < num_expected; ++i) {
                    if (indices[i] != expected[i]) {
                        throw std::runtime_error("sets for gene " + std::to_string(line) + " in 'gene2set.tsv' are inconsistent with 'set2gene.tsv'");
                    }
                }
            }
        );
    }

    // TODO: check for correct tokenization.
    auto tname_info = internal::load_named_ranges(prefix + "tokens-names.tsv.ranges.gz");
    internal::check_indices<false>(prefix + "tokens-names.tsv", total_sets, tname_info);

    auto tdesc_info = internal::load_named_ranges(prefix + "tokens-descriptions.tsv.ranges.gz");
    internal::check_indices<false>(prefix + "tokens-descriptions.tsv", total_sets, tdesc_info);
} 

/**
 * Validate Gesel gene files for a particular species.
 *
 * @param prefix Prefix for the Gesel gene files.
 * This should be of the form `<DIRECTORY>/<SPECIES>_`, where `<SPECIES>` is an NCBI taxonomy ID.
 * @param types Vector of gene name types, e.g., `"ensembl"`, `"symbol"`.
 *
 * @return Number of genes.
 */
inline uint64_t validate_genes(const std::string& prefix, const std::vector<std::string>& types) {
    bool first = true;
    uint64_t num_genes = 0;
    for (auto t : types) {
        auto candidate = internal::check_genes(prefix + t + ".tsv.gz");
        if (first) {
            num_genes = candidate;
        } else if (candidate != num_genes) {
            throw std::runtime_error("inconsistent number of genes between types (" + std::to_string(num_genes) + " for " + types.front() + ", " + std::to_string(candidate) + " for " + t + ")");
        }
    }
    return num_genes;
}

/**
 * Overload for validating Gesel gene files for a particular species.
 * This will scan the directory for all files starting with `prefix`.
 *
 * @param prefix Prefix for the Gesel gene files.
 * This should be of the form `<DIRECTORY>/<SPECIES>_`, where `<SPECIES>` is an NCBI taxonomy ID.
 *
 * @return Number of genes.
 */
inline uint64_t validate_genes(const std::string& prefix) {
    std::vector<std::string> types;

    std::filesystem::path path(prefix);
    auto dir = path.parent_path();
    auto raw_prefix = path.filename().string();

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        if (name.rfind(raw_prefix, 0) != 0) {
            continue;
        }
        types.push_back(name.substr(raw_prefix.size(), name.size()));
    }

    return validate_genes(prefix, types);
}

}

#endif
