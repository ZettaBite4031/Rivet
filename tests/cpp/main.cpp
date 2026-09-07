#include <cstdlib>
#include <exception>
#include <iostream>

namespace Rivet::Tests {
    [[nodiscard]] bool RunSourceFileTests();
}

int main() {
    std::cout << "rivetc0 bootstrap tests\n\n";

    try {
        const bool succeeded = Rivet::Tests::RunSourceFileTests();
        if (!succeeded) {
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