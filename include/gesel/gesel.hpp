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
inline void validate_database_files(const std::string& prefix, uint64_t num_genes) {
    auto coll_info = internal::load_ranges_with_sizes(prefix + "collections.tsv.ranges.gz"");
    internal::check_collection_details(prefix + "collections.tsv", coll_info.first, coll_info.second);
    uint64_t ncolls = coll_info.first.size();
    uint64_t total_sets = safe_accumulate(coll_info.second.begin(), coll_info.second.end());

    auto set_info = internal::load_ranges_with_sizes(prefix + "sets.tsv.ranges.gz"");
    internal::check_set_details(prefix + "sets.tsv", set_info.first, set_info.second);
    if (static_cast<uint64_t>(set_info.first.size()) != total_sets) {
        throw std::runtime_error("total number of sets in 'sets.tsv' does not match with the reported number from 'collections.tsv.ranges.gz'");
    }

    internal::check_indices<true>(
        prefix + "set2gene.tsv",
        num_genes,
        internal::load_ranges(prefix + "set2gene.tsv.ranges.gz"),
        [&](uint64_t line, const std::vector<uint64_t>& indices) {
            if (static_cast<uint64_t>(indices.size()) != coll_info.second[line]) {
                throw std::runtime_error("size of set " + std::to_string(line) + " from 'sets.tsv.ranges.gz' does not match with that in 'set2gene.tsv'"):
            }
        }
    );

    internal::check_indices<true>(
        prefix + "gene2set.tsv",
        total_sets,
        internal::load_ranges(prefix + "gene2set.tsv.ranges.gz")
    );

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
 */
inline void validate_gene_files(const std::string& prefix, const std::vector<std::string>& types) {
    for (auto t : types) {
        check_genes(prefix + t + ".tsv.gz");
    }
}

/**
 * Overload for validating Gesel gene files for a particular species.
 * This will scan the directory for all files starting with `prefix`.
 *
 * @param prefix Prefix for the Gesel gene files.
 * This should be of the form `<DIRECTORY>/<SPECIES>_`, where `<SPECIES>` is an NCBI taxonomy ID.
 */
inline void validate_gene_files(const std::string& prefix) {
    std::vector<std::string> types;

    std::filesystem::path path(prefix);
    auto dir = path.parent_path();
    auto raw_prefix = path.filename().string();

    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        if (name.rfind(raw_prefix, 0) != 0) {
            continue;
        }
        types.push_back(name.substr(raw_prefix.size(), name.size()));
    }

    validate_gene_files(prefix, types);
}

}

#endif
