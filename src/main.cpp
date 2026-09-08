#include "diagnostics/Diagnostics.hpp"
#include "source/Source.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Parser.hpp"
#include "syntax/SyntaxPrinter.hpp"
#include "syntax/Token.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>


namespace {
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
    
    const bool writes_tokens = command == "tokens";
    const bool writes_syntax = command == "syntax";

    if (!writes_tokens && !writes_syntax) {
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
    const std::span<const Rivet::Token> token_span{ tokens };

    if (writes_tokens) Rivet::WriteTokenDump(std::cout, source, token_span);
    else {
        Rivet::Parser parser{ token_span, diagnostics };

        const std::unique_ptr<Rivet::CompilationUnitSyntax> root = parser.ParseCompilationUnit();

        Rivet::WriteSyntaxTree(std::cout, source, *root);
    }

    if (!diagnostics.Empty()) 
        std::cerr << Rivet::RenderCompactDiagnostics(source, diagnostics.Items());

    return diagnostics.HasErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}
