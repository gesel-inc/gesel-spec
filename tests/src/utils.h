#ifndef UTILS_H
#define UTILS_H

#include <stdexcept>
#include <string>
#include <filesystem>
#include <random>

template<typename Function_>
void expect_error(Function_ fun, std::string match) {
    bool failed = false;
    std::string msg;

    try {
        fun();
    } catch(std::exception& e) {
        failed = true;
        msg = e.what();
    }

    ASSERT_TRUE(failed) << "function did not throw an exception with message '" << match << "'";
    ASSERT_TRUE(msg.find(match) != std::string::npos) << "function did not throw an exception with message '" << match << "' (got '" << msg << "' instead)";
}

inline std::string temp_file_path(const std::string& base) {
    auto tmpdir = std::filesystem::temp_directory_path();
    auto prefix = tmpdir / base;
    std::filesystem::path full;

    std::random_device rd;
    std::mt19937_64 rng(rd());
    do {
        full = prefix;
        full += std::to_string(rng());
    } while (std::filesystem::exists(full));

    return full;
}

#endif 
