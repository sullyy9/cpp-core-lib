#include <iostream>

#include "panic.hpp"

namespace {
auto out_stream = std::optional<std::ostream_iterator<char>>{};
}

constexpr auto panic_impl::set_output_stream(const std::ostream_iterator<char>& output) noexcept
    -> void {
    out_stream = output;
}

auto panic_impl::get_output_stream() noexcept -> std::ostream_iterator<char> {
    if (out_stream) {
        return out_stream.value();
    }

    return std::ostream_iterator<char>{std::cerr};
}
