#include "diagnostics/Diagnostics.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
    [[nodiscard]] char SeverityPrefix(Rivet::DiagnosticSeverity severity) {
        switch (severity) {
            case Rivet::DiagnosticSeverity::Error: return 'E';
            case Rivet::DiagnosticSeverity::Warning: return 'W';
            case Rivet::DiagnosticSeverity::Note: return 'N';
        }
        return '?';
    }
}

namespace Rivet {
    void DiagnosticBag::Report(DiagnosticCode code, DiagnosticSeverity severity, SourceSpan span, std::string message) {
        m_Diagnostics.push_back(Diagnostic{
            .Code = code,
            .Severity = severity,
            .Span = span,
            .Message = std::move(message),
        });
    }

    std::span<const Diagnostic> DiagnosticBag::Items() const noexcept {
        return std::span<const Diagnostic>{ m_Diagnostics };
    }

    std::size_t DiagnosticBag::Size() const noexcept {
        return m_Diagnostics.size();
    }

    bool DiagnosticBag::Empty() const noexcept {
        return m_Diagnostics.empty();
    }

    bool DiagnosticBag::HasErrors() const noexcept {
        return std::any_of(
            m_Diagnostics.begin(),
            m_Diagnostics.end(),
            [](const Diagnostic& diagnostic) {
                return diagnostic.Severity == DiagnosticSeverity::Error;
            }
        );
    }

    std::string FormatDiagnosticCode(DiagnosticSeverity severity, DiagnosticCode code) {
        std::ostringstream output;
        output << SeverityPrefix(severity) << std::setfill('0') << std::setw(4) << static_cast<std::uint16_t>(code);
        return output.str();
    }

    std::string RenderCompactDiagnostic(const SourceFile& source, const Diagnostic& diagnostic) {
        if (!source.Contains(diagnostic.Span)) 
            throw std::out_of_range{ "diagnostic span lies outside its source file" };
        
        const SourcePosition beginning = source.PositionAt(diagnostic.Span.Offset);
        const SourcePosition ending = source.PositionAt(diagnostic.Span.End());

        std::ostringstream output;
        output << FormatDiagnosticCode(diagnostic.Severity, diagnostic.Code)
            << ' ' << beginning.Line + 1 << ':' << beginning.Column + 1
            << '-' << ending.Line + 1 << ':' << ending.Column + 1 << ' ' << diagnostic.Message;

        return output.str();
    }

    std::string RenderCompactDiagnostics(const SourceFile& source, std::span<const Diagnostic> diagnostics) {
        std::ostringstream output;
        for (const Diagnostic& diagnostic : diagnostics) 
            output << RenderCompactDiagnostic(source, diagnostic) << '\n';
        return output.str();
    }
}
