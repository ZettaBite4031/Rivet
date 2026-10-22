#include "rir/Rir.hpp"

#include <cassert>
#include <ostream>
#include <utility>

namespace {
    class FunctionLowerer final {
    public:
        FunctionLowerer(std::string name, std::size_t parameter_count) {
            m_Function.Name = std::move(name);
            m_Function.ParameterCount = parameter_count;
            m_NextValue = parameter_count;
            for (std::size_t index = 0; index < parameter_count; ++index)
                m_LocalValues.push_back(index);
        }

        Rivet::RirFunction Lower(const Rivet::BoundBlockStatement& body) {
            for (const auto& statement : body.Statements) {
                using namespace Rivet;
                switch (statement->Kind()) {
                case BoundNodeKind::LocalDeclaration: {
                    const auto& declared = static_cast<const BoundLocalDeclaration&>(*statement);
                    assert(declared.Local == m_LocalValues.size());
                    m_LocalValues.push_back(Expression(*declared.Initializer));
                    break;
                }
                case BoundNodeKind::AssignmentStatement: {
                    const auto& assigned = static_cast<const BoundAssignmentStatement&>(*statement);
                    const std::size_t value = Expression(*assigned.Value);
                    assert(assigned.Local < m_LocalValues.size());
                    m_LocalValues[assigned.Local] = value;
                    break;
                }
                case BoundNodeKind::ReturnStatement: {
                    const auto& returned = static_cast<const BoundReturnStatement&>(*statement);
                    const std::size_t value = Expression(*returned.Expression);
                    m_Function.Instructions.push_back(RirInstruction{
                        .Op = RirOp::Return, .Left = value, .Span = returned.Span(),
                    });
                    return std::move(m_Function);
                }
                default:
                    assert(false && "invalid bound statement reached RIR lowering");
                    break;
                }
            }
            return std::move(m_Function);
        }

    private:
        std::size_t Emit(Rivet::RirInstruction instruction) {
            instruction.Result = m_NextValue++;
            const std::size_t result = instruction.Result;
            m_Function.Instructions.push_back(instruction);
            return result;
        }

        std::size_t Expression(const Rivet::BoundExpression& expression) {
            using namespace Rivet;
            switch (expression.Kind()) {
            case BoundNodeKind::LiteralExpression: {
                const auto& literal = static_cast<const BoundLiteralExpression&>(expression);
                return Emit(RirInstruction{ .Op = RirOp::Const, .Immediate = literal.Value,
                    .Span = literal.Span() });
            }
            case BoundNodeKind::LocalExpression: {
                const auto& local = static_cast<const BoundLocalExpression&>(expression);
                assert(local.Local < m_LocalValues.size());
                return m_LocalValues[local.Local];
            }
            case BoundNodeKind::CallExpression: {
                const auto& call = static_cast<const BoundCallExpression&>(expression);
                std::vector<std::size_t> arguments;
                arguments.reserve(call.Arguments.size());
                for (const auto& argument : call.Arguments)
                    arguments.push_back(Expression(*argument));
                return Emit(RirInstruction{ .Op = RirOp::Call, .Span = call.Span(),
                    .Callee = call.Callee, .Arguments = std::move(arguments) });
            }
            case BoundNodeKind::UnaryExpression: {
                const auto& unary = static_cast<const BoundUnaryExpression&>(expression);
                const std::size_t operand = Expression(*unary.Operand);
                return Emit(RirInstruction{ .Op = unary.OperatorKind == BoundUnaryOperatorKind::Negation
                    ? RirOp::Negate : RirOp::Identity, .Left = operand, .Span = unary.OperatorSpan });
            }
            case BoundNodeKind::BinaryExpression: {
                const auto& binary = static_cast<const BoundBinaryExpression&>(expression);
                const std::size_t left = Expression(*binary.Left);
                const std::size_t right = Expression(*binary.Right);
                RirOp op = RirOp::Add;
                switch (binary.OperatorKind) {
                case BoundBinaryOperatorKind::Addition:       op = RirOp::Add; break;
                case BoundBinaryOperatorKind::Subtraction:    op = RirOp::Subtract; break;
                case BoundBinaryOperatorKind::Multiplication: op = RirOp::Multiply; break;
                case BoundBinaryOperatorKind::Division:       op = RirOp::Divide; break;
                }
                return Emit(RirInstruction{ .Op = op, .Left = left, .Right = right,
                    .Span = binary.OperatorSpan });
            }
            default:
                assert(false && "invalid bound expression reached RIR lowering");
                return 0;
            }
        }

        Rivet::RirFunction m_Function;
        std::size_t m_NextValue{ 0 };
        std::vector<std::size_t> m_LocalValues;
    };

    const char* OperationName(Rivet::RirOp op) {
        using Rivet::RirOp;
        switch (op) {
        case RirOp::Add: return "add";
        case RirOp::Subtract: return "subtract";
        case RirOp::Multiply: return "multiply";
        case RirOp::Divide: return "divide";
        case RirOp::Identity: return "identity";
        case RirOp::Negate: return "negate";
        case RirOp::Const: return "const";
        case RirOp::Call: return "call";
        case RirOp::Return: return "return";
        }
        return "?";
    }
}

namespace Rivet {
    RirModule LowerToRir(const BoundCompilationUnit& unit) {
        RirModule result{ .Name = unit.ModuleName };
        for (const auto& function : unit.Functions) {
            FunctionLowerer lowerer{ function->Symbol.Name, function->Symbol.ParameterCount };
            result.Functions.push_back(lowerer.Lower(*function->Body));
        }
        return result;
    }

    void WriteRir(std::ostream& output, const RirModule& module) {
        for (const RirFunction& function : module.Functions) {
            output << "function int @" << function.Name << '(';
            for (std::size_t index = 0; index < function.ParameterCount; ++index) {
                if (index) output << ", ";
                output << "int $" << index;
            }
            output << ")\n{\nentry:\n";
            for (const RirInstruction& instruction : function.Instructions) {
                output << "    ";
                if (instruction.Op == RirOp::Return) {
                    output << "return $" << instruction.Left << '\n';
                    continue;
                }
                output << "int $" << instruction.Result << " = " << OperationName(instruction.Op);
                if (instruction.Op == RirOp::Const) output << ' ' << instruction.Immediate;
                else if (instruction.Op == RirOp::Call) {
                    output << " @" << instruction.Callee << '(';
                    for (std::size_t index = 0; index < instruction.Arguments.size(); ++index) {
                        if (index) output << ", ";
                        output << '$' << instruction.Arguments[index];
                    }
                    output << ')';
                }
                else if (instruction.Op == RirOp::Negate || instruction.Op == RirOp::Identity)
                    output << " $" << instruction.Left;
                else output << " $" << instruction.Left << ", $" << instruction.Right;
                output << '\n';
            }
            output << "}\n";
        }
    }
}
