#include "../include/rstd/result.hpp"
#include <iostream>
#include <string>

Result<int, std::string> parse_number(const std::string &s) {
    try {
        return Ok<int, std::string>(std::stoi(s));
    } catch (...) {
        return Err<int, std::string>("Parse failed: " + s);
    }
}

Result<int, std::string> get_default() {
    std::cout << "Getting default value...\n";
    return Ok<int, std::string>(0);
}

Result<int, std::string> get_backup() {
    std::cout << "Getting backup value...\n";
    return Ok<int, std::string>(-1);
}

int main() {
    return 0;
}
