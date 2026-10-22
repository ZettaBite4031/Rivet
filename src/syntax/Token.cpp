#include "syntax/Token.hpp"

#include <ostream>

namespace Rivet {
    std::string_view Token::Text(const SourceFile& source) const noexcept {
        return source.Text(Span);
    }

    std::string_view TokenKindName(TokenKind kind) noexcept {
        switch (kind) {
        case TokenKind::Invalid:        return "Invalid";
        case TokenKind::EndOfFile:      return "EndOfFile";

        case TokenKind::Identifier:     return "Identifier";
        case TokenKind::IntegerLiteral: return "IntegerLiteral";
        case TokenKind::TrueKeyword: return "TrueKeyword";
        case TokenKind::FalseKeyword: return "FalseKeyword";

        case TokenKind::ModuleKeyword:   return "ModuleKeyword";
        case TokenKind::IntKeyword:     return "IntKeyword";
        case TokenKind::BoolKeyword:    return "BoolKeyword";
        case TokenKind::MutKeyword:     return "MutKeyword";
        case TokenKind::ReturnKeyword:  return "ReturnKeyword";
        case TokenKind::IfKeyword:      return "IfKeyword";
        case TokenKind::ElseKeyword:    return "ElseKeyword";
        case TokenKind::WhileKeyword:   return "WhileKeyword";

        case TokenKind::OpenParen:      return "OpenParen";
        case TokenKind::CloseParen:     return "CloseParen";
        case TokenKind::OpenBrace:      return "OpenBrace";
        case TokenKind::CloseBrace:     return "CloseBrace";
        case TokenKind::Semicolon:      return "Semicolon";
        case TokenKind::Comma:          return "Comma";
        case TokenKind::Dot:            return "Dot";
        case TokenKind::Equals:         return "Equals";
        case TokenKind::EqualsEquals:   return "EqualsEquals";
        case TokenKind::Bang:           return "Bang";
        case TokenKind::BangEquals:     return "BangEquals";
        case TokenKind::Less:           return "Less";
        case TokenKind::LessEquals:     return "LessEquals";
        case TokenKind::Greater:        return "Greater";
        case TokenKind::GreaterEquals:  return "GreaterEquals";

        case TokenKind::Plus:           return "Plus";
        case TokenKind::Minus:          return "Minus";
        case TokenKind::Star:           return "Star";
        case TokenKind::Slash:          return "Slash";
        }
        return "Unknown";
    }

    std::ostream& operator<<(std::ostream& output, TokenKind kind) {
        return output << TokenKindName(kind);
    }
}
