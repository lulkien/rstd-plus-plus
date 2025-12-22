/**
 * @file result.hpp
 * @brief A Rust-inspired Result type for C++ error handling
 *
 * This file provides a Result<T, E> type that represents either success (Ok) or
 * failure (Err). It offers a type-safe alternative to exceptions and error
 * codes.
 */

#pragma once

#include <optional>
#include <sstream>
#include <type_traits>
#include <utility>
#include <variant>

#include "core.hpp"

namespace rstd
{

namespace __detail
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
} // namespace __detail

/**
 * @brief A type representing either success (Ok) or failure (Err)
 *
 * Result<T, E> is a type that represents either success with a value of type T,
 * or failure with an error of type E. Both T and E can be Void.
 *
 * This provides a type-safe way to handle errors without exceptions, inspired
 * by Rust's Result type.
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
    std::variant<__detail::value_type<T>, __detail::error_type<E>>
        data_; ///< Internal storage for either Ok or Err

    /**
     * @brief Private constructor for Ok variant with const reference
     * @param v The value to store
     */
    Result(__detail::OkTag, const T &v) : data_{__detail::value_type<T>{v}} {}

    /**
     * @brief Private constructor for Ok variant with rvalue reference
     * @param v The value to move
     */
    Result(__detail::OkTag, T &&v)
        : data_{__detail::value_type<T>{std::move(v)}}
    {}

    /**
     * @brief Private constructor for Err variant with const reference
     * @param e The error to store
     */
    Result(__detail::ErrTag, const E &e) : data_{__detail::error_type<E>{e}} {}

    /**
     * @brief Private constructor for Err variant with rvalue reference
     * @param e The error to move
     */
    Result(__detail::ErrTag, E &&e)
        : data_{__detail::error_type<E>{std::move(e)}}
    {}

    template <typename panic_type>
    [[noreturn]] void unwrap_failed(const char *msg,
                                    const panic_type &value) const
        requires Printable<panic_type>
    {
        std::ostringstream oss;
        oss << msg << ": " << value;
        throw std::runtime_error(oss.str());
    }

    template <typename panic_type>
    [[noreturn]] [[deprecated(
        "Use printable types or provide operator<< overload.")]]
    void unwrap_failed(const char *msg,
                       [[maybe_unused]] const panic_type &value) const
        requires(!Printable<panic_type>)
    {
        throw std::runtime_error(msg);
    }

