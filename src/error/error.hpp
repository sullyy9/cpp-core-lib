#pragma once

#include "error_base.hpp"
#include "error_variant.hpp"

/// @brief Derive a specialisation of std::formatter for an error type.
/// 
/// @param ERROR  Error type
/// @param FORMAT Format specifier. Must be a string literal.
/// @param ...    Format args. The error object can be accessed as `self`.
#define ERROR_DERIVE_FMT(ERROR, FORMAT, ...)                                                                  \
namespace std {                                                                                               \
template<>                                                                                                    \
struct formatter<ERROR, char> {                                                                               \
                                                                                                              \
    template<class ParseContext>                                                                              \
    constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {                                       \
        return ctx.begin();                                                                                   \
    }                                                                                                         \
                                                                                                              \
    template<class FmtContext>                                                                                \
    constexpr auto format([[maybe_unused]]const ERROR& self, FmtContext& ctx) const -> FmtContext::iterator { \
        std::format_to(ctx.out(), FORMAT __VA_OPT__(,) __VA_ARGS__);                                          \
        return ctx.out();                                                                                     \
    }                                                                                                         \
};                                                                                                            \
}
