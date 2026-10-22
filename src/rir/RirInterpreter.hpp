#pragma once

#include "rir/Rir.hpp"

#include <cstdint>
#include <expected>
#include <string>

namespace Rivet {
    struct RuntimeFault {
        std::string Code;
        SourceSpan Span;
        std::string Message;
    };

    // Precondition: VerifiyRir(module).empty(). The driver enforces it.
    [[nodiscard]] std::expected<std::int32_t, RuntimeFault> RunRirMain(const RirModule& module);
}
