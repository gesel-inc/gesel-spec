#ifndef GESEL_VALIDATE_GENES_HPP
#define GESEL_VALIDATE_GENES_HPP

#include "check_genes.hpp"
#include "check_version.hpp"
#include "utils.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept> 
#include <filesystem>
#include <optional>

#include "sanisizer/sanisizer.hpp"

/**
 * @file validate_genes.hpp
 * @brief Validate gene files.
 */

namespace gesel {

/**
 * Validate Gesel gene annotation files (version 0.1.0) for a particular species.
 * Any invalid formatting or inconsistency between files will result in an error.
 *
 * In general, users should call the `validate_genes()` overload that does not accept `types`.
 * This specific overload is only provided to enable validation in the unfortunate case where the gene annotation files are saved in the same directory as the database files.
 *
 * @param prefix Prefix for the Gesel gene annotation files.
 * This should be of the form `<DIRECTORY>/<SPECIES>_`, where `<SPECIES>` is an NCBI taxonomy ID.
 * @param types Vector of gene identifier types, e.g., `"ensembl"`, `"symbol"`.
 * This should contain at least one value.
 *
 * @return Number of genes.
 */
inline std::uint64_t validate_genes(const std::string& prefix, const std::vector<std::string>& types) {
    std::optional<std::uint64_t> num_genes;

    for (auto t : types) {
        auto candidate = internal::check_genes(prefix + t + ".tsv.gz");
        if (!num_genes.has_value()) {
            num_genes = candidate;
        } else if (candidate != *num_genes) {
            throw std::runtime_error(
                "inconsistent number of genes between types (" + 
                std::to_string(*num_genes) + " for " + types.front() + ", " +
                std::to_string(candidate) + " for " + t + ")"
            );
        }
    }

    if (!num_genes.has_value()) {
        throw std::runtime_error("at least one gene name type should be present");
    }
    return *num_genes;
}

/**
 * Validate Gesel gene annotation files of any version for a particular species.
 * Any invalid formatting or inconsistency between files will result in an error.
 *
 * For version 0.1.0, this function scans for all files starting with `prefix` and ending with `".tsv.gz"`, i.e., following the `<prefix><type>.tsv.gz` pattern.
 * This may not work correctly when the gene annotation files are saved in the same directory as the database files.
 *
 * @param prefix Prefix for the Gesel gene annotation files.
 *
 * @return Number of genes.
 */
inline std::uint64_t validate_genes(const std::string& prefix) {
    const std::string vpath = prefix + "gene-version.tsv";
    internal::Version version;
    if (std::filesystem::exists(vpath)) { 
        version = internal::check_version(vpath);
    }

    std::vector<std::string> types;
    if (version < internal::Version(0, 2, 0)) {
        // For version 0.1.*, the available types is inferred from the available files in the directory.
        // This is kinda fragile as it assumes that no other files start with 'prefix',
        // so you can't really mix the gene files with the database files.
        // It's also hard to query for the available types if the files aren't already on your filesystem.
        std::filesystem::path path(prefix);
        auto dir = path.parent_path();
        auto raw_prefix = path.filename().string();

        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            std::string name = entry.path().filename().string();
            if (name.rfind(raw_prefix, 0) != 0) {
                continue;
            }
            if (name.size() < 6) {
                continue;
            }
            const auto ext_loc = name.size() - 7;
            if (name.rfind(".tsv.gz", ext_loc) != ext_loc) {
                continue;
            }
            types.push_back(name.substr(raw_prefix.size(), ext_loc - raw_prefix.size()));
        }

        return validate_genes(prefix, types);

    } else {
        // For versions at or above 0.2.0, we use a dedicated 'gene-types.tsv' file to specify the available types.
        // This can be easily consulted to figure out the available types.
        // Each file is also prefixed with 'gene-type-' to avoid conflicts with Gesel database files.
        const auto manifest_path = prefix + "gene-types.tsv";
        byteme::RawFileReader reader(manifest_path.c_str(), {});
        byteme::SerialBufferedReader<char, internal::I<decltype(&reader)> > pb(&reader, sanisizer::cap<std::size_t>(65536));
        if (!pb.valid()) {
            throw std::runtime_error("expected at least one gene identifier type in '" + manifest_path + "'");
        }

        while (1) {
            std::string type;
            char current = pb.get();
            while (current != '\n') {
                if ((current >= 'a' && current <= 'z') || (current >= 'A' && current <= 'Z') || (current >= '0' && current <= '9')) {
                    type += current;
                } else {
                    throw std::runtime_error("gene identifier type should only contain alphanumeric characters in '" + manifest_path + "'");
                }
                if (!pb.advance()) {
                    throw std::runtime_error("premature termination of gene identifier type in '" + manifest_path + "'");
                }
                current = pb.get();
            }

            if (type.size() == 0) {
                throw std::runtime_error("empty gene identifier type in '" + manifest_path + "'");
            }
            types.push_back(std::move(type));

            if (!pb.advance()) {
                break;
            }
        }

        return validate_genes(prefix + "gene-type-", types);
    }
}

}

#endif
