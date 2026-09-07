#include "source/Source.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <source-file>\n";
        return 1;
    }

    auto result = Rivet::SourceFile::Load(argv[1]);
    if (!result) {
        std::cerr << result.error().Path.string() << ": error: " << result.error().Message << '\n';
        return 1;
    }

    Rivet::SourceFile source = std::move(*result);
    std::cout << "Loaded: " << source.Path().string() << " (" << source.Size() << " bytes, " << source.LineCount() << " lines)\n";

    return 0;
}
