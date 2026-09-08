#pragma once

#include "source/Source.hpp"
#include "syntax/Token.hpp"

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

namespace Rivet {
    enum class SyntaxKind : std::uint8_t {
        CompilationUnit,
        FunctionDefinition,
        BlockStatement,
        ReturnStatement,
        LiteralExpression,
        UnaryExpression,
        BinaryExpression,
        ParenthesizedExpression,
        ErrorExpression,
    };

    [[nodiscard]] std::string_view SyntaxKindName(SyntaxKind kind) noexcept;
    std::ostream& operator<<(std::ostream& output, SyntaxKind kind);

    class SyntaxNode {
    public:
        virtual ~SyntaxNode() = default;

        [[nodiscard]] virtual SyntaxKind Kind() const noexcept = 0;
        [[nodiscard]] virtual SourceSpan Span() const noexcept = 0;
    };

    class StatementSyntax : public SyntaxNode {};
    class ExpressionSyntax : public SyntaxNode {};

    class LiteralExpressionSyntax final : public ExpressionSyntax {
    public:
        explicit LiteralExpressionSyntax(Token literal_token) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        Token LiteralToken;
    };

    class ErrorExpressionSyntax final : public ExpressionSyntax {
    public:
        explicit ErrorExpressionSyntax(SourceSpan error_span) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        SourceSpan ErrorSpan;
    };

    class UnaryExpressionSyntax final : public ExpressionSyntax {
    public:
        UnaryExpressionSyntax(Token operator_token, std::unique_ptr<ExpressionSyntax> operand) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        Token OperatorToken;
        std::unique_ptr<ExpressionSyntax> Operand;
    };

    class BinaryExpressionSyntax final : public ExpressionSyntax {
    public:
        BinaryExpressionSyntax(std::unique_ptr<ExpressionSyntax> left, Token operator_token, std::unique_ptr<ExpressionSyntax> right) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        std::unique_ptr<ExpressionSyntax> Left;
        Token OperatorToken;
        std::unique_ptr<ExpressionSyntax> Right;
    };

    class ParenthesizedExpressionSyntax final : public ExpressionSyntax {
    public:
        ParenthesizedExpressionSyntax(Token open_paren_token, std::unique_ptr<ExpressionSyntax> expression, Token close_paren_token) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        Token OpenParenToken;
        std::unique_ptr<ExpressionSyntax> Expression;
        Token CloseParenToken;
    };

    class ReturnStatementSyntax final : public StatementSyntax {
    public:
        ReturnStatementSyntax(Token return_keyword, std::unique_ptr<ExpressionSyntax> expression, Token semicolon_token) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        Token ReturnKeyword;
        std::unique_ptr<ExpressionSyntax> Expression;
        Token SemicolonToken;
    };

    class BlockStatementSyntax final : public StatementSyntax {
    public:
        BlockStatementSyntax(Token open_brace_token, std::vector<std::unique_ptr<StatementSyntax>> statements, Token close_brace_token) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        Token OpenBraceToken;
        std::vector<std::unique_ptr<StatementSyntax>> Statements;
        Token CloseBraceToken;
    };

    class FunctionDefinitionSyntax final : public SyntaxNode {
    public:
        FunctionDefinitionSyntax(Token type_token, Token identifier_token, Token open_paren_token, Token close_paren_token, std::unique_ptr<BlockStatementSyntax> body) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        Token TypeToken;
        Token IdentifierToken;
        Token OpenParenToken;
        Token CloseParenToken;
        std::unique_ptr<BlockStatementSyntax> Body;
    };

    class CompilationUnitSyntax final : public SyntaxNode {
    public:
        CompilationUnitSyntax(std::vector<std::unique_ptr<FunctionDefinitionSyntax>> functions, Token end_of_file_token) noexcept;

        [[nodiscard]] SyntaxKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        std::vector<std::unique_ptr<FunctionDefinitionSyntax>> Functions;
        Token EndOfFileToken;
    };
}
