/**
 * @file result.hpp
 * @brief A Rust-inspired Result type for C++ error handling
 *
 * This file provides a Result<T, E> type that represents either success (Ok) or failure (Err).
 * It offers a type-safe alternative to exceptions and error codes.
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
 * @brief Tag type for constructing Err variants
 */
struct ErrTag
{};

/**
 * @brief Empty struct to represent void/unit types
 *
 * This is used instead of void in template parameters, providing
 * a concrete type that can be instantiated and passed around.
 *
 * @example
 * ```cpp
 * // Function that can fail but has no value on success
 * Result<Void, std::string> validate_input(const std::string& input) {
 *     if (input.empty()) {
 *         return Result<Void, std::string>::Err("Input cannot be empty");
 *     }
 *     return Result<Void, std::string>::Ok(Void{});
 * }
 * ```
 */
struct Void
{
    /**
     * @brief Default constructor
     */
    constexpr Void() noexcept = default;

    /**
     * @brief Equality comparison (all Void instances are equal)
     */
    constexpr auto operator==(const Void &) const noexcept -> bool { return true; }

    /**
     * @brief Inequality comparison (all Void instances are equal)
     */
    constexpr auto operator!=(const Void &) const noexcept -> bool { return false; }
};

/**
 * @brief Stream output operator for Void
 */
inline auto operator<<(std::ostream &os, const Void &) -> std::ostream & { return os << "()"; }

/**
 * @brief Wrapper for value types
 * @tparam T The value type
 */
template <typename T> struct value_type
{
    T data; ///< The contained data

    /**
     * @brief Construct from const reference
     * @param d The data to store
     */
    value_type(const T &d) : data{d} {}

    /**
     * @brief Construct from rvalue reference
     * @param d The data to move
     */
    value_type(T &&d) : data{std::move(d)} {}
};

/**
 * @brief Wrapper for error types
 * @tparam E The error type
 */
template <typename E> struct error_type
{
    E data; ///< The contained error

    /**
     * @brief Construct from const reference
     * @param e The error to store
     */
    error_type(const E &e) : data{e} {}

    /**
     * @brief Construct from rvalue reference
     * @param e The error to move
     */
    error_type(E &&e) : data{std::move(e)} {}
};

/**
 * @brief Concept for types that can be printed to an ostream
 * @tparam T Type to check
 */
template <typename T>
concept Printable = requires(std::ostream &os, const T &v) { os << v; };

/**
 * @brief Concept for default constructible types
 * @tparam T Type to check
 */
template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

/**
 * @brief Concept for copy constructible types
 * @tparam T Type to check
 */
template <typename T>
concept CopyConstructible = std::is_copy_constructible_v<T>;

/**
 * @brief Concept for Void type
 * @tparam T Type to check
 */
template <typename T>
concept IsVoid = std::is_same_v<T, Void>;

/**
 * @brief Concept for non-Void types
 * @tparam T Type to check
 */
template <typename T>
concept NotVoid = !std::is_same_v<T, Void>;

/**
 * @brief A type representing either success (Ok) or failure (Err)
 *
 * Result<T, E> is a type that represents either success with a value of type T,
 * or failure with an error of type E. Both T and E can be Void.
 *
 * This provides a type-safe way to handle errors without exceptions, inspired by Rust's Result
 * type.
 *
 * @tparam T The type of the success value (can be Void)
 * @tparam E The type of the error value (can be Void)
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
    Result(OkTag, const T &v) : data{value_type<T>{v}} {}

    /**
     * @brief Private constructor for Ok variant with rvalue reference
     * @param v The value to move
     */
    Result(OkTag, T &&v) : data{value_type<T>{std::move(v)}} {}

    /**
     * @brief Private constructor for Err variant with const reference
     * @param e The error to store
     */
    Result(ErrTag, const E &e) : data{error_type<E>{e}} {}

    /**
     * @brief Private constructor for Err variant with rvalue reference
     * @param e The error to move
     */
    Result(ErrTag, E &&e) : data{error_type<E>{std::move(e)}} {}

    template <typename panic_type>
    [[noreturn]] void unwrap_failed(const char *msg, const panic_type &value) const
        requires Printable<panic_type>
    {
        std::ostringstream oss;
        oss << msg << ": " << value;
        throw std::runtime_error(oss.str());
    }

    template <typename panic_type>
    [[noreturn]] [[deprecated("Use printable types or provide operator<< overload.")]]
    void unwrap_failed(const char *msg, [[maybe_unused]] const panic_type &value) const
        requires(!Printable<panic_type>)
    {
        throw std::runtime_error(msg);
    }

