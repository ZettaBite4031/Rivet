#pragma once

#include "source/Source.hpp"
#include "syntax/Syntax.hpp"
#include "syntax/Token.hpp"

#include <iosfwd>
#include <span>

namespace Rivet {
    void WriteTokenDump(std::ostream& output, const SourceFile& source, std::span<const Token> tokens);
    void WriteSyntaxTree(std::ostream& output, const SourceFile& source, const SyntaxNode& root);
}