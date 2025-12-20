/**
 * @file result.hpp
 * @brief A Rust-inspired Result type for C++ error handling
 *
 * This file provides a Result<T, E> type that represents either success (Ok) or failure (Err).
 * It offers a type-safe alternative to exceptions and error codes, supporting both void and
 * non-void types for both success and error cases.
 */

#pragma once

#include <optional>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <utility>
#include <variant>

namespace rstd
{

/**
 * @brief Tag type for constructing Ok variants
 */
struct OkTag
{};

/**
 * @brief Wrapper for non-void value types
 * @tparam T The value type
 */
struct ErrTag
{};

/**
 * @brief Wrapper for non-void value types
 * @tparam T The value type
 */
template <typename T> struct value_type
{
    T value; ///< The contained value

    /**
     * @brief Construct from const reference
     * @param d The value to store
     */
    value_type(const T &d) : value{d} {}

    /**
     * @brief Construct from rvalue reference
     * @param d The value to move
     */
    value_type(T &&d) : value{std::move(d)} {}
};

/**
 * @brief Specialization for void value type
 */
template <> struct value_type<void>
{
    /**
     * @brief Default constructor for void variant
     */
    value_type() {}
};

/**
 * @brief Wrapper for non-void error types
 * @tparam E The error type
 */
template <typename E> struct error_type
{
    E error; ///< The contained error

    /**
     * @brief Construct from const reference
     * @param e The error to store
     */
    error_type(const E &e) : error{e} {}

    /**
     * @brief Construct from rvalue reference
     * @param e The error to move
     */
    error_type(E &&e) : error{std::move(e)} {}
};

/**
 * @brief Specialization for void error type
 */
template <> struct error_type<void>
{
    /**
     * @brief Default constructor for void variant
     */
    error_type() {}
};

/**
 * @brief Concept for non-void types
 * @tparam T Type to check
 */
template <typename T>
concept NotVoid = !std::is_void_v<T>;

/**
 * @brief Concept for void type
 * @tparam T Type to check
 */
template <typename T>
concept IsVoid = std::is_void_v<T>;

/**
 * @brief Concept for types that can be printed to an ostream
 * @tparam T Type to check
 */
template <typename T>
concept Printable = requires(std::ostream &os, const T &v) { os << v; };

/**
 * @brief Concept for default constructible non-void types
 * @tparam T Type to check
 */
template <typename T>
concept DefaultConstructible = !std::is_void_v<T> && std::is_default_constructible_v<T>;

/**
 * @brief Concept for copy constructible non-void types
 * @tparam T Type to check
 */
template <typename T>
concept CopyConstructible = !std::is_void_v<T> && std::is_copy_constructible_v<T>;

/**
 * @brief A type representing either success (Ok) or failure (Err)
 *
 * Result<T, E> is a type that represents either success with a value of type T,
 * or failure with an error of type E. Both T and E can be void.
 *
 * This provides a type-safe way to handle errors without exceptions, inspired by Rust's Result
 * type.
 *
 * @tparam T The type of the success value (can be void)
 * @tparam E The type of the error value (can be void)
 *
 * @example
 * ```cpp
 * Result<int, std::string> divide(int a, int b) {
 *     if (b == 0) {
 *         return Result<int, std::string>::Err("Division by zero");
 *     }
 *     return Result<int, std::string>::Ok(a / b);
 * }
 *
 * auto result = divide(10, 2);
 * if (result.is_ok()) {
 *     std::cout << "Result: " << result.unwrap() << std::endl;
 * }
 * ```
 */
template <typename T, typename E> class Result
{
    std::variant<value_type<T>, error_type<E>> data; ///< Internal storage for either Ok or Err

    /**
     * @brief Private constructor for Ok variant with const reference
     * @param v The value to store
     */
    Result(OkTag, const T &v)
        requires NotVoid<T>
        : data{value_type<T>{v}}
    {}

    /**
     * @brief Private constructor for Ok variant with rvalue reference
     * @param v The value to move
     */
    Result(OkTag, T &&v)
        requires NotVoid<T>
        : data{value_type<T>{std::move(v)}}
    {}

    /**
     * @brief Private constructor for Ok variant with void type
     */
    Result(OkTag)
        requires IsVoid<T>
        : data{value_type<void>{}}
    {}

    /**
     * @brief Private constructor for Err variant with const reference
     * @param e The error to store
     */
    Result(ErrTag, const E &e)
        requires NotVoid<E>
        : data{error_type<E>{e}}
    {}

    /**
     * @brief Private constructor for Err variant with rvalue reference
     * @param e The error to move
     */
    Result(ErrTag, E &&e)
        requires NotVoid<E>
        : data{error_type<E>{std::move(e)}}
    {}

    /**
     * @brief Private constructor for Err variant with void type
     */
    Result(ErrTag)
        requires IsVoid<E>
        : data{error_type<void>{}}
    {}

    /**
     * @brief Helper function that throws an exception with formatted error message
     * @tparam ExpectedVariant The variant type that was expected
     * @param msg The error message prefix
     * @throws std::runtime_error Always throws with formatted message
     */
    template <typename expect_type>
    [[noreturn]]
    void unwrap_failed(const char *msg) const
    {
        std::ostringstream oss;
        oss << msg << ": ";

        if constexpr (std::is_same_v<expect_type, value_type<T>>) {
            if constexpr (IsVoid<E>) {
                oss << "()";
            } else {
                const auto &err = std::get<error_type<E>>(data).error;
                if constexpr (Printable<E>) {
                    oss << err;
                } else {
                    oss << "<unprintable>";
                }
            }
        } else {
            if constexpr (IsVoid<T>) {
                oss << "()";
            } else {
                const auto &val = std::get<value_type<T>>(data).value;
                if constexpr (Printable<T>) {
                    oss << val;
                } else {
                    oss << "<unprintable>";
                }
            }
        }

        throw std::runtime_error(oss.str());
    }

public:
    // Friend declarations for factory functions
    template <typename U, typename V>
    friend Result<U, V> Ok(const U &v)
        requires NotVoid<U>;

    template <typename U, typename V>
    friend Result<U, V> Ok(U &&v)
        requires NotVoid<U>;

    template <typename V> friend Result<void, V> Ok();

    template <typename U, typename V>
    friend Result<U, V> Err(const V &e)
        requires NotVoid<V>;

    template <typename U, typename V>
    friend Result<U, V> Err(V &&e)
        requires NotVoid<V>;

    template <typename U> friend Result<U, void> Err();

    // ======================================================================
    // Object creations
    // ======================================================================

    /**
     * @brief Create an Ok result with a const reference value
     * @param value The value to store
     * @return A Result containing the Ok value
     */
    static Result Ok(const T &value)
        requires NotVoid<T>
    {
        return Result(OkTag{}, value);
    }

    /**
     * @brief Create an Ok result with an rvalue reference value
     * @param value The value to move
     * @return A Result containing the Ok value
     */
    static Result Ok(T &&value)
        requires NotVoid<T>
    {
        return Result(OkTag{}, std::move(value));
    }

    /**
     * @brief Create an Ok result with void type
     * @return A Result containing the Ok (void) state
     */
    static Result Ok()
        requires IsVoid<T>
    {
        return Result(OkTag{});
    }

    /**
     * @brief Create an Err result with a const reference error
     * @param error The error to store
     * @return A Result containing the Err value
     */
    static Result Err(const E &error)
        requires NotVoid<E>
    {
        return Result(ErrTag{}, error);
    }

    /**
     * @brief Create an Err result with an rvalue reference error
     * @param error The error to move
     * @return A Result containing the Err value
     */
    static Result Err(E &&error)
        requires NotVoid<E>
    {
        return Result(ErrTag{}, std::move(error));
    }

    /**
     * @brief Create an Err result with void type
     * @return A Result containing the Err (void) state
     */
    static Result Err()
        requires IsVoid<E>
    {
        return Result(ErrTag{});
    }

    // ======================================================================
    // Querying the contained values
    // ======================================================================

    /**
     * @brief Check if the result is Ok
     * @return true if the result contains an Ok value, false otherwise
     */
    bool is_ok() const { return std::holds_alternative<value_type<T>>(data); }

    /**
     * @brief Check if the result is Ok and the value satisfies a predicate (const lvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const T& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_ok_and(Pred pred) const &
        requires NotVoid<T>
    {
        return is_ok() && pred(std::get<value_type<T>>(data).value);
    }

    /**
     * @brief Check if the result is Ok and satisfies a predicate (const lvalue, void T)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes no arguments and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_ok_and(Pred pred) const &
        requires IsVoid<T>
    {
        return is_ok() && pred();
    }

    /**
     * @brief Check if the result is Ok and the value satisfies a predicate (rvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes T&& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_ok_and(Pred pred) &&
        requires NotVoid<T>
    {
        return is_ok() && pred(std::move(std::get<value_type<T>>(data).value));
    }

    /**
     * @brief Check if the result is Ok and satisfies a predicate (rvalue, void T)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes no arguments and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_ok_and(Pred pred) &&
        requires IsVoid<T>
    {
        return is_ok() && pred();
    }

    /**
     * @brief Check if the result is Err
     * @return true if the result contains an Err value, false otherwise
     */
    bool is_err() const { return std::holds_alternative<error_type<E>>(data); }

    /**
     * @brief Check if the result is Err and the error satisfies a predicate (const lvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const E& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_err_and(Pred pred) const &
        requires NotVoid<E>
    {
        return is_err() && pred(std::get<error_type<E>>(data).error);
    }

    /**
     * @brief Check if the result is Err and satisfies a predicate (const lvalue, void E)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes no arguments and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_err_and(Pred pred) const &
        requires IsVoid<E>
    {
        return is_err() && pred();
    }

    /**
     * @brief Check if the result is Err and the error satisfies a predicate (rvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes E&& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_err_and(Pred pred) &&
        requires NotVoid<E>
    {
        return is_err() && pred(std::move(std::get<error_type<E>>(data).error));
    }

    /**
     * @brief Check if the result is Err and satisfies a predicate (rvalue, void E)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes no arguments and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    bool is_err_and(Pred pred) &&
        requires IsVoid<E>
    {
        return is_err() && pred();
    }

    // ======================================================================
    // Adapter for each variant
    // ======================================================================

    /**
     * @brief Convert the Result to an optional containing the Ok value (const lvalue)
     * @return std::optional<T> containing the value if Ok, std::nullopt otherwise
     */
    std::optional<T> ok() const &
        requires CopyConstructible<T>
    {
        return is_ok() ? std::optional<T>(std::get<value_type<T>>(data).value) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional (const lvalue, void T)
     * @return std::optional<std::monostate> containing monostate if Ok, std::nullopt otherwise
     */
    std::optional<std::monostate> ok() const &
        requires IsVoid<T>
    {
        return is_ok() ? std::optional<std::monostate>(std::monostate{}) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Ok value (rvalue)
     * @return std::optional<T> containing the moved value if Ok, std::nullopt otherwise
     */
    std::optional<T> ok() &&
        requires NotVoid<T>
    {
        if (is_ok()) {
            return std::optional<T>(std::move(std::get<value_type<T>>(data).value));
        }
        return std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional (rvalue, void T)
     * @return std::optional<std::monostate> containing monostate if Ok, std::nullopt otherwise
     */
    std::optional<std::monostate> ok() &&
        requires IsVoid<T>
    {
        return is_ok() ? std::optional<std::monostate>(std::monostate{}) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Err value (const lvalue)
     * @return std::optional<E> containing the error if Err, std::nullopt otherwise
     */
    std::optional<E> err() const &
        requires CopyConstructible<E>
    {
        return is_err() ? std::optional<E>(std::get<error_type<E>>(data).error) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional (const lvalue, void E)
     * @return std::optional<std::monostate> containing monostate if Err, std::nullopt otherwise
     */
    std::optional<std::monostate> err() const &
        requires IsVoid<E>
    {
        return is_err() ? std::optional<std::monostate>(std::monostate{}) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Err value (rvalue)
     * @return std::optional<E> containing the moved error if Err, std::nullopt otherwise
     */
    std::optional<E> err() &&
        requires NotVoid<E>
    {
        if (is_err()) {
            return std::optional<E>(std::move(std::get<error_type<E>>(data).error));
        }
        return std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional (rvalue, void E)
     * @return std::optional<std::monostate> containing monostate if Err, std::nullopt otherwise
     */
    std::optional<std::monostate> err() &&
        requires IsVoid<E>
    {
        return is_err() ? std::optional<std::monostate>(std::monostate{}) : std::nullopt;
    }

    // ======================================================================
    // Transforming contained values
    // ======================================================================

    /**
     * @brief Map the Ok value to a new value using a function (const lvalue, non-void T)
     * @tparam Fn Function type
     * @param fn Function that takes const T& and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original error if Err
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) const & -> Result<decltype(fn(std::declval<T>())), E>
        requires NotVoid<T>
    {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).value;
            return Result<U, E>::Ok(fn(v));
        }
        const E &e = std::get<error_type<E>>(data).error;
        return Result<U, E>::Err(e);
    }

    /**
     * @brief Map the Ok value to a new value using a function (const lvalue, void T)
     * @tparam Fn Function type
     * @param fn Function that takes no arguments and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original error if Err
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) const & -> Result<decltype(fn()), E>
        requires IsVoid<T>
    {
        using U = decltype(fn());
        if (is_ok()) {
            return Result<U, E>::Ok(fn());
        }
        const E &e = std::get<error_type<E>>(data).error;
        return Result<U, E>::Err(e);
    }

    /**
     * @brief Map the Ok value to a new value using a function (rvalue, non-void T)
     * @tparam Fn Function type
     * @param fn Function that takes T&& and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original error if Err
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) && -> Result<decltype(fn(std::declval<T>())), E>
        requires NotVoid<T>
    {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok()) {
            T &&v = std::move(std::get<value_type<T>>(data).value);
            return Result<U, E>::Ok(fn(std::forward<T>(v)));
        }
        E &&e = std::move(std::get<error_type<E>>(data).error);
        return Result<U, E>::Err(std::forward<E>(e));
    }

    /**
     * @brief Map the Ok value to a new value using a function (rvalue, void T)
     * @tparam Fn Function type
     * @param fn Function that takes no arguments and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original error if Err
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) && -> Result<decltype(fn()), E>
        requires IsVoid<T>
    {
        using U = decltype(fn());
        if (is_ok()) {
            return Result<U, E>::Ok(fn());
        }
        E &&e = std::move(std::get<error_type<E>>(data).error);
        return Result<U, E>::Err(std::forward<E>(e));
    }

    /**
     * @brief Map the Ok value or return a default value (const lvalue, non-void T)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes const T& and returns U
     * @return The result of fn(value) if Ok, or default_val if Err
     */
    template <typename U, typename Fn>
    constexpr U map_or(U &&default_val, Fn &&fn) const &
        requires NotVoid<T>
    {
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).value;
            return std::forward<Fn>(fn)(v);
        }
        return std::forward<U>(default_val);
    }

    /**
     * @brief Map the Ok value or return a default value (const lvalue, void T)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes no arguments and returns U
     * @return The result of fn() if Ok, or default_val if Err
     */
    template <typename U, typename Fn>
    constexpr U map_or(U &&default_val, Fn &&fn) const &
        requires IsVoid<T>
    {
        if (is_ok()) {
            return std::forward<Fn>(fn)();
        }
        return std::forward<U>(default_val);
    }

    /**
     * @brief Map the Ok value or return a default value (rvalue, non-void T)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes T&& and returns U
     * @return The result of fn(value) if Ok, or default_val if Err
     */
    template <typename U, typename Fn>
    constexpr U map_or(U &&default_val, Fn &&fn) &&
        requires NotVoid<T>
    {
        if (is_ok()) {
            T &&v = std::move(std::get<value_type<T>>(data).value);
            return std::forward<Fn>(fn)(std::forward<T>(v));
        }
        return std::forward<U>(default_val);
    }

    /**
     * @brief Map the Ok value or return a default value (rvalue, void T)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes no arguments and returns U
     * @return The result of fn() if Ok, or default_val if Err
     */
    template <typename U, typename Fn>
    constexpr U map_or(U &&default_val, Fn &&fn) &&
        requires IsVoid<T>
    {
        if (is_ok()) {
            return std::forward<Fn>(fn)();
        }
        return std::forward<U>(default_val);
    }

    // ======================================================================
    // Extract a value
    // ======================================================================

    /**
     * @brief Extract the Ok value with a custom error message (const lvalue, non-void T)
     * @param msg Error message to use if Result is Err
     * @return The contained value
     * @throws std::runtime_error if the Result is Err
     */
    template <NotVoid T2 = T> T2 expect(const char *msg) const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        }
        unwrap_failed<value_type<T>>(msg);
    }

    /**
     * @brief Extract the Ok value with a custom error message (rvalue, non-void T)
     * @param msg Error message to use if Result is Err
     * @return The contained value (moved)
     * @throws std::runtime_error if the Result is Err
     */
    template <NotVoid T2 = T> T2 expect(const char *msg) &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        }
        unwrap_failed<value_type<T>>(msg);
    }

    /**
     * @brief Verify Ok state with a custom error message (const lvalue, void T)
     * @param msg Error message to use if Result is Err
     * @throws std::runtime_error if the Result is Err
     */
    template <IsVoid T2 = T> void expect(const char *msg) const &
    {
        if (is_err()) {
            unwrap_failed<value_type<T>>(msg);
        }
    }

    /**
     * @brief Verify Ok state with a custom error message (rvalue, void T)
     * @param msg Error message to use if Result is Err
     * @throws std::runtime_error if the Result is Err
     */
    template <IsVoid T2 = T> void expect(const char *msg) &&
    {
        if (is_err()) {
            unwrap_failed<value_type<T>>(msg);
        }
    }

    /**
     * @brief Extract the Ok value (const lvalue, non-void T)
     * @return The contained value
     * @throws std::runtime_error if the Result is Err
     */
    template <NotVoid T2 = T> T2 unwrap() const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        }
        unwrap_failed<value_type<T>>("called `Result::unwrap()` on an `Err` value");
    }

