#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>
#include <filesystem>

#include "byteme/temp_file_path.hpp"
#include "gesel/validate_genes.hpp"
#include "utils.h"

TEST(ValidateGenes, Basic) {
    auto path = byteme::temp_file_path("validation");
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    std::filesystem::create_directory(path);

    {
        byteme::GzipFileWriter swriter(path + "/9606_symbol.tsv.gz");
        swriter.write("alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
        byteme::GzipFileWriter ewriter(path + "/9606_ensembl.tsv.gz");
        ewriter.write("ALPHA\nBRAVO\tCHARLIE\nDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    }
    EXPECT_EQ(gesel::validate_genes(path + "/9606_"), 6);
}

TEST(ValidateGenes, Failure) {
    auto path = byteme::temp_file_path("validation");
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
    std::filesystem::create_directory(path);

    {
        byteme::GzipFileWriter swriter(path + "/9606_symbol.tsv.gz");
        swriter.write("alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
        byteme::GzipFileWriter ewriter(path + "/9606_ensembl.tsv.gz");
        ewriter.write("ALPHA\nBRAVO\tCHARLIE\tDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    }
    expect_error([&]() { gesel::validate_genes(path + "/9606_"); }, "inconsistent");

    // Ignores failures in oddly  named things.
    {
        std::filesystem::remove_all(path);
        std::filesystem::create_directory(path);
        byteme::GzipFileWriter swriter(path + "/9606_symbol.tsv.gz");
        swriter.write("alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
        byteme::GzipFileWriter ewriter(path + "/9606_ensembl.tsv");
        ewriter.write("ALPHA\nBRAVO\tCHARLIE\tDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    }
    gesel::validate_genes(path + "/9606_");

    {
        std::filesystem::remove_all(path);
        std::filesystem::create_directory(path);
        byteme::GzipFileWriter swriter(path + "/9606_symbol.tsv.gz");
        swriter.write("alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
        byteme::GzipFileWriter ewriter(path + "/10116_ensembl.tsv.gz");
        ewriter.write("ALPHA\nBRAVO\tCHARLIE\tDELTA\tECHO\tFOXTROT\n\nGOLF\tHOTEL\nINDIA\n");
    }
    gesel::validate_genes(path + "/9606_");
}
