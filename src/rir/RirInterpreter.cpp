#include "rir/RirInterpreter.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    using Rivet::RirFunction;
    using Rivet::RirInstruction;
    using Rivet::RirModule;
    using Rivet::RirOp;
    using Rivet::RuntimeFault;

    using Functions = std::unordered_map<std::string, const RirFunction*>;
    constexpr std::size_t MaxCallDepth = 256;

    std::expected<std::int32_t, RuntimeFault> RunFunction(
        const RirFunction& function, const std::vector<std::int32_t>& arguments,
        const Functions& functions, std::size_t depth, Rivet::SourceSpan call_span
    ) {
        if (depth >= MaxCallDepth)
            return std::unexpected(RuntimeFault{ "R0004", call_span, "call depth limit exceeded" });

        std::unordered_map<std::size_t, std::int32_t> registers;
        for (std::size_t index = 0; index < arguments.size(); ++index)
            registers.emplace(index, arguments[index]);

        for (const RirInstruction& instruction : function.Instructions) {
            if (instruction.Op == RirOp::Return) return registers.at(instruction.Left);

            std::int64_t result = 0;
            switch (instruction.Op) {
            case RirOp::Const: result = instruction.Immediate; break;
            case RirOp::Identity: result = registers.at(instruction.Left); break;
            case RirOp::Negate: result = -static_cast<std::int64_t>(registers.at(instruction.Left)); break;
            case RirOp::Add: result = static_cast<std::int64_t>(registers.at(instruction.Left))
                + registers.at(instruction.Right); break;
            case RirOp::Subtract: result = static_cast<std::int64_t>(registers.at(instruction.Left))
                - registers.at(instruction.Right); break;
            case RirOp::Multiply: result = static_cast<std::int64_t>(registers.at(instruction.Left))
                * registers.at(instruction.Right); break;
            case RirOp::Divide: {
                const std::int32_t divisor = registers.at(instruction.Right);
                if (divisor == 0)
                    return std::unexpected(RuntimeFault{ "R0001", instruction.Span,
                        "integer division by zero" });
                result = static_cast<std::int64_t>(registers.at(instruction.Left)) / divisor;
                break;
            }
            case RirOp::Call: {
                std::vector<std::int32_t> passed;
                passed.reserve(instruction.Arguments.size());
                for (std::size_t argument : instruction.Arguments)
                    passed.push_back(registers.at(argument));
                const auto value = RunFunction(*functions.at(instruction.Callee), passed,
                    functions, depth + 1, instruction.Span);
                if (!value) return std::unexpected(value.error());
                result = *value;
                break;
            }
            case RirOp::Return: assert(false); break;
            }
            if (result < std::numeric_limits<std::int32_t>::min()
                || result > std::numeric_limits<std::int32_t>::max())
                return std::unexpected(RuntimeFault{ "R0002", instruction.Span,
                    "integer arithmetic overflow" });
            registers.emplace(instruction.Result, static_cast<std::int32_t>(result));
        }
        assert(false && "verifier permitted a function without a return");
        return std::unexpected(RuntimeFault{ "R0003", {}, "function has no return" });
    }
}

namespace Rivet {
    std::expected<std::int32_t, RuntimeFault> RunRirMain(const RirModule& module) {
        Functions functions;
        for (const RirFunction& function : module.Functions)
            functions.emplace(function.Name, &function);
        const auto main = functions.find("main");
        if (main == functions.end())
            return std::unexpected(RuntimeFault{ "R0003", {}, "no entry point" });
        return RunFunction(*main->second, {}, functions, 0, {});
    }
}
