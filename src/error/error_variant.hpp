#pragma once

#include "error_base.hpp"

namespace error {

/// Requires a type T to be any one of the type contained in the parameter pack Ts.
template<typename T, typename... Ts>
concept AnyOf = (std::same_as<T, Ts> || ...);

/// Wrapper over std::variant for types implementing ErrorType.
template<ErrorType... Es>
struct Variant: Error {
    friend struct std::formatter<Variant>;

private:
    std::variant<Es...> inner{};

public:
    constexpr Variant(const AnyOf<Es...> auto& error) noexcept : inner{error} {}

    constexpr auto operator==(const AnyOf<Es...> auto& other) const noexcept -> bool {
        if (!this->is<typename std::decay<decltype(other)>::type>()) {
            return false;
        }

        return std::get<typename std::decay<decltype(other)>::type>(this->inner) == other;
    }

    /// @brief Return the source of the error if any.
    ///
    /// @return The source of the error or nullopt.
    constexpr auto source() const noexcept
        -> std::optional<std::reference_wrapper<const Error>> override {
        return std::visit([](const auto& error) { return error.source(); }, this->inner);
    }

    /// @brief Get a reference to the inner error variant of the given type.
    ///
    /// @return The variant if the specified type is held. Returns nullopt otherwise.
    template<AnyOf<Es...> E>
    constexpr auto get() const noexcept -> std::optional<std::reference_wrapper<const E>> {
        if (!std::holds_alternative<E>(this->inner)) {
            return std::nullopt;
        }

        return std::get<E>(this->inner);
    }

    /// @brief Get a mutable reference to the inner error variant of the given type.
    ///
    /// @return The variant if the specified type is held. Returns nullopt otherwise.
    template<AnyOf<Es...> E>
    constexpr auto get_mut() noexcept -> std::optional<std::reference_wrapper<E>> {
        if (!std::holds_alternative<E>(this->inner)) {
            return std::nullopt;
        }

        return std::get<E>(this->inner);
    }

    /// @brief Determine if the error is the given variant.
    template<AnyOf<Es...> E>
    constexpr auto is() const noexcept -> bool {
        return std::holds_alternative<E>(this->inner);
    }

    /// @brief Call the provided invokable on the error variant.
    template<typename Visitor>
        requires(std::invocable<Visitor, Es> && ...)
    constexpr auto visit(Visitor&& visitor) const noexcept -> decltype(auto) {
        return std::visit(std::forward<decltype(visitor)>(visitor), this->inner);
    }

    /// @brief Call the provided invokable on the error variant.
    template<typename Visitor>
        requires(std::invocable<Visitor, Es> && ...)
    constexpr auto visit_mut(Visitor&& visitor) noexcept -> decltype(auto) {
        return std::visit(std::forward<decltype(visitor)>(visitor), this->inner);
    }
};

template<typename T>
concept VariantDerivative = requires() {
    typename T::Variant;
    requires std::derived_from<T, typename T::Variant>;
};

}

namespace std {

template<typename... Ts>
struct formatter<error::Variant<Ts...>, char> {
    template<class ParseContext>
    constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<class FmtContext>
    constexpr auto format(error::Variant<Ts...> error, FmtContext& ctx) const
        -> FmtContext::iterator {
        std::visit([&](auto& inner) { std::format_to(ctx.out(), "{}", inner); }, error.inner);
        return ctx.out();
    }
};

template<error::VariantDerivative T>
struct formatter<T, char> {
    template<class ParseContext>
    constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
        return ctx.begin();
    }

    template<class FmtContext>
    constexpr auto format(T error, FmtContext& ctx) const -> FmtContext::iterator {
        std::visit([&](auto& inner) { std::format_to(ctx.out(), "{}", inner); }, error.inner);
        return ctx.out();
    }
};

}
