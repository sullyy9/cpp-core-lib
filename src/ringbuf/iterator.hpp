#pragma once

#include <cstddef>
#include <format>
#include <span>

namespace core::ringbuf {

template<typename T>
struct Iterator;

struct Sentinel;

////////////////////////////////////////////////////////////////

template<typename T>
constexpr auto operator+(const Iterator<T>& lhs, typename Iterator<T>::difference_type rhs) noexcept
    -> Iterator<T>;

template<typename T>
constexpr auto operator+(typename Iterator<T>::difference_type lhs, const Iterator<T>& rhs) noexcept
    -> Iterator<T>;

template<typename T>
constexpr auto operator-(const Iterator<T>& lhs, typename Iterator<T>::difference_type rhs) noexcept
    -> Iterator<T>;

////////////////////////////////////////////////////////////////

/// Aim to have all arithmetic valid. i.e. the iterator can be moved out of bounds.
/// But check dereferences.
///
/// Comparing iterators from different containers is undefined.
template<typename T>
struct Iterator {
    using difference_type = ssize_t;
    using value_type = T;

    explicit constexpr Iterator() noexcept = default;

    constexpr auto operator==(Sentinel other) const noexcept -> bool;
    constexpr auto operator==(const Iterator& other) const noexcept -> bool;

    constexpr auto operator<=>(const Iterator& other) const noexcept -> std::strong_ordering;

    constexpr auto operator*() const noexcept -> value_type&;

    constexpr auto operator++() noexcept -> Iterator&;
    constexpr auto operator++(int) noexcept -> Iterator;

    constexpr auto operator--() noexcept -> Iterator&;
    constexpr auto operator--(int) noexcept -> Iterator;

    constexpr auto operator-(Sentinel other) const noexcept -> difference_type;
    constexpr auto operator-(const Iterator& other) const noexcept -> difference_type;

    constexpr auto operator+=(difference_type other) noexcept -> Iterator&;
    constexpr auto operator-=(difference_type other) noexcept -> Iterator&;

    constexpr auto operator[](size_t index) const noexcept -> value_type&;

    constexpr Iterator(std::span<T> data, size_t ptr, size_t cycle) noexcept;

private:
    std::span<T> data{};
    size_t ptr{};
    ssize_t cycle{};

    friend struct Sentinel;

    template<typename U, size_t C>
    friend struct RingBuffer;

    friend struct std::formatter<core::ringbuf::Iterator<T>, char>;

    template<typename U>
    friend constexpr auto operator+(const Iterator<U>& lhs,
                                    typename Iterator<U>::difference_type rhs) noexcept
        -> Iterator<U>;

    template<typename U>
    friend constexpr auto operator+(typename Iterator<U>::difference_type lhs,
                                    const Iterator<U>& rhs) noexcept -> Iterator<U>;

    template<typename U>
    friend constexpr auto operator-(const Iterator<U>& lhs,
                                    typename Iterator<U>::difference_type rhs) noexcept
        -> Iterator<U>;
};

////////////////////////////////////////////////////////////////

struct Sentinel {
    using difference_type = Iterator<int>::difference_type;

    explicit constexpr Sentinel() noexcept = default;

    template<typename T>
    constexpr auto operator-(const Iterator<T>& other) const noexcept -> difference_type;

private:
    explicit constexpr Sentinel(size_t ptr, size_t cycle) noexcept;

    size_t ptr{};
    ssize_t cycle{};

    template<typename T>
    friend struct Iterator;

    template<typename T, size_t C>
    friend struct RingBuffer;

