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

/*
 * @class Result
 * @brief A type-safe wrapper representing either success (Ok) or failure (Err).
 *
 * Result is a type that represents either success `Ok` or failure `Err`.
 * It provides a functional approach to error handling without exceptions.
 */
template <typename T, typename E>
class [[nodiscard("Result must be used")]] Result
{
    std::variant<__detail::value_type<T>, __detail::error_type<E>>
        data_; ///< Internal storage for either Ok or Err

    /**
     * @brief Private constructor for Ok variant (copy).
     * @param tag Ok tag for dispatching
     * @param v Value to store
     */
    Result(__detail::OkTag, const T &v) : data_{__detail::value_type<T>{v}} {}

    /**
     * @brief Private constructor for Ok variant (move).
     * @param tag Ok tag for dispatching
     * @param v Value to store
     */
    Result(__detail::OkTag, T &&v)
        : data_{__detail::value_type<T>{std::forward<T>(v)}}
    {}

    /**
     * @brief Private constructor for Err variant (copy).
     * @param tag Err tag for dispatching
     * @param e Error to store
     */
    Result(__detail::ErrTag, const E &e) : data_{__detail::error_type<E>{e}} {}

    /**
     * @brief Private constructor for Err variant (move).
     * @param tag Err tag for dispatching
     * @param e Error to store
     */
    Result(__detail::ErrTag, E &&e)
        : data_{__detail::error_type<E>{std::forward<E>(e)}}
    {}

    /**
     * @brief Helper for unwrap failure with printable types.
     * @param msg Error message
     * @param value The value that caused failure
     */
    template <typename panic_type>
    [[noreturn]] void unwrap_failed(const char *msg,
                                    const panic_type &value) const
        requires Printable<panic_type>
    {
        std::ostringstream oss;
        oss << msg << ": " << value;
        throw std::runtime_error(oss.str());
    }

    /**
     * @brief Helper for unwrap failure with non-printable types.
     * @param msg Error message
     * @param value The value that caused failure
     */
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
     * @brief Creates a successful Result with a copy of the value.
     * @param value The success value to store
     * @return Result containing the value
     */
    [[nodiscard("Result must be used")]] static auto Ok(const T &value)
        -> Result
    {
        return Result(__detail::OkTag{}, value);
    }

    /**
     * @brief Creates a successful Result by moving the value.
     * @param value The success value to move
     * @return Result containing the moved value
     */
    [[nodiscard("Result must be used")]] static auto Ok(T &&value) -> Result
    {
        return Result(__detail::OkTag{}, std::move(value));
    }

    /**
     * @brief Creates a failed Result with a copy of the error.
     * @param error The error value to store
     * @return Result containing the error
     */
    [[nodiscard("Result must be used")]] static auto Err(const E &error)
        -> Result
    {
        return Result(__detail::ErrTag{}, error);
    }

    /**
     * @brief Creates a failed Result by moving the error.
     * @param error The error value to move
     * @return Result containing the moved error
     */
    [[nodiscard("Result must be used")]] static auto Err(E &&error) -> Result
    {
        return Result(__detail::ErrTag{}, std::move(error));
    }

    // ======================================================================
    // Querying the contained values
    // ======================================================================

    /**
     * @brief Checks if the Result contains a value (Ok).
     *
     * Returns `true` if the result is Ok.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<int, const char*>::Ok(5);
     * assert(x.is_ok() == true);
     *
     * auto x = Result<int, const char*>::Err("Some error message");
     * assert(x.is_ok() == false);
     * @endcode
     */
    [[nodiscard]] auto is_ok() const -> bool
    {
        return std::holds_alternative<__detail::value_type<T>>(data_);
    }

