#pragma once

#include <optional>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>

namespace rstd
{

struct OkTag
{};
struct ErrTag
{};

template <typename T> struct value_type;
template <typename E> struct error_type;
template <typename T, typename E> class Result;

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

template <typename T, typename E> class Result
{
    std::variant<value_type<T>, error_type<E>> data;

    /* private ctors – only friends can call */
    Result(OkTag, const T &v) : data{value_type<T>{v}} {}
    Result(OkTag, T &&v) : data{value_type<T>{std::move(v)}} {}

    Result(ErrTag, const E &e) : data{error_type<E>{e}} {}
    Result(ErrTag, E &&e) : data{error_type<E>{std::move(e)}} {}

    template <typename ErrorType>
    [[noreturn]] static void unwrap_failed(const char *msg,
                                           const ErrorType &error)
    {
        std::ostringstream oss;
        oss << msg << ": ";
        if constexpr (std::is_same_v<ErrorType, std::string> ||
                      std::is_same_v<ErrorType, const char *> ||
                      std::is_convertible_v<ErrorType, std::string_view>) {
            oss << error;
        } else if constexpr (requires(std::ostream &os, const ErrorType &e) {
                                 os << e;
                             }) {
            oss << error;
        } else {
            oss << "[object of type " << typeid(ErrorType).name() << "]";
        }
        throw std::runtime_error(oss.str());
    }

public:
    // ==============================
    //        Object creations
    // ==============================

    // Static factory methods
    static Result Ok(const T &value) { return Result(OkTag{}, value); }
    static Result Ok(T &&value) { return Result(OkTag{}, std::move(value)); }
    static Result Err(const E &error) { return Result(ErrTag{}, error); }
    static Result Err(E &&error) { return Result(ErrTag{}, std::move(error)); }

    // Friend with other Result<U, V>
    template <typename U, typename V> friend class Result;

    // Convenience factory friend functions
    template <typename U, typename V> friend Result<U, V> Ok(const U &v);
    template <typename U, typename V> friend Result<U, V> Ok(U &&v);
    template <typename U, typename V> friend Result<U, V> Err(const V &e);
    template <typename U, typename V> friend Result<U, V> Err(V &&e);

    // ==============================
    // Querying the contained values
    // ==============================
    bool is_ok() const { return std::holds_alternative<value_type<T>>(data); }

    template <typename Pred> bool is_ok_and(Pred pred) const
    {
        return is_ok() && pred(std::get<value_type<T>>(data).value);
    }

    bool is_err() const { return std::holds_alternative<error_type<E>>(data); }

    template <typename Pred> bool is_err_and(Pred pred) const
    {
        return is_err() && pred(std::get<error_type<E>>(data).error);
    }

    std::optional<T> ok() const
    {
        return is_ok() ? std::optional<T>(std::get<value_type<T>>(data).value)
                       : std::nullopt;
    }

    std::optional<E> err() const
    {
        return is_err() ? std::optional<E>(std::get<error_type<E>>(data).error)
                        : std::nullopt;
    }

    // ==============================
    // Transforming contained values
    // ==============================

    template <typename Fn>
    constexpr auto
    map(Fn &&fn) const & -> Result<decltype(fn(std::declval<T>())), E>
    {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).value;
            return Result<U, E>(OkTag{}, fn(v));
        }
        const E &e = std::get<error_type<E>>(data).error;
        return Result<U, E>(ErrTag{}, e);
    }

    template <typename Fn>
    constexpr auto map(Fn &&fn) && -> Result<decltype(fn(std::declval<T>())), E>
    {
        using U = decltype(fn(std::declval<T>()));

        if (is_ok()) {
            T &&v = std::move(std::get<value_type<T>>(data).value);
            return Result<U, E>(OkTag{}, fn(std::forward<T>(v)));
        }

        E &&e = std::move(std::get<error_type<E>>(data).error);
        return Result<U, E>(ErrTag{}, std::forward<E>(e));
    }

    template <typename U, typename Fn>
    constexpr U map_or(U &&default_val, Fn &&fn) const &
    {
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).value;
            return std::forward<Fn>(fn)(v);
        }
        return std::forward<U>(default_val);
    }

    template <typename U, typename Fn>
    constexpr U map_or(U &&default_val, Fn &&fn) &&
    {
        if (is_ok()) {
            T &&v = std::move(std::get<value_type<T>>(data).value);
            return std::forward<Fn>(fn)(std::forward<T>(v));
        }
        return std::forward<U>(default_val);
    }

    // ==============================
    //        Extract a value
    // ==============================
    T expect(const char *msg) &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed(msg, e);
        }
    }

    T expect(const char *msg) const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed(msg, e);
        }
    }

    T unwrap() &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
        }
    }

    T unwrap() const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
        }
    }

    template <typename U = T> U unwrap_or_default() &&
    {
        static_assert(std::is_default_constructible_v<U>,
                      "T must be default constructible for "
                      "unwrap_or_default()");

        if (is_ok()) {
            return std::move(std::get<value_type<U>>(data).value);
        } else {
            return U{};
        }
    }

    template <typename U = T> U unwrap_or_default() const &
    {
        static_assert(std::is_default_constructible_v<U>,
                      "T must be default constructible for "
                      "unwrap_or_default()");

        if (is_ok()) {
            return std::get<value_type<U>>(data).value;
        } else {
            return U{};
        }
    }
};

// ==============================
//     Helper factory methods
// ==============================

template <typename U, typename V> Result<U, V> Ok(const U &v)
{
    return Result<U, V>(OkTag{}, v);
}

template <typename U, typename V> Result<U, V> Ok(U &&v)
{
    return Result<U, V>(OkTag{}, std::move(v));
}

template <typename U, typename V> Result<U, V> Err(const V &e)
{
    return Result<U, V>(ErrTag{}, e);
}

template <typename U, typename V> Result<U, V> Err(V &&e)
{
    return Result<U, V>(ErrTag{}, std::move(e));
}

} // namespace rstd
