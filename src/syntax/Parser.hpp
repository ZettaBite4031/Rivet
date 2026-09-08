#pragma once

#include "diagnostics/Diagnostics.hpp"
#include "syntax/Syntax.hpp"
#include "syntax/Token.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace Rivet {
    class Parser final {
    public:
        Parser(std::span<const Token> tokens, DiagnosticBag& diagnostics) noexcept;

        [[nodiscard]] std::unique_ptr<CompilationUnitSyntax> ParseCompilationUnit();
    private:
        [[nodiscard]] std::size_t TokenIndex(std::size_t distance) const noexcept;
        [[nodiscard]] const Token& Current() const noexcept;
        [[nodiscard]] const Token& Peek(std::size_t distance) const noexcept;

        Token ConsumeToken() noexcept;

        Token MatchToken(TokenKind expected, std::string_view message);

        [[nodiscard]] std::unique_ptr<FunctionDefinitionSyntax> ParseFunctionDefinition();
        [[nodiscard]] std::unique_ptr<BlockStatementSyntax> ParseBlockStatement();
        [[nodiscard]] std::unique_ptr<StatementSyntax> ParseStatement();
        [[nodiscard]] std::unique_ptr<ReturnStatementSyntax> ParseReturnStatement();

        [[nodiscard]] std::unique_ptr<ExpressionSyntax> ParseExpression(int parent_precedence = 0);
        [[nodiscard]] std::unique_ptr<ExpressionSyntax> ParsePrimaryExpression();
        [[nodiscard]] std::unique_ptr<ExpressionSyntax> MakeMissingExpression(std::string message);

        [[nodiscard]] bool IsFunctionStart() const noexcept;
        [[nodiscard]] static bool IsExpressionBoundary(TokenKind kind) noexcept;
        [[nodiscard]] static int UnaryOperatorPrecedence(TokenKind kind) noexcept;
        [[nodiscard]] static int BinaryOperatorPrecedence(TokenKind kind) noexcept;
        [[nodiscard]] static std::string_view OperatorSpelling(TokenKind kind) noexcept;

        std::span<const Token> m_Tokens;
        DiagnosticBag& m_Diagnostics;
        std::size_t m_Position{ 0 };
    };
}