    /**
     * @brief Extract the Ok value (rvalue, non-void T)
     * @return The contained value (moved)
     * @throws std::runtime_error if the Result is Err
     */
    template <NotVoid T2 = T> T2 unwrap() &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        }
        unwrap_failed<value_type<T>>("called `Result::unwrap()` on an `Err` value");
    }

    /**
     * @brief Verify Ok state (const lvalue, void T)
     * @throws std::runtime_error if the Result is Err
     */
    template <IsVoid T2 = T> void unwrap() const &
    {
        if (is_err()) {
            unwrap_failed<value_type<T>>("called `Result::unwrap()` on an `Err` value");
        }
    }

    /**
     * @brief Verify Ok state (rvalue, void T)
     * @throws std::runtime_error if the Result is Err
     */
    template <IsVoid T2 = T> void unwrap() &&
    {
        if (is_err()) {
            unwrap_failed<value_type<T>>("called `Result::unwrap()` on an `Err` value");
        }
    }

    /**
     * @brief Extract the Ok value or return a default-constructed value (const lvalue)
     * @return The contained value if Ok, or a default-constructed T if Err
     */
    template <DefaultConstructible T2 = T> T2 unwrap_or_default() const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        }
        return T2{};
    }

    /**
     * @brief Extract the Ok value or return a default-constructed value (rvalue)
     * @return The contained value (moved) if Ok, or a default-constructed T if Err
     */
    template <DefaultConstructible T2 = T> T2 unwrap_or_default() &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        }
        return T2{};
    }

    /**
     * @brief Extract the Err value with a custom error message (const lvalue, non-void E)
     * @param msg Error message to use if Result is Ok
     * @return The contained error
     * @throws std::runtime_error if the Result is Ok
     */
    template <NotVoid E2 = E> E2 expect_err(const char *msg) const &
    {
        if (is_err()) {
            return std::get<error_type<E>>(data).error;
        }
        unwrap_failed<error_type<E>>(msg);
    }

    /**
     * @brief Extract the Err value with a custom error message (rvalue, non-void E)
     * @param msg Error message to use if Result is Ok
     * @return The contained error
     * @throws std::runtime_error if the Result is Ok
     */
    template <NotVoid E2 = E> E2 expect_err(const char *msg) &&
    {
        if (is_err()) {
            return std::move(std::get<error_type<E>>(data).error);
        }
        unwrap_failed<error_type<E>>(msg);
    }

    /**
     * @brief Verify Err state with a custom error message (const lvalue, void E)
     * @param msg Error message to use if Result is Ok
     * @throws std::runtime_error if the Result is Ok
     */
    template <IsVoid E2 = E> void expect_err(const char *msg) const &
    {
        if (is_ok()) {
            unwrap_failed<error_type<E>>(msg);
        }
    }

    /**
     * @brief Verify Err state with a custom error message (rvalue, void E)
     * @param msg Error message to use if Result is Ok
     * @throws std::runtime_error if the Result is Ok
     */
    template <IsVoid E2 = E> void expect_err(const char *msg) &&
    {
        if (is_ok()) {
            unwrap_failed<error_type<E>>(msg);
        }
    }

    /**
     * @brief Extract the Err value (const lvalue, non-void E)
     * @return The contained error
     * @throws std::runtime_error if the Result is Ok
     */
    template <NotVoid E2 = E> E2 unwrap_err() const &
    {
        if (is_err()) {
            return std::get<error_type<E>>(data).error;
        }
        unwrap_failed<error_type<E>>("called `Result::unwrap_err()` on an `Ok` value");
    }

    /**
     * @brief Extract the Err value (rvalue, non-void E)
     * @return The contained error (moved)
     * @throws std::runtime_error if the Result is Ok
     */
    template <NotVoid E2 = E> E2 unwrap_err() &&
    {
        if (is_err()) {
            return std::move(std::get<error_type<E>>(data).error);
        }
        unwrap_failed<error_type<E>>("called `Result::unwrap_err()` on an `Ok` value");
    }

    /**
     * @brief Verify Err state (const lvalue, void E)
     * @throws std::runtime_error if the Result is Ok
     */
    template <IsVoid E2 = E> void unwrap_err() const &
    {
        if (is_ok()) {
            unwrap_failed<error_type<E>>("called `Result::unwrap_err()` on an `Ok` value");
        }
    }

    /**
     * @brief Verify Err state (rvalue, void E)
     * @throws std::runtime_error if the Result is Ok
     */
    template <IsVoid E2 = E> void unwrap_err() &&
    {
        if (is_ok()) {
            unwrap_failed<error_type<E>>("called `Result::unwrap_err()` on an `Ok` value");
        }
    }
};

