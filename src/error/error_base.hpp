#pragma once

#include <expected>
#include <format>
#include <optional>

namespace error {

/// Inheritable error base class.
/// Allows dynamically returning a derived error type.
struct Error {
    constexpr Error() noexcept = default;
    constexpr Error(const Error& error) noexcept = default;
    constexpr Error(Error&& error) noexcept = default;
    virtual constexpr ~Error() noexcept = default;

    constexpr auto operator=(const Error& other) noexcept -> Error& = default;
    constexpr auto operator=(Error&& other) noexcept -> Error& = default;

    constexpr auto operator==(const Error& other) const noexcept -> bool = default;

    virtual constexpr auto source() const noexcept
        -> std::optional<std::reference_wrapper<const Error>> {
        return std::nullopt;
    }
};

/// Constraint for error types.
/// This doesn't require that Self derives from Error. However this means that other errors that wrap
/// Self won't be able to return Self as source.
template<typename Self>
concept ErrorInterface = requires(Self self) {
    { self.source() } -> std::same_as<std::optional<std::reference_wrapper<const Error>>>;
};

template<typename Self>
concept ErrorType = std::formattable<Self, char> && ErrorInterface<Self>;

}
