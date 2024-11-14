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

namespace gesel {

inline void validate_indices(const std::string& prefix, uint64_t num_genes) {
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
        prefix + "gene2set.tsv", g2s_info, 
        total_sets,
        internal::load_ranges(prefix + "gene2set.tsv.ranges.gz"),
    );

    auto tname_info = internal::load_named_ranges(prefix + "tokens-names.tsv.ranges.gz");
    internal::check_indices<false>(
        prefix + "tokens-names.tsv",
        total_sets,
        tname_info.second
    );

    auto tdesc_info = internal::load_named_ranges(prefix + "tokens-descriptions.tsv.ranges.gz");
    internal::check_indices<false>(
        prefix + "tokens-descriptions.tsv",
        total_sets,
        tdesc_info.second
    );
} 

}


#endif
