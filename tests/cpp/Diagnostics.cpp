#include "test.hpp"
#include "diagnostics/Diagnostics.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace {
    void TestBagStartsEmpty() {
        const Rivet::DiagnosticBag diagnostics;

        REQUIRE(diagnostics.Empty());
        REQUIRE_EQ(diagnostics.Size(), std::size_t{0});
        REQUIRE(diagnostics.Items().empty());
        REQUIRE(!diagnostics.HasErrors());
    }

    void TestBagPreservesOrderAndSeverity() {
        Rivet::DiagnosticBag diagnostics;

        diagnostics.Report(
            Rivet::DiagnosticCode::InvalidCharacter,
            Rivet::DiagnosticSeverity::Warning,
            {.Offset = 1, .Length = 1},
            "first diagnostic"
        );

        REQUIRE(!diagnostics.HasErrors());

        diagnostics.Report(
            Rivet::DiagnosticCode::InvalidCharacter,
            Rivet::DiagnosticSeverity::Error,
            {.Offset = 3, .Length = 1},
            "second diagnostic"
        );

        REQUIRE(diagnostics.HasErrors());
        REQUIRE_EQ(diagnostics.Size(), std::size_t{2});

        const auto items = diagnostics.Items();

        REQUIRE(items[0].Severity == Rivet::DiagnosticSeverity::Warning);
        REQUIRE(items[1].Severity == Rivet::DiagnosticSeverity::Error);
        REQUIRE_EQ(items[0].Message, std::string{ "first diagnostic" });
        REQUIRE_EQ(items[1].Message, std::string{ "second diagnostic" });

        REQUIRE_EQ(items[0].Span.Offset, std::size_t{1});
        REQUIRE_EQ(items[1].Span.Offset, std::size_t{3});
    }

    void TestDiagnosticCodeFormatting() {
        REQUIRE_EQ(
            Rivet::FormatDiagnosticCode(
                Rivet::DiagnosticSeverity::Error,
                Rivet::DiagnosticCode::InvalidCharacter
            ),
            std::string{"E0001"}
        );

        REQUIRE_EQ(
            Rivet::FormatDiagnosticCode(
                Rivet::DiagnosticSeverity::Warning,
                Rivet::DiagnosticCode::InvalidCharacter
            ),
            std::string{"W0001"}
        );
    }

    void TestSingleLineRendering() {
        const Rivet::SourceFile source{
            "invalid.rivet",
            "int main() { return 4 $ 2; }\n"
        };

        const Rivet::Diagnostic diagnostic{
            .Code = Rivet::DiagnosticCode::InvalidCharacter,
            .Severity = Rivet::DiagnosticSeverity::Error,
            .Span = { .Offset = 22, .Length = 1, },
            .Message = "invalid character '$'",
        };

        REQUIRE_EQ(
            Rivet::RenderCompactDiagnostic(source, diagnostic),
            std::string{ "E0001 1:23-1:24 invalid character '$'" }
        );
    }

    void TestMultilineAndEofRendering() {
        const Rivet::SourceFile source{
            "multiline.rivet",
            "abc\ndef\n"
        };

        const Rivet::Diagnostic multiline{
            .Code = Rivet::DiagnosticCode::InvalidCharacter,
            .Severity = Rivet::DiagnosticSeverity::Error,
            .Span = { .Offset = 2, .Length = 3, },
            .Message = "multiline range",
        };

        const Rivet::Diagnostic eof{
            .Code = Rivet::DiagnosticCode::InvalidCharacter,
            .Severity = Rivet::DiagnosticSeverity::Error,
            .Span = source.EofSpan(),
            .Message = "expected token",
        };

        REQUIRE_EQ(
            Rivet::RenderCompactDiagnostic(source, multiline),
            std::string{ "E0001 1:3-2:2 multiline range" }
        );

        REQUIRE_EQ(
            Rivet::RenderCompactDiagnostic(source, eof),
            std::string{ "E0001 3:1-3:1 expected token" }
        );
    }

    void TestCollectionRendering() {
        const Rivet::SourceFile source{
            "ordered.rivet",
            "abc"
        };

        Rivet::DiagnosticBag diagnostics;

        diagnostics.Report(
            Rivet::DiagnosticCode::InvalidCharacter,
            Rivet::DiagnosticSeverity::Warning,
            {.Offset = 0, .Length = 0},
            "first"
        );

        diagnostics.Report(
            Rivet::DiagnosticCode::InvalidCharacter,
            Rivet::DiagnosticSeverity::Error,
            {.Offset = 1, .Length = 1},
            "second"
        );

        REQUIRE_EQ(
            Rivet::RenderCompactDiagnostics(source, diagnostics.Items()),
            std::string{
                "W0001 1:1-1:1 first\n"
                "E0001 1:2-1:3 second\n"
            }
        );
    }

    void TestInvalidSpanIsRejected() {
        const Rivet::SourceFile source{
            "invalid-span.rivet",
            "abc"
        };

        const Rivet::Diagnostic diagnostic{
            .Code = Rivet::DiagnosticCode::InvalidCharacter,
            .Severity = Rivet::DiagnosticSeverity::Error,
            .Span = { .Offset = 3, .Length = 1, },
            .Message = "outside source",
        };

        bool threw = false;

        try {
            static_cast<void>(Rivet::RenderCompactDiagnostic(source, diagnostic));
        } catch (const std::out_of_range&) {
            threw = true;
        }

        REQUIRE(threw);
    }

    constexpr std::array<Rivet::Tests::TestCase, 7> DiagnosticTests{{
        {"empty diagnostic bag", TestBagStartsEmpty},
        {"diagnostic ordering and severity", TestBagPreservesOrderAndSeverity},
        {"diagnostic code formatting", TestDiagnosticCodeFormatting},
        {"single-line rendering", TestSingleLineRendering},
        {"multiline and EOF rendering", TestMultilineAndEofRendering},
        {"collection rendering", TestCollectionRendering},
        {"invalid span rejection", TestInvalidSpanIsRejected},
    }};
}

namespace Rivet::Tests {
    bool RunDiagnosticTests() {
        return RunSuite("Diagnostics", DiagnosticTests);
    }
}
