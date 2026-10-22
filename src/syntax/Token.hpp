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
        TrueKeyword,
        FalseKeyword,

        ModuleKeyword,
        IntKeyword,
        BoolKeyword,
        MutKeyword,
        ReturnKeyword,
        IfKeyword,
        ElseKeyword,
        WhileKeyword,

        OpenParen,
        CloseParen,
        OpenBrace,
        CloseBrace,
        Semicolon,
        Comma,
        Dot,
        Equals,
        EqualsEquals,
        Bang,
        BangEquals,
        Less,
        LessEquals,
        Greater,
        GreaterEquals,

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
