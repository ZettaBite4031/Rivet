#include "test.hpp"

#include "diagnostics/Diagnostics.hpp"
#include "source/Source.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Token.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Rivet::Tests {
    namespace {
        struct ExpectedToken {
            TokenKind Kind;
            std::size_t Offset;
            std::size_t Length;
            std::string_view Text;
        };

        template<std::size_t Count>
        void RequireTokens(
            const SourceFile& source,
            const std::vector<Token>& actual,
            const std::array<ExpectedToken, Count>& expected
        ) {
            REQUIRE_EQ(actual.size(), expected.size());

            for (std::size_t index = 0; index < expected.size(); index++) {
                const Token& actual_token = actual[index];
                const ExpectedToken& expected_token = expected[index];

                REQUIRE_EQ(actual_token.Kind, expected_token.Kind);
                REQUIRE_EQ(actual_token.Span.Offset, expected_token.Offset);
                REQUIRE_EQ(actual_token.Span.Length, expected_token.Length);
                REQUIRE_EQ(actual_token.Text(source), expected_token.Text);
            }
        }

        void EmptySourceProducesOnlyEndOfFile() {
            SourceFile source{
                "empty.rivet",
                std::string{}
            };

            DiagnosticBag diagnostics;
            const std::vector<Token> tokens =
                LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{
                    TokenKind::EndOfFile,
                    0,
                    0,
                    ""
                },
            };

            RequireTokens(source, tokens, expected);
            REQUIRE(diagnostics.Empty());
        }

        void LexesMinimalProgram() {
            SourceFile source{
                "minimal.rivet",
                "int main()\n"
                "{\n"
                "    return 40 + 2;\n"
                "}\n"
            };

            DiagnosticBag diagnostics;
            const std::vector<Token> tokens =
                LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{ TokenKind::IntKeyword,     0,  3, "int" },
                ExpectedToken{ TokenKind::Identifier,     4,  4, "main" },
                ExpectedToken{ TokenKind::OpenParen,      8,  1, "(" },
                ExpectedToken{ TokenKind::CloseParen,     9,  1, ")" },
                ExpectedToken{ TokenKind::OpenBrace,      11, 1, "{" },
                ExpectedToken{ TokenKind::ReturnKeyword,  17, 6, "return" },
                ExpectedToken{ TokenKind::IntegerLiteral, 24, 2, "40" },
                ExpectedToken{ TokenKind::Plus,           27, 1, "+" },
                ExpectedToken{ TokenKind::IntegerLiteral, 29, 1, "2" },
                ExpectedToken{ TokenKind::Semicolon,      30, 1, ";" },
                ExpectedToken{ TokenKind::CloseBrace,     32, 1, "}" },
                ExpectedToken{ TokenKind::EndOfFile,      34, 0, "" },
            };

            RequireTokens(source, tokens, expected);
            REQUIRE(diagnostics.Empty());
        }

        void DistinguishesKeywordsFromIdentifiers() {
            SourceFile source{
                "keywords-identifiers.rivet",
                "int integer return returning main _name name2\n"
            };

            DiagnosticBag diagnostics;
            const std::vector<Token> tokens =
                LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{ TokenKind::IntKeyword,    0,  3, "int" },
                ExpectedToken{ TokenKind::Identifier,    4,  7, "integer" },
                ExpectedToken{ TokenKind::ReturnKeyword, 12, 6, "return" },
                ExpectedToken{ TokenKind::Identifier,    19, 9, "returning" },
                ExpectedToken{ TokenKind::Identifier,    29, 4, "main" },
                ExpectedToken{ TokenKind::Identifier,    34, 5, "_name" },
                ExpectedToken{ TokenKind::Identifier,    40, 5, "name2" },
                ExpectedToken{ TokenKind::EndOfFile,     46, 0, "" },
            };

            RequireTokens(source, tokens, expected);
            REQUIRE(diagnostics.Empty());
        }

        void SkipsWhitespaceAndLineComments() {
            SourceFile source{
                "comments-whitespace.rivet",
                "  int\tmain() // entry\n"
                "{\n"
                "    // result\n"
                "    return 6*7;\n"
                "}\n"
            };

            DiagnosticBag diagnostics;
            const std::vector<Token> tokens =
                LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{ TokenKind::IntKeyword,     2,  3, "int" },
                ExpectedToken{ TokenKind::Identifier,     6,  4, "main" },
                ExpectedToken{ TokenKind::OpenParen,      10, 1, "(" },
                ExpectedToken{ TokenKind::CloseParen,     11, 1, ")" },
                ExpectedToken{ TokenKind::OpenBrace,      22, 1, "{" },
                ExpectedToken{ TokenKind::ReturnKeyword,  42, 6, "return" },
                ExpectedToken{ TokenKind::IntegerLiteral, 49, 1, "6" },
                ExpectedToken{ TokenKind::Star,           50, 1, "*" },
                ExpectedToken{ TokenKind::IntegerLiteral, 51, 1, "7" },
                ExpectedToken{ TokenKind::Semicolon,      52, 1, ";" },
                ExpectedToken{ TokenKind::CloseBrace,     54, 1, "}" },
                ExpectedToken{ TokenKind::EndOfFile,      56, 0, "" },
            };

            RequireTokens(source, tokens, expected);
            REQUIRE(diagnostics.Empty());
        }

        void LexesAllBootstrapPunctuation() {
            SourceFile source{ "punctuation.rivet", "(){};+-*/" };

            DiagnosticBag diagnostics;
            const std::vector<Token> tokens = LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{ TokenKind::OpenParen,  0, 1, "(" },
                ExpectedToken{ TokenKind::CloseParen, 1, 1, ")" },
                ExpectedToken{ TokenKind::OpenBrace,  2, 1, "{" },
                ExpectedToken{ TokenKind::CloseBrace, 3, 1, "}" },
                ExpectedToken{ TokenKind::Semicolon,  4, 1, ";" },
                ExpectedToken{ TokenKind::Plus,       5, 1, "+" },
                ExpectedToken{ TokenKind::Minus,      6, 1, "-" },
                ExpectedToken{ TokenKind::Star,       7, 1, "*" },
                ExpectedToken{ TokenKind::Slash,      8, 1, "/" },
                ExpectedToken{ TokenKind::EndOfFile,  9, 0, "" },
            };

            RequireTokens(source, tokens, expected);
            REQUIRE(diagnostics.Empty());
        }

        void ConsumesBothCharactersOfComparisonTokens() {
            SourceFile source{ "comparison-tokens.rivet", "== != <= >= = ! < >" };
            DiagnosticBag diagnostics;
            const std::vector<Token> tokens = LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{ TokenKind::EqualsEquals, 0, 2, "==" },
                ExpectedToken{ TokenKind::BangEquals, 3, 2, "!=" },
                ExpectedToken{ TokenKind::LessEquals, 6, 2, "<=" },
                ExpectedToken{ TokenKind::GreaterEquals, 9, 2, ">=" },
                ExpectedToken{ TokenKind::Equals, 12, 1, "=" },
                ExpectedToken{ TokenKind::Bang, 14, 1, "!" },
                ExpectedToken{ TokenKind::Less, 16, 1, "<" },
                ExpectedToken{ TokenKind::Greater, 18, 1, ">" },
                ExpectedToken{ TokenKind::EndOfFile, 19, 0, "" },
            };
            RequireTokens(source, tokens, expected);
            REQUIRE(diagnostics.Empty());
        }

        void ReportsInvalidCharactersAndContinues() {
            SourceFile source{
                "invalid-character.rivet",
                "int main() { return 4 $ 2; }\n"
            };

            DiagnosticBag diagnostics;
            const std::vector<Token> tokens = LexAll(source, diagnostics);

            constexpr std::array expected{
                ExpectedToken{ TokenKind::IntKeyword,     0,  3, "int" },
                ExpectedToken{ TokenKind::Identifier,     4,  4, "main" },
                ExpectedToken{ TokenKind::OpenParen,      8,  1, "(" },
                ExpectedToken{ TokenKind::CloseParen,     9,  1, ")" },
                ExpectedToken{ TokenKind::OpenBrace,      11, 1, "{" },
                ExpectedToken{ TokenKind::ReturnKeyword,  13, 6, "return" },
                ExpectedToken{ TokenKind::IntegerLiteral, 20, 1, "4" },
                ExpectedToken{ TokenKind::Invalid,        22, 1, "$" },
                ExpectedToken{ TokenKind::IntegerLiteral, 24, 1, "2" },
                ExpectedToken{ TokenKind::Semicolon,      25, 1, ";" },
                ExpectedToken{ TokenKind::CloseBrace,     27, 1, "}" },
                ExpectedToken{ TokenKind::EndOfFile,      29, 0, "" },
            };

            RequireTokens(source, tokens, expected);

            REQUIRE_EQ(diagnostics.Size(), std::size_t{ 1 });
            REQUIRE(diagnostics.HasErrors());

            const Diagnostic& diagnostic = diagnostics.Items().front();

            REQUIRE(diagnostic.Code == DiagnosticCode::InvalidCharacter);
            REQUIRE(diagnostic.Severity == DiagnosticSeverity::Error);

            REQUIRE_EQ(diagnostic.Span.Offset, std::size_t{ 22 });
            REQUIRE_EQ(diagnostic.Span.Length, std::size_t{ 1 });

            REQUIRE_EQ(diagnostic.Message, std::string{ "invalid character '$'" });

            REQUIRE_EQ(RenderCompactDiagnostics(source, diagnostics.Items()), 
                       std::string{ "E0001 1:23-1:24 invalid character '$'\n" });
        }

        void NextTokenReturnsStableEndOfFile() {
            SourceFile source{ "on-demand.rivet", "int" };

            DiagnosticBag diagnostics;
            Lexer lexer(source, diagnostics);

            const Token keyword = lexer.NextToken();
            const Token first_eof = lexer.NextToken();
            const Token second_eof = lexer.NextToken();

            REQUIRE_EQ(keyword.Kind, TokenKind::IntKeyword);
            REQUIRE_EQ(keyword.Span.Offset, std::size_t{ 0 });
            REQUIRE_EQ(keyword.Span.Length, std::size_t{ 3 });

            REQUIRE_EQ(first_eof.Kind, TokenKind::EndOfFile);
            REQUIRE_EQ(first_eof.Span.Offset, std::size_t{ 3 });
            REQUIRE_EQ(first_eof.Span.Length, std::size_t{ 0 });

            REQUIRE_EQ(second_eof.Kind, TokenKind::EndOfFile);
            REQUIRE_EQ(second_eof.Span.Offset, std::size_t{ 3 });
            REQUIRE_EQ(second_eof.Span.Length, std::size_t{ 0 });

            REQUIRE(diagnostics.Empty());
        }
    }

    bool RunLexerTests() {
        const std::array tests{
            TestCase{
                "empty source produces only EOF",
                EmptySourceProducesOnlyEndOfFile
            },
            TestCase{
                "lexes a minimal program",
                LexesMinimalProgram
            },
            TestCase{
                "distinguishes keywords from identifiers",
                DistinguishesKeywordsFromIdentifiers
            },
            TestCase{
                "skips whitespace and line comments",
                SkipsWhitespaceAndLineComments
            },
            TestCase{
                "lexes all bootstrap punctuation",
                LexesAllBootstrapPunctuation
            },
            TestCase{
                "consumes both characters of comparison tokens",
                ConsumesBothCharactersOfComparisonTokens
            },
            TestCase{
                "reports invalid characters and continues",
                ReportsInvalidCharactersAndContinues
            },
            TestCase{
                "on-demand lexer returns stable EOF",
                NextTokenReturnsStableEndOfFile
            },
        };

        return RunSuite("Lexer", tests);
    }
}
