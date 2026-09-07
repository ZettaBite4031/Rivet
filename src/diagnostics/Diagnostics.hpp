#pragma once

#include "source/Source.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Rivet {
    enum class DiagnosticSeverity : std::uint8_t {
        Error, 
        Warning,
    };

    enum class DiagnosticCode : std::uint16_t {
        InvalidCharacter = 1,
    };

    struct Diagnostic {
        DiagnosticCode Code;
        DiagnosticSeverity Severity;
        SourceSpan Span;
        std::string Message;
    };

    class DiagnosticBag final {
    public:
        void Report(DiagnosticCode code, DiagnosticSeverity severity, SourceSpan span, std::string message);

        [[nodiscard]] std::span<const Diagnostic> Items() const noexcept;

        [[nodiscard]] std::size_t Size() const noexcept;
        [[nodiscard]] bool Empty() const noexcept;
        [[nodiscard]] bool HasErrors() const noexcept;

    private:    
        std::vector<Diagnostic> m_Diagnostics;
    };

    [[nodiscard]] std::string FormatDiagnosticCode(DiagnosticSeverity severity, DiagnosticCode code);

    [[nodiscard]] std::string RenderCompactDiagnostic(const SourceFile& source, const Diagnostic& diagnostic);

    [[nodiscard]] std::string RenderCompactDiagnostics(const SourceFile& source, std::span<const Diagnostic> diagnostics);
}