#include "syntax/Parser.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Rivet {
    Parser::Parser(std::span<const Token> tokens, DiagnosticBag& diagnostics) noexcept 
        : m_Tokens(tokens), m_Diagnostics(diagnostics) {
        assert(!m_Tokens.empty());
        assert(m_Tokens.back().Kind == TokenKind::EndOfFile);
    }

    std::unique_ptr<CompilationUnitSyntax> Parser::ParseCompilationUnit() {
        std::unique_ptr<ModuleDeclarationSyntax> module;
        if (Current().Kind == TokenKind::ModuleKeyword) {
            module = ParseModuleDeclaration();
        } else {
            m_Diagnostics.Report(DiagnosticCode::MissingModule, DiagnosticSeverity::Error,
                Current().Span, "source file must begin with a module declaration");
            const SourceSpan missing{ .Offset = Current().Span.Offset, .Length = 0 };
            module = std::make_unique<ModuleDeclarationSyntax>(
                Token{ TokenKind::ModuleKeyword, missing }, std::vector<Token>{}, Token{ TokenKind::Semicolon, missing });
        }
        std::vector<std::unique_ptr<FunctionDefinitionSyntax>> functions;

        while (Current().Kind != TokenKind::EndOfFile) {
            if (Current().Kind == TokenKind::ModuleKeyword) {
                m_Diagnostics.Report(DiagnosticCode::DuplicateModule, DiagnosticSeverity::Error,
                    Current().Span, "a source file may declare only one module, at the beginning");
                static_cast<void>(ParseModuleDeclaration());
                continue;
            }
            const std::size_t beginning = m_Position;
            functions.push_back(ParseFunctionDefinition());

            // this guard protects the compiler from future recovery bugs.
            if (m_Position == beginning) static_cast<void>(ConsumeToken());
        }

        Token eof = MatchToken(TokenKind::EndOfFile, "expected end of file");

        return std::make_unique<CompilationUnitSyntax>(std::move(module), std::move(functions), eof);
    }

    std::unique_ptr<ModuleDeclarationSyntax> Parser::ParseModuleDeclaration() {
        const Token keyword = ConsumeToken();
        std::vector<Token> path;
        path.push_back(MatchToken(TokenKind::Identifier, "expected module name"));
        while (Current().Kind == TokenKind::Dot) {
            path.push_back(ConsumeToken());
            path.push_back(MatchToken(TokenKind::Identifier, "expected module name after '.'"));
        }
        const Token semicolon = MatchToken(TokenKind::Semicolon, "expected ';' after module name");
        return std::make_unique<ModuleDeclarationSyntax>(keyword, std::move(path), semicolon);
    }

    std::size_t Parser::TokenIndex(std::size_t distance) const noexcept {
        const std::size_t last = m_Tokens.size() - 1;
        std::size_t index = m_Position <= last ? m_Position : last;

        while (index < last && m_Tokens[index].Kind == TokenKind::Invalid) index++;

        for (std::size_t step = 0; step < distance; step++) {
            if (index < last) index++;
            while (index < last && m_Tokens[index].Kind == TokenKind::Invalid) index++;
        }

        return index;
    }

    const Token& Parser::Current() const noexcept { return Peek(0); }
    const Token& Parser::Peek(std::size_t distance) const noexcept { 
        return m_Tokens[TokenIndex(distance)];
    }

    Token Parser::ConsumeToken() noexcept {
        const std::size_t index = TokenIndex(0);
        const Token token = m_Tokens[index];
        const std::size_t last = m_Tokens.size() - 1;
        m_Position = index < last ? index + 1 : last;
        return token;
    }

    Token Parser::MatchToken(TokenKind expected, std::string_view message) {
        if (Current().Kind == expected) return ConsumeToken();

        m_Diagnostics.Report(DiagnosticCode::MissingToken, DiagnosticSeverity::Error, Current().Span, std::string{ message });
        return Token{ .Kind = expected, .Span = SourceSpan{ .Offset = Current().Span.Offset, .Length = 0 } };
    }

    std::unique_ptr<FunctionDefinitionSyntax> Parser::ParseFunctionDefinition() {
        Token type_token = MatchToken(TokenKind::IntKeyword, "expected function return type 'int'");
        Token identifier_token = MatchToken(TokenKind::Identifier, "expected function name");
        Token open_paren_token = MatchToken(TokenKind::OpenParen, "expected '(' after function name");
        Token close_paren_token = MatchToken(TokenKind::CloseParen, "expected ')' after function parameters");

        std::unique_ptr<BlockStatementSyntax> body = ParseBlockStatement();
        return std::make_unique<FunctionDefinitionSyntax>(type_token, identifier_token, open_paren_token, close_paren_token, std::move(body));
    }

    std::unique_ptr<BlockStatementSyntax> Parser::ParseBlockStatement() {
        Token open_brace_token = MatchToken(TokenKind::OpenBrace, "expected '{' to begin function body");
        std::vector<std::unique_ptr<StatementSyntax>> statements;

        while (Current().Kind != TokenKind::CloseBrace && Current().Kind != TokenKind::EndOfFile && !IsFunctionStart()) {
            const std::size_t beginning = m_Position;
            statements.push_back(ParseStatement());
            if (m_Position == beginning) static_cast<void>(ConsumeToken());
        }

        Token close_brace_token = MatchToken(TokenKind::CloseBrace, "expected '}' to close function body");
        return std::make_unique<BlockStatementSyntax>(open_brace_token, std::move(statements), close_brace_token);
    }

    std::unique_ptr<StatementSyntax> Parser::ParseStatement() {
        if (Current().Kind == TokenKind::IntKeyword || Current().Kind == TokenKind::MutKeyword)
            return ParseLocalDeclaration();
        if (Current().Kind == TokenKind::Identifier && Peek(1).Kind == TokenKind::Equals)
            return ParseAssignmentStatement();
        return ParseReturnStatement();
    }

    std::unique_ptr<LocalDeclarationSyntax> Parser::ParseLocalDeclaration() {
        Token mut{ TokenKind::MutKeyword, { Current().Span.Offset, 0 } };
        if (Current().Kind == TokenKind::MutKeyword) mut = ConsumeToken();
        Token type = MatchToken(TokenKind::IntKeyword, "expected local type 'int'");
        Token name = MatchToken(TokenKind::Identifier, "expected local name");
        Token equals = MatchToken(TokenKind::Equals, "expected '=' after local name");
        auto initializer = ParseExpression();
        Token semicolon = MatchToken(TokenKind::Semicolon, "expected ';' after local declaration");
        return std::make_unique<LocalDeclarationSyntax>(mut, type, name, equals,
            std::move(initializer), semicolon);
    }

    std::unique_ptr<AssignmentStatementSyntax> Parser::ParseAssignmentStatement() {
        Token name = ConsumeToken();
        Token equals = ConsumeToken();
        auto value = ParseExpression();
        Token semicolon = MatchToken(TokenKind::Semicolon, "expected ';' after assignment");
        return std::make_unique<AssignmentStatementSyntax>(name, equals, std::move(value), semicolon);
    }

    std::unique_ptr<ReturnStatementSyntax> Parser::ParseReturnStatement() {
        Token return_keyword = MatchToken(TokenKind::ReturnKeyword, "expected 'return' statement");
        std::unique_ptr<ExpressionSyntax> expression = ParseExpression();
        Token semicolon_token = MatchToken(TokenKind::Semicolon, "expected ';' after return expression");
        return std::make_unique<ReturnStatementSyntax>(return_keyword, std::move(expression), semicolon_token);
    }

    std::unique_ptr<ExpressionSyntax> Parser::ParseExpression(int parent_precendence) {
        std::unique_ptr<ExpressionSyntax> left;
        const int unary_precedence = UnaryOperatorPrecedence(Current().Kind);
        if (unary_precedence != 0 && unary_precedence >= parent_precendence) {
            const Token operator_token = ConsumeToken();
            std::unique_ptr<ExpressionSyntax> operand;
            if (IsExpressionBoundary(Current().Kind) || IsFunctionStart()) {
                std::string message{ "expected expression after unary operator '"};
                message += OperatorSpelling(operator_token.Kind);
                message.push_back('\'');
                operand = MakeMissingExpression(std::move(message));
            } else {
                operand = ParseExpression(unary_precedence);
            }

            left = std::make_unique<UnaryExpressionSyntax>(operator_token, std::move(operand));
        } else {
            left = ParsePrimaryExpression();
        }

        while (true) {
            const int binary_precedence = BinaryOperatorPrecedence(Current().Kind);
            if (binary_precedence == 0 || binary_precedence <= parent_precendence) break;

            const Token operator_token = ConsumeToken();
            std::unique_ptr<ExpressionSyntax> right = ParseExpression(binary_precedence);
            left = std::make_unique<BinaryExpressionSyntax>(std::move(left), operator_token, std::move(right));
        }

        return left;
    }

    std::unique_ptr<ExpressionSyntax> Parser::ParsePrimaryExpression() {
        if (Current().Kind == TokenKind::OpenParen) {
            const Token open_paren_token = ConsumeToken();
            std::unique_ptr<ExpressionSyntax> expression = ParseExpression();

            Token close_paren_token = MatchToken(TokenKind::CloseParen, "expected ')' after expression");

            return std::make_unique<ParenthesizedExpressionSyntax>(open_paren_token, std::move(expression), close_paren_token);
        }

        if (Current().Kind == TokenKind::IntegerLiteral) 
            return std::make_unique<LiteralExpressionSyntax>(ConsumeToken());
        if (Current().Kind == TokenKind::Identifier)
            return std::make_unique<NameExpressionSyntax>(ConsumeToken());

        m_Diagnostics.Report(DiagnosticCode::ExpectedExpression, DiagnosticSeverity::Error, Current().Span, "expected expression");

        if (IsExpressionBoundary(Current().Kind) || IsFunctionStart())
            return std::make_unique<ErrorExpressionSyntax>(SourceSpan{ .Offset = Current().Span.Offset, .Length = 0 });
        return std::make_unique<ErrorExpressionSyntax>(ConsumeToken().Span);
    }

    std::unique_ptr<ExpressionSyntax> Parser::MakeMissingExpression(std::string message) {
        m_Diagnostics.Report(DiagnosticCode::ExpectedExpression, DiagnosticSeverity::Error, Current().Span, std::move(message));
        return std::make_unique<ErrorExpressionSyntax>(SourceSpan{ .Offset = Current().Span.Offset, .Length = 0 });
    }

    bool Parser::IsFunctionStart() const noexcept {
        return Current().Kind == TokenKind::IntKeyword && Peek(1).Kind == TokenKind::Identifier && Peek(2).Kind == TokenKind::OpenParen;
    }

    bool Parser::IsExpressionBoundary(TokenKind kind) noexcept {
        switch (kind) {
        case TokenKind::Semicolon:
        case TokenKind::CloseParen:
        case TokenKind::CloseBrace:
        case TokenKind::EndOfFile:
            return true;
        default:
            return false;
        }
    }

    int Parser::UnaryOperatorPrecedence(TokenKind kind) noexcept {
        switch (kind) {
        case TokenKind::Plus:
        case TokenKind::Minus:
            return 3;

        default:
            return 0;
        }
    }

    int Parser::BinaryOperatorPrecedence(TokenKind kind) noexcept {
        switch (kind) {
        case TokenKind::Star:
        case TokenKind::Slash:
            return 2;

        case TokenKind::Plus:
        case TokenKind::Minus:
            return 1;

        default:
            return 0;
        }
    }

    std::string_view Parser::OperatorSpelling(TokenKind kind) noexcept {
        switch (kind) {
        case TokenKind::Plus:  return "+";
        case TokenKind::Minus: return "-";
        case TokenKind::Star:  return "*";
        case TokenKind::Slash: return "/";
        default:               return "?";
        }
    }
}
