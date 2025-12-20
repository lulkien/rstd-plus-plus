#pragma once

#include <optional>
#include <ostream>
#include <sstream>
#include <utility>
#include <variant>

namespace rstd
{

/**
 * @brief Tag type for creating successful results.
 *
 * Used internally to differentiate between Ok and Err constructors.
 */
struct OkTag
{};

/**
 * @brief Tag type for creating error results.
 *
 * Used internally to differentiate between Ok and Err constructors.
 */
struct ErrTag
{};

// Forward declarations
template <typename T> struct value_type;
template <typename E> struct error_type;
template <typename T, typename E> class Result;

/**
 * @brief Wrapper type for success values in Result.
 *
 * Stores the actual value of type T when Result is in the Ok state.
 */
template <typename T> struct value_type
{
    T value;
    value_type(const T &d) : value{d} {}
    value_type(T &&d) : value{std::move(d)} {}
};

/**
 * @brief Wrapper type for error values in Result.
 *
 * Stores the actual error of type E when Result is in the Err state.
 */
template <typename E> struct error_type
{
    E error;
    error_type(const E &e) : error{e} {}
    error_type(E &&e) : error{std::move(e)} {}
};

/**
 * @brief Type for returning and propagating errors.
 *
 * Result<T, E> is a type that can represent either success (Ok) containing a
 * value of type T, or failure (Err) containing an error value of type E.
 *
 * Functions return Result whenever errors are expected and recoverable.
 *
 * @tparam T Type of the success value.
 * @tparam E Type of the error value.
 *
 * @note This class is marked with [[nodiscard]] semantics - ignoring a Result
 * may indicate a bug.
 *
 * # Examples
 *
 * Basic usage:
 * @code
 * Result<int, std::string> parse_number(const std::string& s) {
 *     try {
 *         return rstd::Result<int, std::string>::Ok(std::stoi(s));
 *     } catch (...) {
 *         return rstd::Result<int, std::string>::Err("Invalid number");
 *     }
 * }
 *
 * auto result = parse_number("42");
 * if (result.is_ok()) {
 *     std::cout << "Success: " << result.unwrap() << std::endl;
 * } else {
 *     std::cout << "Error: " << result.unwrap_err() << std::endl;
 * }
 * @endcode
 */
template <typename T, typename E> class Result
{
    std::variant<value_type<T>, error_type<E>> data;

    /* private ctors – only friends can call */
    Result(OkTag, const T &v) : data{value_type<T>{v}} {}
    Result(OkTag, T &&v) : data{value_type<T>{std::move(v)}} {}

    Result(ErrTag, const E &e) : data{error_type<E>{e}} {}
    Result(ErrTag, E &&e) : data{error_type<E>{std::move(e)}} {}

    template <typename PanicType>
    [[noreturn]] static void unwrap_failed(const char *msg, const PanicType &value)
    {
        std::ostringstream oss;
        oss << msg << ": ";

        if constexpr (std::is_same_v<PanicType, std::string> ||
                      std::is_same_v<PanicType, const char *> ||
                      std::is_convertible_v<PanicType, std::string_view>) {
            oss << value;
        } else if constexpr (requires(std::ostream &os, const PanicType &v) { os << v; }) {
            oss << value;
        } else {
            oss << "[object of type " << typeid(PanicType).name() << "]";
        }

        throw std::runtime_error(oss.str());
    }

public:
    // ==============================
    //        Object creations
    // ==============================

    /**
     * @brief Creates a successful Result containing the given value.
     *
     * @param value The success value to store.
     * @return Result containing the value in the Ok state.
     *
     * @note This is a static factory method. Use Result<T, E>::Ok(value)
     * syntax.
     */
    [[nodiscard]] static Result Ok(const T &value) { return Result(OkTag{}, value); }

    /**
     * @brief Creates a successful Result by moving the given value.
     *
     * @param value The success value to move into the Result.
     * @return Result containing the moved value in the Ok state.
     */
    static Result Ok(T &&value) { return Result(OkTag{}, std::move(value)); }

    /**
     * @brief Creates an error Result containing the given error.
     *
     * @param error The error value to store.
     * @return Result containing the error in the Err state.
     */
    static Result Err(const E &error) { return Result(ErrTag{}, error); }

    /**
     * @brief Creates an error Result by moving the given error.
     *
     * @param error The error value to move into the Result.
     * @return Result containing the moved error in the Err state.
     */
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

    /**
     * @brief Returns true if the result is Ok.
     *
     * @return true if the result contains a success value.
     * @return false if the result contains an error.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * Result<int, std::string> y = Err("error");
     * assert(x.is_ok() && !y.is_ok());
     * @endcode
     */
    bool is_ok() const { return std::holds_alternative<value_type<T>>(data); }

    /**
     * @brief Returns true if the result is Ok and the value matches a
     *        predicate.
     *
     * @tparam Pred Type of the predicate function.
     * @param pred Predicate function that takes the contained value by
     *        const reference.
     * @return true if the result is Ok and pred(value) returns true.
     * @return false otherwise.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * assert(x.is_ok_and([](int v) { return v > 1; }));  // true
     * assert(!x.is_ok_and([](int v) { return v > 5; })); // false
     * @endcode
     */
    template <typename Pred> bool is_ok_and(Pred pred) const &
    {
        return is_ok() && pred(std::get<value_type<T>>(data).value);
    }

    /**
     * @brief Returns true if the result is Ok and the value matches a
     * predicate.
     *
     * Rvalue overload that can move the value into the predicate.
     *
     * @tparam Pred Type of the predicate function.
     * @param pred Predicate function that can take the contained value by
     *        value (move).
     * @return true if the result is Ok and pred(value) returns true.
     * @return false otherwise.
     *
     * # Examples
     * @code
     * Result<std::unique_ptr<int>, std::string> x =
     * Ok(std::make_unique<int>(2)); bool check =
     *   std::move(x).is_ok_and([](std::unique_ptr<int> p) {
     *     return *p == 2;
     *   });
     * @endcode
     */
    template <typename Pred> bool is_ok_and(Pred pred) &&
    {
        return is_ok() && pred(std::move(std::get<value_type<T>>(data).value));
    }

    /**
     * @brief Returns true if the result is Err.
     *
     * @return true if the result contains an error.
     * @return false if the result contains a success value.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * Result<int, std::string> y = Err("error");
     * assert(y.is_err() && !x.is_err());
     * @endcode
     */
    bool is_err() const { return std::holds_alternative<error_type<E>>(data); }

    /**
     * @brief Returns true if the result is Err and the error matches a
     *        predicate.
     *
     * @tparam Pred Type of the predicate function.
     * @param pred Predicate function that takes the contained error by
     *        const reference.
     * @return true if the result is Err and pred(error) returns true.
     * @return false otherwise.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Err("not found");
     * assert(x.is_err_and([](const std::string& e) {
     *   return e.find("not") != std::string::npos;
     * }));
     * @endcode
     */
    template <typename Pred> bool is_err_and(Pred pred) const &
    {
        return is_err() && pred(std::get<error_type<E>>(data).error);
    }

    /**
     * @brief Returns true if the result is Err and the error matches a
     *        predicate.
     *
     * Rvalue overload that can move the error into the predicate.
     *
     * @tparam Pred Type of the predicate function.
     * @param pred Predicate function that can take the contained error
     *        by value (move).
     * @return true if the result is Err and pred(error) returns true.
     * @return false otherwise.
     */
    template <typename Pred> bool is_err_and(Pred pred) &&
    {
        return is_err() && pred(std::move(std::get<error_type<E>>(data).error));
    }

    /**
     * @brief Returns optional containing copy of value if Ok, nullopt if Err.
     *
     * Const lvalue version. Requires T to be copy constructible.
     * Preserves original Result.
     *
     * @return std::optional<T> Copy of value or nullopt.
     * @pre T must be copy constructible.
     *
     * @code
     * Result<int, string> r = Ok(42);
     * auto opt = r.ok();  // optional(42), r still valid
     * @endcode
     */
    std::optional<T> ok() const &
    {
        static_assert(std::is_copy_constructible_v<T>,
                      "T must be copy constructible for ok() const&");
        return is_ok() ? std::optional<T>(std::get<value_type<T>>(data).value) : std::nullopt;
    }

    /**
     * @brief Returns optional containing moved value if Ok, nullopt if Err.
     *
     * Rvalue version. Works with move-only types.
     * Consumes Result (leaves moved-from state).
     *
     * @return std::optional<T> Moved value or nullopt.
     *
     * @code
     * Result<unique_ptr<int>, string> r = Ok(make_unique<int>(42));
     * auto opt = std::move(r).ok();  // Moves unique_ptr
     * @endcode
     */
    std::optional<T> ok() &&
    {
        if (is_ok()) {
            return std::optional<T>(std::move(std::get<value_type<T>>(data).value));
        }
        return std::nullopt;
    }

    /**
     * @brief Returns optional containing copy of error if Err, nullopt if Ok.
     *
     * Const lvalue version. Requires E to be copy constructible.
     * Preserves original Result.
     *
     * @return std::optional<E> Copy of error or nullopt.
     * @pre E must be copy constructible.
     *
     * @code
     * Result<int, string> r = Err("failed");
     * auto opt = r.err();  // optional("failed"), r still valid
     * @endcode
     */
    std::optional<E> err() const &
    {
        static_assert(std::is_copy_constructible_v<E>,
                      "E must be copy constructible for err() const&");
        return is_err() ? std::optional<E>(std::get<error_type<E>>(data).error) : std::nullopt;
    }

    /**
     * @brief Returns optional containing moved error if Err, nullopt if Ok.
     *
     * Rvalue version. Works with move-only error types.
     * Consumes Result (leaves moved-from state).
     *
     * @return std::optional<E> Moved error or nullopt.
     *
     * @code
     * Result<int, unique_ptr<string>> r = Err(make_unique<string>("error"));
     * auto opt = std::move(r).err();  // Moves unique_ptr<string>
     * @endcode
     */
    std::optional<E> err() &&
    {
        if (is_err()) {
            return std::optional<E>(std::move(std::get<error_type<E>>(data).error));
        }
        return std::nullopt;
    }

    // ==============================
    // Transforming contained values
    // ==============================

    /**
     * @brief Maps a Result<T, E> to Result<U, E> by applying a function
     *        to a contained Ok value.
     *
     * Leaves an Err value untouched.
     *
     * @tparam Fn Type of the mapping function.
     * @param fn Function to apply to the contained Ok value.
     * @return Result<U, E> where U is the return type of fn.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * auto y = x.map([](int n) { return n * 2; });  // Ok(4)
     *
     * Result<int, std::string> z = Err("error");
     * auto w = z.map([](int n) { return n * 2; });  // Err("error")
     * @endcode
     */
    template <typename Fn>
    constexpr auto map(Fn &&fn) const & -> Result<decltype(fn(std::declval<T>())), E>
    {
        using U = decltype(fn(std::declval<T>()));
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).value;
            return Result<U, E>(OkTag{}, fn(v));
        }
        const E &e = std::get<error_type<E>>(data).error;
        return Result<U, E>(ErrTag{}, e);
    }

    /**
     * @brief Maps a Result<T, E> to Result<U, E> by applying a function
     *        to a contained Ok value.
     *
     * Rvalue overload that can move the value into the function.
     *
     * @tparam Fn Type of the mapping function.
     * @param fn Function to apply to the contained Ok value.
     * @return Result<U, E> where U is the return type of fn.
     *
     * # Examples
     * @code
     * Result<std::string, int> x = Ok("hello");
     * auto y = std::move(x).map([](std::string s) {
     *     s += " world";
     *     return s;
     * });  // Moves the string into the lambda
     * @endcode
     */
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

    /**
     * @brief Returns the provided default if Err, or applies a function
     *        to the contained Ok value.
     *
     * @tparam U Type of the default value (and return type).
     * @tparam Fn Type of the mapping function.
     * @param default_val Default value to return if the result is Err.
     * @param fn Function to apply to the contained Ok value.
     * @return U The result of applying fn to the Ok value, or default_val
     *         if Err.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * Result<int, std::string> y = Err("error");
     * assert(x.map_or(42, [](int n) { return n * 2; }) == 4);
     * assert(y.map_or(42, [](int n) { return n * 2; }) == 42);
     * @endcode
     */
    template <typename U, typename Fn> constexpr U map_or(U &&default_val, Fn &&fn) const &
    {
        if (is_ok()) {
            const T &v = std::get<value_type<T>>(data).value;
            return std::forward<Fn>(fn)(v);
        }
        return std::forward<U>(default_val);
    }

    /**
     * @brief Returns the provided default if Err, or applies a function
     *        to the contained Ok value.
     *
     * Rvalue overload that can move the value into the function.
     *
     * @tparam U Type of the default value (and return type).
     * @tparam Fn Type of the mapping function.
     * @param default_val Default value to return if the result is Err.
     * @param fn Function to apply to the contained Ok value.
     * @return U The result of applying fn to the Ok value, or default_val if
     * Err.
     */
    template <typename U, typename Fn> constexpr U map_or(U &&default_val, Fn &&fn) &&
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

    /**
     * @brief Returns the contained Ok value, consuming the self value.
     *
     * Const lvalue overload that returns a copy of the value.
     *
     * @param msg Custom panic message to display if the result is Err.
     * @return T A copy of the contained success value.
     * @throws std::runtime_error if the value is an Err.
     *
     * @note This version preserves the original Result object.
     */
    T expect(const char *msg) const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed(msg, e);
        }
    }

    /**
     * @brief Returns the contained Ok value, consuming the self value.
     *
     * @param msg Custom panic message to display if the result is Err.
     * @return T The contained success value.
     * @throws std::runtime_error if the value is an Err, with a panic message
     *         including the passed message and the content of the Err.
     *
     * @note Because this function may throw, its use is generally discouraged.
     *       Instead, prefer to use pattern matching and handle the Err case
     *       explicitly, or call unwrap_or, unwrap_or_else, or
     *       unwrap_or_default.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * assert(x.expect("should be Ok") == 2);
     *
     * Result<int, std::string> y = Err("error");
     * // y.expect("Testing expect");  // throws: "Testing expect: error"
     * @endcode
     */
    T expect(const char *msg) &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed(msg, e);
        }
    }

    /**
     * @brief Returns the contained Ok value, consuming the self value.
     *
     * Const lvalue overload that returns a copy of the value.
     *
     * @return T A copy of the contained success value.
     * @throws std::runtime_error if the value is an Err.
     *
     * @note This version preserves the original Result object.
     */
    T unwrap() const &
    {
        if (is_ok()) {
            return std::get<value_type<T>>(data).value;
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
        }
    }

    /**
     * @brief Returns the contained Ok value, consuming the self value.
     *
     * @return T The contained success value.
     * @throws std::runtime_error if the value is an Err, with a generic
     *         panic message.
     *
     * @note Because this function may throw, its use is generally discouraged.
     *       Panics are meant for unrecoverable errors.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * assert(x.unwrap() == 2);
     *
     * Result<int, std::string> y = Err("error");
     * // y.unwrap();
     * // throws: "called `Result::unwrap()` on an `Err` value: error"
     * @endcode
     */
    T unwrap() &&
    {
        if (is_ok()) {
            return std::move(std::get<value_type<T>>(data).value);
        } else {
            const E &e = std::get<error_type<E>>(data).error;
            unwrap_failed("called `Result::unwrap()` on an `Err` value", e);
        }
    }

    /**
     * @brief Returns the contained Ok value or a default.
     *
     * Const lvalue overload that returns a copy of the value or default.
     *
     * @tparam U Type to return (defaults to T).
     * @return U A copy of the contained value if Ok, or U{} if Err.
     *
     * @pre T must be default constructible.
     */
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

    /**
     * @brief Returns the contained Ok value or a default.
     *
     * Consumes the self argument then, if Ok, returns the contained value,
     * otherwise if Err, returns the default value for that type.
     *
     * @tparam U Type to return (defaults to T).
     * @return U The contained value if Ok, or U{} if Err.
     *
     * @pre T must be default constructible.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Ok(2);
     * Result<int, std::string> y = Err("error");
     * assert(x.unwrap_or_default() == 2);
     * assert(y.unwrap_or_default() == 0);  // int default is 0
     * @endcode
     */
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

    /**
     * @brief Returns the contained Err value, consuming the self value.
     *
     * Const lvalue overload that returns a copy of the error.
     *
     * @param msg Custom panic message to display if the result is Ok.
     * @return E A copy of the contained error value.
     * @throws std::runtime_error if the value is an Ok.
     */
    E expect_err(const char *msg) const &
    {
        if (is_err()) {
            return std::get<error_type<E>>(data).error;
        } else {
            const T &t = std::get<value_type<T>>(data).value;
            unwrap_failed(msg, t);
        }
    }

    /**
     * @brief Returns the contained Err value, consuming the self value.
     *
     * @param msg Custom panic message to display if the result is Ok.
     * @return E The contained error value.
     * @throws std::runtime_error if the value is an Ok, with a panic message
     *         including the passed message and the content of the Ok.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Err("error");
     * assert(x.expect_err("should be Err") == "error");
     *
     * Result<int, std::string> y = Ok(2);
     * // y.expect_err("Testing expect_err");  // throws: "Testing expect_err: 2"
     * @endcode
     */
    E expect_err(const char *msg) &&
    {
        if (is_err()) {
            return std::move(std::get<error_type<E>>(data).error);
        } else {
            const T &t = std::get<value_type<T>>(data).value;
            unwrap_failed(msg, t);
        }
    }

    /**
     * @brief Returns the contained Err value, consuming the self value.
     *
     * Const lvalue overload that returns a copy of the error.
     *
     * @return E A copy of the contained error value.
     * @throws std::runtime_error if the value is an Ok.
     */
    E unwrap_err() const &
    {
        if (is_err()) {
            return std::get<error_type<E>>(data).error;
        } else {
            const T &t = std::get<value_type<T>>(data).value;
            unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", t);
        }
    }

    /**
     * @brief Returns the contained Err value, consuming the self value.
     *
     * @return E The contained error value.
     * @throws std::runtime_error if the value is an Ok, with a generic
     *         panic message.
     *
     * # Examples
     * @code
     * Result<int, std::string> x = Err("error");
     * assert(x.unwrap_err() == "error");
     *
     * Result<int, std::string> y = Ok(2);
     * // y.unwrap_err();  // throws: "called `Result::unwrap_err()` on an `Ok` value: 2"
     * @endcode
     */
    E unwrap_err() &&
    {
        if (is_err()) {
            return std::move(std::get<error_type<E>>(data).error);
        } else {
            const T &t = std::get<value_type<T>>(data).value;
            unwrap_failed("called `Result::unwrap_err()` on an `Ok` value", t);
        }
    }
};

