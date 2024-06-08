/// Tests for RingBuffer iterators.

#include <random>
#include <ranges>

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include "ringbuf.hpp"

////////////////////////////////////////////////////////////////

template<typename T, size_t Capacity>
using RingBuffer = core::ringbuf::RingBuffer<T, Capacity>;

using Error = core::ringbuf::Error;

////////////////////////////////////////////////////////////////

namespace Catch {

template<typename T>
struct StringMaker<core::ringbuf::Iterator<T>> {
    static auto convert(const core::ringbuf::Iterator<T>& iter) -> std::string {
        return std::format("{:?}", iter);
    }
};

template<>
struct StringMaker<core::ringbuf::Sentinel> {
    static auto convert(const core::ringbuf::Sentinel& iter) -> std::string {
        return std::format("{:?}", iter);
    }
};

}

////////////////////////////////////////////////////////////////

/// Construct an empty RingBuffer but randomise the starting position of it's internal pointers.
template<size_t Capacity>
constexpr auto empty_ringbuf_randomised() -> RingBuffer<uint32_t, Capacity> {
    auto buf = RingBuffer<uint32_t, Capacity>{};

    // Randomise the starting position of internal pointers by writing and then reading soe data.
    std::random_device dev;
    std::mt19937 rng(dev());
    auto dist = std::uniform_int_distribution<std::mt19937::result_type>(0, Capacity * 2);

    for ([[maybe_unused]] auto i : std::views::iota(0u, dist(rng))) {
        [[maybe_unused]] auto _1 = buf.push(0);
        [[maybe_unused]] auto _2 = buf.pop();
    }

    return buf;
}

////////////////////////////////////////////////////////////////

template<size_t C>
constexpr auto fill_ringbuf_by_index(RingBuffer<uint32_t, C>& buf) -> void {
    for ([[maybe_unused]] auto i : std::views::iota(0u, C)) {
        [[maybe_unused]] auto _1 = buf.push(i);
    }
}

////////////////////////////////////////////////////////////////

SCENARIO("RingBuffer iterator addition") {
    GIVEN("An iterator to the beginning of a full RingBuffer") {
        constexpr auto CAPACITY = size_t{64};
        auto buf = empty_ringbuf_randomised<CAPACITY>();
        fill_ringbuf_by_index(buf);
        REQUIRE(buf.full());

        auto iter = buf.begin();

        GIVEN("An index") {
            auto index = GENERATE(ssize_t{0}, CAPACITY / 4, CAPACITY / 2, CAPACITY - 1);

            WHEN("The iterator is post-incremented to the index") {
                for ([[maybe_unused]] auto i : std::views::iota(ssize_t{0}, index)) {
                    iter++;
                }

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == index);
                }
            }

            WHEN("The iterator is pre-incremented to the index") {
                for ([[maybe_unused]] auto i : std::views::iota(ssize_t{0}, index)) {
                    ++iter;
                }

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == index);
                }
            }

            WHEN("The index is add assigned to the iterator") {
                iter += index;

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == index);
                }
            }

            WHEN("The index (rhs) is added to the iterator (lhs)") {
                iter = iter + index;

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == index);
                }
            }

            WHEN("The index (lhs) is added to the iterator (rhs)") {
                iter = index + iter;

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == index);
                }
            }
        }
    }
}

