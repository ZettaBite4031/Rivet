#include "syntax/Syntax.hpp"

#include <cassert>
#include <ostream>
#include <utility>

namespace {
    [[nodiscard]] Rivet::SourceSpan CoveringSpan(Rivet::SourceSpan first, Rivet::SourceSpan last) noexcept {
        assert(last.End() >= first.Offset);
        return Rivet::SourceSpan{ .Offset = first.Offset, .Length = last.End() - first.Offset };
    }
}

namespace Rivet {
    std::string_view SyntaxKindName(SyntaxKind kind) noexcept {
        switch (kind) {
        case SyntaxKind::CompilationUnit:         return "CompilationUnit";
        case SyntaxKind::FunctionDefinition:      return "FunctionDefinition";
        case SyntaxKind::BlockStatement:          return "BlockStatement";
        case SyntaxKind::ReturnStatement:         return "ReturnStatement";
        case SyntaxKind::LiteralExpression:       return "LiteralExpression";
        case SyntaxKind::UnaryExpression:         return "UnaryExpression";
        case SyntaxKind::BinaryExpression:        return "BinaryExpression";
        case SyntaxKind::ParenthesizedExpression: return "ParenthesizedExpression";
        case SyntaxKind::ErrorExpression:         return "ErrorExpression";
        }
        return "Unknown";
    }

    std::ostream& operator<<(std::ostream& output, SyntaxKind kind) {
        return output << SyntaxKindName(kind);
    }

     LiteralExpressionSyntax::LiteralExpressionSyntax(Token literal_token) noexcept
        : LiteralToken(literal_token) {}

    SyntaxKind LiteralExpressionSyntax::Kind() const noexcept {
        return SyntaxKind::LiteralExpression;
    }

    SourceSpan LiteralExpressionSyntax::Span() const noexcept {
        return LiteralToken.Span;
    }

    ErrorExpressionSyntax::ErrorExpressionSyntax(SourceSpan error_span) noexcept
        : ErrorSpan(error_span) {}

    SyntaxKind ErrorExpressionSyntax::Kind() const noexcept {
        return SyntaxKind::ErrorExpression;
    }

    SourceSpan ErrorExpressionSyntax::Span() const noexcept {
        return ErrorSpan;
    }

    UnaryExpressionSyntax::UnaryExpressionSyntax(Token operator_token, std::unique_ptr<ExpressionSyntax> operand) noexcept
        : OperatorToken(operator_token), Operand(std::move(operand)) { assert(Operand != nullptr); }

    SyntaxKind UnaryExpressionSyntax::Kind() const noexcept {
        return SyntaxKind::UnaryExpression;
    }

    SourceSpan UnaryExpressionSyntax::Span() const noexcept {
        return CoveringSpan(OperatorToken.Span, Operand->Span());
    }

    BinaryExpressionSyntax::BinaryExpressionSyntax(std::unique_ptr<ExpressionSyntax> left, Token operator_token, std::unique_ptr<ExpressionSyntax> right) noexcept
        : Left(std::move(left)), OperatorToken(operator_token), Right(std::move(right)) { assert(Left != nullptr && Right != nullptr); }

    SyntaxKind BinaryExpressionSyntax::Kind() const noexcept {
        return SyntaxKind::BinaryExpression;
    }

    SourceSpan BinaryExpressionSyntax::Span() const noexcept {
        return CoveringSpan(Left->Span(), Right->Span());
    }

    ParenthesizedExpressionSyntax::ParenthesizedExpressionSyntax(Token open_paren_token, std::unique_ptr<ExpressionSyntax> expression, Token close_paren_token) noexcept
        : OpenParenToken(open_paren_token), Expression(std::move(expression)), CloseParenToken(close_paren_token) { assert(Expression != nullptr); }

    SyntaxKind ParenthesizedExpressionSyntax::Kind() const noexcept {
        return SyntaxKind::ParenthesizedExpression;
    }

    SourceSpan ParenthesizedExpressionSyntax::Span() const noexcept {
        return CoveringSpan(OpenParenToken.Span, CloseParenToken.Span);
    }

    ReturnStatementSyntax::ReturnStatementSyntax(Token return_keyword, std::unique_ptr<ExpressionSyntax> expression, Token semicolon_token) noexcept
        : ReturnKeyword(return_keyword), Expression(std::move(expression)), SemicolonToken(semicolon_token) { assert(Expression != nullptr); }

    SyntaxKind ReturnStatementSyntax::Kind() const noexcept {
        return SyntaxKind::ReturnStatement;
    }

    SourceSpan ReturnStatementSyntax::Span() const noexcept {
        return CoveringSpan(ReturnKeyword.Span, SemicolonToken.Span);
    }

    BlockStatementSyntax::BlockStatementSyntax(Token open_brace_token, std::vector<std::unique_ptr<StatementSyntax>> statements, Token close_brace_token) noexcept
        : OpenBraceToken(open_brace_token), Statements(std::move(statements)), CloseBraceToken(close_brace_token) {}

    SyntaxKind BlockStatementSyntax::Kind() const noexcept {
        return SyntaxKind::BlockStatement;
    }

    SourceSpan BlockStatementSyntax::Span() const noexcept {
        return CoveringSpan(OpenBraceToken.Span, CloseBraceToken.Span);
    }

    FunctionDefinitionSyntax::FunctionDefinitionSyntax(Token type_token, Token identifier_token, Token open_paren_token, Token close_paren_token, std::unique_ptr<BlockStatementSyntax> body) noexcept 
        : TypeToken(type_token), IdentifierToken(identifier_token), OpenParenToken(open_paren_token), CloseParenToken(close_paren_token), Body(std::move(body)) { assert(Body != nullptr); }

    SyntaxKind FunctionDefinitionSyntax::Kind() const noexcept {
        return SyntaxKind::FunctionDefinition;
    }

    SourceSpan FunctionDefinitionSyntax::Span() const noexcept {
        return CoveringSpan(TypeToken.Span, Body->Span());
    }

    CompilationUnitSyntax::CompilationUnitSyntax(std::vector<std::unique_ptr<FunctionDefinitionSyntax>> functions, Token eof_token) noexcept
        : Functions(std::move(functions)), EndOfFileToken(eof_token) {}

    SyntaxKind CompilationUnitSyntax::Kind() const noexcept {
        return SyntaxKind::CompilationUnit;
    }

    SourceSpan CompilationUnitSyntax::Span() const noexcept {
        return SourceSpan{
            .Offset = 0,
            .Length = EndOfFileToken.Span.Offset,
        };
    }
}
