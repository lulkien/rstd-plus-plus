#pragma once

#include <optional>
#include <sstream>
#include <type_traits>
#include <utility>
#include <variant>

#include "core.hpp"

namespace rstd::result
{

template <typename T, typename E> class Result;

namespace __detail
{

struct OkTag
{};

struct ErrTag
{};

template <typename T> struct value_type
{
    T value;
    value_type(const T &d) : value{d} {}
    value_type(T &&d) : value{std::move(d)} {}
};

template <typename E> struct error_type
{
    E error;
    error_type(const E &e) : error{e} {}
    error_type(E &&e) : error{std::move(e)} {}
};

template <typename> struct is_result_helper : std::false_type
{};

template <typename U, typename V>
struct is_result_helper<Result<U, V>> : std::true_type
{};

template <typename T>
concept is_result_v = is_result_helper<std::remove_cvref_t<T>>::value;

} // namespace __detail

template <typename T, typename E>
class [[nodiscard("Result must be used")]] Result
{
    std::variant<__detail::value_type<T>, __detail::error_type<E>> data_;

    Result(__detail::OkTag, const T &v) : data_{__detail::value_type<T>{v}} {}
    Result(__detail::OkTag, T &&v)
        : data_{__detail::value_type<T>{std::forward<T>(v)}}
    {}
    Result(__detail::ErrTag, const E &e) : data_{__detail::error_type<E>{e}} {}
    Result(__detail::ErrTag, E &&e)
        : data_{__detail::error_type<E>{std::forward<E>(e)}}
    {}

    Result(const Result &other) = default;
    auto operator=(const Result &other) -> Result & = default;

    Result(Result &&other) = default;
    auto operator=(Result &&other) -> Result & = default;

    template <typename panic_type>
    [[noreturn]] void unwrap_failed(const char *msg,
                                    const panic_type &value) const
        requires is_printable<panic_type>
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
        requires(!is_printable<panic_type>)
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

    [[nodiscard("Result must be used")]] static auto Ok(const T &value)
        -> Result
    {
        return Result(__detail::OkTag{}, value);
    }

    [[nodiscard("Result must be used")]] static auto Ok(T &&value) -> Result
    {
        return Result(__detail::OkTag{}, std::move(value));
    }

    [[nodiscard("Result must be used")]] static auto Err(const E &error)
        -> Result
    {
        return Result(__detail::ErrTag{}, error);
    }

