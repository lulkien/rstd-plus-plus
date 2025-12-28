#pragma once

#include <concepts>
#include <functional>
#include <ostream>

namespace rstd
{

template <typename T>
concept is_printable = requires(std::ostream &os, const T &val) { os << val; };

template <typename T>
concept is_default_constructible = std::is_default_constructible_v<T>;

template <typename T>
concept is_cloneable = requires(const T &obj) {
    { T(obj) } -> std::same_as<T>;
    requires std::is_copy_constructible_v<T>;
    requires std::is_copy_assignable_v<T>;
    requires std::is_destructible_v<T>;
};

template <typename Fn, typename... Args>
concept fn_return_boolean = requires(Fn &&fn, Args &&...args) {
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)
    } -> std::same_as<bool>;
};

template <typename Fn, typename... Args>
concept fn_return_void = requires(Fn &&fn, Args &&...args) {
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)
    } -> std::same_as<void>;
};

/**
 * @brief Empty struct to represent void/unit types
 *
 * This is used instead of void in template parameters, providing
 * a concrete type that can be instantiated and passed around.
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
    constexpr auto operator==(const Void &) const noexcept -> bool
    {
        return true;
    }

    /**
     * @brief Inequality comparison (all Void instances are equal)
     */
    constexpr auto operator!=(const Void &) const noexcept -> bool
    {
        return false;
    }
};

/**
 * @brief Stream output operator for Void
 */
inline auto operator<<(std::ostream &os, const Void &) -> std::ostream &
{
    return os << "()";
}

} // namespace rstd
