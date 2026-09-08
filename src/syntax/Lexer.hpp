#pragma once

#include "diagnostics/Diagnostics.hpp"
#include "syntax/Token.hpp"

#include <cstddef>
#include <vector>

namespace Rivet {
    class Lexer final {
    public:
        Lexer(const SourceFile& source, DiagnosticBag& diagnostics) noexcept;

        [[nodiscard]] Token NextToken();

    private:
        [[nodiscard]] bool AtEnd() const noexcept;

        [[nodiscard]] char Current() const noexcept;

        [[nodiscard]] char Peek(std::size_t distance) const noexcept;

        void Advance() noexcept;
        void SkipTrivia() noexcept;

        [[nodiscard]] Token TokenLexIdentifierOrKeyword();

        [[nodiscard]] Token LexIntegerLiteral();

        [[nodiscard]] Token LexPunctuationOrInvalid();

        [[nodiscard]] Token MakeToken(TokenKind kind, std::size_t beginning) const noexcept;

        [[nodiscard]] static constexpr bool IsWhitespace(char character) noexcept;
        
        [[nodiscard]] static constexpr bool IsDigit(char character) noexcept;

        [[nodiscard]] static constexpr bool IsIdentifierStart(char character) noexcept;

        [[nodiscard]] static constexpr bool IsIdentifierContinuation(char character) noexcept;

        const SourceFile& m_Source;
        DiagnosticBag& m_Diagnostics;
        std::size_t m_Position{ 0 };
    };

    [[nodiscard]] std::vector<Token> LexAll(const SourceFile& source, DiagnosticBag& diagnostics);
}