    [[nodiscard("Result must be used")]] static auto Err(E &&error) -> Result
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
        requires fn_return_boolean<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) const & -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(
                              std::get<__detail::value_type<T>>(data_).value);
    }

    template <typename Pred>
        requires fn_return_boolean<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) && -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(std::move(
                              std::get<__detail::value_type<T>>(data_).value));
    }

    [[nodiscard]] auto is_err() const -> bool
    {
        return std::holds_alternative<__detail::error_type<E>>(data_);
    }

    template <typename Pred>
        requires fn_return_boolean<Pred, E>
    [[nodiscard]] auto is_err_and(Pred &&pred) const & -> bool
    {
        return is_err() && std::forward<Pred>(pred)(
                               std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename Pred>
        requires fn_return_boolean<Pred, E>
    [[nodiscard]] auto is_err_and(Pred &&pred) && -> bool
    {
        return is_err() && std::forward<Pred>(pred)(std::move(
                               std::get<__detail::error_type<E>>(data_).error));
    }

    // ======================================================================
    // Adapter for each variant
    // ======================================================================

    [[nodiscard]] auto ok() const & -> std::optional<T>
    {
        if (is_ok()) {
            return std::optional<T>(
                std::get<__detail::value_type<T>>(data_).value);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto ok() && -> std::optional<T>
    {
        if (is_ok()) {
            return std::optional<T>(
                std::move(std::get<__detail::value_type<T>>(data_).value));
        }
        return std::nullopt;
    }

    [[nodiscard]] auto err() const & -> std::optional<E>
    {
        if (is_err()) {
            return std::optional<E>(
                std::get<__detail::error_type<E>>(data_).error);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto err() && -> std::optional<E>
    {
        if (is_err()) {
            return std::optional<E>(
                std::move(std::get<__detail::error_type<E>>(data_).error));
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
                std::get<__detail::value_type<T>>(data_).value));
        }
        return Result<U, E>::Err(
            std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename FnOk>
    constexpr auto map(FnOk &&fn) && -> Result<std::invoke_result_t<FnOk, T>, E>
    {
        using U = std::invoke_result_t<FnOk, T>;
        if (is_ok()) {
            return Result<U, E>::Ok(std::forward<FnOk>(fn)(
                std::move(std::get<__detail::value_type<T>>(data_).value)));
        }
        return Result<U, E>::Err(
            std::move(std::get<__detail::error_type<E>>(data_).error));
    }

    template <typename U, typename FnOk>
        requires std::is_same_v<U, std::invoke_result_t<FnOk, T>>
    constexpr auto map_or(const U &default_val, FnOk &&fn) const & -> U
    {
        if (is_ok()) {
            return std::forward<FnOk>(fn)(
                std::get<__detail::value_type<T>>(data_).value);
        }
        return default_val;
    }

    template <typename U, typename FnMap>
        requires std::is_same_v<U, std::invoke_result_t<FnMap, T>>
    constexpr auto map_or(const U &default_val, FnMap &&fn) && -> U
    {
        if (is_ok()) {
            return std::forward<FnMap>(fn)(std::forward<T>(
                std::move(std::get<__detail::value_type<T>>(data_).value)));
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
                std::get<__detail::value_type<T>>(data_).value);
        }
        return std::forward<FnErr>(fn_err)(
            std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename FnErr, typename FnOk>
        requires std::is_same_v<std::invoke_result_t<FnErr, E>,
                                std::invoke_result_t<FnOk, T>>
    constexpr auto map_or_else(FnErr &&fn_err,
                               FnOk &&fn_ok) && -> std::invoke_result_t<FnOk, T>
    {
        if (is_ok()) {
            return std::forward<FnOk>(fn_ok)(
                std::move(std::get<__detail::value_type<T>>(data_).value));
        }
        return std::forward<FnErr>(fn_err)(
            std::move(std::get<__detail::error_type<E>>(data_).error));
    }

    template <typename FnErr>
    constexpr auto
    map_err(FnErr &&fn) const & -> Result<T, std::invoke_result_t<FnErr, E>>
    {
        using V = std::invoke_result_t<FnErr, E>;
        if (is_err()) {
            return Result<T, V>::Err(std::forward<FnErr>(fn)(
                std::get<__detail::error_type<E>>(data_).error));
        }
        return Result<T, V>::Ok(std::get<__detail::value_type<T>>(data_).value);
    }

    template <typename FnErr>
    constexpr auto
    map_err(FnErr &&fn) && -> Result<T, std::invoke_result_t<FnErr, E>>
    {
        using V = std::invoke_result_t<FnErr, E>;
        if (is_err()) {
            return Result<T, V>::Err(std::forward<FnErr>(fn)(
                std::move(std::get<__detail::error_type<E>>(data_).error)));
        }
        return Result<T, V>::Ok(
            std::move(std::get<__detail::value_type<T>>(data_).value));
    }

    template <typename Fn>
        requires fn_return_void<Fn, T>
    constexpr auto inspect(Fn &&fn) const & -> const Result &
    {
        if (is_ok()) {
            std::forward<Fn>(fn)(
                std::get<__detail::value_type<T>>(data_).value);
        }
        return *this;
    }

    template <typename Fn>
        requires fn_return_void<Fn, T>
    constexpr auto inspect(Fn &&fn) && -> Result &&
    {
        if (is_ok()) {
            std::forward<Fn>(fn)(
                std::move(std::get<__detail::value_type<T>>(data_).value));
        }
        return std::move(*this);
    }

    template <typename Fn>
        requires fn_return_void<Fn, E>
    constexpr auto inspect_err(Fn &&fn) const & -> const Result &
    {
        if (is_err()) {
            std::forward<Fn>(fn)(
                std::get<__detail::error_type<E>>(data_).error);
        }
        return *this;
    }

    template <typename Fn>
        requires fn_return_void<Fn, E>
    constexpr auto inspect_err(Fn &&fn) && -> Result &&
    {
        if (is_err()) {
            std::forward<Fn>(fn)(
                std::move(std::get<__detail::error_type<E>>(data_).error));
        }
        return std::move(*this);
    }

    // ======================================================================
    // Extract a value
    // ======================================================================

    auto expect(const char *msg) const & -> T
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).value;
        }
        const E &e = std::get<__detail::error_type<E>>(data_).error;
        unwrap_failed(msg, e);
    }

    auto expect(const char *msg) && -> T
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).value);
        }
        const E &e = std::get<__detail::error_type<E>>(data_).error;
        unwrap_failed(msg, e);
    }

    auto unwrap() const & -> T
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).value;
        }
        const E &e = std::get<__detail::error_type<E>>(data_).error;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    auto unwrap() && -> T
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).value);
        }
        const E &e = std::get<__detail::error_type<E>>(data_).error;
        unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
    }

    auto unwrap_or_default() const & -> T
        requires is_default_constructible<T>
    {
        if (is_ok()) {
            return std::get<__detail::value_type<T>>(data_).value;
        }
        return T{};
    }

    auto unwrap_or_default() && -> T
        requires is_default_constructible<T>
    {
        if (is_ok()) {
            return std::move(std::get<__detail::value_type<T>>(data_).value);
        }
        return T{};
    }

    auto expect_err(const char *msg) const & -> E
    {
        if (is_err()) {
            return std::get<__detail::error_type<E>>(data_).error;
        }
        const T &v = std::get<__detail::value_type<T>>(data_).value;
        unwrap_failed(msg, v);
    }

    auto expect_err(const char *msg) && -> E
    {
        if (is_err()) {
            return std::move(std::get<__detail::error_type<E>>(data_).error);
        }
        const T &v = std::get<__detail::value_type<T>>(data_).value;
        unwrap_failed(msg, v);
    }

    auto unwrap_err() const & -> E
    {
        if (is_err()) {
            return std::get<__detail::error_type<E>>(data_).error;
        }
        const T &v = std::get<__detail::value_type<T>>(data_).value;
        unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", v);
    }

    auto unwrap_err() && -> E
    {
        if (is_err()) {
            return std::move(std::get<__detail::error_type<E>>(data_).error);
        }
        const T &v = std::get<__detail::value_type<T>>(data_).value;
        unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", v);
    }

    template <typename U>
    constexpr auto and_(Result<U, E> &res) const & -> Result<U, E>
    {
        if (is_ok()) {
            return res;
        }
        return Result<U, E>::Err(
            std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename U>
    constexpr auto and_(Result<U, E> &&res) const & -> Result<U, E>
    {
        if (is_ok()) {
            return std::forward<Result<U, E>>(res);
        }
        return Result<U, E>::Err(
            std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename U>
    constexpr auto and_(Result<U, E> &&res) && -> Result<U, E>
    {
        if (is_ok()) {
            return std::forward<Result<U, E>>(res);
        }
        return Result<U, E>::Err(
            std::move(std::get<__detail::error_type<E>>(data_).error));
    }

    template <typename Fn>
        requires __detail::is_result_v<std::invoke_result_t<Fn, T>>
    constexpr auto and_then(Fn &&fn) const &
    {
        using Ret = std::invoke_result_t<Fn, T>;
        if (is_ok()) {
            return std::forward<Fn>(fn)(
                std::get<__detail::value_type<T>>(data_).value);
        }
        return Ret::Err(std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename Fn>
        requires __detail::is_result_v<std::invoke_result_t<Fn, T>>
    constexpr auto and_then(Fn &&fn) &&
    {
        using Ret = std::invoke_result_t<Fn, T>;
        if (is_ok()) {
            return std::forward<Fn>(fn)(
                std::move(std::get<__detail::value_type<T>>(data_).value));
        }
        return Ret::Err(
            std::move(std::get<__detail::error_type<E>>(data_).error));
    }

    constexpr auto or_(const Result<T, E> &res) const & -> Result<T, E>
    {
        if (is_ok()) {
            return Result<T, E>::Ok(
                std::get<__detail::value_type<T>>(data_).value);
        }
        return res;
    }

    constexpr auto or_(Result<T, E> &&res) const & -> Result<T, E>
    {
        if (is_ok()) {
            return Result<T, E>::Ok(
                std::get<__detail::value_type<T>>(data_).value);
        }
        return std::forward<Result<T, E>>(res);
    }

    constexpr auto or_(Result<T, E> &&res) && -> Result<T, E>
    {
        if (is_ok()) {
            return Result<T, E>::Ok(
                std::move(std::get<__detail::value_type<T>>(data_).value));
        }
        return std::forward<Result<T, E>>(res);
    }

    template <typename Fn>
        requires __detail::is_result_v<std::invoke_result_t<Fn, E>>
    constexpr auto or_else(Fn &&fn) const &
    {
        using Ret = std::invoke_result_t<Fn, E>;
        if (is_ok()) {
            return Ret::Ok(std::get<__detail::value_type<T>>(data_).value);
        }

        return std::forward<Fn>(fn)(
            std::get<__detail::error_type<E>>(data_).error);
    }

    template <typename Fn>
        requires __detail::is_result_v<std::invoke_result_t<Fn, E>>
    constexpr auto or_else(Fn &&fn) &&
    {
        using Ret = std::invoke_result_t<Fn, E>;
        if (is_ok()) {
            return Ret::Ok(
                std::move(std::get<__detail::value_type<T>>(data_).value));
        }

        return std::forward<Fn>(fn)(
            std::move(std::get<__detail::error_type<E>>(data_).error));
    }

    // ======================================================================
    // Impl Clone for Result
    // ======================================================================

    auto clone() const & -> Result
        requires(is_cloneable<T> && is_cloneable<E>)
    {
        return Result(*this);
    }

    [[deprecated("Cloning an rvalue Result is unnecessary. Use std::move() or "
                 "just assign the rvalue directly.")]] auto
    clone() && -> Result
        requires(is_cloneable<T> && is_cloneable<E>)
    {
        return std::move(*this);
    }

    auto clone_from(const Result &other) -> void
        requires(is_cloneable<T> && is_cloneable<E>)
    {
        if (this == &other) {
            return;
        }

        if (other.is_ok()) {
            const T &val = std::get<__detail::value_type<T>>(other.data_).value;
            if (is_ok()) {
                std::get<__detail::value_type<T>>(data_).value = val;
            } else {
                data_.template emplace<__detail::value_type<T>>(val);
            }
        } else {
            const E &err = std::get<__detail::error_type<E>>(other.data_).error;
            if (is_err()) {
                std::get<__detail::error_type<E>>(data_).error = err;
            } else {
                data_.template emplace<__detail::error_type<E>>(err);
            }
        }
    }

    [[deprecated(
        "clone_from() with rvalue is inefficient. Use move_from() for moving "
        "from rvalues, or clone_from(const Result&) for copying.")]]
    auto clone_from(Result &&other) -> void
        requires(is_cloneable<T> && is_cloneable<E>)
    {
        if (this == &other) {
            return;
        }

        if (other.is_ok()) {
            auto &&val =
                std::move(std::get<__detail::value_type<T>>(other.data_).value);
            if (is_ok()) {
                std::get<__detail::value_type<T>>(data_).value =
                    std::forward<T>(val);
            } else {
                data_.template emplace<__detail::value_type<T>>(
                    std::forward<T>(val));
            }
        } else {
            auto &&err =
                std::move(std::get<__detail::error_type<E>>(other.data_).error);
            if (is_err()) {
                std::get<__detail::error_type<E>>(data_).error =
                    std::forward<E>(err);
            } else {
                data_.template emplace<__detail::error_type<E>>(
                    std::forward<E>(err));
            }
        }
    }

    auto move_from(const Result &other) -> void = delete;

    auto move_from(Result &&other) -> void
    {
        if (this == &other) {
            return;
        }
        data_ = std::move(other.data_);
    }

    // ======================================================================
    // Comparison
    // ======================================================================

    [[nodiscard]] friend auto operator==(const Result &lhs, const Result &rhs)
        -> bool
    {
        if (lhs.is_ok() && rhs.is_ok()) {
            return std::get<__detail::value_type<T>>(lhs.data_).value ==
                   std::get<__detail::value_type<T>>(rhs.data_).value;
        }
        if (lhs.is_err() && rhs.is_err()) {
            return std::get<__detail::error_type<E>>(lhs.data_).error ==
                   std::get<__detail::error_type<E>>(rhs.data_).error;
        }
        return false;
    }

    [[nodiscard]] friend auto operator!=(const Result &lhs, const Result &rhs)
        -> bool
    {
        return !(lhs == rhs);
    }
};

// ======================================================================
// Helper factory methods
// ======================================================================

template <typename U, typename V>
[[nodiscard("Result must be used")]] auto Ok(const U &v) -> Result<U, V>
{
    return Result<U, V>(__detail::OkTag{}, v);
}

template <typename U, typename V>
[[nodiscard("Result must be used")]] auto Ok(U &&v) -> Result<U, V>
{
    return Result<U, V>(__detail::OkTag{}, std::move(v));
}

template <typename U, typename V>
[[nodiscard("Result must be used")]] auto Err(const V &e) -> Result<U, V>
{
    return Result<U, V>(__detail::ErrTag{}, e);
}

template <typename U, typename V>
[[nodiscard("Result must be used")]] auto Err(V &&e) -> Result<U, V>
{
    return Result<U, V>(__detail::ErrTag{}, std::move(e));
}

} // namespace rstd::result
