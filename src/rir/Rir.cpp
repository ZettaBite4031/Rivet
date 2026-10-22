#include "rir/Rir.hpp"

#include <cassert>
#include <ostream>
#include <string>
#include <utility>

namespace {
    using namespace Rivet;
    RirValueType ValueType(const TypeSymbol& type) {
        return type.Kind == TypeKind::Bool ? RirValueType::Bool : RirValueType::Int;
    }
    const char* TypeName(RirValueType type) { return type == RirValueType::Bool ? "bool" : "int"; }

    class FunctionLowerer final {
    public:
        explicit FunctionLowerer(const BoundFunction& source) {
            m_Function.Name = source.Symbol.Name;
            m_Function.ParameterCount = source.Symbol.ParameterCount;
            m_NextValue = source.Symbol.ParameterCount;
            for (const LocalSymbol& local : source.Locals)
                m_Function.LocalTypes.push_back(ValueType(*local.Type));
            for (std::size_t i = 0; i < m_Function.ParameterCount; ++i)
                m_Function.Instructions.push_back(RirInstruction{
                    .Op = RirOp::StoreLocal, .Left = i, .Slot = i });
        }
        RirFunction Lower(const BoundBlockStatement& body) {
            Block(body);
            return std::move(m_Function);
        }
    private:
        std::size_t Emit(RirInstruction instruction) {
            instruction.Result = m_NextValue++;
            const auto value = instruction.Result;
            m_Function.Instructions.push_back(std::move(instruction));
            return value;
        }
        std::size_t LabelId() { return m_NextLabel++; }
        void Label(std::size_t id) {
            m_Function.Instructions.push_back(RirInstruction{ .Op = RirOp::Label, .Target = id });
            m_Terminated = false;
        }
        void Jump(std::size_t id) {
            m_Function.Instructions.push_back(RirInstruction{ .Op = RirOp::Jump, .Target = id });
            m_Terminated = true;
        }
        void Block(const BoundBlockStatement& block) {
            for (const auto& statement : block.Statements) {
                if (m_Terminated) break;
                Statement(*statement);
            }
        }
        void Statement(const BoundStatement& statement) {
            switch (statement.Kind()) {
            case BoundNodeKind::BlockStatement:
                Block(static_cast<const BoundBlockStatement&>(statement)); break;
            case BoundNodeKind::LocalDeclaration: {
                const auto& declaration = static_cast<const BoundLocalDeclaration&>(statement);
                const auto value = Expression(*declaration.Initializer);
                m_Function.Instructions.push_back(RirInstruction{
                    .Op = RirOp::StoreLocal, .Left = value, .Span = statement.Span(), .Slot = declaration.Local });
                break;
            }
            case BoundNodeKind::AssignmentStatement: {
                const auto& assignment = static_cast<const BoundAssignmentStatement&>(statement);
                const auto value = Expression(*assignment.Value);
                m_Function.Instructions.push_back(RirInstruction{
                    .Op = RirOp::StoreLocal, .Left = value, .Span = statement.Span(), .Slot = assignment.Local });
                break;
            }
            case BoundNodeKind::ReturnStatement: {
                const auto& returned = static_cast<const BoundReturnStatement&>(statement);
                const auto value = Expression(*returned.Expression);
                m_Function.Instructions.push_back(RirInstruction{
                    .Op = RirOp::Return, .Left = value, .Span = statement.Span() });
                m_Terminated = true;
                break;
            }
            case BoundNodeKind::IfStatement: {
                const auto& branch = static_cast<const BoundIfStatement&>(statement);
                const auto condition = Expression(*branch.Condition);
                const auto then_id = LabelId();
                const auto end_id = LabelId();
                const auto else_id = branch.Else ? LabelId() : end_id;
                m_Function.Instructions.push_back(RirInstruction{
                    .Op = RirOp::Branch, .Left = condition, .Span = branch.Condition->Span(),
                    .Target = then_id, .FalseTarget = else_id });
                m_Terminated = true;
                Label(then_id);
                Block(*branch.Then);
                const bool then_falls_through = !m_Terminated;
                if (then_falls_through) Jump(end_id);
                bool else_falls_through = false;
                if (branch.Else) {
                    Label(else_id);
                    Block(*branch.Else);
                    else_falls_through = !m_Terminated;
                    if (else_falls_through) Jump(end_id);
                }
                if (!branch.Else || then_falls_through || else_falls_through) Label(end_id);
                break;
            }
            case BoundNodeKind::WhileStatement: {
                const auto& loop = static_cast<const BoundWhileStatement&>(statement);
                const auto test_id = LabelId(), body_id = LabelId(), end_id = LabelId();
                Jump(test_id);
                Label(test_id);
                const auto condition = Expression(*loop.Condition);
                m_Function.Instructions.push_back(RirInstruction{
                    .Op = RirOp::Branch, .Left = condition, .Span = loop.Condition->Span(),
                    .Target = body_id, .FalseTarget = end_id });
                m_Terminated = true;
                Label(body_id);
                Block(*loop.Body);
                if (!m_Terminated) Jump(test_id);
                Label(end_id);
                break;
            }
            default: assert(false && "invalid bound statement reached RIR lowering"); break;
            }
        }
        std::size_t Expression(const BoundExpression& expression) {
            switch (expression.Kind()) {
            case BoundNodeKind::LiteralExpression: {
                const auto& literal = static_cast<const BoundLiteralExpression&>(expression);
                return Emit(RirInstruction{ .Op = RirOp::Const, .Immediate = literal.Value,
                    .Span = literal.Span(), .Type = ValueType(literal.Type()) });
            }
            case BoundNodeKind::LocalExpression: {
                const auto& local = static_cast<const BoundLocalExpression&>(expression);
                return Emit(RirInstruction{ .Op = RirOp::LoadLocal, .Span = local.Span(),
                    .Type = ValueType(local.Type()), .Slot = local.Local });
            }
            case BoundNodeKind::CallExpression: {
                const auto& call = static_cast<const BoundCallExpression&>(expression);
                std::vector<std::size_t> arguments;
                for (const auto& arg : call.Arguments) arguments.push_back(Expression(*arg));
                return Emit(RirInstruction{ .Op = RirOp::Call, .Span = call.Span(),
                    .Callee = call.Callee, .Arguments = std::move(arguments) });
            }
            case BoundNodeKind::UnaryExpression: {
                const auto& unary = static_cast<const BoundUnaryExpression&>(expression);
                const auto value = Expression(*unary.Operand);
                const RirOp op = unary.OperatorKind == BoundUnaryOperatorKind::Not ? RirOp::Not :
                    unary.OperatorKind == BoundUnaryOperatorKind::Negation ? RirOp::Negate : RirOp::Identity;
                return Emit(RirInstruction{ .Op = op, .Left = value, .Span = unary.OperatorSpan,
                    .Type = ValueType(unary.Type()) });
            }
            case BoundNodeKind::BinaryExpression: {
                const auto& binary = static_cast<const BoundBinaryExpression&>(expression);
                const auto left = Expression(*binary.Left), right = Expression(*binary.Right);
                RirOp op = RirOp::Add;
                switch (binary.OperatorKind) {
                case BoundBinaryOperatorKind::Addition: op = RirOp::Add; break;
                case BoundBinaryOperatorKind::Subtraction: op = RirOp::Subtract; break;
                case BoundBinaryOperatorKind::Multiplication: op = RirOp::Multiply; break;
                case BoundBinaryOperatorKind::Division: op = RirOp::Divide; break;
                case BoundBinaryOperatorKind::Less: op = RirOp::Less; break;
                case BoundBinaryOperatorKind::LessEquals: op = RirOp::LessEquals; break;
                case BoundBinaryOperatorKind::Greater: op = RirOp::Greater; break;
                case BoundBinaryOperatorKind::GreaterEquals: op = RirOp::GreaterEquals; break;
                case BoundBinaryOperatorKind::Equals: op = RirOp::Equals; break;
                case BoundBinaryOperatorKind::NotEquals: op = RirOp::NotEquals; break;
                }
                return Emit(RirInstruction{ .Op = op, .Left = left, .Right = right,
                    .Span = binary.OperatorSpan, .Type = ValueType(binary.Type()) });
            }
            default: assert(false && "invalid bound expression reached RIR lowering"); return 0;
            }
        }
        RirFunction m_Function;
        std::size_t m_NextValue{ 0 }, m_NextLabel{ 0 };
        bool m_Terminated{ false };
    };

