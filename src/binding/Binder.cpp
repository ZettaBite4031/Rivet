#include "binding/Binder.hpp"

#include "syntax/Token.hpp"

#include <cassert>
#include <algorithm>
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

    std::unique_ptr<BoundCompilationUnit> Binder::BindCompilationUnit(
        const CompilationUnitSyntax& syntax
    ) {
        assert(m_Declarations.empty());
        assert(m_FunctionIndices.empty());

        DeclareFunctions(syntax);
        ValidateEntryPoint(syntax);

        std::vector<std::unique_ptr<BoundFunction>> functions;
        functions.reserve(m_Declarations.size());

        for (const DeclaredFunction& declaration : m_Declarations)
            functions.push_back(BindFunction(declaration));

        return std::make_unique<BoundCompilationUnit>(
            syntax.Module->Name(m_Source),
            std::move(functions),
            syntax.Span()
        );
    }

    void Binder::DeclareFunctions(const CompilationUnitSyntax& syntax) {
        m_Declarations.reserve(syntax.Functions.size());

        for (const auto& function_pointer : syntax.Functions) {
            assert(function_pointer != nullptr);
            const FunctionDefinitionSyntax& function = *function_pointer;

            if (
                function.TypeToken.Span.Empty() ||
                function.IdentifierToken.Span.Empty()
            ) {
                m_HasStructurallyInvalidDeclaration = true;
                continue;
            }

            std::string name{
                function.IdentifierToken.Text(m_Source)
            };

            const auto [existing, inserted] =
                m_FunctionIndices.emplace(
                    name,
                    m_Declarations.size()
                );

            if (!inserted) {
                const DeclaredFunction& previous =
                    m_Declarations[existing->second];

                std::string message{ "function '" };
                message += name;
                message += "()' is already defined";

                m_Diagnostics.Report(
                    DiagnosticCode::DuplicateFunction,
                    DiagnosticSeverity::Error,
                    function.IdentifierToken.Span,
                    std::move(message)
                );

                m_Diagnostics.Report(
                    DiagnosticCode::DuplicateFunction,
                    DiagnosticSeverity::Note,
                    previous.Symbol.DeclarationSpan,
                    "previous definition is here"
                );

                continue;
            }

            m_Declarations.push_back(DeclaredFunction{
                .Symbol = FunctionSymbol{
                    .Name = std::move(name),
                    .ReturnType = &BuiltinTypes::Int,
                    .DeclarationSpan = function.IdentifierToken.Span,
                    .ParameterCount = function.Parameters.size(),
                },
                .Syntax = &function,
            });
        }
    }

    void Binder::ValidateEntryPoint(
        const CompilationUnitSyntax& syntax
    ) {
        if (m_HasStructurallyInvalidDeclaration)
            return;

        const auto main = m_FunctionIndices.find("main");
        if (main != m_FunctionIndices.end()) {
            const FunctionSymbol& symbol = m_Declarations[main->second].Symbol;
            if (symbol.ParameterCount != 0)
                m_Diagnostics.Report(DiagnosticCode::InvalidEntryPoint, DiagnosticSeverity::Error,
                    symbol.DeclarationSpan, "entry point must be 'int main()' with no parameters");
            return;
        }

        m_Diagnostics.Report(
            DiagnosticCode::MissingEntryPoint,
            DiagnosticSeverity::Error,
            syntax.Span(),
            "executable does not define 'int main()'"
        );
    }

    std::unique_ptr<BoundFunction> Binder::BindFunction(
        const DeclaredFunction& declaration
    ) {
        assert(declaration.Syntax != nullptr);
        assert(declaration.Syntax->Body != nullptr);
        m_Locals.clear();
        m_Scopes.clear();
        m_Scopes.emplace_back();
        for (const auto& parameter : declaration.Syntax->Parameters) {
            if (parameter.NameToken.Span.Empty()) continue;
            std::string name{ parameter.NameToken.Text(m_Source) };
            const auto previous = m_Scopes.back().find(name);
            if (previous != m_Scopes.back().end()) {
                m_Diagnostics.Report(DiagnosticCode::DuplicateLocal, DiagnosticSeverity::Error,
                    parameter.NameToken.Span, "parameter '" + name + "' is already declared");
                m_Diagnostics.Report(DiagnosticCode::DuplicateLocal, DiagnosticSeverity::Note,
                    m_Locals[previous->second].DeclarationSpan, "previous declaration is here");
                continue;
            }
            const std::size_t id = m_Locals.size();
            m_Scopes.back().emplace(name, id);
            m_Locals.push_back(LocalSymbol{ std::move(name), &BuiltinTypes::Int,
                false, parameter.NameToken.Span });
        }

        std::unique_ptr<BoundBlockStatement> body =
            BindBlockStatement(*declaration.Syntax->Body, false);

        if (!AlwaysReturns(*body)) {
            m_Diagnostics.Report(DiagnosticCode::MissingReturn,
                DiagnosticSeverity::Error, declaration.Syntax->Body->CloseBraceToken.Span,
                "function returning 'int' must return a value");
        }

        return std::make_unique<BoundFunction>(
            declaration.Symbol,
            std::move(body),
            std::move(m_Locals),
            declaration.Symbol.Name == "main",
            declaration.Syntax->Span()
        );
    }

    std::unique_ptr<BoundBlockStatement> Binder::BindBlockStatement(
        const BlockStatementSyntax& syntax, bool new_scope
    ) {
        if (new_scope) m_Scopes.emplace_back();
        std::vector<std::unique_ptr<BoundStatement>> statements;
        statements.reserve(syntax.Statements.size());

        for (const auto& statement : syntax.Statements) {
            assert(statement != nullptr);
            statements.push_back(BindStatement(*statement));
        }
        if (new_scope) m_Scopes.pop_back();

        return std::make_unique<BoundBlockStatement>(
            std::move(statements),
            syntax.Span()
        );
    }

    std::optional<std::size_t> Binder::LookupLocal(const std::string& name) const {
        for (auto scope = m_Scopes.rbegin(); scope != m_Scopes.rend(); ++scope) {
            const auto found = scope->find(name);
            if (found != scope->end()) return found->second;
        }
        return std::nullopt;
    }

    bool Binder::AlwaysReturns(const BoundStatement& statement) {
        switch (statement.Kind()) {
        case BoundNodeKind::ReturnStatement: return true;
        case BoundNodeKind::BlockStatement: {
            const auto& block = static_cast<const BoundBlockStatement&>(statement);
            for (const auto& child : block.Statements)
                if (AlwaysReturns(*child)) return true;
            return false;
        }
        case BoundNodeKind::IfStatement: {
            const auto& branch = static_cast<const BoundIfStatement&>(statement);
            return branch.Else && AlwaysReturns(*branch.Then) && AlwaysReturns(*branch.Else);
        }
        default: return false; // A while condition may be false initially.
        }
    }

    std::unique_ptr<BoundStatement> Binder::BindStatement(
        const StatementSyntax& syntax
    ) {
        if (syntax.Kind() == SyntaxKind::ReturnStatement) {
            return BindReturnStatement(
                static_cast<const ReturnStatementSyntax&>(syntax)
            );
        }

        if (syntax.Kind() == SyntaxKind::LocalDeclaration)
            return BindLocalDeclaration(static_cast<const LocalDeclarationSyntax&>(syntax));
        if (syntax.Kind() == SyntaxKind::AssignmentStatement)
            return BindAssignment(static_cast<const AssignmentStatementSyntax&>(syntax));
        if (syntax.Kind() == SyntaxKind::BlockStatement)
            return BindBlockStatement(static_cast<const BlockStatementSyntax&>(syntax));
        if (syntax.Kind() == SyntaxKind::IfStatement)
            return BindIf(static_cast<const IfStatementSyntax&>(syntax));
        if (syntax.Kind() == SyntaxKind::WhileStatement)
            return BindWhile(static_cast<const WhileStatementSyntax&>(syntax));

        assert(false && "unhandled statement syntax kind");
        return std::make_unique<BoundErrorStatement>(syntax.Span());
    }

    std::unique_ptr<BoundReturnStatement> Binder::BindReturnStatement(
        const ReturnStatementSyntax& syntax
    ) {
        assert(syntax.Expression != nullptr);

        auto value = BindExpression(*syntax.Expression);
        if (value->Type().Kind != TypeKind::Error && value->Type().Kind != TypeKind::Int)
            m_Diagnostics.Report(DiagnosticCode::TypeMismatch, DiagnosticSeverity::Error,
                syntax.Expression->Span(), "return value must be 'int'");
        return std::make_unique<BoundReturnStatement>(std::move(value), syntax.Span());
    }

    std::unique_ptr<BoundStatement> Binder::BindLocalDeclaration(const LocalDeclarationSyntax& syntax) {
        // An initializer sees preceding declarations, never the declaration being initialized.
        auto initializer = BindExpression(*syntax.Initializer);
        if (syntax.NameToken.Span.Empty())
            return std::make_unique<BoundErrorStatement>(syntax.Span());

        const std::string name{ syntax.NameToken.Text(m_Source) };
        const auto previous = m_Scopes.back().find(name);
        if (previous != m_Scopes.back().end()) {
            m_Diagnostics.Report(DiagnosticCode::DuplicateLocal, DiagnosticSeverity::Error,
                syntax.NameToken.Span, "local '" + name + "' is already declared");
            m_Diagnostics.Report(DiagnosticCode::DuplicateLocal, DiagnosticSeverity::Note,
                m_Locals[previous->second].DeclarationSpan, "previous declaration is here");
            return std::make_unique<BoundErrorStatement>(syntax.Span());
        }
        const std::size_t id = m_Locals.size();
        const TypeSymbol* type = syntax.TypeToken.Kind == TokenKind::BoolKeyword
            ? &BuiltinTypes::Bool : &BuiltinTypes::Int;
        if (initializer->Type().Kind != TypeKind::Error && initializer->Type().Kind != type->Kind)
            m_Diagnostics.Report(DiagnosticCode::TypeMismatch, DiagnosticSeverity::Error,
                syntax.Initializer->Span(), "initializer has wrong type for '" + name + "'");
        m_Scopes.back().emplace(name, id);
        m_Locals.push_back(LocalSymbol{ std::move(name), type,
            !syntax.MutKeyword.Span.Empty(), syntax.NameToken.Span });
        return std::make_unique<BoundLocalDeclaration>(id, std::move(initializer), syntax.Span());
    }

    std::unique_ptr<BoundStatement> Binder::BindAssignment(const AssignmentStatementSyntax& syntax) {
        auto value = BindExpression(*syntax.Value);
        const std::string name{ syntax.NameToken.Text(m_Source) };
        const auto found = LookupLocal(name);
        if (!found) {
            m_Diagnostics.Report(DiagnosticCode::UnknownLocal, DiagnosticSeverity::Error,
                syntax.NameToken.Span, "unknown local '" + name + "'");
            return std::make_unique<BoundErrorStatement>(syntax.Span());
        }
        const std::size_t id = *found;
        if (!m_Locals[id].Mutable) {
            m_Diagnostics.Report(DiagnosticCode::ImmutableLocal, DiagnosticSeverity::Error,
                syntax.NameToken.Span, "cannot assign to immutable local '" + name + "'");
            return std::make_unique<BoundErrorStatement>(syntax.Span());
        }
        if (value->Type().Kind != TypeKind::Error && value->Type().Kind != m_Locals[id].Type->Kind)
            m_Diagnostics.Report(DiagnosticCode::TypeMismatch, DiagnosticSeverity::Error,
                syntax.Value->Span(), "assignment has wrong type for '" + name + "'");
        return std::make_unique<BoundAssignmentStatement>(id, std::move(value), syntax.Span());
    }

    std::unique_ptr<BoundStatement> Binder::BindIf(const IfStatementSyntax& syntax) {
        auto condition = BindExpression(*syntax.Condition);
        if (condition->Type().Kind != TypeKind::Bool && condition->Type().Kind != TypeKind::Error)
            m_Diagnostics.Report(DiagnosticCode::InvalidCondition, DiagnosticSeverity::Error,
                syntax.Condition->Span(), "if condition must be 'bool'");
        auto then_block = BindBlockStatement(*syntax.Then);
        auto else_block = syntax.Else ? BindBlockStatement(*syntax.Else) : nullptr;
        return std::make_unique<BoundIfStatement>(std::move(condition), std::move(then_block),
            std::move(else_block), syntax.Span());
    }

    std::unique_ptr<BoundStatement> Binder::BindWhile(const WhileStatementSyntax& syntax) {
        auto condition = BindExpression(*syntax.Condition);
        if (condition->Type().Kind != TypeKind::Bool && condition->Type().Kind != TypeKind::Error)
            m_Diagnostics.Report(DiagnosticCode::InvalidCondition, DiagnosticSeverity::Error,
                syntax.Condition->Span(), "while condition must be 'bool'");
        auto body = BindBlockStatement(*syntax.Body);
        return std::make_unique<BoundWhileStatement>(std::move(condition), std::move(body), syntax.Span());
    }

    std::unique_ptr<BoundExpression> Binder::BindExpression(
        const ExpressionSyntax& syntax
    ) {
        switch (syntax.Kind()) {
            case SyntaxKind::LiteralExpression:
                return BindLiteralExpression(
                    static_cast<const LiteralExpressionSyntax&>(syntax)
                );

            case SyntaxKind::NameExpression: {
                const auto& name = static_cast<const NameExpressionSyntax&>(syntax);
                const std::string text{ name.NameToken.Text(m_Source) };
                const auto found = LookupLocal(text);
                if (!found) {
                    m_Diagnostics.Report(DiagnosticCode::UnknownLocal, DiagnosticSeverity::Error,
                        name.NameToken.Span, "unknown local '" + text + "'");
                    return std::make_unique<BoundErrorExpression>(syntax.Span());
                }
                return std::make_unique<BoundLocalExpression>(*found, syntax.Span(), m_Locals[*found].Type);
            }

            case SyntaxKind::CallExpression:
                return BindCallExpression(static_cast<const CallExpressionSyntax&>(syntax));

            case SyntaxKind::UnaryExpression:
                return BindUnaryExpression(
                    static_cast<const UnaryExpressionSyntax&>(syntax)
                );

            case SyntaxKind::BinaryExpression:
                return BindBinaryExpression(
                    static_cast<const BinaryExpressionSyntax&>(syntax)
                );

            case SyntaxKind::ParenthesizedExpression: {
                const auto& parenthesized =
                    static_cast<const ParenthesizedExpressionSyntax&>(syntax);
                assert(parenthesized.Expression != nullptr);
                return BindExpression(*parenthesized.Expression);
            }

            case SyntaxKind::ErrorExpression:
                return std::make_unique<BoundErrorExpression>(
                    syntax.Span()
                );

            default:
                assert(false && "unhandled expression syntax kind");
                return std::make_unique<BoundErrorExpression>(
                    syntax.Span()
                );
        }
    }

    std::unique_ptr<BoundExpression> Binder::BindLiteralExpression(
        const LiteralExpressionSyntax& syntax
    ) {
        if (syntax.LiteralToken.Kind == TokenKind::TrueKeyword ||
            syntax.LiteralToken.Kind == TokenKind::FalseKeyword)
            return std::make_unique<BoundLiteralExpression>(
                syntax.LiteralToken.Kind == TokenKind::TrueKeyword ? 1 : 0,
                syntax.Span(), &BuiltinTypes::Bool);
        const std::string_view text =
            syntax.LiteralToken.Text(m_Source);

        std::uint64_t value = 0;
        const char* const beginning = text.data();
        const char* const ending = beginning + text.size();

        const std::from_chars_result result =
            std::from_chars(beginning, ending, value, 10);

        constexpr std::uint64_t maximum_int =
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int32_t>::max()
            );

        if (
            result.ec != std::errc{} ||
            result.ptr != ending ||
            value > maximum_int
        ) {
            std::string message{ "integer literal '" };
            message += text;
            message += "' is not representable as 'int'";

            m_Diagnostics.Report(
                DiagnosticCode::IntegerLiteralOutOfRange,
                DiagnosticSeverity::Error,
                syntax.LiteralToken.Span,
                std::move(message)
            );

            return std::make_unique<BoundErrorExpression>(
                syntax.Span()
            );
        }

        return std::make_unique<BoundLiteralExpression>(
            static_cast<std::int32_t>(value),
            syntax.Span()
        );
    }

    std::unique_ptr<BoundExpression> Binder::BindUnaryExpression(
        const UnaryExpressionSyntax& syntax
    ) {
        assert(syntax.Operand != nullptr);

        std::unique_ptr<BoundExpression> operand =
            BindExpression(*syntax.Operand);

        if (operand->Type().Kind == TypeKind::Error) {
            return std::make_unique<BoundErrorExpression>(
                syntax.Span()
            );
        }

        BoundUnaryOperatorKind operator_kind =
            BoundUnaryOperatorKind::Identity;

        switch (syntax.OperatorToken.Kind) {
            case TokenKind::Plus:
                operator_kind = BoundUnaryOperatorKind::Identity;
                break;

            case TokenKind::Minus:
                operator_kind = BoundUnaryOperatorKind::Negation;
                break;
            case TokenKind::Bang:
                operator_kind = BoundUnaryOperatorKind::Not;
                break;

            default:
                assert(false && "unhandled unary operator token");
                return std::make_unique<BoundErrorExpression>(
                    syntax.Span()
                );
        }
        const TypeKind expected = operator_kind == BoundUnaryOperatorKind::Not
            ? TypeKind::Bool : TypeKind::Int;
        if (operand->Type().Kind != expected) {
            m_Diagnostics.Report(DiagnosticCode::InvalidOperator, DiagnosticSeverity::Error,
                syntax.OperatorToken.Span, "unary operator has invalid operand type");
            return std::make_unique<BoundErrorExpression>(syntax.Span());
        }

        return std::make_unique<BoundUnaryExpression>(
            operator_kind,
            syntax.OperatorToken.Span,
            std::move(operand),
            syntax.Span()
        );
    }

    std::unique_ptr<BoundExpression> Binder::BindBinaryExpression(
        const BinaryExpressionSyntax& syntax
    ) {
        assert(syntax.Left != nullptr);
        assert(syntax.Right != nullptr);

        std::unique_ptr<BoundExpression> left =
            BindExpression(*syntax.Left);
        std::unique_ptr<BoundExpression> right =
            BindExpression(*syntax.Right);

        if (
            left->Type().Kind == TypeKind::Error ||
            right->Type().Kind == TypeKind::Error
        ) {
            return std::make_unique<BoundErrorExpression>(
                syntax.Span()
            );
        }

        BoundBinaryOperatorKind operator_kind =
            BoundBinaryOperatorKind::Addition;

        switch (syntax.OperatorToken.Kind) {
            case TokenKind::Plus:
                operator_kind = BoundBinaryOperatorKind::Addition;
                break;

            case TokenKind::Minus:
                operator_kind = BoundBinaryOperatorKind::Subtraction;
                break;

            case TokenKind::Star:
                operator_kind = BoundBinaryOperatorKind::Multiplication;
                break;

            case TokenKind::Slash:
                operator_kind = BoundBinaryOperatorKind::Division;
                break;
            case TokenKind::Less: operator_kind = BoundBinaryOperatorKind::Less; break;
            case TokenKind::LessEquals: operator_kind = BoundBinaryOperatorKind::LessEquals; break;
            case TokenKind::Greater: operator_kind = BoundBinaryOperatorKind::Greater; break;
            case TokenKind::GreaterEquals: operator_kind = BoundBinaryOperatorKind::GreaterEquals; break;
            case TokenKind::EqualsEquals: operator_kind = BoundBinaryOperatorKind::Equals; break;
            case TokenKind::BangEquals: operator_kind = BoundBinaryOperatorKind::NotEquals; break;

            default:
                assert(false && "unhandled binary operator token");
                return std::make_unique<BoundErrorExpression>(
                    syntax.Span()
                );
        }
        const bool equality = operator_kind == BoundBinaryOperatorKind::Equals ||
            operator_kind == BoundBinaryOperatorKind::NotEquals;
        const bool valid = equality
            ? left->Type().Kind == right->Type().Kind
            : left->Type().Kind == TypeKind::Int && right->Type().Kind == TypeKind::Int;
        if (!valid) {
            m_Diagnostics.Report(DiagnosticCode::InvalidOperator, DiagnosticSeverity::Error,
                syntax.OperatorToken.Span, "binary operator has invalid operand types");
            return std::make_unique<BoundErrorExpression>(syntax.Span());
        }

        return std::make_unique<BoundBinaryExpression>(
            std::move(left),
            operator_kind,
            syntax.OperatorToken.Span,
            std::move(right),
            syntax.Span()
        );
    }

    std::unique_ptr<BoundExpression> Binder::BindCallExpression(const CallExpressionSyntax& syntax) {
        std::vector<std::unique_ptr<BoundExpression>> arguments;
        arguments.reserve(syntax.Arguments.size());
        bool invalid_argument = false;
        for (const auto& argument : syntax.Arguments) {
            auto bound = BindExpression(*argument);
            invalid_argument |= bound->Type().Kind == TypeKind::Error;
            if (bound->Type().Kind == TypeKind::Bool) {
                m_Diagnostics.Report(DiagnosticCode::TypeMismatch, DiagnosticSeverity::Error,
                    argument->Span(), "function argument must be 'int'");
                invalid_argument = true;
            }
            arguments.push_back(std::move(bound));
        }
        const std::string name{ syntax.NameToken.Text(m_Source) };
        const auto found = m_FunctionIndices.find(name);
        if (found == m_FunctionIndices.end()) {
            m_Diagnostics.Report(DiagnosticCode::UnknownFunction, DiagnosticSeverity::Error,
                syntax.NameToken.Span, "unknown function '" + name + "'");
            return std::make_unique<BoundErrorExpression>(syntax.Span());
        }
        const FunctionSymbol& symbol = m_Declarations[found->second].Symbol;
        if (arguments.size() != symbol.ParameterCount) {
            m_Diagnostics.Report(DiagnosticCode::ArgumentCountMismatch, DiagnosticSeverity::Error,
                syntax.Span(), "function '" + name + "' expects "
                    + std::to_string(symbol.ParameterCount) + " argument(s), got "
                    + std::to_string(arguments.size()));
            return std::make_unique<BoundErrorExpression>(syntax.Span());
        }
        if (invalid_argument) return std::make_unique<BoundErrorExpression>(syntax.Span());
        return std::make_unique<BoundCallExpression>(name, std::move(arguments), syntax.Span());
    }
}