// ======================================================================
// Helper factory methods
// ======================================================================

/**
 * @brief Create an Ok result with a const reference value
 * @tparam U Value type
 * @tparam V Error type
 * @param v The value to store
 * @return Result<U, V> containing the Ok value
 */
template <typename U, typename V>
[[nodiscard]] Result<U, V> Ok(const U &v)
    requires NotVoid<U>
{
    return Result<U, V>(OkTag{}, v);
}

/**
 * @brief Create an Ok result with an rvalue reference value
 * @tparam U Value type
 * @tparam V Error type
 * @param v The value to move
 * @return Result<U, V> containing the Ok value
 */
template <typename U, typename V>
[[nodiscard]] Result<U, V> Ok(U &&v)
    requires NotVoid<U>
{
    return Result<U, V>(OkTag{}, std::move(v));
}

/**
 * @brief Create an Ok result with void value type
 * @tparam V Error type
 * @return Result<void, V> containing the Ok (void) state
 */
template <typename V> [[nodiscard]] Result<void, V> Ok() { return Result<void, V>(OkTag{}); }

/**
 * @brief Create an Err result with a const reference error
 * @tparam U Value type
 * @tparam V Error type
 * @param e The error to store
 * @return Result<U, V> containing the Err value
 */
template <typename U, typename V>
[[nodiscard]] Result<U, V> Err(const V &e)
    requires NotVoid<V>
{
    return Result<U, V>(ErrTag{}, e);
}

/**
 * @brief Create an Err result with an rvalue reference error
 * @tparam U Value type
 * @tparam V Error type
 * @param e The error to move
 * @return Result<U, V> containing the Err value
 */
template <typename U, typename V>
[[nodiscard]] Result<U, V> Err(V &&e)
    requires NotVoid<V>
{
    return Result<U, V>(ErrTag{}, std::move(e));
}

/**
 * @brief Create an Err result with void error type
 * @tparam U Value type
 * @return Result<U, void> containing the Err (void) state
 */
template <typename U> [[nodiscard]] Result<U, void> Err() { return Result<U, void>(ErrTag{}); }

} // namespace rstd
