#include <ranges>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>

#include "ringbuf.hpp"

template<typename T, size_t Capacity>
using RingBuffer = core::ringbuf::RingBuffer<T, Capacity>;

using Error = core::ringbuf::Error;

SCENARIO("Empty RingBuffer properties") {
    GIVEN("An empty RingBuffer") {
        constexpr auto CAPACITY = 64;
        auto buf = RingBuffer<uint8_t, CAPACITY>{};
        REQUIRE(buf.capacity() == CAPACITY);

        // Write, then read some data to the buffer in order to test it with the internal pointers
        // in different positions.
        auto read_writes_counts = GENERATE(std::vector{0},
                                           std::vector{CAPACITY / 2},
                                           std::vector{CAPACITY},
                                           std::vector{CAPACITY, CAPACITY / 2});

        for (auto count : read_writes_counts) {
            for (auto i : std::views::iota(0, count)) {
                REQUIRE(buf.push((uint8_t)i));
            }

            for ([[maybe_unused]] auto i : std::views::iota(0, count)) {
                REQUIRE(buf.pop());
            }
        }

        THEN("The value returned by size() should equal 0") {
            REQUIRE(buf.size() == 0);
        }

        THEN("The value returned by free() should equal capacity") {
            REQUIRE(buf.free() == CAPACITY);
        }

        THEN("The value of empty() should be true") {
            REQUIRE(buf.empty());
        }

        THEN("The value of full() should be false") {
            REQUIRE(!buf.full());
        }

        THEN("Calling pop() should return an error") {
            auto result = buf.pop();
            REQUIRE(!result.has_value());
            REQUIRE(result.error() == Error::Empty());
        }

        WHEN("Inserting a byte") {
            REQUIRE(buf.push(0));

            THEN("The value of empty() should be false") {
                REQUIRE(!buf.empty());
            }
        }
    }
}

SCENARIO("Full RingBuffer properties") {
    GIVEN("An empty RingBuffer") {
        constexpr auto CAPACITY = 64;
        auto buf = RingBuffer<uint8_t, CAPACITY>{};

        // Write, then read some data to the buffer in order to test it with the internal pointers
        // in different positions.
        auto read_writes_counts = GENERATE(std::vector{0},
                                           std::vector{CAPACITY / 2},
                                           std::vector{CAPACITY},
                                           std::vector{CAPACITY, CAPACITY / 2});

        for (auto count : read_writes_counts) {
            for (auto i : std::views::iota(0, count)) {
                REQUIRE(buf.push((uint8_t)i));
            }

            for ([[maybe_unused]] auto i : std::views::iota(0, count)) {
                REQUIRE(buf.pop());
            }
        }

        WHEN("The buffer is filled") {
            for (auto byte : std::views::iota(0, CAPACITY)) {
                REQUIRE(buf.push((uint8_t)byte));
            }

            THEN("The value returned by size() should equal capacity") {
                REQUIRE(buf.size() == CAPACITY);
            }

            THEN("The value returned by free() should equal 0") {
                REQUIRE(buf.free() == 0);
            }

            THEN("The value of empty() should be false") {
                REQUIRE(!buf.empty());
            }

            THEN("The value of full() should be true") {
                REQUIRE(buf.full());
            }

            WHEN("Removing a byte") {
                REQUIRE(buf.pop());

                THEN("The value of full() should be false") {
                    REQUIRE(!buf.full());
                }
            }

            THEN("Calling push() should return an error") {
                auto result = buf.push(0);
                REQUIRE(!result.has_value());
                REQUIRE(result.error() == Error::Full());
            }
        }
    }
}

SCENARIO("Data is read in the order it is written") {
    GIVEN("An empty RingBuffer") {
        constexpr auto CAPACITY = 64;
        auto buf = RingBuffer<uint8_t, CAPACITY>{};

        THEN("The buffer should be empty") {
            REQUIRE(buf.size() == 0);
            REQUIRE(buf.free() == CAPACITY);
            REQUIRE(buf.capacity() == CAPACITY);
        }

        auto write_data = GENERATE(take(1, chunk(16, random(0, 255))));

        WHEN("Data is pushed via push()") {
            for (auto byte : write_data) {
                REQUIRE(buf.push((uint8_t)byte));
            }

            THEN("The size should increase") {
                REQUIRE(buf.size() == write_data.size());
                REQUIRE(buf.free() == (CAPACITY - write_data.size()));
            }

            AND_WHEN("All data is read via pop()") {
                auto read_data = std::vector<int32_t>{};

                for ([[maybe_unused]] auto i : std::views::iota(0u, write_data.size())) {
                    const auto data = buf.pop();
                    REQUIRE(data.has_value());
                    read_data.push_back(data.value());
                }

                THEN("The data should match what was written and the buffer should be empty") {
                    REQUIRE(read_data == write_data);
                    REQUIRE(buf.size() == 0);
                    REQUIRE(buf.free() == CAPACITY);
                }
            }
        }
    }
}

TEST_CASE("Benchmarks") {
    constexpr auto CAPACITY = 64;
    auto buf = RingBuffer<uint8_t, CAPACITY>{};

    BENCHMARK("push()") {
        buf.push(0);
        return buf.pop_unchecked();
    };

    BENCHMARK("push_unchecked()") {
        buf.push_unchecked(0);
        return buf.pop_unchecked();
    };
}
