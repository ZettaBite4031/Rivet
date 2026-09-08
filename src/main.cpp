#include "diagnostics/Diagnostics.hpp"
#include "source/Source.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Token.hpp"

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string_view>
#include <utility>


namespace {
    void WriteQuotedText(std::ostream& output, std::string_view text) {
        output << '"';

        for (const char character : text) {
            switch (character) {
            case '\\': output << "\\\\"; break;
            case '"' : output << "\\\""; break;
            case '\0': output << "\\0" ; break;
            case '\n': output << "\\n" ; break;
            case '\r': output << "\\r" ; break;
            case '\t': output << "\\t" ; break;
            case '\f': output << "\\f" ; break;
            case '\v': output << "\\v" ; break;
            default:   output.put(character); break;
            }
        }

        output << '"';
    }

    void WriteToken(std::ostream& output, const Rivet::SourceFile& source, const Rivet::Token& token) {
        output << Rivet::TokenKindName(token.Kind)
            << ' ' << token.Span.Offset
            << ' ' << token.Span.Length
            << ' ';

        WriteQuotedText(output, token.Text(source));
        output << '\n';
    }

    void WriteUsage(std::ostream& output, std::string_view executable) {
        output << "usage: " << executable
            << " tokens <source-file>\n";
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        WriteUsage(std::cerr, argv[0]);
        return EXIT_FAILURE;
    }

    const std::string_view command{ argv[1] };
    if (command != "tokens") {
        std::cerr << "unknown command: " << command << '\n';
        WriteUsage(std::cerr, argv[0]);
        return EXIT_FAILURE;
    }

    auto source_result = Rivet::SourceFile::Load(argv[2]);
    if (!source_result) {
        const Rivet::SourceLoadError& error = source_result.error();
        std::cerr << error.Path.string() << ": error: " << error.Message << '\n';
        return EXIT_FAILURE;
    }

    Rivet::SourceFile source = std::move(*source_result);
    Rivet::DiagnosticBag diagnostics;

    const std::vector<Rivet::Token> tokens = Rivet::LexAll(source, diagnostics);
    for (const Rivet::Token& token : tokens) 
        WriteToken(std::cout, source, token);

    if (!diagnostics.Empty())
        std::cerr << Rivet::RenderCompactDiagnostics(source, diagnostics.Items());

    return diagnostics.HasErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}