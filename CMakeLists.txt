cmake_minimum_required(VERSION 3.24)

project(gesel
    VERSION 0.1.0
    DESCRIPTION "Validating the Gesel database files"
    LANGUAGES CXX)

add_library(gesel INTERFACE)
add_library(gesel::gesel ALIAS gesel)

include(GNUInstallDirs)
target_include_directories(gesel INTERFACE 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gesel_gesel>")

option(GESEL_FETCH_EXTERN "Automatically fetch gesel's external dependencies." ON)
if(GESEL_FETCH_EXTERN)
    add_subdirectory(extern)
else()
    find_package(ltla_byteme CONFIG REQUIRED)
endif()

target_link_libraries(gesel INTERFACE ltla::byteme)

option(GESEL_FIND_ZLIB "Try to find and link to Zlib for gesel." ON)
if(GESEL_FIND_ZLIB)
    find_package(ZLIB)
    if (ZLIB_FOUND)
        target_link_libraries(gesel INTERFACE ZLIB::ZLIB)
    endif()
endif()

# Building the test-related machinery, if we are compiling this library directly.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(GESEL_TESTS "Build gesel's test suite." ON)
else()
    option(GESEL_TESTS "Build gesel's test suite." OFF)
endif()

if(GESEL_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

# Installing for find_package.
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gesel_gesel)

install(TARGETS gesel
    EXPORT geselTargets)

install(EXPORT geselTargets
    FILE gesel_geselTargets.cmake
    NAMESPACE gesel::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gesel_gesel)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/gesel_geselConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gesel_gesel)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/gesel_geselConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gesel_geselConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/gesel_geselConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gesel_gesel)
