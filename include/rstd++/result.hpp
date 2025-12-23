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

template <typename T, typename E> class Result;

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

template <typename> struct is_result_helper : std::false_type
{};

template <typename U, typename V>
struct is_result_helper<Result<U, V>> : std::true_type
{};

template <typename T>
concept is_result_v = is_result_helper<std::remove_cvref_t<T>>::value;

} // namespace __detail

template <typename T, typename E> class [[nodiscard]] Result
{
    std::variant<__detail::value_type<T>, __detail::error_type<E>>
        data_; ///< Internal storage for either Ok or Err

    Result(__detail::OkTag, const T &v) : data_{__detail::value_type<T>{v}} {}

    Result(__detail::OkTag, T &&v)
        : data_{__detail::value_type<T>{std::move(v)}}
    {}

    Result(__detail::ErrTag, const E &e) : data_{__detail::error_type<E>{e}} {}

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

    static auto Ok(const T &value) -> Result
    {
        return Result(__detail::OkTag{}, value);
    }

    static auto Ok(T &&value) -> Result
    {
        return Result(__detail::OkTag{}, std::move(value));
    }

    static auto Err(const E &error) -> Result
    {
        return Result(__detail::ErrTag{}, error);
    }

    static auto Err(E &&error) -> Result
    {
        return Result(__detail::ErrTag{}, std::move(error));
    }

    // ======================================================================
    // Querying the contained values
    // ======================================================================

    [[nodiscard]] auto is_ok() const -> bool
    {
        return std::holds_alternative<__detail::value_type<T>>(data_);
    }

    template <typename Pred>
        requires BoolReturning<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) const & -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(
                              std::get<__detail::value_type<T>>(data_).data);
    }

    template <typename Pred>
        requires BoolReturning<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) && -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(std::move(
                              std::get<__detail::value_type<T>>(data_).data));
    }

    [[nodiscard]] auto is_err() const -> bool
    {
        return std::holds_alternative<__detail::error_type<E>>(data_);
    }

    template <typename Pred>
        requires BoolReturning<Pred, E>
    [[nodiscard]] auto is_err_and(Pred &&pred) const & -> bool
    {
        return is_err() && std::forward<Pred>(pred)(
                               std::get<__detail::error_type<E>>(data_).data);
    }

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

    [[nodiscard]] auto ok() const & -> std::optional<T>
    {
        return is_ok() ? std::optional<T>(
                             std::get<__detail::value_type<T>>(data_).data)
                       : std::nullopt;
    }

    [[nodiscard]] auto ok() && -> std::optional<T>
    {
        if (is_ok()) {
            return std::optional<T>(
                std::move(std::get<__detail::value_type<T>>(data_).data));
        }
        return std::nullopt;
    }

    [[nodiscard]] auto err() const & -> std::optional<E>
    {
        if (is_err()) {
            return std::optional<E>(
                std::get<__detail::error_type<E>>(data_).data);
        }
        return std::nullopt;
    }

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

    auto expect(const char *msg) const & -> T
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).data;
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
        unwrap_failed(msg, e);
    }

    auto expect(const char *msg) && -> T
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).data);
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
        unwrap_failed(msg, e);
    }

    auto unwrap() const & -> T
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).data;
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    auto unwrap() && -> T
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).data);
        }
        const E &e = std::get<__detail::error_type<E>>(data_).data;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    auto unwrap_or_default() const & -> T
        requires DefaultConstructible<T>
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).data;
        }
        return T{};
    }

    auto unwrap_or_default() && -> T
        requires DefaultConstructible<T>
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).data);
        }
        return T{};
    }

    auto expect_err(const char *msg) const & -> E
    {
        if (is_err()) {
            return std::get<__detail::error_type<E>>(data_).data;
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
        unwrap_failed(msg, v);
    }

    auto expect_err(const char *msg) && -> E
    {
        if (is_err()) {
            return std::move(std::get<__detail::error_type<E>>(data_).data);
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
        unwrap_failed(msg, v);
    }

    auto unwrap_err() const & -> E
    {
        if (is_err()) {
            return std::get<__detail::error_type<E>>(data_).data;
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
        unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", v);
    }

    auto unwrap_err() && -> E
    {
        if (is_err()) {
            return std::move(std::get<__detail::error_type<E>>(data_).data);
        }
        const T &v = std::get<__detail::value_type<T>>(data_).data;
        unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", v);
    }

    template <class U>
    constexpr auto and_(Result<U, E> &res) const & -> Result<U, E>
    {
        if (is_ok()) {
            return res;
        }
        return Result<U, E>::Err(std::get<__detail::error_type<E>>(data_).data);
    }

    template <class U>
    constexpr auto and_(Result<U, E> &&res) const & -> Result<U, E>
    {
        if (is_ok()) {
            return std::forward<Result<U, E>>(res);
        }
        return Result<U, E>::Err(std::get<__detail::error_type<E>>(data_).data);
    }

    template <class U>
    constexpr auto and_(Result<U, E> &&res) && -> Result<U, E>
    {
        if (is_ok()) {
            return std::forward<Result<U, E>>(res);
        }
        return Result<U, E>::Err(
            std::move(std::get<__detail::error_type<E>>(data_).data));
    }

    template <class Fn>
        requires __detail::is_result_v<std::invoke_result_t<Fn, T>>
    constexpr auto and_then(Fn &&fn) const &
    {
        using Ret = std::invoke_result_t<Fn, T>;
        if (is_ok()) {
            return std::forward<Fn>(fn)(
                std::get<__detail::value_type<T>>(data_).data);
        }
        return Ret::Err(std::get<__detail::error_type<E>>(data_).data);
    }

    template <class Fn>
        requires __detail::is_result_v<std::invoke_result_t<Fn, T>>
    constexpr auto and_then(Fn &&fn) &&
    {
        using Ret = std::invoke_result_t<Fn, T>;
        if (is_ok()) {
            return std::forward<Fn>(fn)(
                std::move(std::get<__detail::value_type<T>>(data_).data));
        }
        return Ret::Err(
            std::move(std::get<__detail::error_type<E>>(data_).data));
    }
};

// ======================================================================
// Helper factory methods
// ======================================================================

template <typename U, typename V>
[[nodiscard]] auto Ok(const U &v) -> Result<U, V>
{
    return Result<U, V>(__detail::OkTag{}, v);
}

template <typename U, typename V> [[nodiscard]] auto Ok(U &&v) -> Result<U, V>
{
    return Result<U, V>(__detail::OkTag{}, std::move(v));
}

template <typename U, typename V>
[[nodiscard]] auto Err(const V &e) -> Result<U, V>
{
    return Result<U, V>(__detail::ErrTag{}, e);
}

template <typename U, typename V> [[nodiscard]] auto Err(V &&e) -> Result<U, V>
{
    return Result<U, V>(__detail::ErrTag{}, std::move(e));
}

} // namespace rstd