SCENARIO("RingBuffer iterator subtraction") {
    GIVEN("An iterator to the last element of a full RingBuffer") {
        constexpr auto CAPACITY = size_t{64};
        constexpr auto LAST = CAPACITY - 1;
        auto buf = empty_ringbuf_randomised<CAPACITY>();
        fill_ringbuf_by_index(buf);
        REQUIRE(buf.full());

        auto iter = buf.begin() + LAST;
        REQUIRE(*iter == LAST);

        GIVEN("An offset") {
            auto offset = GENERATE(ssize_t{0}, CAPACITY / 4, CAPACITY / 2, auto{LAST});

            WHEN("The iterator is post-decremented a number of times equal to the offset") {
                for ([[maybe_unused]] auto i : std::views::iota(ssize_t{0}, offset)) {
                    iter--;
                }

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == (ssize_t{LAST} - offset));
                }

                THEN("The distance to the sentinel should equal the offset + 1") {
                    REQUIRE((iter - buf.end()) == -(offset + 1));
                    REQUIRE((buf.end() - iter) == (offset + 1));
                }

                THEN(
                    "The distance to the begin iterator should be the index of the last element minus the offset") {
                    REQUIRE((iter - buf.begin()) == ssize_t{LAST} - offset);
                }
            }

            WHEN("The iterator is pre-decremented a number of times equal to the offset") {
                for ([[maybe_unused]] auto i : std::views::iota(ssize_t{0}, offset)) {
                    --iter;
                }

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == (ssize_t{LAST} - offset));
                }
            }

            WHEN("The offset is subtract assigned to the iterator") {
                iter -= offset;

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == (ssize_t{LAST} - offset));
                }
            }

            WHEN("The offset (rhs) is subtracted from the iterator (lhs)") {
                iter = iter - offset;

                THEN("Dereferencing should return the correct value") {
                    REQUIRE(*iter == (ssize_t{LAST} - offset));
                }
            }
        }
    }
}

SCENARIO("RingBuffer iterator arithmetic") {
    GIVEN("A RingBuffer with a single element") {
        constexpr auto CAPACITY = 64;
        auto buf = empty_ringbuf_randomised<CAPACITY>();
        REQUIRE(buf.empty());

        const auto value = 5;
        REQUIRE(buf.push(5));

        GIVEN("The begin iterator is obtained") {
            auto iter = buf.begin();

            THEN("Dereferencing it should return the inserted value") {
                REQUIRE(*iter == value);
            }

            WHEN("The iterator is incremented") {
                iter++;

                THEN("It should equal the end iterator") {
                    REQUIRE(iter == buf.end());
                }
            }
        }
    }

    GIVEN("A full RingBuffer") {
        constexpr auto CAPACITY = size_t{64};
        auto buf = empty_ringbuf_randomised<CAPACITY>();
        fill_ringbuf_by_index(buf);
        REQUIRE(buf.full());

        GIVEN("The begin iterator is obtained") {
            auto iter = buf.begin();
            auto index = GENERATE(range(size_t{0}, auto{CAPACITY}));

            THEN("Indexing should return the correct value") {
                REQUIRE(iter[index] == index);
            }
        }
    }
}

SCENARIO("RingBuffer iterator equality") {
    GIVEN("An empty RingBuffer") {
        constexpr auto CAPACITY = 64;
        auto buf = empty_ringbuf_randomised<CAPACITY>();
        REQUIRE(buf.empty());

        THEN("The begin and end iterators should be equal") {
            REQUIRE(buf.begin() == buf.end());
        }

        WHEN("An element is inserted") {
            REQUIRE(buf.push(5));

            THEN("The begin iterator should not equal the end iterator") {
                REQUIRE(buf.begin() != buf.end());
            }
        }
    }

    GIVEN("A full RingBuffer") {
        constexpr auto CAPACITY = size_t{64};
        auto buf = empty_ringbuf_randomised<CAPACITY>();
        fill_ringbuf_by_index(buf);
        REQUIRE(buf.full());

        THEN("The begin and end iterators should not be equal") {
            REQUIRE(buf.begin() != buf.end());
        }

        THEN("Two begin iterators should be equal") {
            REQUIRE(buf.begin() == buf.begin());
        }

        GIVEN("The begin iterator is obtained") {
            auto iter = buf.begin();

            WHEN("The iterator is advanced to the end") {
                iter += CAPACITY;

                THEN("The iterator should equal the end iterator") {
                    REQUIRE(iter == buf.end());
                }
            }

            WHEN("The iterator is incremented") {
                auto index = GENERATE(ssize_t{1}, CAPACITY / 4, CAPACITY / 2, auto{CAPACITY});
                iter += index;

                THEN("The iterator should not equal the begin iterator") {
                    REQUIRE(iter != buf.begin());
                }

                THEN("The iterator should be greater than the begin iterator") {
                    REQUIRE(iter > buf.begin());
                }

                THEN("The begin iterator should be less than the iterator") {
                    REQUIRE(buf.begin() < iter);
                }
            }
        }
    }
}
