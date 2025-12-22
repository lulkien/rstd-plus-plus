#pragma once

#include <concepts>
#include <functional>
#include <ostream>

namespace rstd
{

template <typename T>
concept Printable = requires(std::ostream &os, const T &val) { os << val; };

template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename Fn, typename... Args>
concept BoolReturning = requires(Fn &&fn, Args &&...args) {
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)
    } -> std::same_as<bool>;
};

template <typename Fn, typename... Args>
concept VoidReturning = requires(Fn &&fn, Args &&...args) {
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)
    } -> std::same_as<void>;
};

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
