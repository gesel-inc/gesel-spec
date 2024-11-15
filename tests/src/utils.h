#ifndef UTILS_H
#define UTILS_H

#include <stdexcept>
#include <string>

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

#endif 