    const char* OperationName(RirOp op) {
        switch (op) {
        case RirOp::Const: return "const";
        case RirOp::Identity: return "identity";
        case RirOp::Negate: return "negate";
        case RirOp::Add: return "add";
        case RirOp::Subtract: return "subtract";
        case RirOp::Multiply: return "multiply";
        case RirOp::Divide: return "divide";
        case RirOp::Call: return "call";
        case RirOp::Not: return "not";
        case RirOp::Less: return "less";
        case RirOp::LessEquals: return "less_equals";
        case RirOp::Greater: return "greater";
        case RirOp::GreaterEquals: return "greater_equals";
        case RirOp::Equals: return "equals";
        case RirOp::NotEquals: return "not_equals";
        default: return "?";
        }
    }
}

namespace Rivet {
    RirModule LowerToRir(const BoundCompilationUnit& unit) {
        RirModule result{ .Name = unit.ModuleName };
        for (const auto& function : unit.Functions) {
            FunctionLowerer lowerer{ *function };
            result.Functions.push_back(lowerer.Lower(*function->Body));
        }
        return result;
    }

    void WriteRir(std::ostream& output, const RirModule& module) {
        for (const RirFunction& function : module.Functions) {
            output << "function int @" << function.Name << '(';
            for (std::size_t i = 0; i < function.ParameterCount; ++i) {
                if (i) output << ", ";
                output << "int $" << i;
            }
            output << ")\n{\nentry:\n";
            for (std::size_t i = 0; i < function.LocalTypes.size(); ++i)
                output << "    slot " << TypeName(function.LocalTypes[i]) << ' ' << i << '\n';
            for (const RirInstruction& instruction : function.Instructions) {
                if (instruction.Op == RirOp::Label) {
                    output << "block" << instruction.Target << ":\n"; continue;
                }
                output << "    ";
                switch (instruction.Op) {
                case RirOp::Return: output << "return $" << instruction.Left << '\n'; continue;
                case RirOp::Jump: output << "jump block" << instruction.Target << '\n'; continue;
                case RirOp::Branch:
                    output << "branch $" << instruction.Left << ", block" << instruction.Target
                        << ", block" << instruction.FalseTarget << '\n'; continue;
                case RirOp::StoreLocal:
                    output << "store slot" << instruction.Slot << ", $" << instruction.Left << '\n'; continue;
                default: break;
                }
                output << TypeName(instruction.Type) << " $" << instruction.Result << " = ";
                if (instruction.Op == RirOp::LoadLocal) output << "load slot" << instruction.Slot;
                else {
                    output << OperationName(instruction.Op);
                    if (instruction.Op == RirOp::Const)
                        output << ' ' << (instruction.Type == RirValueType::Bool
                            ? (instruction.Immediate ? "true" : "false") : std::to_string(instruction.Immediate));
                    else if (instruction.Op == RirOp::Call) {
                        output << " @" << instruction.Callee << '(';
                        for (std::size_t i = 0; i < instruction.Arguments.size(); ++i) {
                            if (i) output << ", ";
                            output << '$' << instruction.Arguments[i];
                        }
                        output << ')';
                    } else if (instruction.Op == RirOp::Negate || instruction.Op == RirOp::Identity ||
                        instruction.Op == RirOp::Not) output << " $" << instruction.Left;
                    else output << " $" << instruction.Left << ", $" << instruction.Right;
                }
                output << '\n';
            }
            output << "}\n";
        }
    }
}
