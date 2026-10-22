#include "binding/BoundPrinter.hpp"

#include <cstddef>
#include <ostream>
#include <vector>

namespace {
    void WriteIndent(std::ostream& output, std::size_t depth) {
        for (std::size_t index = 0; index < depth; index++)
            output << "  ";
    }

    void WriteExpression(
        std::ostream& output,
        const Rivet::BoundExpression& expression,
        std::size_t depth,
        const std::vector<Rivet::LocalSymbol>& locals
    ) {
        using namespace Rivet;

        WriteIndent(output, depth);

        switch (expression.Kind()) {
            case BoundNodeKind::LiteralExpression: {
                const auto& literal =
                    static_cast<const BoundLiteralExpression&>(expression);
                output << "Literal " << literal.Value
                    << " : " << literal.Type().Name << '\n';
                break;
            }

            case BoundNodeKind::LocalExpression: {
                const auto& local = static_cast<const BoundLocalExpression&>(expression);
                output << "Local " << locals.at(local.Local).Name << " : int\n";
                break;
            }

            case BoundNodeKind::UnaryExpression: {
                const auto& unary =
                    static_cast<const BoundUnaryExpression&>(expression);
                output << "Unary "
                    << BoundUnaryOperatorKindName(unary.OperatorKind)
                    << " : " << unary.Type().Name << '\n';
                WriteExpression(output, *unary.Operand, depth + 1, locals);
                break;
            }

            case BoundNodeKind::BinaryExpression: {
                const auto& binary =
                    static_cast<const BoundBinaryExpression&>(expression);
                output << "Binary "
                    << BoundBinaryOperatorKindName(binary.OperatorKind)
                    << " : " << binary.Type().Name << '\n';
                WriteExpression(output, *binary.Left, depth + 1, locals);
                WriteExpression(output, *binary.Right, depth + 1, locals);
                break;
            }

            case BoundNodeKind::ErrorExpression:
                output << "Error : " << expression.Type().Name << '\n';
                break;

            default:
                output << "<unexpected expression>\n";
                break;
        }
    }

    void WriteStatement(
        std::ostream& output,
        const Rivet::BoundStatement& statement,
        std::size_t depth,
        const std::vector<Rivet::LocalSymbol>& locals
    ) {
        using namespace Rivet;

        WriteIndent(output, depth);

        switch (statement.Kind()) {
            case BoundNodeKind::BlockStatement: {
                const auto& block =
                    static_cast<const BoundBlockStatement&>(statement);
                output << "Block\n";
                for (const auto& child : block.Statements)
                    WriteStatement(output, *child, depth + 1, locals);
                break;
            }

            case BoundNodeKind::LocalDeclaration: {
                const auto& declared = static_cast<const BoundLocalDeclaration&>(statement);
                const auto& symbol = locals.at(declared.Local);
                output << (symbol.Mutable ? "MutLocal " : "LocalDecl ")
                    << symbol.Name << " : " << symbol.Type->Name << '\n';
                WriteExpression(output, *declared.Initializer, depth + 1, locals);
                break;
            }

            case BoundNodeKind::AssignmentStatement: {
                const auto& assigned = static_cast<const BoundAssignmentStatement&>(statement);
                output << "Assign " << locals.at(assigned.Local).Name << "\n";
                WriteExpression(output, *assigned.Value, depth + 1, locals);
                break;
            }

            case BoundNodeKind::ReturnStatement: {
                const auto& return_statement =
                    static_cast<const BoundReturnStatement&>(statement);
                output << "Return : "
                    << return_statement.Expression->Type().Name
                    << '\n';
                WriteExpression(
                    output,
                    *return_statement.Expression,
                    depth + 1,
                    locals
                );
                break;
            }

            case BoundNodeKind::ErrorStatement:
                output << "ErrorStatement\n";
                break;

            default:
                output << "<unexpected statement>\n";
                break;
        }
    }
}

namespace Rivet {
    void WriteBoundTree(
        std::ostream& output,
        const BoundCompilationUnit& unit
    ) {
        output << "Module " << unit.ModuleName << '\n';

        for (const auto& function : unit.Functions) {
            WriteIndent(output, 1);
            output << "Function " << function->Symbol.Name
                << "() -> " << function->Symbol.ReturnType->Name;

            if (function->IsEntryPoint)
                output << " [entry]";

            output << '\n';
            WriteStatement(output, *function->Body, 2, function->Locals);
        }
    }
}
