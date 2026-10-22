#include "test.hpp"

#include "diagnostics/Diagnostics.hpp"
#include "source/Source.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Parser.hpp"
#include "syntax/Syntax.hpp"
#include "syntax/Token.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Rivet::Tests {
    namespace {
        struct ParsedSource {
            SourceFile Source;
            DiagnosticBag Diagnostics;
            std::unique_ptr<CompilationUnitSyntax> Root;
        };

        ParsedSource ParseText(std::string text) {
            SourceFile source{
                "parser-test.rivet",
                "module parser_test;\n" + text
            };

            DiagnosticBag diagnostics;

            const std::vector<Token> tokens =
                LexAll(source, diagnostics);

            Parser parser{
                std::span<const Token>{ tokens },
                diagnostics
            };

            std::unique_ptr<CompilationUnitSyntax> root =
                parser.ParseCompilationUnit();

            return ParsedSource{
                .Source = std::move(source),
                .Diagnostics = std::move(diagnostics),
                .Root = std::move(root),
            };
        }

        template<typename NodeType>
        const NodeType& RequireNode(const SyntaxNode& node, SyntaxKind expected_kind) {
            REQUIRE_EQ(node.Kind(), expected_kind);

            const auto* typed_node = dynamic_cast<const NodeType*>(&node);

            REQUIRE(typed_node != nullptr);
            return *typed_node;
        }

        const FunctionDefinitionSyntax& RequireFunction(const ParsedSource& parsed, std::size_t index) {
            REQUIRE(parsed.Root != nullptr);
            REQUIRE(index < parsed.Root->Functions.size());
            REQUIRE(parsed.Root->Functions[index] != nullptr);

            return *parsed.Root->Functions[index];
        }

        const ReturnStatementSyntax& RequireOnlyReturn(const ParsedSource& parsed, std::size_t function_index = 0) {
            const FunctionDefinitionSyntax& function = RequireFunction(parsed, function_index);

            REQUIRE(function.Body != nullptr);
            REQUIRE_EQ(function.Body->Statements.size(), std::size_t{ 1 });

            REQUIRE(function.Body->Statements.front() != nullptr);

            return RequireNode<ReturnStatementSyntax>(*function.Body->Statements.front(), SyntaxKind::ReturnStatement);
        }

        void RequireLiteralText(const ParsedSource& parsed, const ExpressionSyntax& expression, std::string_view expected_text) {
            const LiteralExpressionSyntax& literal = RequireNode<LiteralExpressionSyntax>(expression, SyntaxKind::LiteralExpression);

            REQUIRE_EQ(literal.LiteralToken.Text(parsed.Source), expected_text);
        }

        void ParsesMinimalFunction() {
            ParsedSource parsed = ParseText(
                "int main()\n"
                "{\n"
                "    return 42;\n"
                "}\n"
            );

            REQUIRE(parsed.Diagnostics.Empty());
            REQUIRE(parsed.Root != nullptr);
            REQUIRE_EQ(parsed.Root->Kind(), SyntaxKind::CompilationUnit);

            REQUIRE_EQ(parsed.Root->Functions.size(), std::size_t{ 1 });

            REQUIRE_EQ(parsed.Root->Span().Offset, std::size_t{ 0 });

            REQUIRE_EQ(parsed.Root->Span().Length, parsed.Source.Size());

            const FunctionDefinitionSyntax& function = RequireFunction(parsed, 0);

            REQUIRE_EQ(function.Kind(), SyntaxKind::FunctionDefinition);

            REQUIRE_EQ(function.TypeToken.Kind, TokenKind::IntKeyword);

            REQUIRE_EQ(function.TypeToken.Text(parsed.Source), std::string_view{ "int" });

            REQUIRE_EQ(function.IdentifierToken.Kind, TokenKind::Identifier);

            REQUIRE_EQ(function.IdentifierToken.Text(parsed.Source), std::string_view{ "main" });

            REQUIRE_EQ(function.OpenParenToken.Kind, TokenKind::OpenParen);

            REQUIRE_EQ(function.CloseParenToken.Kind, TokenKind::CloseParen);

            REQUIRE(function.Body != nullptr);

            REQUIRE_EQ(function.Body->Kind(), SyntaxKind::BlockStatement);

            const ReturnStatementSyntax& statement = RequireOnlyReturn(parsed);

            REQUIRE_EQ(statement.ReturnKeyword.Kind, TokenKind::ReturnKeyword);

            REQUIRE_EQ(statement.SemicolonToken.Kind, TokenKind::Semicolon);

            REQUIRE(!statement.SemicolonToken.Span.Empty());

            RequireLiteralText(parsed, *statement.Expression, "42");
        }

        void AppliesBinaryOperatorPrecedence() {
            ParsedSource parsed = ParseText(
                "int main()\n"
                "{\n"
                "    return 1 + 2 * 3 - 4 / 2;\n"
                "}\n"
            );

            REQUIRE(parsed.Diagnostics.Empty());

            const ReturnStatementSyntax& statement = RequireOnlyReturn(parsed);

            // The weakest final operator is subtraction:
            //
            //     (1 + (2 * 3)) - (4 / 2)
            const BinaryExpressionSyntax& subtraction = RequireNode<BinaryExpressionSyntax>(*statement.Expression, SyntaxKind::BinaryExpression);

            REQUIRE_EQ(subtraction.OperatorToken.Kind, TokenKind::Minus);

            const BinaryExpressionSyntax& addition = RequireNode<BinaryExpressionSyntax>(*subtraction.Left, SyntaxKind::BinaryExpression);

            REQUIRE_EQ(addition.OperatorToken.Kind, TokenKind::Plus);

            RequireLiteralText(parsed, *addition.Left, "1");

            const BinaryExpressionSyntax& multiplication = RequireNode<BinaryExpressionSyntax>(*addition.Right, SyntaxKind::BinaryExpression);

            REQUIRE_EQ(multiplication.OperatorToken.Kind, TokenKind::Star);

            RequireLiteralText(parsed, *multiplication.Left, "2");

            RequireLiteralText(parsed, *multiplication.Right, "3");

            const BinaryExpressionSyntax& division = RequireNode<BinaryExpressionSyntax>(*subtraction.Right, SyntaxKind::BinaryExpression);

            REQUIRE_EQ(division.OperatorToken.Kind, TokenKind::Slash);

            RequireLiteralText(parsed, *division.Left, "4");
            RequireLiteralText(parsed, *division.Right, "2");
        }

        void ParsesParenthesesAndUnaryOperators() {
            ParsedSource parsed = ParseText(
                "int main()\n"
                "{\n"
                "    return -(1 + +2) * 3;\n"
                "}\n"
            );

            REQUIRE(parsed.Diagnostics.Empty());

            const ReturnStatementSyntax& statement = RequireOnlyReturn(parsed);

            const BinaryExpressionSyntax& multiplication = RequireNode<BinaryExpressionSyntax>(*statement.Expression, SyntaxKind::BinaryExpression);

            REQUIRE_EQ(multiplication.OperatorToken.Kind, TokenKind::Star);

            const UnaryExpressionSyntax& negation = RequireNode<UnaryExpressionSyntax>(*multiplication.Left, SyntaxKind::UnaryExpression);

            REQUIRE_EQ(negation.OperatorToken.Kind, TokenKind::Minus);

            const ParenthesizedExpressionSyntax& parentheses = RequireNode<ParenthesizedExpressionSyntax>(*negation.Operand, SyntaxKind::ParenthesizedExpression);

            REQUIRE_EQ(parentheses.OpenParenToken.Kind, TokenKind::OpenParen);

            REQUIRE_EQ(parentheses.CloseParenToken.Kind, TokenKind::CloseParen);

            const BinaryExpressionSyntax& addition = RequireNode<BinaryExpressionSyntax>(*parentheses.Expression, SyntaxKind::BinaryExpression);

            REQUIRE_EQ(addition.OperatorToken.Kind, TokenKind::Plus);

            RequireLiteralText(parsed, *addition.Left, "1");

            const UnaryExpressionSyntax& unary_plus = RequireNode<UnaryExpressionSyntax>(*addition.Right, SyntaxKind::UnaryExpression);

            REQUIRE_EQ(unary_plus.OperatorToken.Kind, TokenKind::Plus );

            RequireLiteralText(parsed, *unary_plus.Operand, "2");

            RequireLiteralText(parsed, *multiplication.Right, "3");
        }

        void ParsesMultipleFunctions() {
            ParsedSource parsed = ParseText(
                "int helper()\n"
                "{\n"
                "    return 1;\n"
                "}\n"
                "\n"
                "int main()\n"
                "{\n"
                "    return 42;\n"
                "}\n"
            );

            REQUIRE(parsed.Diagnostics.Empty());

            REQUIRE_EQ(parsed.Root->Functions.size(), std::size_t{ 2 });

            const FunctionDefinitionSyntax& helper = RequireFunction(parsed, 0);

            const FunctionDefinitionSyntax& main = RequireFunction(parsed, 1);

            REQUIRE_EQ(helper.IdentifierToken.Text(parsed.Source), std::string_view{ "helper" });

            REQUIRE_EQ(main.IdentifierToken.Text(parsed.Source), std::string_view{ "main" });

            const ReturnStatementSyntax& helper_return = RequireOnlyReturn(parsed, 0);

            const ReturnStatementSyntax& main_return = RequireOnlyReturn(parsed, 1);

            RequireLiteralText(parsed, *helper_return.Expression, "1");

            RequireLiteralText(parsed, *main_return.Expression, "42");
        }

        void SynthesizesMissingSemicolon() {
            ParsedSource parsed = ParseText(
                "int main()\n"
                "{\n"
                "    return 42\n"
                "}\n"
            );

            REQUIRE_EQ(parsed.Diagnostics.Size(), std::size_t{ 1 });

            REQUIRE(parsed.Diagnostics.HasErrors());

            const ReturnStatementSyntax& statement = RequireOnlyReturn(parsed);

            RequireLiteralText(parsed, *statement.Expression, "42");

            REQUIRE_EQ(statement.SemicolonToken.Kind, TokenKind::Semicolon);

            REQUIRE(statement.SemicolonToken.Span.Empty());

            const FunctionDefinitionSyntax& function = RequireFunction(parsed, 0);

            REQUIRE_EQ(statement.SemicolonToken.Span.Offset, function.Body->CloseBraceToken.Span.Offset);

            const Diagnostic& diagnostic = parsed.Diagnostics.Items().front();

            REQUIRE(diagnostic.Code == DiagnosticCode::MissingToken);

            REQUIRE(diagnostic.Severity == DiagnosticSeverity::Error);

            REQUIRE_EQ(diagnostic.Message, std::string{ "expected ';' after return expression" });

            REQUIRE_EQ(RenderCompactDiagnostics(parsed.Source, parsed.Diagnostics.Items()),
                std::string{ "E0102 5:1-5:2 expected ';' after return expression\n" });
        }

        void ProducesErrorExpressionWhenMissing() {
            ParsedSource parsed = ParseText(
                "int main()\n"
                "{\n"
                "    return ;\n"
                "}\n"
            );

            REQUIRE_EQ(parsed.Diagnostics.Size(), std::size_t{ 1 });

            const ReturnStatementSyntax& statement = RequireOnlyReturn(parsed);

            const ErrorExpressionSyntax& error = RequireNode<ErrorExpressionSyntax>(*statement.Expression, SyntaxKind::ErrorExpression);

            REQUIRE(error.Span().Empty());

            REQUIRE_EQ(error.Span().Offset, statement.SemicolonToken.Span.Offset);

            // The semicolon was retained for ReturnStatementSyntax rather
            // than being swallowed as the malformed expression.
            REQUIRE_EQ(statement.SemicolonToken.Kind, TokenKind::Semicolon);

            REQUIRE(!statement.SemicolonToken.Span.Empty());

            const Diagnostic& diagnostic = parsed.Diagnostics.Items().front();

            REQUIRE(diagnostic.Code == DiagnosticCode::ExpectedExpression);

            REQUIRE_EQ(diagnostic.Message, std::string{ "expected expression" });

            REQUIRE_EQ(RenderCompactDiagnostics(parsed.Source, parsed.Diagnostics.Items()),
                std::string{ "E0103 4:12-4:13 expected expression\n" });
        }

        void RecoversAfterMissingUnaryOperand() {
            ParsedSource parsed = ParseText(
                "int broken()\n"
                "{\n"
                "    return +;\n"
                "}\n"
                "\n"
                "int main()\n"
                "{\n"
                "    return 42;\n"
                "}\n"
            );

            REQUIRE_EQ(parsed.Diagnostics.Size(), std::size_t{ 1 });

            REQUIRE_EQ(parsed.Root->Functions.size(), std::size_t{ 2 });

            const ReturnStatementSyntax& broken_return = RequireOnlyReturn(parsed, 0);

            const UnaryExpressionSyntax& unary = RequireNode<UnaryExpressionSyntax>(*broken_return.Expression, SyntaxKind::UnaryExpression);

            REQUIRE_EQ(unary.OperatorToken.Kind, TokenKind::Plus);

            const ErrorExpressionSyntax& error = RequireNode<ErrorExpressionSyntax>(*unary.Operand, SyntaxKind::ErrorExpression);

            REQUIRE(error.Span().Empty());

            const FunctionDefinitionSyntax& main = RequireFunction(parsed, 1);

            REQUIRE_EQ(main.IdentifierToken.Text(parsed.Source), std::string_view{ "main" });

            const ReturnStatementSyntax& main_return = RequireOnlyReturn(parsed, 1);

            RequireLiteralText(parsed, *main_return.Expression, "42");

            REQUIRE_EQ(RenderCompactDiagnostics(parsed.Source, parsed.Diagnostics.Items()),
                std::string{ "E0103 4:13-4:14 expected expression after unary operator '+'\n" });
        }

        void RecoversNextFunctionAfterMissingBrace() {
            ParsedSource parsed = ParseText(
                "int broken()\n"
                "{\n"
                "    return 1;\n"
                "\n"
                "int main()\n"
                "{\n"
                "    return 42;\n"
                "}\n"
            );

            REQUIRE_EQ(parsed.Diagnostics.Size(), std::size_t{ 1 });

            REQUIRE_EQ(parsed.Root->Functions.size(), std::size_t{ 2 });

            const FunctionDefinitionSyntax& broken = RequireFunction(parsed, 0);

            const FunctionDefinitionSyntax& main = RequireFunction(parsed, 1);

            REQUIRE(broken.Body->CloseBraceToken.Span.Empty());

            REQUIRE_EQ(broken.Body->CloseBraceToken.Span.Offset, main.TypeToken.Span.Offset);

            REQUIRE_EQ(main.IdentifierToken.Text(parsed.Source), std::string_view{ "main" });

            const ReturnStatementSyntax& main_return = RequireOnlyReturn(parsed, 1);

            RequireLiteralText(parsed, *main_return.Expression, "42");

            REQUIRE_EQ(RenderCompactDiagnostics(parsed.Source, parsed.Diagnostics.Items()),
                std::string{ "E0102 6:1-6:4 expected '}' to close function body\n" });
        }
    }

    bool RunParserTests() {
        const std::array tests{
            TestCase{
                "parses a minimal function",
                ParsesMinimalFunction
            },
            TestCase{
                "applies binary operator precedence",
                AppliesBinaryOperatorPrecedence
            },
            TestCase{
                "parses parentheses and unary operators",
                ParsesParenthesesAndUnaryOperators
            },
            TestCase{
                "parses multiple functions",
                ParsesMultipleFunctions
            },
            TestCase{
                "synthesizes a missing semicolon",
                SynthesizesMissingSemicolon
            },
            TestCase{
                "produces an error expression when missing",
                ProducesErrorExpressionWhenMissing
            },
            TestCase{
                "recovers after a missing unary operand",
                RecoversAfterMissingUnaryOperand
            },
            TestCase{
                "recovers the next function after a missing brace",
                RecoversNextFunctionAfterMissingBrace
            },
        };

        return RunSuite("Parser", tests);
    }
}
