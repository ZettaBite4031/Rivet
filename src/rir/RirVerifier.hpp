#pragma once

#include "rir/Rir.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace Rivet {
    // A verifier problem denotes invalid compiler IR, not a Rivet source error.
    struct RirIssue { std::string Code; std::string Message; };
    [[nodiscard]] std::vector<RirIssue> VerifyRir(const RirModule& module);

    // Developer-only text ingestion for `verify-rir` and verifier fixtures.
    // This accepts the bootstrap printer's single-block textual subset.
    [[nodiscard]] std::vector<RirIssue> ParseRirText(std::string_view text, RirModule& output);
}