    /**
     * @brief Checks if Result is Ok and value satisfies predicate (const ref).
     *
     * Returns `true` if the result is (Ok) and the value inside satisfies
     * predicate.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<int, const char*>::Ok(5);
     * auto y = x.is_ok_and([](const int& val) -> bool { return val > 0; })
     * assert(y == true);
     *
     * auto x = Result<int, const char*>::Ok(0);
     * auto y = x.is_ok_and([](const int& val) -> bool { return val > 0; })
     * assert(y == false);
     *
     * auto x = Result<int, const char*>::Err("Some error message");
     * auto y = x.is_ok_and([](const int& val) -> bool { return val > 0; })
     * assert(y == false);
     * @endcode
     */
    template <typename Pred>
        requires BoolReturning<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) const & -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(
                              std::get<__detail::value_type<T>>(data_).data);
    }

    /**
     * @brief Checks if Result is Ok and value satisfies predicate (rvalue ref).
     *
     * Returns `true` if the result is (Ok) and the value inside satisfies
     * predicate.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<int, const char*>::Ok(5).is_ok_and(
     *              [](const int& val) -> bool { return val > 0; }
     *          )
     * assert(x == true);
     *
     * auto x = Result<int, const char*>::Ok(0).is_ok_and(
                    [](const int& val) -> bool { return val > 0; }
                )
     * assert(x == false);
     *
     * auto x = Result<int, const char*>::Err("Some error message").is_ok_and(
                    [](const int& val) -> bool { return val > 0; }
                )
     * assert(x == false);
     * @endcode
     */
    template <typename Pred>
        requires BoolReturning<Pred, T>
    [[nodiscard]] auto is_ok_and(Pred &&pred) && -> bool
    {
        return is_ok() && std::forward<Pred>(pred)(std::move(
                              std::get<__detail::value_type<T>>(data_).data));
    }

    /**
     * @brief Checks if the Result contains an error (Err).
     *
     * Returns `true` if the result is (Err).
     *
     * Examples
     * @code{.cpp}
     * auto y = Result<Void, const char*>::Err("Some error message");
     * assert(x.is_err() == true);
     *
     * auto x = Result<int, Void>::Ok(5);
     * assert(x.is_err() == false);
     * @endcode
     */
    [[nodiscard]] auto is_err() const -> bool
    {
        return std::holds_alternative<__detail::error_type<E>>(data_);
    }

    /**
     * @brief Checks if Result is Err and error satisfies predicate (const ref).
     *
     * Returns `true` if the result is (Err) and the error inside satisfies
     * predicate.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<Void, int>::Err(127);
     * auto y = x.is_err_and([](const int& code) -> bool { return code != 0; });
     * assert(y == true);
     *
     * auto x = Result<Void, int>::Err(0);
     * auto y = x.is_err_and([](const int& code) -> bool { return val != 0; })
     * assert(y == false);
     *
     * auto x = Result<std::string, int>::Err("Some payload data");
     * auto y = x.is_err_and([](const std::string&) -> bool { return true; })
     * assert(y == false);
     * @endcode
     */
    template <typename Pred>
        requires BoolReturning<Pred, E>
    [[nodiscard]] auto is_err_and(Pred &&pred) const & -> bool
    {
        return is_err() && std::forward<Pred>(pred)(
                               std::get<__detail::error_type<E>>(data_).data);
    }

    /**
     * @brief Checks if Result is Err and error satisfies predicate
     *        (rvalue ref).
     *
     * Returns `true` if the result is (Err) and the error inside satisfies
     * predicate.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<Void, int>::Err(127).is_err_and(
     *              [](const int& code) -> bool { return code != 0; })
     * assert(x == true);
     *
     * auto x = Result<Void, int>::Err(0).is_err_and(
     *              [](const int& code) -> bool { return code != 0; })
     * assert(x == false);
     *
     * auto x = Result<std::string, int>::Ok("Some payload data").is_err_and(
     *              [](const std::string&) -> bool { return true; })
     * assert(x == false);
     * @endcode
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
     * @brief Converts from `Result<T, E>` to `std::option<T>` (const ref).
     *
     * Converts (Ok) into an `std::option<T>`, and discarding the error, if
     * any.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<int, Void>::Ok(100);
     * auto y = x.ok();
     * assert(y.has_value() == true);
     * assert(y.value() == 100);
     *
     * auto x = Result<Void, int>::Err(-99);
     * auto y = x.ok();
     * assert(y.has_value() == false);
     * @endcode
     */
    [[nodiscard]] auto ok() const & -> std::optional<T>
    {
        if (is_ok()) {
            return std::optional<T>(
                std::get<__detail::value_type<T>>(data_).data);
        }
        return std::nullopt;
    }

    /**
     * @brief Converts from `Result<T, E>` to `std::option<T>` (rvalue ref).
     *
     * Converts (Ok) into an `std::option<T>`, and discarding the error, if
     * any.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<int, Void>::Ok(100).ok();
     * assert(x.has_value() == true);
     * assert(x.value() == 100);
     *
     * auto x = Result<Void, int>::Err(-99).ok();
     * assert(y.has_value() == false);
     * @endcode
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
     * @brief Converts from `Result<T, E>` to `std::option<T>` (const ref).
     *
     * Converts (Err) into an `std::option<T>`, and discarding the success
     * value, if any.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<Void, int>::Err(100);
     * auto y = x.err();
     * assert(y.has_value() == true);
     * assert(y.value() == 100);
     *
     * auto x = Result<int, Void>::Ok(-99);
     * auto y = x.err();
     * assert(y.has_value() == false);
     * @endcode
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
     * @brief Converts from `Result<T, E>` to `std::option<T>` (rvalue ref).
     *
     * Converts (Err) into an `std::option<T>`, and discarding the success
     * value, if any.
     *
     * Examples:
     * @code{.cpp}
     * auto x = Result<Void, int>::Err(100).err();
     * assert(x.has_value() == true);
     * assert(x.value() == 100);
     *
     * auto x = Result<int, Void>::Ok(-99).err();
     * assert(y.has_value() == false);
     * @endcode
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
     * @brief Maps an (Ok) value to a new type, preserving (Err) (const ref).
     *
     * Maps a `Result<T, E>` to `Result<U, E>` by applying a function to a
     * contained (Ok) value, leaving an (Err) value untouched.
     *
     * Examples:
     * @code
     * auto x = Result<int, Void>::Ok(100);
     * auto y = x.map([](const int &value) -> int { return value / 2; });
     * assert(y.unwrap() == 50);
     *
     * auto x = Result<int, const char*>::Err("Error");
     * auto y = x.map([](const int &value) -> int { return value / 2; });
     * assert(y.is_err() == true);
     * assert(y.unwrap_err() == "Error");
     * @endcode
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
     * @brief Maps an (Ok) value to a new type, preserving (Err) (rvalue ref).
     *
     * Maps a `Result<T, E>` to `Result<U, E>` by applying a function to a
     * contained (Ok) value, leaving an (Err) value untouched.
     *
     * Examples:
     * @code
     * auto x = Result<int, Void>::Ok(100).map([](const int &value) -> int {
     *              return value / 2;
     *          });
     * assert(x.unwrap() == 50);
     *
     * auto x = Result<int, const char*>::Err("Error").map(
     *              [](const int &value) -> int { return value / 2; });
     * assert(x.is_err() == true);
     * assert(x.unwrap_err() == "Error");
     * @endcode
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
     * @brief Returns default if Err, otherwise applies function to Ok value
     * (const ref).
     *
     * Return the provided default value if (Err) or apply a function to the
     * contained (Ok) value.
     *
     * Examples:
     * @code
     * auto x = Result<int, Void>::Ok(100);
     * auto y = x.map_or(9999, [](const int &value) -> int { return value / 2;
     * }); assert(y == 50);
     *
     * auto x = Result<int, const char*>::Err("Error");
     * auto y = x.map_or(9999, [](const int &value) -> int { return value / 2;
     * }); assert(y == 9999);
     * @endcode
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
     * @brief Returns default if Err, otherwise applies function to Ok value
     * (rvalue ref).
     *
     * Return the provided default value if (Err) or apply a function to the
     * contained (Ok) value.
     *
     * Examples:
     * @code
     * auto x = Result<int, Void>::Err({}).map_or(9999,
     *                                            [](const int &value) -> int {
     *                                                return value / 2;
     *                                            });
     * assert(x == 9999);
     *
     * auto x = Result<int, const char*>::Err("Error").map_or(
     *              1234,
     *              [](const int &value) -> int { return value / 2; });
     * assert(x == 1234);
     * @endcode
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

} // namespace rstd
