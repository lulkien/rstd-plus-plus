#include <iostream>
#include <string>

#include "rstd++/result.hpp"

using namespace rstd;

// Example 1: Division with error handling
Result<int, std::string> divide(int a, int b)
{
    if (b == 0) {
        return Result<int, std::string>::Err("Division by zero");
    }
    return Result<int, std::string>::Ok(a / b);
}

void division_example()
{
    std::cout << "=== Division Example ===" << std::endl;

    auto r1 = divide(10, 2);
    if (r1.is_ok()) {
        std::cout << "10 / 2 = " << r1.unwrap() << std::endl;
    }

    auto r2 = divide(10, 0);
    if (r2.is_err()) {
        std::cout << "Error: " << r2.unwrap_err() << std::endl;
    }
    std::cout << std::endl;
}

// Example 2: Validation with Void
Result<Void, std::string> validate_age(int age)
{
    if (age < 0) {
        return Result<Void, std::string>::Err("Age cannot be negative");
    }
    if (age > 150) {
        return Result<Void, std::string>::Err("Age too high");
    }
    return Result<Void, std::string>::Ok(Void{});
}

void validation_example()
{
    std::cout << "=== Validation Example ===" << std::endl;

    int ages[] = {25, -5, 200};

    for (int age : ages) {
        auto result = validate_age(age);
        if (result.is_ok()) {
            std::cout << "Age " << age << " is valid" << std::endl;
        } else {
            std::cout << "Age " << age << " is invalid: " << result.unwrap_err() << std::endl;
        }
    }
    std::cout << std::endl;
}

// Example 3: Parsing with Void error
Result<int, Void> parse_number(const std::string &s)
{
    try {
        return Result<int, Void>::Ok(std::stoi(s));
    } catch (...) {
        return Result<int, Void>::Err(Void{});
    }
}

void parsing_example()
{
    std::cout << "=== Parsing Example ===" << std::endl;

    std::string inputs[] = {"42", "123", "not a number", "456abc"};

    for (const auto &input : inputs) {
        auto result = parse_number(input);
        if (result.is_ok()) {
            std::cout << "Parsed '" << input << "' -> " << result.unwrap() << std::endl;
        } else {
            std::cout << "Failed to parse '" << input << "'" << std::endl;
        }
    }
    std::cout << std::endl;
}

// Example 4: Chaining operations
void chaining_example()
{
    std::cout << "=== Chaining Example ===" << std::endl;

    auto result = divide(100, 5).map([](int x) { return x * 2; }).map([](int x) {
        return std::to_string(x);
    });

    if (result.is_ok()) {
        std::cout << "Result: " << result.unwrap() << std::endl;
    }
    std::cout << std::endl;
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

int main()
{
    std::cout << "rstd++ Result Examples" << std::endl;
    std::cout << "=======================" << std::endl << std::endl;

    run_all();


    return 0;
}