public:
    // Friend declarations for factory functions
    template <typename U, typename V>
    friend auto Ok(const U &v) -> Result<U, V>;
    template <typename U, typename V> friend auto Ok(U &&v) -> Result<U, V>;
    template <typename U, typename V>
    friend auto Err(const V &e) -> Result<U, V>;
    template <typename U, typename V> friend auto Err(V &&e) -> Result<U, V>;

    // ======================================================================
    // Object creations
    // ======================================================================

    /**
     * @brief Create an Ok result with a const reference value
     * @param value The value to store
     * @return A Result containing the Ok value
     */
    static auto Ok(const T &value) -> Result
    {
        return Result(__detail::OkTag{}, value);
    }

    /**
     * @brief Create an Ok result with an rvalue reference value
     * @param value The value to move
     * @return A Result containing the Ok value
     */
    static auto Ok(T &&value) -> Result
    {
        return Result(__detail::OkTag{}, std::move(value));
    }

    /**
     * @brief Create an Err result with a const reference error
     * @param error The error to store
     * @return A Result containing the Err value
     */
    static auto Err(const E &error) -> Result
    {
        return Result(__detail::ErrTag{}, error);
    }

    /**
     * @brief Create an Err result with an rvalue reference error
     * @param error The error to move
     * @return A Result containing the Err value
     */
    static auto Err(E &&error) -> Result
    {
        return Result(__detail::ErrTag{}, std::move(error));
    }

    // ======================================================================
    // Querying the contained values
    // ======================================================================

    /**
     * @brief Check if the result is Ok
     * @return true if the result contains an Ok value, false otherwise
     */
    [[nodiscard]] auto is_ok() const -> bool
    {
        return std::holds_alternative<__detail::value_type<T>>(data_);
    }

    /**
     * @brief Check if the result is Ok and the value satisfies a predicate
     * (const lvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const T& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
        requires BoolReturning<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) const & -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(
                              std::get<__detail::value_type<T>>(data_).data);
    }

    /**
     * @brief Check if the result is Ok and the value satisfies a predicate
     * (rvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes T&& and returns bool
     * @return true if Ok and predicate returns true, false otherwise
     */
    template <typename Pred>
        requires BoolReturning<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) && -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(std::move(
                              std::get<__detail::value_type<T>>(data_).data));
    }

    /**
     * @brief Check if the result is Err
     * @return true if the result contains an Err value, false otherwise
     */
    [[nodiscard]] auto is_err() const -> bool
    {
        return std::holds_alternative<__detail::error_type<E>>(data_);
    }

    /**
     * @brief Check if the result is Err and the error satisfies a predicate
     * (const lvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes const E& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
        requires BoolReturning<Pred, E>
    [[nodiscard]] auto is_err_and(Pred &&pred) const & -> bool
    {
        return is_err() && std::forward<Pred>(pred)(
                               std::get<__detail::error_type<E>>(data_).data);
    }

    /**
     * @brief Check if the result is Err and the error satisfies a predicate
     * (rvalue)
     * @tparam Pred Predicate function type
     * @param pred Predicate that takes E&& and returns bool
     * @return true if Err and predicate returns true, false otherwise
     */
    template <typename Pred>
        requires BoolReturning<Pred, E>
    [[nodiscard]] auto is_err_and(Pred &&pred) && -> bool
    {
        return is_err() && std::forward<Pred>(pred)(std::move(
                               std::get<__detail::error_type<E>>(data_).data));
    }

    // ======================================================================
    // Adapter for each variant
    // ======================================================================

    /**
     * @brief Convert the Result to an optional containing the Ok value (const
     * lvalue)
     * @return std::optional<T> containing the value if Ok, std::nullopt
     * otherwise
     */
    [[nodiscard]] auto ok() const & -> std::optional<T>
    {
        return is_ok() ? std::optional<T>(
                             std::get<__detail::value_type<T>>(data_).data)
                       : std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Ok value (rvalue)
     * @return std::optional<T> containing the moved value if Ok, std::nullopt
     * otherwise
     */
    [[nodiscard]] auto ok() && -> std::optional<T>
    {
        if (is_ok()) {
            return std::optional<T>(
                std::move(std::get<__detail::value_type<T>>(data_).data));
        }
        return std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Err value (const
     * lvalue)
     * @return std::optional<E> containing the error if Err, std::nullopt
     * otherwise
     */
    [[nodiscard]] auto err() const & -> std::optional<E>
    {
        if (is_err()) {
            return std::optional<E>(
                std::get<__detail::error_type<E>>(data_).data);
        }
        return std::nullopt;
    }

    /**
     * @brief Convert the Result to an optional containing the Err value
     * (rvalue)
     * @return std::optional<E> containing the moved error if Err, std::nullopt
     * otherwise
     */
    [[nodiscard]] auto err() && -> std::optional<E>
    {
        if (is_err()) {
            return std::optional<E>(
                std::move(std::get<__detail::error_type<E>>(data_).data));
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
     * @return Result<U, E> with the transformed value if Ok, or the original
     * error if Err
     */
    template <typename FnOk>
    constexpr auto
    map(FnOk &&fn) const & -> Result<std::invoke_result_t<FnOk, T>, E>
    {
        using U = std::invoke_result_t<FnOk, T>;
        if (is_ok()) {
            return Result<U, E>::Ok(std::forward<FnOk>(fn)(
                std::get<__detail::value_type<T>>(data_).data));
        }
        return Result<U, E>::Err(std::get<__detail::error_type<E>>(data_).data);
    }

    /**
     * @brief Map the Ok value to a new value using a function (rvalue)
     * @tparam Fn Function type
     * @param fn Function that takes T&& and returns U
     * @return Result<U, E> with the transformed value if Ok, or the original
     * error if Err
     */
    template <typename FnOk>
    constexpr auto map(FnOk &&fn) && -> Result<std::invoke_result_t<FnOk, T>, E>
    {
        using U = std::invoke_result_t<FnOk, T>;
        if (is_ok()) {
            return Result<U, E>::Ok(std::forward<FnOk>(fn)(
                std::move(std::get<__detail::value_type<T>>(data_).data)));
        }
        return Result<U, E>::Err(
            std::move(std::get<__detail::error_type<E>>(data_).data));
    }

    /**
     * @brief Map the Ok value or return a default value (const lvalue)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes const T& and returns U
     * @return The result of fn(value) if Ok, or default_val if Err
     */
    template <typename U, typename FnOk>
        requires std::is_same_v<U, std::invoke_result_t<FnOk, T>>
    constexpr auto map_or(const U &default_val, FnOk &&fn) const & -> U
    {
        if (is_ok()) {
            return std::forward<FnOk>(fn)(
                std::get<__detail::value_type<T>>(data_).data);
        }
        return default_val;
    }

    /**
     * @brief Map the Ok value or return a default value (rvalue)
     * @tparam U Return type
     * @tparam Fn Function type
     * @param default_val Default value to return if Err
     * @param fn Function that takes T&& and returns U
     * @return The result of fn(value) if Ok, or default_val if Err
     */
    template <typename U, typename FnMap>
        requires std::is_same_v<U, std::invoke_result_t<FnMap, T>>
    constexpr auto map_or(const U &default_val, FnMap &&fn) && -> U
    {
        if (is_ok()) {
            return std::forward<FnMap>(fn)(std::forward<T>(
                std::move(std::get<__detail::value_type<T>>(data_).data)));
        }
        return default_val;
    }

    template <typename FnErr, typename FnOk>
        requires std::is_same_v<std::invoke_result_t<FnErr, E>,
                                std::invoke_result_t<FnOk, T>>
    constexpr auto
    map_or_else(FnErr &&fn_err,
                FnOk &&fn_ok) const & -> std::invoke_result_t<FnOk, T>
    {
        if (is_ok()) {
            return std::forward<FnOk>(fn_ok)(
                std::get<__detail::value_type<T>>(data_).data);
        }
        return std::forward<FnErr>(fn_err)(
            std::get<__detail::error_type<E>>(data_).data);
    }

    template <typename FnErr, typename FnOk>
        requires std::is_same_v<std::invoke_result_t<FnErr, E>,
                                std::invoke_result_t<FnOk, T>>
    constexpr auto map_or_else(FnErr &&fn_err,
                               FnOk &&fn_ok) && -> std::invoke_result_t<FnOk, T>
    {
        if (is_ok()) {
            return std::forward<FnOk>(fn_ok)(
                std::move(std::get<__detail::value_type<T>>(data_).data));
        }
        return std::forward<FnErr>(fn_err)(
            std::move(std::get<__detail::error_type<E>>(data_).data));
    }

    template <typename FnErr>
    constexpr auto
    map_err(FnErr &&fn) const & -> Result<T, std::invoke_result_t<FnErr, E>>
    {
        using V = std::invoke_result_t<FnErr, E>;
        if (is_err()) {
            return Result<T, V>::Err(std::forward<FnErr>(fn)(
                std::get<__detail::error_type<E>>(data_).data));
        }
        return Result<T, V>::Ok(std::get<__detail::value_type<T>>(data_).data);
    }

    template <typename FnErr>
    constexpr auto
    map_err(FnErr &&fn) && -> Result<T, std::invoke_result_t<FnErr, E>>
    {
        using V = std::invoke_result_t<FnErr, E>;
        if (is_err()) {
            return Result<T, V>::Err(std::forward<FnErr>(fn)(
                std::move(std::get<__detail::error_type<E>>(data_).data)));
        }
        return Result<T, V>::Ok(
            std::move(std::get<__detail::value_type<T>>(data_).data));
    }

    template <typename Fn>
        requires VoidReturning<Fn, T>
    constexpr auto inspect(Fn &&fn) const & -> const Result &
    {
        if (is_ok()) {
            std::forward<Fn>(fn)(std::get<__detail::value_type<T>>(data_).data);
        }
        return *this;
    }

    template <typename Fn>
        requires VoidReturning<Fn, T>
    constexpr auto inspect(Fn &&fn) && -> Result &&
    {
        if (is_ok()) {
            std::forward<Fn>(fn)(
                std::move(std::get<__detail::value_type<T>>(data_).data));
        }
        return std::move(*this);
    }

    template <typename Fn>
        requires VoidReturning<Fn, E>
    constexpr auto inspect_err(Fn &&fn) const & -> const Result &
    {
        if (is_err()) {
            std::forward<Fn>(fn)(std::get<__detail::error_type<E>>(data_).data);
        }
        return *this;
    }

    template <typename Fn>
        requires VoidReturning<Fn, E>
    constexpr auto inspect_err(Fn &&fn) && -> Result &&
    {
        if (is_err()) {
            std::forward<Fn>(fn)(
                std::move(std::get<__detail::error_type<E>>(data_).data));
        }
        return std::move(*this);
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
            return std::get<__detail::value_type<T>>(data_).data;
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
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
            return std::move(std::get<__detail::value_type<T>>(data_).data);
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
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
            return std::get<__detail::value_type<T>>(data_).data;
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
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
            return std::move(std::get<__detail::value_type<T>>(data_).data);
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    /**
     * @brief Extract the Ok value or return a default-constructed value (const
     * lvalue)
     * @return The contained value if Ok, or a default-constructed T if Err
     */
    auto unwrap_or_default() const & -> T
        requires DefaultConstructible<T>
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).data;
        }
        return T{};
    }

    /**
     * @brief Extract the Ok value or return a default-constructed value
     * (rvalue)
     * @return The contained value (moved) if Ok, or a default-constructed T if
     * Err
     */
    auto unwrap_or_default() && -> T
        requires DefaultConstructible<T>
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).data);
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
            return std::get<__detail::error_type<E>>(data_).data;
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
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
            return std::move(std::get<__detail::error_type<E>>(data_).data);
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
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
            return std::get<__detail::error_type<E>>(data_).data;
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
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
            return std::move(std::get<__detail::error_type<E>>(data_).data);
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
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
template <typename U, typename V>
[[nodiscard]] auto Ok(const U &v) -> Result<U, V>
{
    return Result<U, V>(__detail::OkTag{}, v);
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
    return Result<U, V>(__detail::OkTag{}, std::move(v));
}

/**
 * @brief Create an Err result with a const reference error
 * @tparam U Value type
 * @tparam V Error type
 * @param e The error to store
 * @return Result<U, V> containing the Err value
 */
template <typename U, typename V>
[[nodiscard]] auto Err(const V &e) -> Result<U, V>
{
    return Result<U, V>(__detail::ErrTag{}, e);
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
    return Result<U, V>(__detail::ErrTag{}, std::move(e));
}

} // namespace rstd
