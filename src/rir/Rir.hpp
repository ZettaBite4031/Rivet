#pragma once

#include "binding/Bound.hpp"
#include "source/Source.hpp"

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace Rivet {
    enum class RirValueType : std::uint8_t { Int, Bool };
    enum class RirOp : std::uint8_t {
        Const, Identity, Negate, Add, Subtract, Multiply, Divide, Call, Return,
        Not, Less, LessEquals, Greater, GreaterEquals, Equals, NotEquals,
        LoadLocal, StoreLocal, Label, Jump, Branch,
    };

    // Registers hold expression values within a block. Local slots carry
    // mutable state across branch and loop edges.
    struct RirInstruction {
        RirOp Op;
        std::size_t Result{ 0 };
        std::size_t Left{ 0 };
        std::size_t Right{ 0 };
        std::int32_t Immediate{ 0 };
        SourceSpan Span{};
        std::string Callee{};
        std::vector<std::size_t> Arguments{};
        RirValueType Type{ RirValueType::Int };
        std::size_t Slot{ 0 };
        std::size_t Target{ 0 };
        std::size_t FalseTarget{ 0 };
    };

    struct RirFunction {
        std::string Name;
        std::size_t ParameterCount{ 0 };
        std::vector<RirInstruction> Instructions{};
        std::vector<RirValueType> LocalTypes{};
    };

    struct RirModule {
        std::string Name;
        std::vector<RirFunction> Functions{};
    };

    [[nodiscard]] RirModule LowerToRir(const BoundCompilationUnit& unit);
    void WriteRir(std::ostream& output, const RirModule& module);
}
