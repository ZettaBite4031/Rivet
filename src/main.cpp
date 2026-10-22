#include "diagnostics/Diagnostics.hpp"
#include "binding/Binder.hpp"
#include "binding/BoundPrinter.hpp"
#include "rir/Rir.hpp"
#include "rir/RirInterpreter.hpp"
#include "rir/RirVerifier.hpp"
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
            << " <tokens|syntax|check|rir|run|verify-rir> <source-file>\n";
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
    const bool checks = command == "check";
    const bool writes_rir = command == "rir";
    const bool runs = command == "run";
    const bool verifies_text_rir = command == "verify-rir";

    if (!writes_tokens && !writes_syntax && !checks && !writes_rir && !runs && !verifies_text_rir) {
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
    if (verifies_text_rir) {
        Rivet::RirModule module;
        const std::vector<Rivet::RirIssue> parse_issues = Rivet::ParseRirText(source.Text(), module);
        const std::vector<Rivet::RirIssue> issues = parse_issues.empty()
            ? Rivet::VerifyRir(module) : parse_issues;
        for (const auto& issue : issues) std::cerr << issue.Code << ' ' << issue.Message << '\n';
        return issues.empty() ? EXIT_SUCCESS : 2;
    }

    Rivet::DiagnosticBag diagnostics;

    const std::vector<Rivet::Token> tokens = Rivet::LexAll(source, diagnostics);
    const std::span<const Rivet::Token> token_span{ tokens };

    if (writes_tokens) Rivet::WriteTokenDump(std::cout, source, token_span);
    else {
        Rivet::Parser parser{ token_span, diagnostics };

        const std::unique_ptr<Rivet::CompilationUnitSyntax> root = parser.ParseCompilationUnit();

        if (writes_syntax) Rivet::WriteSyntaxTree(std::cout, source, *root);
        else if (!diagnostics.HasErrors()) {
            Rivet::Binder binder{ source, diagnostics };
            const std::unique_ptr<Rivet::BoundCompilationUnit> bound = binder.BindCompilationUnit(*root);
            if (checks && !diagnostics.HasErrors()) Rivet::WriteBoundTree(std::cout, *bound);
            else if (!diagnostics.HasErrors()) {
                const Rivet::RirModule module = Rivet::LowerToRir(*bound);
                const std::vector<Rivet::RirIssue> issues = Rivet::VerifyRir(module);
                if (!issues.empty()) {
                    for (const auto& issue : issues) std::cerr << issue.Code << ' ' << issue.Message << '\n';
                    return 2;
                }
                if (writes_rir) Rivet::WriteRir(std::cout, module);
                else if (runs) {
                    const auto result = Rivet::RunRirMain(module);
                    if (result) {
                        std::cout << "exit = " << *result << "\nstdout = \"\"\nstderr = \"\"\n";
                    } else {
                        const Rivet::SourcePosition position = source.PositionAt(result.error().Span.Offset);
                        const Rivet::SourcePosition end = source.PositionAt(result.error().Span.End());
                        std::cerr << result.error().Code << ' ' << position.Line + 1 << ':' << position.Column + 1
                            << '-' << end.Line + 1 << ':' << end.Column + 1 << ' ' << result.error().Message << '\n';
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }

    if (!diagnostics.Empty()) 
        std::cerr << Rivet::RenderCompactDiagnostics(source, diagnostics.Items());

    return diagnostics.HasErrors() ? EXIT_FAILURE : EXIT_SUCCESS;
}
