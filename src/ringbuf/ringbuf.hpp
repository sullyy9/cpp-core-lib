#pragma once

#include <array>
#include <cstddef>
#include <expected>
#include <span>

#include "error.hpp"
#include "iterator.hpp"

namespace core::ringbuf {

template<typename T>
struct Iterator;

struct Sentinel;

}

////////////////////////////////////////////////////////////////
// error handling.
////////////////////////////////////////////////////////////////

namespace core::ringbuf::error {
struct Full: ::error::Error {};
struct Empty: ::error::Error {};
}

ERROR_DERIVE_FMT(core::ringbuf::error::Full, "Buffer full");
ERROR_DERIVE_FMT(core::ringbuf::error::Empty, "Buffer empty");

static_assert(error::ErrorType<core::ringbuf::error::Full>);
static_assert(error::ErrorType<core::ringbuf::error::Empty>);

namespace core::ringbuf {

struct Error: ::error::Variant<error::Full, error::Empty> {
    using Full = error::Full;
    using Empty = error::Empty;

    using Variant = ::error::Variant<error::Full, error::Empty>;
    using Variant::Variant;
};

static_assert(::error::ErrorType<Error>);

}

/*------------------------------------------------------------------------------------------------*/
// Error handling.
/*------------------------------------------------------------------------------------------------*/

namespace core::ringbuf {

template<typename T, size_t Capacity>
struct RingBuffer {
    constexpr auto begin() noexcept -> Iterator<T>;
    constexpr auto end() const noexcept -> Sentinel;

    auto push(T value) noexcept -> std::expected<void, Error>;
    auto push_unchecked(T value) noexcept -> void;

    auto push_buffer(std::span<const T> buffer) noexcept -> std::expected<void, Error>;

    auto pop() noexcept -> std::expected<T, Error>;
    auto pop_unchecked() noexcept -> T;

    auto pop_buffer(std::span<T> buffer) noexcept -> std::expected<void, Error>;

    auto clear() noexcept -> void;

    auto empty() const noexcept -> bool;
    auto full() const noexcept -> bool;

    auto size() const noexcept -> size_t;
    auto free() const noexcept -> size_t;
    auto capacity() const noexcept -> size_t;

private:
    std::array<T, Capacity> _buffer{};
    size_t _write_ptr{};
    size_t _read_ptr{};
    bool _is_full{};

