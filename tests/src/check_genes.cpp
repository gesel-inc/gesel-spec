#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "gesel/check_genes.hpp"
#include "byteme/temp_file_path.hpp"

#include "utils.h"

TEST(CheckGenes, Success) {
    auto path = byteme::temp_file_path("check_genes") + ".gz";

    {
        byteme::GzipFileWriter gwriter(path);
        gwriter.write("alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    }
    EXPECT_EQ(gesel::internal::check_genes(path), 6);

    // Duplicate names across lines is okay.
    {
        byteme::GzipFileWriter gwriter(path);
        gwriter.write("alpha\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\talpha\n");
    }
    EXPECT_EQ(gesel::internal::check_genes(path), 6);
}

TEST(CheckGenes, Failure) {
    auto path = byteme::temp_file_path("check_genes") + ".gz";

    {
        byteme::GzipFileWriter gwriter(path);
        gwriter.write("alpha\t\nbravo\tcharlie\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    }
    expect_error([&]() { gesel::internal::check_genes(path); }, "empty name");

    {
        byteme::GzipFileWriter gwriter(path);
        gwriter.write("alpha\nbravo\tcharlie\tbravo\ndelta\techo\tfoxtrot\n\ngolf\thotel\nindia\n");
    }
    expect_error([&]() { gesel::internal::check_genes(path); }, "duplicated names");
}