// ==============================
//     Helper factory methods
// ==============================

/**
 * @brief Creates a successful Result containing the given value.
 *
 * @tparam U Type of the success value.
 * @tparam V Type of the error value.
 * @param v The success value to store.
 * @return Result<U, V> containing the value in the Ok state.
 *
 * @note This is a free function alternative to Result<U, V>::Ok().
 */
template <typename U, typename V> Result<U, V> Ok(const U &v) { return Result<U, V>(OkTag{}, v); }

/**
 * @brief Creates a successful Result by moving the given value.
 *
 * @tparam U Type of the success value.
 * @tparam V Type of the error value.
 * @param v The success value to move into the Result.
 * @return Result<U, V> containing the moved value in the Ok state.
 */
template <typename U, typename V> Result<U, V> Ok(U &&v)
{
    return Result<U, V>(OkTag{}, std::move(v));
}

/**
 * @brief Creates an error Result containing the given error.
 *
 * @tparam U Type of the success value.
 * @tparam V Type of the error value.
 * @param e The error value to store.
 * @return Result<U, V> containing the error in the Err state.
 */
template <typename U, typename V> Result<U, V> Err(const V &e) { return Result<U, V>(ErrTag{}, e); }

/**
 * @brief Creates an error Result by moving the given error.
 *
 * @tparam U Type of the success value.
 * @tparam V Type of the error value.
 * @param e The error value to move into the Result.
 * @return Result<U, V> containing the moved error in the Err state.
 */
template <typename U, typename V> Result<U, V> Err(V &&e)
{
    return Result<U, V>(ErrTag{}, std::move(e));
}

} // namespace rstd
