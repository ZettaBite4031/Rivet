#include "rir/RirInterpreter.hpp"

#include <cassert>
#include <cstdint>
#include <limits>
#include <unordered_map>

namespace Rivet {
    std::expected<std::int32_t, RuntimeFault> RunRirMain(const RirModule& module) {
        for (const RirFunction& function : module.Functions) {
            if (function.Name != "main") continue;
            std::unordered_map<std::size_t, std::int32_t> registers;
            for (const RirInstruction& instruction : function.Instructions) {
                if (instruction.Op == RirOp::Return) return registers.at(instruction.Left);

                std::int64_t result = 0;
                switch (instruction.Op) {
                case RirOp::Const: result = instruction.Immediate; break;
                case RirOp::Identity: result = registers.at(instruction.Left); break;
                case RirOp::Negate: result = -static_cast<std::int64_t>(registers.at(instruction.Left)); break;
                case RirOp::Add: result = static_cast<std::int64_t>(registers.at(instruction.Left)) + registers.at(instruction.Right); break;
                case RirOp::Subtract: result = static_cast<std::int64_t>(registers.at(instruction.Left)) - registers.at(instruction.Right); break;
                case RirOp::Multiply: result = static_cast<std::int64_t>(registers.at(instruction.Left)) * registers.at(instruction.Right); break;
                case RirOp::Divide: {
                    const std::int32_t divisor = registers.at(instruction.Right);
                    if (divisor == 0) return std::unexpected(RuntimeFault{ "R0001", instruction.Span, "integer division by zero" });
                    result = static_cast<std::int64_t>(registers.at(instruction.Left)) / divisor;
                    break;
                }
                case RirOp::Return: assert(false); break;
                }
                if (result < std::numeric_limits<std::int32_t>::min() || result > std::numeric_limits<std::int32_t>::max())
                    return std::unexpected(RuntimeFault{ "R0002", instruction.Span, "integer arithmetic overflow" });
                registers.emplace(instruction.Result, static_cast<std::int32_t>(result));
            }
            assert(false && "verifier permitted a function without a return");
        }
        assert(false && "binder permitted a program without main");
        return std::unexpected(RuntimeFault{ "R0003", {}, "no entry point" });
    }
}
