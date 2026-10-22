#include <cstdlib>
#include <exception>
#include <iostream>

namespace Rivet::Tests {
    [[nodiscard]] bool RunSourceFileTests();
    [[nodiscard]] bool RunDiagnosticTests();
    [[nodiscard]] bool RunLexerTests();
    [[nodiscard]] bool RunParserTests();
    [[nodiscard]] bool RunPhaseOneTests();
}

int main() {
    std::cout << "rivetc0 bootstrap tests\n\n";

    try {
        const bool source_succeeded = Rivet::Tests::RunSourceFileTests();

        std::cout << '\n';

        const bool diagnostics_succeeded = Rivet::Tests::RunDiagnosticTests();

        std::cout << '\n';

        const bool lexer_succeeded = Rivet::Tests::RunLexerTests();

        std::cout << '\n'; 

        const bool parser_succeeded = Rivet::Tests::RunParserTests();

        std::cout << '\n';
        const bool phase_one_succeeded = Rivet::Tests::RunPhaseOneTests();

        if (!source_succeeded || !diagnostics_succeeded || !lexer_succeeded || !parser_succeeded || !phase_one_succeeded) {
            std::cerr << "\nBootstrap tests failed.\n";
            return EXIT_FAILURE;
        }

        std::cout << "\nAll bootstrap tests passed.\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "\nUnhandled exception in test runner: " << ex.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "\nUnknown exception in test runner.\n";
        return EXIT_FAILURE;
    }
}
