#include <format>
#include <iterator>
#include <source_location>

namespace panic_impl {

enum class Behaviour { Terminate, Halt };

#if defined(PANIC_BEHAVIOUR_TERMINATE)
constexpr auto BEHAVIOUR = Behaviour::Terminate;
#elif defined(PANIC_BEHAVIOUR_HALT)
constexpr auto BEHAVIOUR = Behaviour::Halt;
#else
constexpr auto BEHAVIOUR = Behaviour::Terminate;
#endif

template<typename... Args>
struct Format {
    template<typename T>
    consteval Format(const T& format,
                     std::source_location location = std::source_location::current()) noexcept :
        fmt{format},
        loc{location} {}

    std::format_string<Args...> fmt;
    std::source_location loc;
};

constexpr auto set_output_stream(const std::ostream_iterator<char>& output) noexcept -> void;
auto get_output_stream() noexcept -> std::ostream_iterator<char>;

};

/// @brief Print a message to the stderr stream and terminate.
///
/// The stream used to print the panic message can be set via `set_output_stream()`. Additionally
/// the termination behaviour can be selected via the `PANIC_BEHAVIOUR_*` flags at compile time.
template<typename... Args>
[[noreturn]] auto panic(panic_impl::Format<std::type_identity_t<Args>...> fmt,
                        Args&&... args) noexcept -> void {
    auto out = panic_impl::get_output_stream();

    std::format_to(out, "{}:{} panic!: ", fmt.loc.file_name(), fmt.loc.line());
    std::format_to(out, fmt.fmt, std::forward<Args>(args)...);
    std::format_to(out, "\r\n");

    using panic_impl::Behaviour;

    switch (panic_impl::BEHAVIOUR) {
        case Behaviour::Terminate: std::terminate();
        case Behaviour::Halt:
            while (true) continue;
    }
}
