#pragma once

#include "source/Source.hpp"

#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Rivet {
    enum class TypeKind : std::uint8_t {
        Error,
        Int,
        Bool,
    };

    struct TypeSymbol final {
        TypeKind Kind;
        std::string_view Name;
    };

    namespace BuiltinTypes {
        inline constexpr TypeSymbol Error{ TypeKind::Error, "<error>" };
        inline constexpr TypeSymbol Int{ TypeKind::Int, "int" };
        inline constexpr TypeSymbol Bool{ TypeKind::Bool, "bool" };
    }

    struct FunctionSymbol final {
        std::string Name;
        const TypeSymbol* ReturnType;
        SourceSpan DeclarationSpan;
        std::size_t ParameterCount{ 0 };
    };

    struct LocalSymbol final {
        std::string Name;
        const TypeSymbol* Type;
        bool Mutable;
        SourceSpan DeclarationSpan;
    };

    enum class BoundNodeKind : std::uint8_t {
        CompilationUnit,
        Function,
        BlockStatement,
        ReturnStatement,
        LocalDeclaration,
        AssignmentStatement,
        IfStatement,
        WhileStatement,
        ErrorStatement,
        LiteralExpression,
        LocalExpression,
        CallExpression,
        UnaryExpression,
        BinaryExpression,
        ErrorExpression,
    };

    enum class BoundUnaryOperatorKind : std::uint8_t {
        Identity,
        Negation,
        Not,
    };

    enum class BoundBinaryOperatorKind : std::uint8_t {
        Addition,
        Subtraction,
        Multiplication,
        Division,
        Less,
        LessEquals,
        Greater,
        GreaterEquals,
        Equals,
        NotEquals,
    };

    [[nodiscard]] std::string_view BoundNodeKindName(BoundNodeKind kind) noexcept;
    [[nodiscard]] std::string_view BoundUnaryOperatorKindName(BoundUnaryOperatorKind kind) noexcept;
    [[nodiscard]] std::string_view BoundBinaryOperatorKindName(BoundBinaryOperatorKind kind) noexcept;

    std::ostream& operator<<(std::ostream& output, BoundNodeKind kind);
    std::ostream& operator<<(std::ostream& output, BoundUnaryOperatorKind kind);
    std::ostream& operator<<(std::ostream& output, BoundBinaryOperatorKind kind);

    class BoundNode {
    public:
        virtual ~BoundNode() = default;
        [[nodiscard]] virtual BoundNodeKind Kind() const noexcept = 0;
        [[nodiscard]] virtual SourceSpan Span() const noexcept = 0;
    };

    class BoundStatement : public BoundNode {};
    class BoundBlockStatement;

    class BoundExpression : public BoundNode {
    public:
        [[nodiscard]] virtual const TypeSymbol& Type() const noexcept = 0;
    };

    class BoundLiteralExpression final : public BoundExpression {
    public:
        BoundLiteralExpression(std::int32_t value, SourceSpan span,
            const TypeSymbol* type = &BuiltinTypes::Int) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        [[nodiscard]] const TypeSymbol& Type() const noexcept override;

        std::int32_t Value;
        SourceSpan LiteralSpan;
        const TypeSymbol* LiteralType;
    };

    class BoundLocalExpression final : public BoundExpression {
    public:
        BoundLocalExpression(std::size_t local, SourceSpan span, const TypeSymbol* type) noexcept;
        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        [[nodiscard]] const TypeSymbol& Type() const noexcept override;
        std::size_t Local;
        SourceSpan NameSpan;
        const TypeSymbol* LocalType;
    };

    class BoundCallExpression final : public BoundExpression {
    public:
        BoundCallExpression(std::string callee,
            std::vector<std::unique_ptr<BoundExpression>> arguments, SourceSpan span) noexcept;
        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        [[nodiscard]] const TypeSymbol& Type() const noexcept override;
        std::string Callee;
        std::vector<std::unique_ptr<BoundExpression>> Arguments;
        SourceSpan CallSpan;
    };

    class BoundUnaryExpression final : public BoundExpression {
    public:
        BoundUnaryExpression(
            BoundUnaryOperatorKind operator_kind,
            SourceSpan operator_span,
            std::unique_ptr<BoundExpression> operand,
            SourceSpan expression_span
        ) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        [[nodiscard]] const TypeSymbol& Type() const noexcept override;

        BoundUnaryOperatorKind OperatorKind;
        SourceSpan OperatorSpan;
        std::unique_ptr<BoundExpression> Operand;
        SourceSpan ExpressionSpan;
    };

    class BoundBinaryExpression final : public BoundExpression {
    public:
        BoundBinaryExpression(
            std::unique_ptr<BoundExpression> left,
            BoundBinaryOperatorKind operator_kind,
            SourceSpan operator_span,
            std::unique_ptr<BoundExpression> right,
            SourceSpan expression_span
        ) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        [[nodiscard]] const TypeSymbol& Type() const noexcept override;

        std::unique_ptr<BoundExpression> Left;
        BoundBinaryOperatorKind OperatorKind;
        SourceSpan OperatorSpan;
        std::unique_ptr<BoundExpression> Right;
        SourceSpan ExpressionSpan;
    };

    class BoundErrorExpression final : public BoundExpression {
    public:
        explicit BoundErrorExpression(SourceSpan span) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        [[nodiscard]] const TypeSymbol& Type() const noexcept override;

        SourceSpan ErrorSpan;
    };

    class BoundReturnStatement final : public BoundStatement {
    public:
        BoundReturnStatement(
            std::unique_ptr<BoundExpression> expression,
            SourceSpan span
        ) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        std::unique_ptr<BoundExpression> Expression;
        SourceSpan StatementSpan;
    };

    class BoundLocalDeclaration final : public BoundStatement {
    public:
        BoundLocalDeclaration(std::size_t local, std::unique_ptr<BoundExpression> initializer,
            SourceSpan span) noexcept;
        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        std::size_t Local;
        std::unique_ptr<BoundExpression> Initializer;
        SourceSpan StatementSpan;
    };

    class BoundAssignmentStatement final : public BoundStatement {
    public:
        BoundAssignmentStatement(std::size_t local, std::unique_ptr<BoundExpression> value,
            SourceSpan span) noexcept;
        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        std::size_t Local;
        std::unique_ptr<BoundExpression> Value;
        SourceSpan StatementSpan;
    };

    class BoundIfStatement final : public BoundStatement {
    public:
        BoundIfStatement(std::unique_ptr<BoundExpression> condition,
            std::unique_ptr<BoundBlockStatement> then_block,
            std::unique_ptr<BoundBlockStatement> else_block, SourceSpan span) noexcept;
        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        std::unique_ptr<BoundExpression> Condition;
        std::unique_ptr<BoundBlockStatement> Then;
        std::unique_ptr<BoundBlockStatement> Else;
        SourceSpan StatementSpan;
    };

    class BoundWhileStatement final : public BoundStatement {
    public:
        BoundWhileStatement(std::unique_ptr<BoundExpression> condition,
            std::unique_ptr<BoundBlockStatement> body, SourceSpan span) noexcept;
        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;
        std::unique_ptr<BoundExpression> Condition;
        std::unique_ptr<BoundBlockStatement> Body;
        SourceSpan StatementSpan;
    };

    class BoundErrorStatement final : public BoundStatement {
    public:
        explicit BoundErrorStatement(SourceSpan span) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        SourceSpan ErrorSpan;
    };

    class BoundBlockStatement final : public BoundStatement {
    public:
        BoundBlockStatement(
            std::vector<std::unique_ptr<BoundStatement>> statements,
            SourceSpan span
        ) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        std::vector<std::unique_ptr<BoundStatement>> Statements;
        SourceSpan BlockSpan;
    };

    class BoundFunction final : public BoundNode {
    public:
        BoundFunction(
            FunctionSymbol symbol,
            std::unique_ptr<BoundBlockStatement> body,
            std::vector<LocalSymbol> locals,
            bool is_entry_point,
            SourceSpan span
        ) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        FunctionSymbol Symbol;
        std::unique_ptr<BoundBlockStatement> Body;
        std::vector<LocalSymbol> Locals;
        bool IsEntryPoint;
        SourceSpan FunctionSpan;
    };

    class BoundCompilationUnit final : public BoundNode {
    public:
        BoundCompilationUnit(
            std::string module_name,
            std::vector<std::unique_ptr<BoundFunction>> functions,
            SourceSpan span
        ) noexcept;

        [[nodiscard]] BoundNodeKind Kind() const noexcept override;
        [[nodiscard]] SourceSpan Span() const noexcept override;

        std::string ModuleName;
        std::vector<std::unique_ptr<BoundFunction>> Functions;
        SourceSpan UnitSpan;
    };
}