    friend struct Iterator<T>;
    friend struct Sentinel;
};

static_assert(std::ranges::range<RingBuffer<int, 8>>);
static_assert(std::ranges::random_access_range<RingBuffer<int, 8>>);
static_assert(std::ranges::sized_range<RingBuffer<int, 8>>);

/*------------------------------------------------------------------------------------------------*/
// Class method definitions.
/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
constexpr auto RingBuffer<T, Capacity>::begin() noexcept -> Iterator<T> {
    return Iterator<T>(std::span<T, std::dynamic_extent>{this->_buffer}, this->_read_ptr, 0);
}

template<typename T, size_t Capacity>
constexpr auto RingBuffer<T, Capacity>::end() const noexcept -> Sentinel {
    if (this->_write_ptr < this->_read_ptr || this->full()) {
        return Sentinel(this->_write_ptr, 1);
    }

    return Sentinel(this->_write_ptr, 0);
}

////////////////////////////////////////////////////////////////

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::push(const T value) noexcept -> std::expected<void, Error> {
    if (this->_is_full) {
        return std::unexpected{Error::Full()};
    }

    this->_buffer[this->_write_ptr++] = value;
    this->_write_ptr = this->_write_ptr % Capacity;

    if (this->_write_ptr == this->_read_ptr) {
        this->_is_full = true;
    }

    return {};
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::push_unchecked(const T value) noexcept -> void {
    this->_buffer[this->_write_ptr++] = value;
    this->_write_ptr = this->_write_ptr % Capacity;

    if (this->_write_ptr == this->_read_ptr) {
        this->_is_full = true;
    }
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::push_buffer(const std::span<const T> buffer) noexcept
    -> std::expected<void, Error> {
    if (buffer.size() > this->free()) {
        if (this->_is_full) {
            return std::unexpected{Error::Full()};
        }
        return std::unexpected{Error::Full()};
    }

    const auto space_until_wrap = Capacity - this->_write_ptr;

    if (buffer.size() > space_until_wrap) {
        const auto chunk1 = buffer.first(space_until_wrap);
        const auto chunk2 = buffer.last(buffer.size() - space_until_wrap);

        std::copy(chunk1.begin(), chunk1.end(), std::next(this->_buffer.begin(), this->_write_ptr));
        std::copy(chunk2.begin(), chunk2.end(), this->_buffer.begin());

    } else {
        std::copy(buffer.begin(), buffer.end(), std::next(this->_buffer.begin(), this->_write_ptr));
    }

    this->_write_ptr = (this->_write_ptr + buffer.size()) % Capacity;

    if (this->_write_ptr == this->_read_ptr) {
        this->_is_full = true;
    }

    return {};
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::pop() noexcept -> std::expected<T, Error> {
    if (this->empty()) {
        return std::unexpected{Error::Empty()};
    }

    const auto value = this->_buffer[this->_read_ptr++];
    this->_read_ptr = this->_read_ptr % Capacity;
    this->_is_full = false;

    return value;
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::pop_unchecked() noexcept -> T {
    const auto value = this->_buffer[this->_read_ptr++];
    this->_read_ptr = this->_read_ptr % Capacity;
    this->_is_full = false;

    return value;
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::pop_buffer(const std::span<T> buffer) noexcept
    -> std::expected<void, Error> {
    if (buffer.size() > this->size()) {
        if (this->empty()) {
            return std::unexpected{Error::Empty()};
        }
        return std::unexpected{Error::Empty()};
    }

    const auto items_until_wrap = Capacity - this->_read_ptr;

    if (buffer.size() > items_until_wrap) {
        const auto chunk1 = std::span(this->_buffer).last(items_until_wrap);
        const auto chunk2 = std::span(this->_buffer).first(buffer.size() - items_until_wrap);

        std::copy(chunk1.begin(), chunk1.end(), buffer.begin());
        std::copy(chunk2.begin(), chunk2.end(), std::next(buffer.begin(), items_until_wrap));

    } else {
        const auto begin = std::next(this->_buffer.begin(), this->_read_ptr);
        const auto end = std::next(begin, items_until_wrap);

        std::copy(begin, end, buffer.begin());
    }

    this->_read_ptr = (this->_read_ptr + buffer.size()) % Capacity;
    this->_is_full = false;

    return {};
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::clear() noexcept -> void {
    this->_write_ptr = 0;
    this->_read_ptr = 0;
    this->_is_full = false;
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::empty() const noexcept -> bool {
    return this->_write_ptr == this->_read_ptr && !this->_is_full;
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::full() const noexcept -> bool {
    return this->_is_full;
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::size() const noexcept -> size_t {
    if (this->_is_full) {
        return Capacity;
    }

    if (this->_write_ptr >= this->_read_ptr) {
        return this->_write_ptr - this->_read_ptr;
    }

    return this->_write_ptr + (Capacity - this->_read_ptr);
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
[[nodiscard]] auto RingBuffer<T, Capacity>::free() const noexcept -> size_t {
    if (this->_is_full) {
        return 0;
    }

    if (this->_write_ptr >= this->_read_ptr) {
        return (Capacity - this->_write_ptr) + this->_read_ptr;
    }

    return this->_read_ptr - this->_write_ptr;
}

/*------------------------------------------------------------------------------------------------*/

template<typename T, size_t Capacity>
auto RingBuffer<T, Capacity>::capacity() const noexcept -> size_t {
    return Capacity;
}

}

/*------------------------------------------------------------------------------------------------*/
