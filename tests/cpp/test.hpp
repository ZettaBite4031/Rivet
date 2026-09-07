#pragma once

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace Rivet::Tests {
    class TestFailure final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    inline void Require(bool condition, std::string_view expression, const std::source_location location) {
        if (condition) return;

        std::ostringstream message;
        message << location.file_name() << ':' << location.line()
            << ": requirement failed: " << expression;

        throw TestFailure(message.str());
    }

    template<typename Actual, typename Expected>
    void RequireEqual(const Actual& actual, const Expected& expected, std::string_view actual_expression, std::string_view expected_expression, const std::source_location location) {
        if (actual == expected) return;

        std::ostringstream message;
        message << location.file_name() << ':' << location.line()
            << ": expected " << actual_expression << " == " << expected_expression
            << "\n  actual:   " << actual << "\n  expected: " << expected;

        throw TestFailure(message.str());
    }

    struct TestCase {
        std::string_view Name;
        void (*Body)();
    };

    template<std::size_t Count>
    [[nodiscard]]
    bool RunSuite(std::string_view suite_name, const std::array<TestCase, Count>& tests) {
        std::size_t failures = 0;

        std::cout << suite_name << '\n';

        for (const TestCase& test : tests) {
            std::cout << "  [ RUN  ] " << test.Name << '\n';

            try {
                test.Body();
                std::cout << "  [ PASS ] " << test.Name << '\n';
            } catch (const std::exception& exception) {
                failures++;

                std::cerr << "  [ FAIL ] " << test.Name << '\n' << "           " << exception.what() << '\n';
            } catch (...) {
                failures++;

                std::cerr << "  [ FAIL ] " << test.Name << "\n           unknown exception\n";
            }
        }

        std::cout << "  " << tests.size() << " tests, " << failures << " failures\n";
        return failures == 0;
    }
}

#define REQUIRE(expression)                                                   \
    ::Rivet::Tests::Require(                                                  \
        static_cast<bool>(expression),                                        \
        #expression,                                                          \
        std::source_location::current()                                       \
    )

#define REQUIRE_EQ(actual, expected)                                          \
    ::Rivet::Tests::RequireEqual(                                             \
        (actual),                                                             \
        (expected),                                                           \
        #actual,                                                              \
        #expected,                                                            \
        std::source_location::current()                                       \
    )
