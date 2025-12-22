#pragma once

#include <concepts>
#include <functional>
#include <ostream>

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
