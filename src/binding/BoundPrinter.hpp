#pragma once

#include "binding/Bound.hpp"

#include <iosfwd>

namespace Rivet {
    void WriteBoundTree(std::ostream& output, const BoundCompilationUnit& unit);
}
