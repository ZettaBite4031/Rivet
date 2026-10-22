#include "syntax/SyntaxPrinter.hpp"

#include <cstddef>
#include <ostream>
#include <string_view>

namespace {
    void WriteIndent(std::ostream& output, std::size_t depth) {
        for (std::size_t index = 0; index < depth; index++)
            output << "  ";
    }

    void WriteQuotedText(std::ostream& output, std::string_view text) {
        output << '"';
        for (const char character : text) {
            switch (character) {
                case '\\': output << "\\\\"; break;
                case '"':  output << "\\\""; break;
                case '\0': output << "\\0";  break;
                case '\n': output << "\\n";  break;
                case '\r': output << "\\r";  break;
                case '\t': output << "\\t";  break;
                case '\f': output << "\\f";  break;
                case '\v': output << "\\v";  break;
                default:   output.put(character); break;
            }
        }
        output << '"';
    }

    void WriteTokenLine(std::ostream& output, const Rivet::SourceFile& source, const Rivet::Token& token, std::size_t depth) {
        WriteIndent(output, depth);
        output << Rivet::TokenKindName(token.Kind);

        if (token.Kind != Rivet::TokenKind::EndOfFile && token.Span.Empty()) {
            output << " <missing>\n";
            return;
        }

        output << ' ';
        WriteQuotedText(output, token.Text(source));
        output << '\n';
    }

    void WriteNode(std::ostream& output, const Rivet::SourceFile& source, const Rivet::SyntaxNode& node, std::size_t depth) {
        using namespace Rivet;

        WriteIndent(output, depth);
        output << SyntaxKindName(node.Kind());

        if (node.Kind() == SyntaxKind::ErrorExpression) {
            if (node.Span().Empty()) {
                output << " <missing>\n";
            } else {
                output << ' ';
                WriteQuotedText(output, source.Text(node.Span()));
                output << '\n';
            }
            return;
        }

        output << '\n';

        switch (node.Kind()) {
            case SyntaxKind::CompilationUnit: {
                const auto& unit = static_cast<const CompilationUnitSyntax&>(node);
                WriteNode(output, source, *unit.Module, depth + 1);
                for (const auto& function : unit.Functions)
                    WriteNode(output, source, *function, depth + 1);
                WriteTokenLine(output, source, unit.EndOfFileToken, depth + 1);
                break;
            }

            case SyntaxKind::ModuleDeclaration: {
                const auto& module = static_cast<const ModuleDeclarationSyntax&>(node);
                WriteTokenLine(output, source, module.ModuleKeyword, depth + 1);
                for (const Token& token : module.PathTokens) WriteTokenLine(output, source, token, depth + 1);
                WriteTokenLine(output, source, module.SemicolonToken, depth + 1);
                break;
            }

            case SyntaxKind::FunctionDefinition: {
                const auto& function = static_cast<const FunctionDefinitionSyntax&>(node);
                WriteTokenLine(output, source, function.TypeToken, depth + 1);
                WriteTokenLine(output, source, function.IdentifierToken, depth + 1);
                WriteTokenLine(output, source, function.OpenParenToken, depth + 1);
                WriteTokenLine(output, source, function.CloseParenToken, depth + 1);
                WriteNode(output, source, *function.Body, depth + 1);
                break;
            }

            case SyntaxKind::BlockStatement: {
                const auto& block = static_cast<const BlockStatementSyntax&>(node);
                WriteTokenLine(output, source, block.OpenBraceToken, depth + 1);
                for (const auto& statement : block.Statements)
                    WriteNode(output, source, *statement, depth + 1);
                WriteTokenLine(output, source, block.CloseBraceToken, depth + 1);
                break;
            }

            case SyntaxKind::ReturnStatement: {
                const auto& statement = static_cast<const ReturnStatementSyntax&>(node);
                WriteTokenLine(output, source, statement.ReturnKeyword, depth + 1);
                WriteNode(output, source, *statement.Expression, depth + 1);
                WriteTokenLine(output, source, statement.SemicolonToken, depth + 1);
                break;
            }

            case SyntaxKind::LocalDeclaration: {
                const auto& local = static_cast<const LocalDeclarationSyntax&>(node);
                if (!local.MutKeyword.Span.Empty()) WriteTokenLine(output, source, local.MutKeyword, depth + 1);
                WriteTokenLine(output, source, local.TypeToken, depth + 1);
                WriteTokenLine(output, source, local.NameToken, depth + 1);
                WriteTokenLine(output, source, local.EqualsToken, depth + 1);
                WriteNode(output, source, *local.Initializer, depth + 1);
                WriteTokenLine(output, source, local.SemicolonToken, depth + 1);
                break;
            }

            case SyntaxKind::AssignmentStatement: {
                const auto& assignment = static_cast<const AssignmentStatementSyntax&>(node);
                WriteTokenLine(output, source, assignment.NameToken, depth + 1);
                WriteTokenLine(output, source, assignment.EqualsToken, depth + 1);
                WriteNode(output, source, *assignment.Value, depth + 1);
                WriteTokenLine(output, source, assignment.SemicolonToken, depth + 1);
                break;
            }

            case SyntaxKind::LiteralExpression: {
                const auto& expression = static_cast<const LiteralExpressionSyntax&>(node);
                WriteTokenLine(output, source, expression.LiteralToken, depth + 1);
                break;
            }

            case SyntaxKind::NameExpression: {
                const auto& expression = static_cast<const NameExpressionSyntax&>(node);
                WriteTokenLine(output, source, expression.NameToken, depth + 1);
                break;
            }

            case SyntaxKind::UnaryExpression: {
                const auto& expression = static_cast<const UnaryExpressionSyntax&>(node);
                WriteTokenLine(output, source, expression.OperatorToken, depth + 1);
                WriteNode(output, source, *expression.Operand, depth + 1);
                break;
            }

            case SyntaxKind::BinaryExpression: {
                const auto& expression = static_cast<const BinaryExpressionSyntax&>(node);
                WriteNode(output, source, *expression.Left, depth + 1);
                WriteTokenLine(output, source, expression.OperatorToken, depth + 1);
                WriteNode(output, source, *expression.Right, depth + 1);
                break;
            }

            case SyntaxKind::ParenthesizedExpression: {
                const auto& expression = static_cast<const ParenthesizedExpressionSyntax&>(node);
                WriteTokenLine(output, source, expression.OpenParenToken, depth + 1);
                WriteNode(output, source, *expression.Expression, depth + 1);
                WriteTokenLine(output, source, expression.CloseParenToken, depth + 1);
                break;
            }

            case SyntaxKind::ErrorExpression:
                break;
        }
    }
}

namespace Rivet {
    void WriteTokenDump(std::ostream& output, const SourceFile& source, std::span<const Token> tokens) {
        for (const Token& token : tokens) {
            output << TokenKindName(token.Kind)
                << ' ' << token.Span.Offset
                << ' ' << token.Span.Length
                << ' ';
            WriteQuotedText(output, token.Text(source));
            output << '\n';
        }
    }

    void WriteSyntaxTree(std::ostream& output, const SourceFile& source, const SyntaxNode& root) {
        WriteNode(output, source, root, 0);
    }
}
