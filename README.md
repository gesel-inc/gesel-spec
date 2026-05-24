# Gesel file specifications 

![Unit tests](https://github.com/gesel-inc/gesel-spec/actions/workflows/run-tests.yaml/badge.svg)
![Documentation](https://github.com/gesel-inc/gesel-spec/actions/workflows/doxygenate.yaml/badge.svg)
[![codecov](https://codecov.io/gh/gesel-inc/gesel-spec/graph/badge.svg?token=8J0JL45VU7)](https://codecov.io/gh/gesel-inc/gesel-spec)

## Overview

The [Gesel database](https://doi.org/10.21105/joss.05777) uses client-side HTTP range requests to extract gene sets and other details.
This document describes the file formats used by Gesel clients.
Once created, the files can be hosted on a static file server without requiring any more logic.

## Specifications

The Gesel database files contain information about the gene set collections.
Links to the specifications for these files are listed below:

- [0.1.0](docs/specifications/db_0.1.0.md)

The Gesel gene annotation files contain information about the genes involved in the gene sets.
Links to the specifications for these files are listed below:

- [0.1.0](docs/specifications/gene_0.1.0.md)
- [0.2.0](docs/specifications/gene_0.2.0.md)

Check out the [feedstock repository](https://github.com/gesel-inc/feedstock) for some concrete instances of these specifications.

## Validation

### Quick start

Given a suite of Gesel files, we can use the **gesel** C++ library to validate their formatting.

```cpp
#include "gesel/gesel.hpp"

auto num_genes = gesel::validate_genes("my/path/to/genes/9606_");
gesel::validate_database("my/path/to/db/9606_", num_genes);
```

This takes the path prefix to a species-specific suite of Gesel files and validates their contents,
throwing an error if any invalid formatting is detected.
Note that the gene mapping files can be stored in a different directory from the other files.

Check out the [reference documentation](https://gesel-inc.github.io/gesel-spec) for more information.

### Versions

The following table lists the correspondence between the **gesel** library and the various Gesel file specifications.

|Library|Database|Gene|
|----|------|----|
|0.1.\*|0.1.0|0.1.0|
|0.2.\*|0.1.0|0.1.0, 0.2.0|

### Building projects

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  gesel 
  GIT_REPOSITORY https://github.com/gesel-inc/gesel-spec
  GIT_TAG master # or any version of interest
)

FetchContent_MakeAvailable(gesel)
```

Then you can link to **gesel** to make the headers available during compilation:

```cmake
# For executables:
target_link_libraries(myexe gesel)

# For libaries
target_link_libraries(mylib INTERFACE gesel)
```

Alternatively, you can install the library by cloning a suitable version of this repository and running the following commands:

```sh
mkdir build && cd build
cmake .. -DGESEL_TESTS=OFF
cmake --build . --target install
```

Then you can use `find_package()` as usual:

```cmake
find_package(gesel_gesel CONFIG REQUIRED)
target_link_libraries(mylib INTERFACE gesel::gesel)
```

If you're not using CMake, the simple approach is to just copy the files in the `include/` subdirectory - 
either directly or with Git submodules - and include their path during compilation with, e.g., GCC's `-I`.
You will also need to link to the dependencies listed in the [`extern/CMakeLists.txt`](extern/CMakeLists.txt) directory. 
