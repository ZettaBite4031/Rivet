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
        case SyntaxKind::ModuleDeclaration:        return "ModuleDeclaration";
        case SyntaxKind::FunctionDefinition:      return "FunctionDefinition";
        case SyntaxKind::BlockStatement:          return "BlockStatement";
        case SyntaxKind::ReturnStatement:         return "ReturnStatement";
        case SyntaxKind::LocalDeclaration:        return "LocalDeclaration";
        case SyntaxKind::AssignmentStatement:     return "AssignmentStatement";
        case SyntaxKind::LiteralExpression:       return "LiteralExpression";
        case SyntaxKind::NameExpression:          return "NameExpression";
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

    ModuleDeclarationSyntax::ModuleDeclarationSyntax(Token keyword, std::vector<Token> path_tokens, Token semicolon) noexcept
        : ModuleKeyword(keyword), PathTokens(std::move(path_tokens)), SemicolonToken(semicolon) {}

    SyntaxKind ModuleDeclarationSyntax::Kind() const noexcept { return SyntaxKind::ModuleDeclaration; }

    SourceSpan ModuleDeclarationSyntax::Span() const noexcept {
        return CoveringSpan(ModuleKeyword.Span, SemicolonToken.Span);
    }

    std::string ModuleDeclarationSyntax::Name(const SourceFile& source) const {
        std::string result;
        for (const Token& token : PathTokens) result += token.Text(source);
        return result;
    }

     LiteralExpressionSyntax::LiteralExpressionSyntax(Token literal_token) noexcept
        : LiteralToken(literal_token) {}

    SyntaxKind LiteralExpressionSyntax::Kind() const noexcept {
        return SyntaxKind::LiteralExpression;
    }

    SourceSpan LiteralExpressionSyntax::Span() const noexcept {
        return LiteralToken.Span;
    }

    NameExpressionSyntax::NameExpressionSyntax(Token name) noexcept : NameToken(name) {}
    SyntaxKind NameExpressionSyntax::Kind() const noexcept { return SyntaxKind::NameExpression; }
    SourceSpan NameExpressionSyntax::Span() const noexcept { return NameToken.Span; }

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

    LocalDeclarationSyntax::LocalDeclarationSyntax(Token mut_keyword, Token type, Token name, Token equals,
        std::unique_ptr<ExpressionSyntax> initializer, Token semicolon) noexcept
        : MutKeyword(mut_keyword), TypeToken(type), NameToken(name), EqualsToken(equals),
          Initializer(std::move(initializer)), SemicolonToken(semicolon) { assert(Initializer != nullptr); }
    SyntaxKind LocalDeclarationSyntax::Kind() const noexcept { return SyntaxKind::LocalDeclaration; }
    SourceSpan LocalDeclarationSyntax::Span() const noexcept {
        return CoveringSpan(MutKeyword.Span.Empty() ? TypeToken.Span : MutKeyword.Span, SemicolonToken.Span);
    }

    AssignmentStatementSyntax::AssignmentStatementSyntax(Token name, Token equals,
        std::unique_ptr<ExpressionSyntax> value, Token semicolon) noexcept
        : NameToken(name), EqualsToken(equals), Value(std::move(value)), SemicolonToken(semicolon) {
        assert(Value != nullptr);
    }
    SyntaxKind AssignmentStatementSyntax::Kind() const noexcept { return SyntaxKind::AssignmentStatement; }
    SourceSpan AssignmentStatementSyntax::Span() const noexcept {
        return CoveringSpan(NameToken.Span, SemicolonToken.Span);
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

    CompilationUnitSyntax::CompilationUnitSyntax(std::unique_ptr<ModuleDeclarationSyntax> module,
        std::vector<std::unique_ptr<FunctionDefinitionSyntax>> functions, Token eof_token) noexcept
        : Module(std::move(module)), Functions(std::move(functions)), EndOfFileToken(eof_token) { assert(Module != nullptr); }

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