public:
    // Friend declarations for factory functions
    template <typename U, typename V> friend auto Ok(const U &v) -> Result<U, V>;
    template <typename U, typename V> friend auto Ok(U &&v) -> Result<U, V>;
    template <typename U, typename V> friend auto Err(const V &e) -> Result<U, V>;
    template <typename U, typename V> friend auto Err(V &&e) -> Result<U, V>;

    // ======================================================================
    // Object creations
    // ======================================================================

    /**
     * @brief Create an Ok result with a const reference value
     * @param value The value to store
     * @return A Result containing the Ok value
     */
    static auto Ok(const T &value) -> Result { return Result(OkTag{}, value); }

    /**
     * @brief Create an Ok result with an rvalue reference value
     * @param value The value to move
     * @return A Result containing the Ok value
     */
    static auto Ok(T &&value) -> Result { return Result(OkTag{}, std::move(value)); }

    /**
     * @brief Create an Err result with a const reference error
     * @param error The error to store
     * @return A Result containing the Err value
     */
    static auto Err(const E &error) -> Result { return Result(ErrTag{}, error); }

    /**
     * @brief Create an Err result with an rvalue reference error
     * @param error The error to move
     * @return A Result containing the Err value
     */
    static auto Err(E &&error) -> Result { return Result(ErrTag{}, std::move(error)); }

    // ======================================================================
    // Querying the contained values
    // ======================================================================

    /**
     * @brief Check if the result is Ok
     * @return true if the result contains an Ok value, false otherwise
     */
    [[nodiscard]] auto is_ok() const -> bool { return std::holds_alternative<value_type<T>>(data); }

    /**
     * @brief Check if the result is Ok and the value satisfies a predicate (const lvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const T& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_ok_and(Pred pred) const & -> bool
        requires NotVoid<T>
    {
        return is_ok() && pred(std::get<value_type<T>>(data).data);
    }

    /**
     * @brief Check if the result is Ok and satisfies a predicate (const lvalue, Void T)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const Void& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_ok_and(Pred pred) const & -> bool
        requires IsVoid<T>
    {
        return is_ok() && pred(std::get<value_type<T>>(data).data);
    }

    /**
     * @brief Check if the result is Ok and the value satisfies a predicate (rvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes T&& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_ok_and(Pred pred) && -> bool
        requires NotVoid<T>
    {
        return is_ok() && pred(std::move(std::get<value_type<T>>(data).data));
    }

    /**
     * @brief Check if the result is Ok and satisfies a predicate (rvalue, Void T)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes Void&& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_ok_and(Pred pred) && -> bool
        requires IsVoid<T>
    {
        return is_ok() && pred(std::move(std::get<value_type<T>>(data).data));
    }

    /**
     * @brief Check if the result is Err
     * @return true if the result contains an Err value, false otherwise
     */
    [[nodiscard]] auto is_err() const -> bool
    {
        return std::holds_alternative<error_type<E>>(data);
    }

    /**
     * @brief Check if the result is Err and the error satisfies a predicate (const lvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const E& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_err_and(Pred pred) const & -> bool
        requires NotVoid<E>
    {
        return is_err() && pred(std::get<error_type<E>>(data).data);
    }

    /**
     * @brief Check if the result is Err and satisfies a predicate (const lvalue, Void E)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const Void& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_err_and(Pred pred) const & -> bool
        requires IsVoid<E>
    {
        return is_err() && pred(std::get<error_type<E>>(data).data);
    }

    /**
     * @brief Check if the result is Err and the error satisfies a predicate (rvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes E&& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_err_and(Pred pred) && -> bool
        requires NotVoid<E>
    {
        return is_err() && pred(std::move(std::get<error_type<E>>(data).data));
    }

    /**
     * @brief Check if the result is Err and satisfies a predicate (rvalue, Void E)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes Void&& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
    auto is_err_and(Pred pred) && -> bool
        requires IsVoid<E>
    {
        return is_err() && pred(std::move(std::get<error_type<E>>(data).data));
    }

    // ======================================================================
    // Adapter for each variant
    // ======================================================================

    /**
     * @brief Convert the Result to an optional containing the Ok value (const lvalue)
     * @return std::optional<T> containing the value if Ok, std::nullopt otherwise
     */
    auto ok() const & -> std::optional<T>
        requires CopyConstructible<T>
    {
        return is_ok() ? std::optional<T>(std::get<value_type<T>>(data).data) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Ok value (rvalue)
     * @return std::optional<T> containing the moved value if Ok, std::nullopt otherwise
     */
    auto ok() && -> std::optional<T>
    {
        if (is_ok()) {
            return std::optional<T>(std::move(std::get<value_type<T>>(data).data));
        }
        return std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Err value (const lvalue)
     * @return std::optional<E> containing the error if Err, std::nullopt otherwise
     */
    auto err() const & -> std::optional<E>
        requires CopyConstructible<E>
    {
        return is_err() ? std::optional<E>(std::get<error_type<E>>(data).data) : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Err value (rvalue)
     * @return std::optional<E> containing the moved error if Err, std::nullopt otherwise
     */
    auto err() && -> std::optional<E>
    {
        if (is_err()) {
            return std::optional<E>(std::move(std::get<error_type<E>>(data).data));
        }
        return std::nullopt;
    }

    // ======================================================================
    // Transforming contained values
    // ======================================================================

    /**
     * @brief Map the Ok value to a new value using a function (const lvalue)
     * @tparam Fn Function type
     * @param fn Function that takes const T& and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original error if Err
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) const & -> Result<decltype(fn(std::declval<const T &>())), E>
    {
        using U = decltype(fn(std::declval<const T &>()));
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).data;
            return Result<U, E>::Ok(fn(v));
        }
        const E &e = std::get<error_type<E>>(data).data;
        return Result<U, E>::Err(e);
    }

    /**
     * @brief Map the Ok value to a new value using a function (rvalue)
     * @tparam Fn Function type
     * @param fn Function that takes T&& and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original error if Err
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) && -> Result<decltype(fn(std::declval<T>())), E>
    {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok()) {
            T &&v = std::move(std::get<value_type<T>>(data).data);
            return Result<U, E>::Ok(fn(std::forward<T>(v)));
        }
        E &&e = std::move(std::get<error_type<E>>(data).data);
        return Result<U, E>::Err(std::forward<E>(e));
    }

    /**
     * @brief Map the Ok value or return a default value (const lvalue)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes const T& and returns U
     * @return The result of fn(value) if Ok, or default_val if Err
     */
    template <typename U, typename Fn> constexpr auto map_or(U &&default_val, Fn &&fn) const & -> U
    {
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).data;
            return std::forward<Fn>(fn)(v);
        }
        return std::forward<U>(default_val);
    }

    /**
     * @brief Map the Ok value or return a default value (rvalue)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes T&& and returns U
     * @return The result of fn(value) if Ok, or default_val if Err
     */
    template <typename U, typename Fn> constexpr auto map_or(U &&default_val, Fn &&fn) && -> U
    {
        if (is_ok()) {
            T &&v = std::move(std::get<value_type<T>>(data).data);
            return std::forward<Fn>(fn)(std::forward<T>(v));
        }
        return std::forward<U>(default_val);
    }

    // ======================================================================
    // Extract a value
    // ======================================================================

    /**
     * @brief Extract the Ok value with a custom error message (const lvalue)
     * @param msg Error message to use if Result is Err
     * @return The contained value
     * @throws std::runtime_error if the Result is Err
     */
    auto expect(const char *msg) const & -> T
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).data;
        }
        const E &e = std::get<error_type<E>>(data).data;
        unwrap_failed(msg, e);
    }

    /**
     * @brief Extract the Ok value with a custom error message (rvalue)
     * @param msg Error message to use if Result is Err
     * @return The contained value (moved)
     * @throws std::runtime_error if the Result is Err
     */
    auto expect(const char *msg) && -> T
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).data);
        }
        const E &e = std::get<error_type<E>>(data).data;
        unwrap_failed(msg, e);
    }

    /**
     * @brief Extract the Ok value (const lvalue)
     * @return The contained value
     * @throws std::runtime_error if the Result is Err
     */
    auto unwrap() const & -> T
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).data;
        }
        const E &e = std::get<error_type<E>>(data).data;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    /**
     * @brief Extract the Ok value (rvalue)
     * @return The contained value (moved)
     * @throws std::runtime_error if the Result is Err
     */
    auto unwrap() && -> T
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).data);
        }
        const E &e = std::get<error_type<E>>(data).data;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    /**
     * @brief Extract the Ok value or return a default-constructed value (const lvalue)
     * @return The contained value if Ok, or a default-constructed T if Err
     */
    auto unwrap_or_default() const & -> T
        requires DefaultConstructible<T>
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).data;
        }
        return T{};
    }

    /**
     * @brief Extract the Ok value or return a default-constructed value (rvalue)
     * @return The contained value (moved) if Ok, or a default-constructed T if Err
     */
    auto unwrap_or_default() && -> T
        requires DefaultConstructible<T>
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).data);
        }
        return T{};
    }

    /**
     * @brief Extract the Err value with a custom error message (const lvalue)
     * @param msg Error message to use if Result is Ok
     * @return The contained error
     * @throws std::runtime_error if the Result is Ok
     */
    auto expect_err(const char *msg) const & -> E
    {
        if (is_err()) {
            return std::get<error_type<E>>(data).data;
        }
        const T &v = std::get<value_type<T>>(data).data;
        unwrap_failed(msg, v);
    }

    /**
     * @brief Extract the Err value with a custom error message (rvalue)
     * @param msg Error message to use if Result is Ok
     * @return The contained error
     * @throws std::runtime_error if the Result is Ok
     */
    auto expect_err(const char *msg) && -> E
    {
        if (is_err()) {
            return std::move(std::get<error_type<E>>(data).data);
        }
        const T &v = std::get<value_type<T>>(data).data;
        unwrap_failed(msg, v);
    }

    /**
     * @brief Extract the Err value (const lvalue)
     * @return The contained error
     * @throws std::runtime_error if the Result is Ok
     */
    auto unwrap_err() const & -> E
    {
        if (is_err()) {
            return std::get<error_type<E>>(data).data;
        }
        const T &v = std::get<value_type<T>>(data).data;
        unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", v);
    }

    /**
     * @brief Extract the Err value (rvalue)
     * @return The contained error (moved)
     * @throws std::runtime_error if the Result is Ok
     */
    auto unwrap_err() && -> E
    {
        if (is_err()) {
            return std::move(std::get<error_type<E>>(data).data);
        }
        const T &v = std::get<value_type<T>>(data).data;
        unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", v);
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
template <typename U, typename V> [[nodiscard]] auto Ok(const U &v) -> Result<U, V>
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
template <typename U, typename V> [[nodiscard]] auto Ok(U &&v) -> Result<U, V>
{
    return Result<U, V>(OkTag{}, std::move(v));
}

/**
 * @brief Create an Err result with a const reference error
 * @tparam U Value type
 * @tparam V Error type
 * @param e The error to store
 * @return Result<U, V> containing the Err value
 */
template <typename U, typename V> [[nodiscard]] auto Err(const V &e) -> Result<U, V>
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
template <typename U, typename V> [[nodiscard]] auto Err(V &&e) -> Result<U, V>
{
    return Result<U, V>(ErrTag{}, std::move(e));
}

} // namespace rstd
