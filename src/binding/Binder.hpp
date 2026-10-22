#pragma once

#include "binding/Bound.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "source/Source.hpp"
#include "syntax/Syntax.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Rivet {
    class Binder final {
    public:
        Binder(const SourceFile& source, DiagnosticBag& diagnostics) noexcept;

        [[nodiscard]] std::unique_ptr<BoundCompilationUnit> BindCompilationUnit(const CompilationUnitSyntax& syntax);

    private:
        struct DeclaredFunction {
            FunctionSymbol Symbol;
            const FunctionDefinitionSyntax* Syntax;
        };

        void DeclareFunctions(const CompilationUnitSyntax& syntax);
        void ValidateEntryPoint(const CompilationUnitSyntax& syntax);

        [[nodiscard]] std::unique_ptr<BoundFunction> BindFunction(const DeclaredFunction& declaration);
        [[nodiscard]] std::unique_ptr<BoundBlockStatement> BindBlockStatement(const BlockStatementSyntax& syntax);
        [[nodiscard]] std::unique_ptr<BoundStatement> BindStatement(const StatementSyntax& syntax);
        [[nodiscard]] std::unique_ptr<BoundReturnStatement> BindReturnStatement(const ReturnStatementSyntax& syntax);
        [[nodiscard]] std::unique_ptr<BoundExpression> BindExpression(const ExpressionSyntax& syntax);
        [[nodiscard]] std::unique_ptr<BoundExpression> BindLiteralExpression(const LiteralExpressionSyntax& syntax);
        [[nodiscard]] std::unique_ptr<BoundExpression> BindUnaryExpression(const UnaryExpressionSyntax& syntax);
        [[nodiscard]] std::unique_ptr<BoundExpression> BindBinaryExpression(const BinaryExpressionSyntax& syntax);

        const SourceFile& m_Source;
        DiagnosticBag& m_Diagnostics;

        std::vector<DeclaredFunction> m_Declarations;
        std::unordered_map<std::string, std::size_t> m_FunctionIndices;
        bool m_HasStructurallyInvalidDeclaration{ false };
    };
}