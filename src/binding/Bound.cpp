#include "binding/Bound.hpp"

#include <cassert>
#include <ostream>
#include <utility>

namespace Rivet {
    std::string_view BoundNodeKindName(BoundNodeKind kind) noexcept {
        switch (kind) {
            case BoundNodeKind::CompilationUnit:   return "CompilationUnit";
            case BoundNodeKind::Function:          return "Function";
            case BoundNodeKind::BlockStatement:    return "BlockStatement";
            case BoundNodeKind::ReturnStatement:   return "ReturnStatement";
            case BoundNodeKind::ErrorStatement:    return "ErrorStatement";
            case BoundNodeKind::LiteralExpression: return "LiteralExpression";
            case BoundNodeKind::UnaryExpression:   return "UnaryExpression";
            case BoundNodeKind::BinaryExpression:  return "BinaryExpression";
            case BoundNodeKind::ErrorExpression:   return "ErrorExpression";
        }
        return "Unknown";
    }

    std::string_view BoundUnaryOperatorKindName(BoundUnaryOperatorKind kind) noexcept {
        switch (kind) {
            case BoundUnaryOperatorKind::Identity: return "Identity";
            case BoundUnaryOperatorKind::Negation: return "Negate";
        }
        return "Unknown";
    }

    std::string_view BoundBinaryOperatorKindName(BoundBinaryOperatorKind kind) noexcept {
        switch (kind) {
            case BoundBinaryOperatorKind::Addition:       return "Add";
            case BoundBinaryOperatorKind::Subtraction:    return "Subtract";
            case BoundBinaryOperatorKind::Multiplication: return "Multiply";
            case BoundBinaryOperatorKind::Division:       return "Divide";
        }
        return "Unknown";
    }

    std::ostream& operator<<(std::ostream& output, BoundNodeKind kind) {
        return output << BoundNodeKindName(kind);
    }

    std::ostream& operator<<(std::ostream& output, BoundUnaryOperatorKind kind) {
        return output << BoundUnaryOperatorKindName(kind);
    }

    std::ostream& operator<<(std::ostream& output, BoundBinaryOperatorKind kind) {
        return output << BoundBinaryOperatorKindName(kind);
    }

    BoundLiteralExpression::BoundLiteralExpression(
        std::int32_t value,
        SourceSpan span
    ) noexcept
        : Value(value), LiteralSpan(span) {}

    BoundNodeKind BoundLiteralExpression::Kind() const noexcept {
        return BoundNodeKind::LiteralExpression;
    }

    SourceSpan BoundLiteralExpression::Span() const noexcept {
        return LiteralSpan;
    }

    const TypeSymbol& BoundLiteralExpression::Type() const noexcept {
        return BuiltinTypes::Int;
    }

    BoundUnaryExpression::BoundUnaryExpression(
        BoundUnaryOperatorKind operator_kind,
        SourceSpan operator_span,
        std::unique_ptr<BoundExpression> operand,
        SourceSpan expression_span
    ) noexcept
        : OperatorKind(operator_kind),
          OperatorSpan(operator_span),
          Operand(std::move(operand)),
          ExpressionSpan(expression_span) {
        assert(Operand != nullptr);
    }

    BoundNodeKind BoundUnaryExpression::Kind() const noexcept {
        return BoundNodeKind::UnaryExpression;
    }

    SourceSpan BoundUnaryExpression::Span() const noexcept {
        return ExpressionSpan;
    }

    const TypeSymbol& BoundUnaryExpression::Type() const noexcept {
        return BuiltinTypes::Int;
    }

    BoundBinaryExpression::BoundBinaryExpression(
        std::unique_ptr<BoundExpression> left,
        BoundBinaryOperatorKind operator_kind,
        SourceSpan operator_span,
        std::unique_ptr<BoundExpression> right,
        SourceSpan expression_span
    ) noexcept
        : Left(std::move(left)),
          OperatorKind(operator_kind),
          OperatorSpan(operator_span),
          Right(std::move(right)),
          ExpressionSpan(expression_span) {
        assert(Left != nullptr);
        assert(Right != nullptr);
    }

    BoundNodeKind BoundBinaryExpression::Kind() const noexcept {
        return BoundNodeKind::BinaryExpression;
    }

    SourceSpan BoundBinaryExpression::Span() const noexcept {
        return ExpressionSpan;
    }

    const TypeSymbol& BoundBinaryExpression::Type() const noexcept {
        return BuiltinTypes::Int;
    }

    BoundErrorExpression::BoundErrorExpression(SourceSpan span) noexcept
        : ErrorSpan(span) {}

    BoundNodeKind BoundErrorExpression::Kind() const noexcept {
        return BoundNodeKind::ErrorExpression;
    }

    SourceSpan BoundErrorExpression::Span() const noexcept {
        return ErrorSpan;
    }

    const TypeSymbol& BoundErrorExpression::Type() const noexcept {
        return BuiltinTypes::Error;
    }

    BoundReturnStatement::BoundReturnStatement(
        std::unique_ptr<BoundExpression> expression,
        SourceSpan span
    ) noexcept
        : Expression(std::move(expression)), StatementSpan(span) {
        assert(Expression != nullptr);
    }

    BoundNodeKind BoundReturnStatement::Kind() const noexcept {
        return BoundNodeKind::ReturnStatement;
    }

    SourceSpan BoundReturnStatement::Span() const noexcept {
        return StatementSpan;
    }

    BoundErrorStatement::BoundErrorStatement(SourceSpan span) noexcept
        : ErrorSpan(span) {}

    BoundNodeKind BoundErrorStatement::Kind() const noexcept {
        return BoundNodeKind::ErrorStatement;
    }

    SourceSpan BoundErrorStatement::Span() const noexcept {
        return ErrorSpan;
    }

    BoundBlockStatement::BoundBlockStatement(
        std::vector<std::unique_ptr<BoundStatement>> statements,
        SourceSpan span
    ) noexcept
        : Statements(std::move(statements)), BlockSpan(span) {}

    BoundNodeKind BoundBlockStatement::Kind() const noexcept {
        return BoundNodeKind::BlockStatement;
    }

    SourceSpan BoundBlockStatement::Span() const noexcept {
        return BlockSpan;
    }

    BoundFunction::BoundFunction(
        FunctionSymbol symbol,
        std::unique_ptr<BoundBlockStatement> body,
        bool is_entry_point,
        SourceSpan span
    ) noexcept
        : Symbol(std::move(symbol)),
          Body(std::move(body)),
          IsEntryPoint(is_entry_point),
          FunctionSpan(span) {
        assert(Symbol.ReturnType != nullptr);
        assert(Body != nullptr);
    }

    BoundNodeKind BoundFunction::Kind() const noexcept {
        return BoundNodeKind::Function;
    }

    SourceSpan BoundFunction::Span() const noexcept {
        return FunctionSpan;
    }

    BoundCompilationUnit::BoundCompilationUnit(
        std::string module_name,
        std::vector<std::unique_ptr<BoundFunction>> functions,
        SourceSpan span
    ) noexcept
        : ModuleName(std::move(module_name)),
          Functions(std::move(functions)),
          UnitSpan(span) {}

    BoundNodeKind BoundCompilationUnit::Kind() const noexcept {
        return BoundNodeKind::CompilationUnit;
    }

    SourceSpan BoundCompilationUnit::Span() const noexcept {
        return UnitSpan;
    }
}
