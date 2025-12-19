#pragma once
#include <optional>
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

/* -------------------------------------------------------------
      Helper wrappers
         ------------------------------------------------------------- */
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

/* -------------------------------------------------------------
      Result class
         ------------------------------------------------------------- */
template <typename T, typename E> class Result
{
    std::variant<value_type<T>, error_type<E>> data;

    /* private ctors – only friends can call */
    Result(OkTag, const T &v) : data{value_type<T>{v}} {}
    Result(OkTag, T &&v) : data{value_type<T>{std::move(v)}} {}

    Result(ErrTag, const E &e) : data{error_type<E>{e}} {}
    Result(ErrTag, E &&e) : data{error_type<E>{std::move(e)}} {}

public:
    /* factories are friends */
    template <typename U, typename V> friend Result<U, V> Ok(const U &v);
    template <typename U, typename V> friend Result<U, V> Ok(U &&v);
    template <typename U, typename V> friend Result<U, V> Err(const V &e);
    template <typename U, typename V> friend Result<U, V> Err(V &&e);

    /* ---------------------------------------------------------
              Query helpers
                     ---------------------------------------------------------
     */
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

    /* ---------------------------------------------------------
              map – transform Ok, keep Err unchanged
                     ---------------------------------------------------------
     */
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

    /* ---------------------------------------------------------
              map_or – default on Err, otherwise apply fn
                     ---------------------------------------------------------
     */
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
};

/* -------------------------------------------------------------
      Factory functions
         ------------------------------------------------------------- */
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