    friend struct std::formatter<core::ringbuf::Sentinel, char>;
};

static_assert(std::sized_sentinel_for<Sentinel, Iterator<int>>);
static_assert(std::random_access_iterator<Iterator<int>>);

////////////////////////////////////////////////////////////////
// implementation
////////////////////////////////////////////////////////////////

template<typename T>
constexpr Iterator<T>::Iterator(const std::span<T> data,
                                const size_t ptr,
                                const size_t cycle) noexcept :
    data{data},
    ptr{ptr},
    cycle{static_cast<ssize_t>(cycle)} {}

constexpr Sentinel::Sentinel(const size_t ptr, const size_t cycle) noexcept :
    ptr{ptr},
    cycle{static_cast<ssize_t>(cycle)} {}

template<typename T>
constexpr auto Iterator<T>::operator==(const Sentinel other) const noexcept -> bool {
    return this->cycle == other.cycle && this->ptr == other.ptr;
}

template<typename T>
constexpr auto Iterator<T>::operator==(const Iterator& other) const noexcept -> bool {
    return this->cycle == other.cycle && this->ptr == other.ptr;
}

template<typename T>
constexpr auto Iterator<T>::operator<=>(const Iterator& other) const noexcept
    -> std::strong_ordering {
    if (this->cycle == other.cycle) {
        return this->ptr <=> other.ptr;
    }

    return this->cycle <=> other.cycle;
}

template<typename T>
constexpr auto Iterator<T>::operator*() const noexcept -> value_type& {
    return this->data[this->ptr];
}

template<typename T>
constexpr auto Iterator<T>::operator++() noexcept -> Iterator& {
    if (this->ptr >= (this->data.size() - 1)) {
        this->ptr = 0;
        this->cycle++;
    } else {
        this->ptr++;
    }

    return *this;
}

template<typename T>
constexpr auto Iterator<T>::operator++(int) noexcept -> Iterator {
    const auto value = *this;

    if (this->ptr >= (this->data.size() - 1)) {
        this->ptr = 0;
        this->cycle++;
    } else {
        this->ptr++;
    }

    return value;
}

template<typename T>
constexpr auto Iterator<T>::operator--() noexcept -> Iterator& {
    if (this->ptr == 0) {
        this->ptr = this->data.size() - 1;
        this->cycle--;
    } else {
        this->ptr--;
    }

    return *this;
}

template<typename T>
constexpr auto Iterator<T>::operator--(int) noexcept -> Iterator {
    const auto value = *this;

    if (this->ptr == 0) {
        this->ptr = this->data.size() - 1;
        this->cycle--;
    } else {
        this->ptr--;
    }

    return value;
}

template<typename T>
constexpr auto Iterator<T>::operator-(const Sentinel other) const noexcept -> difference_type {
    const auto this_ptr = static_cast<ssize_t>(this->ptr);
    const auto other_ptr = static_cast<ssize_t>(other.ptr);
    const auto data_size = static_cast<ssize_t>(this->data.size());

    // We can assume the sentinel corresponds to the same container as this.
    // Anything else is undefined according to the standard.
    // So there's no issue using this' data size for both.
    // TODO: can this be enforced through the type system?
    const auto this_pos = this_ptr + (this->cycle * data_size);
    const auto other_pos = other_ptr + (other.cycle * data_size);

    return this_pos - other_pos;
}

template<typename T>
constexpr auto Iterator<T>::operator-(const Iterator& other) const noexcept -> difference_type {
    const auto this_ptr = static_cast<ssize_t>(this->ptr);
    const auto other_ptr = static_cast<ssize_t>(other.ptr);
    const auto this_size = static_cast<ssize_t>(this->data.size());
    const auto other_size = static_cast<ssize_t>(other.data.size());

    const auto this_pos = this_ptr + (this->cycle * this_size);
    const auto other_pos = other_ptr + (other.cycle * other_size);

    return this_pos - other_pos;
}

template<typename T>
constexpr auto Iterator<T>::operator+=(const difference_type other) noexcept -> Iterator& {
    const auto old_ptr = static_cast<ssize_t>(this->ptr);
    const auto data_size = static_cast<ssize_t>(this->data.size());

    auto new_ptr = (old_ptr + other);

    // Check upper bound and wrap if required.
    if (new_ptr >= data_size) {
        const auto result = std::div(new_ptr, data_size);
        this->cycle += result.quot;
        new_ptr = result.rem;
    }

    // Check upper bound and wrap if required.
    if (new_ptr < 0) {
        const auto result = std::div(new_ptr, data_size);
        this->cycle += result.quot;
        new_ptr = data_size + result.rem;
    }

    this->ptr = static_cast<size_t>(new_ptr);
    return *this;
}

template<typename T>
constexpr auto Iterator<T>::operator-=(const difference_type other) noexcept -> Iterator& {
    const auto old_ptr = static_cast<ssize_t>(this->ptr);
    const auto data_size = static_cast<ssize_t>(this->data.size());

    auto new_ptr = old_ptr - other;

    // Check upper bound and wrap if required.
    if (new_ptr >= data_size) {
        const auto result = std::div(new_ptr, data_size);
        this->cycle += result.quot;
        new_ptr = result.rem;
    }

    // Check upper bound and wrap if required.
    if (new_ptr < 0) {
        const auto result = std::div(new_ptr, data_size);
        this->cycle += result.quot;
        new_ptr = data_size + result.rem;
    }

    this->ptr = static_cast<size_t>(new_ptr);
    return *this;
}

template<typename T>
constexpr auto Iterator<T>::operator[](const size_t index) const noexcept -> value_type& {
    const auto index_adj = (this->ptr + index) % this->data.size();
    return this->data[index_adj];
}

template<typename T>
constexpr auto operator+(const Iterator<T>& lhs,
                         const typename Iterator<T>::difference_type rhs) noexcept -> Iterator<T> {
    const auto lhs_ptr = static_cast<ssize_t>(lhs.ptr);
    const auto data_size = static_cast<ssize_t>(lhs.data.size());

    auto ptr = lhs_ptr + rhs;
    auto cycle = lhs.cycle;

    // Check upper bound and wrap if required.
    if (ptr >= data_size) {
        const auto result = std::div(ptr, data_size);
        cycle += result.quot;
        ptr = result.rem;
    }

    // Check upper bound and wrap if required.
    if (ptr < 0) {
        const auto result = std::div(ptr, data_size);
        cycle += result.quot;
        ptr = data_size + result.rem;
    }

    return Iterator<T>(lhs.data, static_cast<size_t>(ptr), static_cast<size_t>(cycle));
}

template<typename T>
constexpr auto operator+(const typename Iterator<T>::difference_type lhs,
                         const Iterator<T>& rhs) noexcept -> Iterator<T> {
    return rhs + lhs;
}

template<typename T>
constexpr auto operator-(const Iterator<T>& lhs,
                         const typename Iterator<T>::difference_type rhs) noexcept -> Iterator<T> {
    const auto lhs_ptr = static_cast<ssize_t>(lhs.ptr);
    const auto data_size = static_cast<ssize_t>(lhs.data.size());

    auto ptr = lhs_ptr - rhs;
    auto cycle = lhs.cycle;

    // Check upper bound and wrap if required.
    if (ptr >= data_size) {
        const auto result = std::div(ptr, data_size);
        cycle += result.quot;
        ptr = result.rem;
    }

    // Check upper bound and wrap if required.
    if (ptr < 0) {
        const auto result = std::div(ptr, data_size);
        cycle += result.quot;
        ptr = data_size + result.rem;
    }

    return Iterator<T>(lhs.data, static_cast<size_t>(ptr), static_cast<size_t>(cycle));
}

template<typename T>
constexpr auto Sentinel::operator-(const Iterator<T>& other) const noexcept -> difference_type {
    const auto this_pos = (ssize_t)this->ptr + (this->cycle * (ssize_t)other.data.size());
    const auto other_pos = (ssize_t)other.ptr + (other.cycle * (ssize_t)other.data.size());

    return this_pos - other_pos;
}

}

