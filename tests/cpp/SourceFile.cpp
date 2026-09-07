#include "source/Source.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {
    class TestFailure final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    void Require(bool condition, std::string_view expression, const std::source_location location) {
        if (condition) return;

        std::ostringstream message;
        message << location.file_name() << ':' << location.line() << ": requirement failed: " << expression;
        throw TestFailure(message.str());
    }

    template<typename Actual, typename Expected>
    void RequireEqual(const Actual& actual, const Expected& expected, std::string_view actual_expression, std::string_view expected_expression, const std::source_location location) {
        if (actual == expected) return;

        std::ostringstream message;
        message
            << location.file_name() << ':' << location.line()
            << ": expected " << actual_expression << " == " << expected_expression
            << "\n  actual:   " << actual << "\n  expected: " << expected;
        throw TestFailure(message.str());
    }

#define REQUIRE(expression)                                                   \
    Require(                                                                  \
        static_cast<bool>(expression),                                        \
        #expression,                                                          \
        std::source_location::current()                                       \
    )

#define REQUIRE_EQ(actual, expected)                                          \
    RequireEqual(                                                             \
        (actual),                                                             \
        (expected),                                                           \
        #actual,                                                              \
        #expected,                                                            \
        std::source_location::current()                                       \
    )

    [[nodiscard]]
    std::filesystem::path MakeTemporaryPath() {
        static std::atomic<std::uint64_t> sequence{0};

        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::uint64_t identifier = sequence.fetch_add(1, std::memory_order_relaxed);

        const std::string filename = "rivetc0-source-test-" + std::to_string(timestamp) + "-" + std::to_string(identifier) + ".rivet";
        return std::filesystem::temp_directory_path() / filename;
    }

    class TemporaryFile final {
    public:
        explicit TemporaryFile(std::string_view contents)
            : m_Path(MakeTemporaryPath()) {
            std::ofstream stream(m_Path, std::ios::binary | std::ios::trunc);

            if (!stream.is_open())
                throw std::runtime_error("could not create temporary source file");

            stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
            stream.close();
            if (!stream)
                throw std::runtime_error("could not write temporary source file");
        }

        TemporaryFile(const TemporaryFile&) = delete;
        TemporaryFile& operator=(const TemporaryFile&) = delete;

        ~TemporaryFile() {
            std::error_code error;
            std::filesystem::remove(m_Path, error);
        }

        [[nodiscard]]
        const std::filesystem::path& Path() const noexcept { return m_Path; }

    private:
        std::filesystem::path m_Path;
    };

    void TestEmptySource() {
        const Rivet::SourceFile source{"empty.rivet", ""};

        REQUIRE_EQ(source.Size(), std::size_t{0});
        REQUIRE_EQ(source.LineCount(), std::size_t{1});
        REQUIRE(source.Text().empty());
        REQUIRE(source.LineText(0).empty());

        const Rivet::SourceSpan line = source.LineSpan(0);

        REQUIRE_EQ(line.Offset, std::size_t{0});
        REQUIRE_EQ(line.Length, std::size_t{0});
        REQUIRE(line.Empty());
        REQUIRE_EQ(line.End(), std::size_t{0});

        const Rivet::SourcePosition position = source.PositionAt(0);

        REQUIRE_EQ(position.Line, std::size_t{0});
        REQUIRE_EQ(position.Column, std::size_t{0});

        const Rivet::SourceSpan eof = source.EofSpan();

        REQUIRE_EQ(eof.Offset, std::size_t{0});
        REQUIRE_EQ(eof.Length, std::size_t{0});
        REQUIRE(source.Contains(eof));
    }

    void TestLfLineTable() {
        const Rivet::SourceFile source{ "lines.rivet", "first\nsecond\nthird" };

        REQUIRE_EQ(source.Size(), std::size_t{18});
        REQUIRE_EQ(source.LineCount(), std::size_t{3});

        REQUIRE_EQ(source.LineText(0), std::string_view{"first"});
        REQUIRE_EQ(source.LineText(1), std::string_view{"second"});
        REQUIRE_EQ(source.LineText(2), std::string_view{"third"});

        const Rivet::SourceSpan first = source.LineSpan(0);
        const Rivet::SourceSpan second = source.LineSpan(1);
        const Rivet::SourceSpan third = source.LineSpan(2);

        REQUIRE_EQ(first.Offset, std::size_t{0});
        REQUIRE_EQ(first.Length, std::size_t{5});

        REQUIRE_EQ(second.Offset, std::size_t{6});
        REQUIRE_EQ(second.Length, std::size_t{6});

        REQUIRE_EQ(third.Offset, std::size_t{13});
        REQUIRE_EQ(third.Length, std::size_t{5});

        const Rivet::SourcePosition beginning = source.PositionAt(0);
        const Rivet::SourcePosition first_newline = source.PositionAt(5);
        const Rivet::SourcePosition second_line = source.PositionAt(6);
        const Rivet::SourcePosition second_newline = source.PositionAt(12);
        const Rivet::SourcePosition third_line = source.PositionAt(13);
        const Rivet::SourcePosition eof = source.PositionAt(source.Size());

        REQUIRE_EQ(beginning.Line, std::size_t{0});
        REQUIRE_EQ(beginning.Column, std::size_t{0});

        REQUIRE_EQ(first_newline.Line, std::size_t{0});
        REQUIRE_EQ(first_newline.Column, std::size_t{5});

        REQUIRE_EQ(second_line.Line, std::size_t{1});
        REQUIRE_EQ(second_line.Column, std::size_t{0});

        REQUIRE_EQ(second_newline.Line, std::size_t{1});
        REQUIRE_EQ(second_newline.Column, std::size_t{6});

        REQUIRE_EQ(third_line.Line, std::size_t{2});
        REQUIRE_EQ(third_line.Column, std::size_t{0});

        REQUIRE_EQ(eof.Line, std::size_t{2});
        REQUIRE_EQ(eof.Column, std::size_t{5});
    }

    void TestCrLfAndTrailingEmptyLine() {
        const Rivet::SourceFile source{ "windows-lines.rivet", "alpha\r\nbeta\r\n" };

        REQUIRE_EQ(source.Size(), std::size_t{13});
        REQUIRE_EQ(source.LineCount(), std::size_t{3});

        REQUIRE_EQ(source.LineText(0), std::string_view{"alpha"});
        REQUIRE_EQ(source.LineText(1), std::string_view{"beta"});
        REQUIRE(source.LineText(2).empty());

        const Rivet::SourceSpan first = source.LineSpan(0);
        const Rivet::SourceSpan second = source.LineSpan(1);
        const Rivet::SourceSpan trailing = source.LineSpan(2);

        REQUIRE_EQ(first.Offset, std::size_t{0});
        REQUIRE_EQ(first.Length, std::size_t{5});

        REQUIRE_EQ(second.Offset, std::size_t{7});
        REQUIRE_EQ(second.Length, std::size_t{4});

        REQUIRE_EQ(trailing.Offset, std::size_t{13});
        REQUIRE_EQ(trailing.Length, std::size_t{0});

        const Rivet::SourcePosition second_line = source.PositionAt(7);
        const Rivet::SourcePosition eof = source.PositionAt(13);

        REQUIRE_EQ(second_line.Line, std::size_t{1});
        REQUIRE_EQ(second_line.Column, std::size_t{0});

        REQUIRE_EQ(eof.Line, std::size_t{2});
        REQUIRE_EQ(eof.Column, std::size_t{0});
    }

    void TestSpanContainment() {
        const Rivet::SourceFile source{"spans.rivet", "abcd"};

        REQUIRE(source.Contains({.Offset = 0, .Length = 0}));
        REQUIRE(source.Contains({.Offset = 0, .Length = 4}));
        REQUIRE(source.Contains({.Offset = 2, .Length = 2}));
        REQUIRE(source.Contains({.Offset = 4, .Length = 0}));

        REQUIRE(!source.Contains({.Offset = 4, .Length = 1}));
        REQUIRE(!source.Contains({.Offset = 5, .Length = 0}));

        REQUIRE(!source.Contains({
            .Offset = std::numeric_limits<std::size_t>::max(),
            .Length = 1
        }));

        REQUIRE(!source.Contains({
            .Offset = 0,
            .Length = std::numeric_limits<std::size_t>::max()
        }));

        REQUIRE_EQ(
            source.Text({.Offset = 1, .Length = 2}),
            std::string_view{"bc"}
        );
    }

    void TestLoadPreservesBytes() {
        const std::string contents{"ab\0cd\n", 6};
        const TemporaryFile file{contents};

        const auto result = Rivet::SourceFile::Load(file.Path());

        REQUIRE(result.has_value());

        const Rivet::SourceFile& source = result.value();

        REQUIRE_EQ(source.Path(), file.Path());
        REQUIRE_EQ(source.Size(), contents.size());
        REQUIRE(source.Text() == std::string_view{contents});
        REQUIRE_EQ(source.LineCount(), std::size_t{2});

        REQUIRE_EQ(source.LineText(0).size(), std::size_t{ 5 });

        REQUIRE(source.LineText(1).empty());
    }

    void TestMissingFileProducesError() {
        const std::filesystem::path missing_path = MakeTemporaryPath();

        const auto result = Rivet::SourceFile::Load(missing_path);

        REQUIRE(!result.has_value());
        REQUIRE_EQ(result.error().Path, missing_path);
        REQUIRE_EQ(
            result.error().Message,
            std::string{"could not open source file"}
        );
    }

    struct TestCase {
        std::string_view Name;
        void (*Body)();
    };

    constexpr std::array<TestCase, 6> SourceFileTests{{
        {"empty source", TestEmptySource},
        {"LF line table", TestLfLineTable},
        {"CRLF and trailing line", TestCrLfAndTrailingEmptyLine},
        {"span containment", TestSpanContainment},
        {"loading preserves bytes", TestLoadPreservesBytes},
        {"missing file error", TestMissingFileProducesError},
    }};
}

namespace Rivet::Tests {
    bool RunSourceFileTests() {
        std::size_t failures = 0;

        std::cout << "SourceFile\n";

        for (const TestCase& test : SourceFileTests) {
            std::cout << "  [ RUN  ] " << test.Name << '\n';

            try {
                test.Body();
                std::cout << "  [ PASS ] " << test.Name << '\n';
            } catch (const std::exception& exception) {
                failures++;

                std::cerr
                    << "  [ FAIL ] "
                    << test.Name
                    << '\n'
                    << "           "
                    << exception.what()
                    << '\n';
            } catch (...) {
                failures++;

                std::cerr
                    << "  [ FAIL ] "
                    << test.Name
                    << "\n           unknown exception\n";
            }
        }

        std::cout
            << "  "
            << SourceFileTests.size()
            << " tests, "
            << failures
            << " failures\n";

        return failures == 0;
    }
}

#undef REQUIRE
#undef REQUIRE_EQ