#pragma once

#include "source/Source.hpp"

#include <cstdint>
#include <iosfwd>
#include <string_view>

namespace Rivet {
    enum class TokenKind : std::uint8_t {
        Invalid,
        EndOfFile,

        Identifier,
        IntegerLiteral,

        ModuleKeyword,
        IntKeyword,
        MutKeyword,
        ReturnKeyword,

        OpenParen,
        CloseParen,
        OpenBrace,
        CloseBrace,
        Semicolon,
        Dot,
        Equals,

        Plus,
        Minus,
        Star,
        Slash
    };

    struct Token {
        TokenKind Kind;
        SourceSpan Span;

        [[nodiscard]] std::string_view Text(const SourceFile& source) const noexcept;
    };

    [[nodiscard]] std::string_view TokenKindName(TokenKind kind) noexcept;

    std::ostream& operator<<(std::ostream& output, TokenKind kind);
}
