#include <array>
#include <iostream>
#include <ostream>
#include <string>

#include "rstd++/result.hpp"

using namespace rstd;
using namespace rstd::result;

// Example 1: Division with error handling
auto divide(int a, int b) -> Result<int, const char *>
{
    if (b == 0) {
        return Err<int, const char *>("Division by zero");
    }
    return Ok<int, const char *>(a / b);
}

void division_example()
{
    std::cout << "=== Division Example ===\n";

    auto r1 = divide(10, 2);
    if (r1.is_ok()) {
        std::cout << "10 / 2 = " << r1.unwrap() << "\n";
    }

    auto r2 = divide(10, 0);
    if (r2.is_err()) {
        std::cout << "Error: " << r2.unwrap_err() << "\n";
    }
    std::cout << "\n";
}

// Example 2: Validation with Void
auto validate_age(int age) -> Result<Void, const char *>
{
    if (age < 0) {
        return Err<Void, const char *>("Age cannot be negative");
    }
    if (age > 150) {
        return Err<Void, const char *>("Age too high");
    }
    return Ok<Void, const char *>(Void{});
}

void validation_example()
{
    std::cout << "=== Validation Example ===\n";

    std::array<int, 3> ages = {25, -5, 200};

    for (int age : ages) {
        auto result = validate_age(age);
        if (result.is_ok()) {
            std::cout << "Age " << age << " is valid\n";
        } else {
            std::cout << "Age " << age << " is invalid: " << result.unwrap_err()
                      << "\n";
        }
    }
    std::cout << "\n";
}

// Example 3: Parsing with Void error
auto parse_number(const std::string &s) -> Result<int, Void>
{
    try {
        return Ok<int, Void>(std::stoi(s));
    } catch (...) {
        return Err<int, Void>(Void{});
    }
}

void parsing_example()
{
    std::cout << "=== Parsing Example ===\n";

    std::array<std::string, 4> inputs = {"42", "123", "not a number", "456abc"};

    for (const auto &input : inputs) {
        auto result = parse_number(input);
        if (result.is_ok()) {
            std::cout << "Parsed '" << input << "' -> " << result.unwrap()
                      << "\n";
        } else {
            std::cout << "Failed to parse '" << input << "'\n";
        }
    }
    std::cout << "\n";
}

// Example 4: Chaining operations
void chaining_example()
{
    std::cout << "=== Chaining Example ===\n";

    auto result =
        divide(100, 5)
            .map([](int x) -> int { return x * 2; })
            .map([](int x) -> std::string { return std::to_string(x); });

    if (result.is_ok()) {
        std::cout << "Result: " << result.unwrap() << "\n";
    }
    std::cout << "\n";
}

void run_all()
{
    division_example();
    validation_example();
    parsing_example();
    chaining_example();
}

// ======================================================================
// Main
// ======================================================================

auto main() -> int
{
    std::cout << "rstd++ Result Examples\n";
    std::cout << "=======================\n" << std::endl;

    run_all();

    return 0;
}
