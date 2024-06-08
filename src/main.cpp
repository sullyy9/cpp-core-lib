#include <cstdint>
#include <iostream>
#include <optional>

#include "error.hpp"
#include "panic.hpp"

////////////////////////////////////////////////////////////////

struct Error1: error::Error {
    friend struct std::formatter<Error1>;
};

struct Error2: error::Error {
    friend struct std::formatter<Error2>;

    auto source() const noexcept
        -> std::optional<std::reference_wrapper<const error::Error>> override {
        return std::nullopt;
    }
};

struct Error3 {
    friend struct std::formatter<Error3>;

private:
    Error2 inner{};

public:
    explicit constexpr Error3(const Error2&& error) : inner{error} {}

    auto source() const noexcept -> std::optional<std::reference_wrapper<const error::Error>> {
        return std::optional{std::cref(this->inner)};
    }
};

struct Error4: error::Error {};

ERROR_DERIVE_FMT(Error1, "Error1")
ERROR_DERIVE_FMT(Error2, "Error2")
ERROR_DERIVE_FMT(Error3, "Error3: {}", self.inner)

struct Error: error::Variant<Error1, Error2, Error3> {
    using Error1 = Error1;
    using Error2 = Error2;
    using Error3 = Error3;

    using Variant = Variant<Error1, Error2, Error3>;
    using Variant::Variant;
};

static_assert(error::ErrorType<Error1>);
static_assert(error::ErrorType<Error2>);
static_assert(error::ErrorType<Error3>);
static_assert(error::ErrorType<Error>);

////////////////////////////////////////////////////////////////

template<typename... Ts>
struct Visit: Ts... {
    using Ts::operator()...;
};

auto ohh_no_this_might_fail_uwu() -> std::expected<int32_t, Error>;

auto ohh_no_this_might_fail_uwu() -> std::expected<int32_t, Error> {
    return std::unexpected(Error::Error3(Error::Error2()));
}

/* Type your code here, or load an example. */
int32_t main() {
    auto result = ohh_no_this_might_fail_uwu();
    auto error = result.error();

    if (auto err = error.get<Error1>(); err) {
        std::cout << std::format("We got error 1: {}", err->get()) << "\n";
    }

    if (auto err = error.get<Error2>(); err) {
        std::cout << std::format("We got error 2: {}", err->get()) << "\n";
    }

    if (auto err = error.get<Error3>(); err) {
        std::cout << std::format("We got error 3: {}", err->get()) << "\n";
    }

    error.visit_mut(Visit{
        [](const Error1& err) { std::cout << std::format("Visit error 1: {}", err) << "\n"; },
        [](const Error2& err) { std::cout << std::format("Visit error 2: {}", err) << "\n"; },
        [](const Error3& err) { std::cout << std::format("Visit error 3: {}", err) << "\n"; },
    });

    std::cout << std::format("Ohh no it failed :< : {}", error) << "\n";

    panic("Just testing da panics {}", 567);

    return 0;
}

////////////////////////////////////////////////////////////////
