#include "test.hpp"

#include "binding/Binder.hpp"
#include "binding/BoundPrinter.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "rir/Rir.hpp"
#include "rir/RirInterpreter.hpp"
#include "rir/RirVerifier.hpp"
#include "source/Source.hpp"
#include "syntax/Lexer.hpp"
#include "syntax/Parser.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace Rivet::Tests {
    namespace {
        struct Compiled {
            SourceFile Source;
            DiagnosticBag Diagnostics;
            std::unique_ptr<CompilationUnitSyntax> Syntax;
            std::unique_ptr<BoundCompilationUnit> Bound;
        };

        Compiled Bind(std::string input) {
            Compiled result{
                .Source = SourceFile{ "phase-one.rivet", std::move(input) },
                .Diagnostics = {}, .Syntax = {}, .Bound = {},
            };
            const std::vector<Token> tokens = LexAll(result.Source, result.Diagnostics);
            Parser parser{ std::span<const Token>{ tokens }, result.Diagnostics };
            result.Syntax = parser.ParseCompilationUnit();
            if (!result.Diagnostics.HasErrors()) {
                Binder binder{ result.Source, result.Diagnostics };
                result.Bound = binder.BindCompilationUnit(*result.Syntax);
            }
            return result;
        }

        void ParsesModulePath() {
            Compiled result = Bind("module demo.core;\nint main() { return 42; }\n");
            REQUIRE(result.Diagnostics.Empty());
            REQUIRE_EQ(result.Syntax->Module->Name(result.Source), std::string{ "demo.core" });
            REQUIRE_EQ(result.Bound->ModuleName, std::string{ "demo.core" });
            REQUIRE_EQ(result.Bound->Functions.size(), std::size_t{ 1 });
        }

        void RejectsMissingAndDuplicateModules() {
            Compiled missing = Bind("int main() { return 42; }\n");
            REQUIRE(missing.Diagnostics.HasErrors());
            REQUIRE(missing.Bound == nullptr);
            REQUIRE(missing.Diagnostics.Items().front().Code == DiagnosticCode::MissingModule);

            Compiled repeated = Bind("module first;\nmodule second;\nint main() { return 42; }\n");
            REQUIRE(repeated.Diagnostics.HasErrors());
            REQUIRE(repeated.Diagnostics.Items().front().Code == DiagnosticCode::DuplicateModule);
            REQUIRE_EQ(repeated.Syntax->Module->Name(repeated.Source), std::string{ "first" });
        }

        void BindsAndPrintsTypedTree() {
            Compiled result = Bind("module demo;\nint main() { return 40 + 2; }\n");
            REQUIRE(result.Diagnostics.Empty());
            std::ostringstream output;
            WriteBoundTree(output, *result.Bound);
            REQUIRE_EQ(output.str(), std::string{
                "Module demo\n"
                "  Function main() -> int [entry]\n"
                "    Block\n"
                "      Return : int\n"
                "        Binary Add : int\n"
                "          Literal 40 : int\n"
                "          Literal 2 : int\n" });
        }

        void ReportsBindingErrors() {
            Compiled duplicate = Bind("module demo;\nint main() { return 1; }\nint main() { return 2; }\n");
            REQUIRE_EQ(duplicate.Diagnostics.Size(), std::size_t{ 2 });
            REQUIRE(duplicate.Diagnostics.Items()[0].Code == DiagnosticCode::DuplicateFunction);
            REQUIRE(duplicate.Diagnostics.Items()[1].Severity == DiagnosticSeverity::Note);
            REQUIRE_EQ(duplicate.Bound->Functions.size(), std::size_t{ 1 });

            Compiled missing = Bind("module demo;\nint helper() { return 1; }\n");
            REQUIRE(missing.Diagnostics.Items().front().Code == DiagnosticCode::MissingEntryPoint);

            Compiled overflow = Bind("module demo;\nint main() { return 2147483648; }\n");
            REQUIRE(overflow.Diagnostics.Items().front().Code == DiagnosticCode::IntegerLiteralOutOfRange);
        }

        void LowersAndVerifiesRir() {
            Compiled result = Bind("module demo;\nint main() { return 1 + 2 * 3; }\n");
            REQUIRE(result.Diagnostics.Empty());
            RirModule module = LowerToRir(*result.Bound);
            REQUIRE(VerifyRir(module).empty());
            std::ostringstream output;
            WriteRir(output, module);
            REQUIRE(output.str().find("int $3 = multiply $1, $2") != std::string::npos);
            REQUIRE(output.str().find("return $4") != std::string::npos);
        }

        void RejectsInvalidRir() {
            RirModule module{ .Name = "demo" };
            module.Functions.push_back(RirFunction{ .Name = "main" });
            module.Functions.front().Instructions.push_back(RirInstruction{
                .Op = RirOp::Add, .Result = 0, .Left = 1, .Right = 2 });
            const auto issues = VerifyRir(module);
            REQUIRE_EQ(issues.size(), std::size_t{ 3 });
            REQUIRE_EQ(issues[0].Code, std::string{ "ICE-RIR-002" });
            REQUIRE_EQ(issues[1].Code, std::string{ "ICE-RIR-002" });
            REQUIRE_EQ(issues[2].Code, std::string{ "ICE-RIR-001" });
        }

        void ParsesTextFixtureForVerifier() {
            RirModule module;
            REQUIRE(ParseRirText("function int @main()\n{\nentry:\n    int $0 = const 42\n}\n", module).empty());
            REQUIRE_EQ(VerifyRir(module).size(), std::size_t{ 1 });
        }

        void ExecutesSignedResult() {
            Compiled result = Bind("module demo;\nint main() { return -(40 + 2); }\n");
            REQUIRE(result.Diagnostics.Empty());
            const RirModule module = LowerToRir(*result.Bound);
            REQUIRE(VerifyRir(module).empty());
            const auto value = RunRirMain(module);
            REQUIRE(value.has_value());
            REQUIRE_EQ(*value, std::int32_t{ -42 });
        }

        void ReportsRuntimeArithmeticFaults() {
            Compiled division = Bind("module demo;\nint main() { return 1 / 0; }\n");
            REQUIRE(division.Diagnostics.Empty());
            const auto divided = RunRirMain(LowerToRir(*division.Bound));
            REQUIRE(!divided.has_value());
            REQUIRE_EQ(divided.error().Code, std::string{ "R0001" });

            Compiled overflow = Bind("module demo;\nint main() { return 2147483647 + 1; }\n");
            REQUIRE(overflow.Diagnostics.Empty());
            const auto added = RunRirMain(LowerToRir(*overflow.Bound));
            REQUIRE(!added.has_value());
            REQUIRE_EQ(added.error().Code, std::string{ "R0002" });
        }

        void ExecutesLocalsAndReassignment() {
            Compiled result = Bind("module demo;\nint main() { int base = 40; mut int offset = 1; "
                "offset = offset + 1; return base + offset; }\n");
            REQUIRE(result.Diagnostics.Empty());
            REQUIRE_EQ(result.Bound->Functions.front()->Locals.size(), std::size_t{ 2 });
            REQUIRE(!result.Bound->Functions.front()->Locals.front().Mutable);
            REQUIRE(result.Bound->Functions.front()->Locals.back().Mutable);
            const RirModule module = LowerToRir(*result.Bound);
            REQUIRE(VerifyRir(module).empty());
            const auto value = RunRirMain(module);
            REQUIRE(value.has_value());
            REQUIRE_EQ(*value, std::int32_t{ 42 });

            // A previous value keeps its register when a mutable local changes.
            Compiled snapshot = Bind("module demo; int main() { mut int x = 3; "
                "int old = x; x = 7; return old + x; }");
            REQUIRE(snapshot.Diagnostics.Empty());
            const RirModule snapshot_rir = LowerToRir(*snapshot.Bound);
            REQUIRE(VerifyRir(snapshot_rir).empty());
            REQUIRE_EQ(*RunRirMain(snapshot_rir), std::int32_t{ 10 });
        }

        void ChecksLocalNamesAndMutation() {
            const auto first_error = [](std::string source) {
                Compiled result = Bind(std::move(source));
                REQUIRE(result.Diagnostics.HasErrors());
                return result.Diagnostics.Items().front().Code;
            };
            REQUIRE(first_error("module demo; int main() { int x = 1; x = 2; return x; }")
                == DiagnosticCode::ImmutableLocal);
            REQUIRE(first_error("module demo; int main() { return missing; }")
                == DiagnosticCode::UnknownLocal);
            REQUIRE(first_error("module demo; int main() { missing = 2; return 1; }")
                == DiagnosticCode::UnknownLocal);
            REQUIRE(first_error("module demo; int main() { int x = 1; int x = 2; return x; }")
                == DiagnosticCode::DuplicateLocal);
            REQUIRE(first_error("module demo; int main() { int x = x; return 1; }")
                == DiagnosticCode::UnknownLocal);
            REQUIRE(first_error("module demo; int main() { int x = 1; }")
                == DiagnosticCode::MissingReturn);

            Compiled independent = Bind("module demo; int helper() { int x = 1; return x; } "
                "int main() { return x; }");
            REQUIRE(independent.Diagnostics.HasErrors());
            REQUIRE(independent.Diagnostics.Items().front().Code == DiagnosticCode::UnknownLocal);
        }
    }

    bool RunPhaseOneTests() {
        const std::array tests{
            TestCase{ "parses an explicit module path", ParsesModulePath },
            TestCase{ "rejects missing and duplicate modules", RejectsMissingAndDuplicateModules },
            TestCase{ "binds a typed tree", BindsAndPrintsTypedTree },
            TestCase{ "reports binding failures", ReportsBindingErrors },
            TestCase{ "lowers and verifies RIR", LowersAndVerifiesRir },
            TestCase{ "rejects invalid RIR", RejectsInvalidRir },
            TestCase{ "parses verifier fixture format", ParsesTextFixtureForVerifier },
            TestCase{ "executes a signed result", ExecutesSignedResult },
            TestCase{ "reports runtime arithmetic faults", ReportsRuntimeArithmeticFaults },
            TestCase{ "executes locals and reassignment", ExecutesLocalsAndReassignment },
            TestCase{ "checks local names and mutation", ChecksLocalNamesAndMutation },
        };
        return RunSuite("Phase one", tests);
    }
}
