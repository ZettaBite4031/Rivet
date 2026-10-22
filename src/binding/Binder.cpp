#include "binding/Binder.hpp"

#include "syntax/Token.hpp"

#include <cassert>
#include <charconv>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace Rivet {
    Binder::Binder(
        const SourceFile& source,
        DiagnosticBag& diagnostics
    ) noexcept
        : m_Source(source), m_Diagnostics(diagnostics) {}

    std::unique_ptr<BoundCompilationUnit> Binder::BindCompilationUnit(const CompilationUnitSyntax& syntax) {
        assert(m_Declarations.empty());
        assert(m_FunctionIndices.empty());

        DeclareFunctions(syntax);
        ValidateEntryPoint(syntax);

        std::vector<std::unique_ptr<BoundFunction>> functions;
        functions.reserve(m_Declarations.size());

        for (const DeclaredFunction& declaration : m_Declarations)
            functions.push_back(BindFunction(declaration));

        return std::make_unique<BoundCompilationUnit>(syntax.Module->Name(m_Source), std::move(functions), syntax.Span());
    }

    void Binder::DeclareFunctions(const CompilationUnitSyntax& syntax) {
        m_Declarations.reserve(syntax.Functions.size());

        for (const auto& function_pointer : syntax.Functions) {
            assert(function_pointer != nullptr);
            const FunctionDefinitionSyntax& function = *function_pointer;

            if (function.TypeToken.Span.Empty() || function.IdentifierToken.Span.Empty()) {
                m_HasStructurallyInvalidDeclaration = true;
                continue;
            }

            std::string name{ function.IdentifierToken.Text(m_Source) };

            const auto [existing, inserted] = m_FunctionIndices.emplace(name, m_Declarations.size());
            if (!inserted) {
                const DeclaredFunction& previous = m_Declarations[existing->second];

                std::string message{ "function '" };
                message += name;
                message += "()' is already defined";

                m_Diagnostics.Report(DiagnosticCode::DuplicateFunction, DiagnosticSeverity::Error, function.IdentifierToken.Span, std::move(message));
                m_Diagnostics.Report(DiagnosticCode::DuplicateFunction, DiagnosticSeverity::Note, previous.Symbol.DeclarationSpan, "previous definition is here");
                continue;
            }

            m_Declarations.push_back(DeclaredFunction{
                .Symbol = FunctionSymbol{
                    .Name = std::move(name),
                    .ReturnType = &BuiltinTypes::Int,
                    .DeclarationSpan = function.IdentifierToken.Span,
                },
                .Syntax = &function,
            });
        }
    }

    void Binder::ValidateEntryPoint(const CompilationUnitSyntax& syntax) {
        if (m_HasStructurallyInvalidDeclaration) return;
        if (m_FunctionIndices.contains("main")) return;
        m_Diagnostics.Report(DiagnosticCode::MissingEntryPoint, DiagnosticSeverity::Error, syntax.Span(), "executable does not define 'int main()'");
    }

    std::unique_ptr<BoundFunction> Binder::BindFunction(const DeclaredFunction& declaration) {
        assert(declaration.Syntax != nullptr);
        assert(declaration.Syntax->Body != nullptr);

        std::unique_ptr<BoundBlockStatement> body = BindBlockStatement(*declaration.Syntax->Body);
        if (body->Statements.empty())
            m_Diagnostics.Report(DiagnosticCode::MissingReturn, DiagnosticSeverity::Error, declaration.Syntax->Body->CloseBraceToken.Span, "function returning 'int' must return a value");

        return std::make_unique<BoundFunction>(declaration.Symbol, std::move(body), declaration.Symbol.Name == "main", declaration.Syntax->Span());
    }

    std::unique_ptr<BoundBlockStatement> Binder::BindBlockStatement(const BlockStatementSyntax& syntax) {
        std::vector<std::unique_ptr<BoundStatement>> statements;
        statements.reserve(syntax.Statements.size());

        for (const auto& statement : syntax.Statements) {
            assert(statement != nullptr);
            statements.push_back(BindStatement(*statement));
        }

        return std::make_unique<BoundBlockStatement>(std::move(statements), syntax.Span());
    }

    std::unique_ptr<BoundStatement> Binder::BindStatement(const StatementSyntax& syntax) {
        if (syntax.Kind() == SyntaxKind::ReturnStatement) 
            return BindReturnStatement(static_cast<const ReturnStatementSyntax&>(syntax));

        assert(false && "unhandled statement syntax kind");
        return std::make_unique<BoundErrorStatement>(syntax.Span());
    }

    std::unique_ptr<BoundReturnStatement> Binder::BindReturnStatement(const ReturnStatementSyntax& syntax) {
        assert(syntax.Expression != nullptr);

        return std::make_unique<BoundReturnStatement>(BindExpression(*syntax.Expression), syntax.Span());
    }

    std::unique_ptr<BoundExpression> Binder::BindExpression(const ExpressionSyntax& syntax) {
        switch (syntax.Kind()) {
            case SyntaxKind::LiteralExpression: return BindLiteralExpression(static_cast<const LiteralExpressionSyntax&>(syntax));
            case SyntaxKind::UnaryExpression: return BindUnaryExpression(static_cast<const UnaryExpressionSyntax&>(syntax));
            case SyntaxKind::BinaryExpression: return BindBinaryExpression(static_cast<const BinaryExpressionSyntax&>(syntax));

            case SyntaxKind::ParenthesizedExpression: {
                const auto& parenthesized = static_cast<const ParenthesizedExpressionSyntax&>(syntax);
                assert(parenthesized.Expression != nullptr);
                return BindExpression(*parenthesized.Expression);
            }

            case SyntaxKind::ErrorExpression: return std::make_unique<BoundErrorExpression>(syntax.Span());
            default:
                assert(false && "unhandled expression syntax kind");
                return std::make_unique<BoundErrorExpression>(syntax.Span());
        }
    }

    std::unique_ptr<BoundExpression> Binder::BindLiteralExpression(const LiteralExpressionSyntax& syntax) {
        const std::string_view text = syntax.LiteralToken.Text(m_Source);

        std::uint64_t value = 0;
        const char* const beginning = text.data();
        const char* const ending = beginning + text.size();

        const std::from_chars_result result = std::from_chars(beginning, ending, value, 10);
        constexpr std::uint64_t maximum_int = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
        if (result.ec != std::errc{} || result.ptr != ending || value > maximum_int) {
            std::string message{ "integer literal '" };
            message += text;
            message += "' is not representable as 'int'";

            m_Diagnostics.Report(DiagnosticCode::IntegerLiteralOutOfRange, DiagnosticSeverity::Error, syntax.LiteralToken.Span, std::move(message) );

            return std::make_unique<BoundErrorExpression>(syntax.Span());
        }

        return std::make_unique<BoundLiteralExpression>(static_cast<std::int32_t>(value), syntax.Span());
    }

    std::unique_ptr<BoundExpression> Binder::BindUnaryExpression(const UnaryExpressionSyntax& syntax) {
        assert(syntax.Operand != nullptr);

        std::unique_ptr<BoundExpression> operand = BindExpression(*syntax.Operand);
        if (operand->Type().Kind == TypeKind::Error) 
            return std::make_unique<BoundErrorExpression>(syntax.Span());

        BoundUnaryOperatorKind operator_kind = BoundUnaryOperatorKind::Identity;
        switch (syntax.OperatorToken.Kind) {
            case TokenKind::Plus: operator_kind = BoundUnaryOperatorKind::Identity; break;
            case TokenKind::Minus: operator_kind = BoundUnaryOperatorKind::Negation; break;
            default:
                assert(false && "unhandled unary operator token");
                return std::make_unique<BoundErrorExpression>(syntax.Span());
        }

        return std::make_unique<BoundUnaryExpression>(operator_kind, syntax.OperatorToken.Span, std::move(operand), syntax.Span());
    }

    std::unique_ptr<BoundExpression> Binder::BindBinaryExpression(const BinaryExpressionSyntax& syntax) {
        assert(syntax.Left != nullptr);
        assert(syntax.Right != nullptr);

        std::unique_ptr<BoundExpression> left = BindExpression(*syntax.Left);
        std::unique_ptr<BoundExpression> right = BindExpression(*syntax.Right);

        if (left->Type().Kind == TypeKind::Error || right->Type().Kind == TypeKind::Error) 
            return std::make_unique<BoundErrorExpression>(syntax.Span());

        BoundBinaryOperatorKind operator_kind = BoundBinaryOperatorKind::Addition;
        switch (syntax.OperatorToken.Kind) {
            case TokenKind::Plus: operator_kind = BoundBinaryOperatorKind::Addition; break;
            case TokenKind::Minus: operator_kind = BoundBinaryOperatorKind::Subtraction; break;
            case TokenKind::Star: operator_kind = BoundBinaryOperatorKind::Multiplication; break;
            case TokenKind::Slash: operator_kind = BoundBinaryOperatorKind::Division; break;
            default:
                assert(false && "unhandled binary operator token");
                return std::make_unique<BoundErrorExpression>(syntax.Span());
        }

        return std::make_unique<BoundBinaryExpression>(std::move(left), operator_kind, syntax.OperatorToken.Span, std::move(right), syntax.Span());
    }
}