////////////////////////////////////////////////////////////////

namespace std {

template<typename T>
struct formatter<core::ringbuf::Iterator<T>, char> {
    bool debug = false;

    template<class ParseContext>
    constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
        auto iter = ctx.begin();

        if (iter == ctx.end() || *iter == '}') {
            if (!this->debug) {
                throw std::format_error("Only debug formatting is supported");
            }

            return iter;
        }

        if (*iter++ == '?') {
            this->debug = true;
        }

        return iter;
    }

    template<class FmtContext>
    constexpr auto format(const core::ringbuf::Iterator<T>& iter, FmtContext& ctx) const
        -> FmtContext::iterator {
        std::format_to(ctx.out(), "Iterator {{ptr: {}, cycle {}, data: [", iter.ptr, iter.cycle);

        for (auto& val : iter.data.first(iter.data.size() - 1)) {
            std::format_to(ctx.out(), "{}, ", val);
        }

        std::format_to(ctx.out(), "{}]}}", iter.data.back());
        return ctx.out();
    }
};

template<>
struct formatter<core::ringbuf::Sentinel, char> {
    bool debug = false;

    template<class ParseContext>
    constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator {
        auto iter = ctx.begin();

        if (iter == ctx.end() || *iter == '}') {
            if (!this->debug) {
                throw std::format_error("Only debug formatting is supported");
            }

            return iter;
        }

        if (*iter++ == '?') {
            this->debug = true;
        }

        return iter;
    }

    template<class FmtContext>
    constexpr auto format(const core::ringbuf::Sentinel& iter, FmtContext& ctx) const
        -> FmtContext::iterator {
        std::format_to(ctx.out(), "Sentinel {{ptr: {}, cycle {}}}", iter.ptr, iter.cycle);
        return ctx.out();
    }
};

}

////////////////////////////////////////////////////////////////
