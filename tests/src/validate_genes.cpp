#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>
#include <filesystem>

#include "gesel/validate_genes.hpp"
#include "utils.h"

TEST(ValidateGenes, Basic_010) {
    auto path = temp_file_path("validation");
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    std::filesystem::create_directory(path);

    quick_gzip_write(path + "/9606_symbol.tsv.gz", "alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    quick_gzip_write(path + "/9606_ensembl.tsv.gz", "ALPHA\nBRAVO\tCHARLIE\nDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    EXPECT_EQ(gesel::validate_genes(path + "/9606_"), 6);
}

TEST(ValidateGenes, Failure_010) {
    auto path = temp_file_path("validation");
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    std::filesystem::create_directory(path);

    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "at least one");

    quick_gzip_write(path + "/9606_symbol.tsv.gz", "alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    quick_gzip_write(path + "/9606_ensembl.tsv.gz", "ALPHA\nBRAVO\tCHARLIE\tDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "number of genes is different");

    // Ignores failures in oddly named files.
    std::filesystem::remove_all(path);
    std::filesystem::create_directory(path);
    quick_gzip_write(path + "/9606_symbol.tsv.gz", "alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    quick_gzip_write(path + "/9606_ensembl.tsv", "ALPHA\nBRAVO\tCHARLIE\tDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    gesel::validate_genes(path + "/9606_");

    std::filesystem::remove_all(path);
    std::filesystem::create_directory(path);
    quick_gzip_write(path + "/9606_symbol.tsv.gz", "alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    quick_gzip_write(path + "/10116_ensembl.tsv.gz", "ALPHA\nBRAVO\tCHARLIE\tDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    gesel::validate_genes(path + "/9606_");
}

TEST(ValidateGenes, Basic_020) {
    auto path = temp_file_path("validation");
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    std::filesystem::create_directory(path);

    quick_text_write(path + "/9606_gene-version.tsv", "0.2.0\n");
    quick_text_write(path + "/9606_gene-types.tsv", "symbol\nensembl\n");
    quick_gzip_write(path + "/9606_gene-type-symbol.tsv.gz", "alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    quick_gzip_write(path + "/9606_gene-type-ensembl.tsv.gz", "ALPHA\nBRAVO\tCHARLIE\nDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    EXPECT_EQ(gesel::validate_genes(path + "/9606_"), 6);

    // Ignores failues in oddly named files.
    quick_gzip_write(path + "/9606_gene-type-foo.tsv.gz", "\n");
    EXPECT_EQ(gesel::validate_genes(path + "/9606_"), 6);
}

TEST(ValidateGenes, Error_020) {
    auto path = temp_file_path("validation");
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    std::filesystem::create_directory(path);

    quick_text_write(path + "/9606_gene-version.tsv", "0.2.0\n");
    quick_text_write(path + "/9606_gene-types.tsv", "");
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "expected at least one");

    quick_text_write(path + "/9606_gene-types.tsv", "\n");
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "empty gene identifier");

    quick_text_write(path + "/9606_gene-types.tsv", "sy mbol\nensembl\n");
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "only contain alphanumeric");

    quick_text_write(path + "/9606_gene-types.tsv", "symbol\nensembl");
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "premature");

    quick_text_write(path + "/9606_gene-types.tsv", "symbol\nensembl\n");
    quick_gzip_write(path + "/9606_gene-type-symbol.tsv.gz", "alpha\n");
    quick_gzip_write(path + "/9606_gene-type-ensembl.tsv.gz", "ALPHA\nBRAVO\tCHARLIE\nDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "number of genes is different");
}
