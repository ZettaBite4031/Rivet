#pragma once

#include "binding/Bound.hpp"
#include "source/Source.hpp"

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace Rivet {
    enum class RirOp : std::uint8_t {
        Const, Identity, Negate, Add, Subtract, Multiply, Divide, Call, Return,
    };

    // The lowerer emits dense virtual registers. The verifier also accepts
    // sparse names, but requires every use to refer to a prior definition.
    struct RirInstruction {
        RirOp Op;
        std::size_t Result{ 0 };
        std::size_t Left{ 0 };
        std::size_t Right{ 0 };
        std::int32_t Immediate{ 0 };
        SourceSpan Span{};
        std::string Callee{};
        std::vector<std::size_t> Arguments{};
    };

    struct RirFunction {
        std::string Name;
        std::vector<RirInstruction> Instructions{};
        std::size_t ParameterCount{ 0 };
    };

    struct RirModule {
        std::string Name;
        std::vector<RirFunction> Functions{};
    };

    [[nodiscard]] RirModule LowerToRir(const BoundCompilationUnit& unit);
    void WriteRir(std::ostream& output, const RirModule& module);
}
