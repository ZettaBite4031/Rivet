#include "syntax/Lexer.hpp"

#include <cassert>
#include <string>
#include <string_view>
#include <utility>

namespace Rivet {
    Lexer::Lexer(const SourceFile& source, DiagnosticBag& diagnostics) noexcept
        : m_Source(source), m_Diagnostics(diagnostics) {}

    Token Lexer::NextToken() {
        SkipTrivia();

        if (AtEnd()) 
            return Token{ .Kind = TokenKind::EndOfFile, .Span = m_Source.EofSpan() };

        if (IsIdentifierStart(Current())) return TokenLexIdentifierOrKeyword();
        if (IsDigit(Current())) return LexIntegerLiteral();
        return LexPunctuationOrInvalid();
    }

    bool Lexer::AtEnd() const noexcept {
        return m_Position >= m_Source.Size();
    }

    char Lexer::Current() const noexcept {
        assert(!AtEnd());
        return m_Source.Text()[m_Position];
    }

    char Lexer::Peek(std::size_t distance) const noexcept {
        assert(m_Position <= m_Source.Size());
        const std::size_t remaining = m_Source.Size() - m_Position;
        if (distance >= remaining) return '\0';
        return m_Source.Text()[m_Position + distance];
    }

    void Lexer::Advance() noexcept {
        assert(!AtEnd());
        m_Position++;
    }

    void Lexer::SkipTrivia() noexcept {
        while (true) {
            while (!AtEnd() && IsWhitespace(Current())) Advance();

            const bool begins_line_comment = !AtEnd() && Current() == '/' && Peek(1) == '/';
            if (!begins_line_comment) return;
            
            // Leave the newline for the whitespace loop. This also works for a 
            // comment at EOF because AtEnd() terminates the loop.
            while (!AtEnd() && Current() != '\n') Advance();
        }
    }

    Token Lexer::TokenLexIdentifierOrKeyword() {
        const std::size_t beginning = m_Position;

        // The caller has established that the first character is valid.
        Advance();

        while (!AtEnd() && IsIdentifierContinuation(Current())) Advance();

        Token token = MakeToken(TokenKind::Identifier, beginning);
        const std::string_view spelling = token.Text(m_Source);
        if (spelling == "int") token.Kind = TokenKind::IntKeyword;
        else if (spelling == "return") token.Kind = TokenKind::ReturnKeyword;
        return token;
    }

    Token Lexer::LexIntegerLiteral() {
        const std::size_t beginning = m_Position;

        do {
            Advance();
        } while (!AtEnd() && IsDigit(Current()));

        return MakeToken(TokenKind::IntegerLiteral, beginning);
    }

    Token Lexer::LexPunctuationOrInvalid() {
        const std::size_t beginning = m_Position;
        const char character = Current();

        TokenKind kind = TokenKind::Invalid;

        switch (character) {
            case '(': kind = TokenKind::OpenParen;  break;
            case ')': kind = TokenKind::CloseParen; break;
            case '{': kind = TokenKind::OpenBrace;  break;
            case '}': kind = TokenKind::CloseBrace; break;
            case ';': kind = TokenKind::Semicolon;  break;
            case '+': kind = TokenKind::Plus;       break;
            case '-': kind = TokenKind::Minus;      break;
            case '*': kind = TokenKind::Star;       break;
            case '/': kind = TokenKind::Slash;      break;

            default: {
                Advance();

                const Token token = MakeToken(TokenKind::Invalid, beginning);
                std::string message{ "invalid character '" };
                message.push_back(character);
                message.push_back('\'');

                m_Diagnostics.Report(DiagnosticCode::InvalidCharacter, DiagnosticSeverity::Error, token.Span, std::move(message));
                return token;
            }
        }

        Advance();
        return MakeToken(kind, beginning);
    }

    Token Lexer::MakeToken(TokenKind kind, std::size_t beginning) const noexcept {
        assert(beginning <= m_Position);
        assert(m_Position <= m_Source.Size());

        // All non-EOF tokens must consume input. EOF is produced separately
        // by NextToken(), so reaching this assertion with an empty token means
        // one of the lexer paths has failed to make progress.
        assert(m_Position > beginning);

        return Token{
            .Kind = kind,
            .Span = SourceSpan{
                .Offset = beginning,
                .Length = m_Position - beginning,
            },
        };
    }

    constexpr bool Lexer::IsWhitespace(char character) noexcept {
        return character == ' ' || character == '\t' || character == '\r' || character == '\n' || character == '\f' || character == '\v';
    }

    constexpr bool Lexer::IsDigit(char character) noexcept {
        return character >= '0' && character <= '9';
    }

    constexpr bool Lexer::IsIdentifierStart(char character) noexcept {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
    }

    constexpr bool Lexer::IsIdentifierContinuation(char character) noexcept {
        return IsIdentifierStart(character) || IsDigit(character);
    }

    std::vector<Token> LexAll(const SourceFile& source, DiagnosticBag& diagnostics) {
        Lexer lexer(source, diagnostics);
        std::vector<Token> tokens;

        while (true) {
            const Token token = lexer.NextToken();
            tokens.push_back(token);
            if (token.Kind == TokenKind::EndOfFile) break;
        }

        return tokens;
    }
}