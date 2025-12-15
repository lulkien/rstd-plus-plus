#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

struct OkTag {};
struct ErrTag {};

template <typename E> struct Error {
    E value;

    Error(const E &e) : value(e) {}
    Error(E &&e) : value(std::move(e)) {}
};

template <typename T, typename E> class Result {
private:
    std::variant<T, Error<E>> value;

    Result(OkTag, const T &val) : value(val) {}
    Result(OkTag, T &&val) : value(std::move(val)) {}
    Result(ErrTag, const E &err) : value(Error<E>(err)) {}
    Result(ErrTag, E &&err) : value(Error<E>(std::move(err))) {}

public:
    template <typename U, typename V> friend class Result;
    template <typename U, typename V> friend Result<U, V> Ok(const U &val);
    template <typename U, typename V> friend Result<U, V> Ok(U &&val);
    template <typename U, typename V> friend Result<U, V> Err(const V &err);
    template <typename U, typename V> friend Result<U, V> Err(V &&err);

    // =============================================================

    /**
     * @brief Check if Result contains Ok value.
     * @return bool True if Ok, false if Err.
     */
    bool is_ok() const { return std::holds_alternative<T>(value); }

    /**
     * @brief Check if Result contains Ok value satisfying predicate.
     * @tparam Predicate Callable (T -> bool)
     * @param pred Predicate to test value
     * @return bool True if Ok and pred(value) true, else false.
     *
     * Predicate is only evaluated if Result is Ok.
     */
    template <typename Predicate> bool is_ok_and(Predicate pred) const {
        if (is_ok()) {
            return pred(std::get<T>(value));
        }
        return false;
    }

    /**
     * @brief Check if Result contains Err value.
     * @return bool True if Err, false if Ok.
     */
    bool is_err() const { return std::holds_alternative<Error<E>>(value); }

    /**
     * @brief Check if Result contains Err value satisfying predicate.
     * @tparam Predicate Callable (E -> bool)
     * @param pred Predicate to test error
     * @return bool True if Err and pred(error) true, else false.
     *
     * Predicate is only evaluated if Result is Err.
     */
    template <typename Predicate> bool is_err_and(Predicate pred) const {
        if (is_err()) {
            return pred(std::get<Error<E>>(value).value);
        }
        return false;
    }

    // =============================================================

    /**
     * @brief Unwrap Ok value or throw with custom message.
     * @tparam Msg Message type (convertible to string).
     * @param msg Custom error message.
     * @return T The Ok value.
     * @throws std::runtime_error If Result is Err.
     */
    template <typename Msg> T expect(Msg &&msg) {
        if (is_ok()) {
            return std::get<T>(value);
        }
        throw std::runtime_error(std::forward<Msg>(msg));
    }

    /**
     * @brief Unwrap Ok value or throw generic error.
     * @return T The Ok value.
     * @throws std::runtime_error If Result is Err.
     * @warning Only use when certain Result is Ok.
     */
    T unwrap() {
        if (is_ok()) {
            return std::get<T>(value);
        }
        throw std::runtime_error("Called unwrap on Err");
    }

    /**
     * @brief Unwrap Ok value or return default.
     * @param default_value Default returned if Err.
     * @return T Ok value or default_value.
     */
    T unwrap_or(T default_value) const {
        return is_ok() ? std::get<T>(value) : default_value;
    }

    /**
     * @brief Unwrap Ok value or compute default.
     * @tparam Func Function type (returns T).
     * @param func Function called if Err to produce default.
     * @return T Ok value or func() result.
     */
    template <typename Func> T unwrap_or_else(Func func) const {
        return is_ok() ? std::get<T>(value) : func();
    }

    /**
     * @brief Unwrap Ok value without any checks (unsafe).
     * @return T The Ok value.
     * @warning Causes undefined behavior if Result is Err.
     * @note Only use when performance critical and Result is guaranteed Ok.
     * @see unwrap(), expect()
     */
    T unwrap_unchecked() { return std::get<T>(value); }

    // =============================================================

    /**
     * @brief Unwrap Err value or throw with custom message.
     * @tparam Msg Message type (convertible to string).
     * @param msg Custom error message.
     * @return E The Err value.
     * @throws std::runtime_error If Result is Ok.
     * @note Requires T to have Debug trait (for error message).
     */
    template <typename Msg> E expect_err(Msg &&msg) {
        if (is_err()) {
            return std::get<Error<E>>(value).value;
        }
        throw std::runtime_error(std::forward<Msg>(msg));
    }

    /**
     * @brief Unwrap Err value or throw generic error.
     * @return E The Err value.
     * @throws std::runtime_error If Result is Ok.
     * @note Requires T to have Debug trait (for error message).
     */
    E unwrap_err() {
        if (is_err()) {
            return std::get<Error<E>>(value).value;
        }
        throw std::runtime_error("Called unwrap_err on Ok");
    }

    /**
     * @brief Unwrap Err value without any checks (unsafe).
     * @return E The Err value.
     * @warning Causes undefined behavior if Result is Ok.
     * @note Only use when performance critical and Result is guaranteed Err.
     */
    E unwrap_err_unchecked() { return std::get<Error<E>>(value).value; }

    // =============================================================

    /**
     * @brief Convert Result to optional containing Err value.
     * @return std::optional<E> Some(E) if Err, None if Ok.
     */
    std::optional<E> err() const {
        return is_err() ? std::make_optional(std::get<Error<E>>(value).value)
                        : std::nullopt;
    }

    /**
     * @brief Convert Result to optional containing Ok value.
     * @return std::optional<T> Some(T) if Ok, None if Err.
     */
    std::optional<T> ok() const {
        return is_ok() ? std::make_optional(std::get<T>(value)) : std::nullopt;
    }

    /**
     * @brief Transpose a Result of optional into optional of Result.
     * @return std::optional<Result<T, E>> Some(Result) if optional has value,
     * None if optional is nullopt.
     *
     * Converts Result<std::optional<T>, E> to std::optional<Result<T, E>>.
     * Ok(None) maps to None, Ok(Some(v)) maps to Some(Ok(v)), Err(e) maps to
     * Some(Err(e)).
     */
    template <typename U = T>
    auto transpose() const -> std::optional<Result<typename U::value_type, E>> {
        if (is_err()) {
            return std::make_optional(Result<typename U::value_type, E>(
                ErrTag{}, std::get<Error<E>>(value).value));
        }

        auto &opt = std::get<T>(value);
        if (opt.has_value()) {
            return std::make_optional(
                Result<typename U::value_type, E>(OkTag{}, opt.value()));
        }
        return std::nullopt;
    }

    // =============================================================

    /**
     * @brief Transform Ok value with function.
     * @tparam Func Function type (T -> U).
     * @param func Function applied to Ok value.
     * @return Result<U, E> New Result with transformed value.
     * @note Err values are passed through unchanged.
     */
    template <typename Func>
    auto
    map(Func func) const & -> Result<decltype(func(std::declval<T>())), E> {
        using U = decltype(func(std::declval<T>()));
        return is_ok()
                   ? Result<U, E>(OkTag{}, func(std::get<T>(value)))
                   : Result<U, E>(ErrTag{}, std::get<Error<E>>(value).value);
    }

    /**
     * @brief Peek at Ok value without consuming.
     * @tparam Func Function type (const T& -> void).
     * @param func Function to inspect Ok value.
     * @return Result<T, E>& Reference to this Result for chaining.
     */
    template <typename Func> Result<T, E> &inspect(Func func) & {
        if (is_ok())
            func(std::get<T>(value));
        return *this;
    }

    /**
     * @brief Transform Err value with function.
     * @tparam Func Function type (E -> F).
     * @param func Function applied to Err value.
     * @return Result<T, F> New Result with transformed error.
     * @note Ok values are passed through unchanged.
     */
    template <typename Func>
    auto
    map_err(Func func) const & -> Result<T, decltype(func(std::declval<E>()))> {
        using F = decltype(func(std::declval<E>()));
        return is_err() ? Result<T, F>(ErrTag{},
                                       func(std::get<Error<E>>(value).value))
                        : Result<T, F>(OkTag{}, std::get<T>(value));
    }

    /**
     * @brief Peek at Err value without consuming.
     * @tparam Func Function type (const E& -> void).
     * @param func Function to inspect Err value.
     * @return Result<T, E>& Reference to this Result for chaining.
     */
    template <typename Func> Result<T, E> &inspect_err(Func func) & {
        if (is_err())
            func(std::get<Error<E>>(value).value);
        return *this;
    }

    /**
     * @brief Transform Ok value or return default.
     * @tparam Func Function type (T -> U).
     * @param default_value Default value returned if Err.
     * @param func Function applied to Ok value.
     * @return U Transformed value or default_value.
     */
    template <typename Func>
    auto map_or(typename std::invoke_result<Func, T>::type default_value,
                Func func) const -> typename std::invoke_result<Func, T>::type {
        using U = typename std::invoke_result<Func, T>::type;
        return is_ok() ? func(std::get<T>(value)) : default_value;
    }

    /**
     * @brief Transform Ok value or compute default from Err.
     * @tparam OkFunc Function type (T -> U).
     * @tparam ErrFunc Function type (E -> U).
     * @param ok_func Function applied to Ok value.
     * @param err_func Function applied to Err value.
     * @return U Result of ok_func or err_func.
     */
    template <typename OkFunc, typename ErrFunc>
    auto map_or_else(OkFunc ok_func, ErrFunc err_func) const
        -> decltype(ok_func(std::declval<T>())) {
        using U = decltype(ok_func(std::declval<T>()));
        return is_ok() ? ok_func(std::get<T>(value))
                       : err_func(std::get<Error<E>>(value).value);
    }

    // =============================================================

    /**
     * @brief Return other if Ok, otherwise return self Err.
     * @tparam U Type of other Result's Ok value.
     * @param other Result returned if self is Ok.
     * @return Result<U, E> Other if self Ok, else self Err.
     *
     * Truth table:
     * - Err(e) and other → Err(e)
     * - Ok(x) and Err(d) → Err(d)
     * - Ok(x) and Ok(y) → Ok(y)
     */
    template <typename U> Result<U, E> and_(const Result<U, E> &other) const {
        return is_ok()
                   ? other
                   : Result<U, E>(ErrTag{}, std::get<Error<E>>(value).value);
    }

    /**
     * @brief Return other if Err, otherwise return self Ok.
     * @tparam F Type of other Result's Err value.
     * @param other Result returned if self is Err.
     * @return Result<T, F> Other if self Err, else self Ok.
     *
     * Truth table:
     * - Err(e) or Err(d) → Err(d)
     * - Err(e) or Ok(y) → Ok(y)
     * - Ok(x) or other → Ok(x)
     */
    template <typename F> Result<T, F> or_(const Result<T, F> &other) const {
        return is_err() ? other : Result<T, F>(OkTag{}, std::get<T>(value));
    }

    /**
     * @brief Apply function if Ok, otherwise return self Err.
     * @tparam Func Function type (T -> Result<U, E>).
     * @param func Function applied if self is Ok.
     * @return Result<U, E> func(x) if self Ok(x), else self Err(e).
     *
     * Truth table:
     * - Err(e) and_then func → Err(e)
     * - Ok(x) and_then func → func(x)
     */
    template <typename Func>
    auto and_then(Func func) const & -> decltype(func(std::declval<T>())) {
        using ResultU = decltype(func(std::declval<T>()));
        return is_ok() ? func(std::get<T>(value))
                       : ResultU(ErrTag{}, std::get<Error<E>>(value).value);
    }

    /**
     * @brief Apply function if Err, otherwise return self Ok.
     * @tparam Func Function type (E -> Result<T, F>).
     * @param func Function applied if self is Err.
     * @return Result<T, F> func(e) if self Err(e), else self Ok(x).
     *
     * Truth table:
     * - Err(e) or_else func → func(e)
     * - Ok(x) or_else func → Ok(x)
     */
    template <typename Func>
    auto or_else(Func func) const & -> decltype(func(std::declval<E>())) {
        using ResultF = decltype(func(std::declval<E>()));
        return is_err() ? func(std::get<Error<E>>(value).value)
                        : ResultF(OkTag{}, std::get<T>(value));
    }

    /**
     * @brief Pattern match on Result.
     * @param ok_func Called if Ok with value.
     * @param err_func Called if Err with error.
     * @return Whatever either function returns.
     */
    template <typename OkFunc, typename ErrFunc>
    auto match(OkFunc ok_func, ErrFunc err_func) const {
        if (is_ok()) {
            return ok_func(std::get<T>(value));
        } else {
            return err_func(std::get<Error<E>>(value).value);
        }
    }

    template <typename T2, typename E2>
    friend std::ostream &operator<<(std::ostream &, const Result<T2, E2> &);
};

template <typename T, typename E> Result<T, E> Ok(const T &val) {
    return Result<T, E>(OkTag{}, val);
}

template <typename T, typename E> Result<T, E> Ok(T &&val) {
    return Result<T, E>(OkTag{}, std::move(val));
}

template <typename T, typename E> Result<T, E> Err(const E &err) {
    return Result<T, E>(ErrTag{}, err);
}

template <typename T, typename E> Result<T, E> Err(E &&err) {
    return Result<T, E>(ErrTag{}, std::move(err));
}

template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream &>()
                                             << std::declval<T>())>>
    : std::true_type {};

template <typename T, typename E>
std::ostream &operator<<(std::ostream &os, const Result<T, E> &result) {
    if (result.is_ok()) {
        os << std::string("Ok(");
        if constexpr (is_streamable<T>::value) {
            os << std::get<T>(result.value);
        } else {
            os << std::string("<non-streamable>");
        }
        os << std::string(")");
    } else {
        os << std::string("Err(");
        if constexpr (is_streamable<E>::value) {
            os << std::get<Error<E>>(result.value).value;
        } else {
            os << std::string("<non-streamable>");
        }
        os << std::string(")");
    }
    return os;
}
