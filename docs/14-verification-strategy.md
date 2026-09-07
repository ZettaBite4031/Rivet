# Verification strategy

## Purpose

The bootstrap compiler needs tests at phase boundaries and across the whole pipeline. The objective is not maximal test count; it is fast localization when a program stops behaving correctly.

## Test layers

### Source tests

Verify file loading, empty files, embedded null bytes if prohibited, newline styles, path reporting, and failed reads. `SourceFile` owns its text and returns stable views or spans for its lifetime.

### Lexer tests

Test one token category at a time, then mixed input. Important cases include:

- Empty and whitespace-only sources.
- Every punctuation token.
- Keywords versus longer identifiers such as `returning`.
- Identifier boundaries.
- Decimal integers and invalid suffixes.
- Unix and Windows newlines.
- Comments at EOF.
- Unknown bytes or characters.
- Exactly one EOF token.
- Correct spans.
- Guaranteed progress after invalid input.

### Parser tests

Parser tests verify tree structure and recovery independently of binding:

- Minimal valid module and function.
- Missing module semicolon.
- Missing parentheses or braces.
- Missing return expression.
- Unexpected tokens at top level and inside blocks.
- Multiple statements as soon as supported.
- Recovery that reaches later declarations.

AST dumps make useful snapshots if formatting is deterministic.

### Binder tests

Binding tests cover semantic rules:

- Unknown types and names.
- Duplicate declarations.
- Missing or ambiguous entry points.
- Return type mismatches.
- Source-order independence.
- Implicit member lookup when classes arrive.
- Private and public visibility.
- Imports, re-exports, and cycles.

Diagnostics should identify both the primary error and relevant previous declarations.

### RIR tests

Each instruction needs verifier and execution tests. Invalid hand-built RIR should be rejected for wrong operand types, unknown registers, missing terminators, and invalid control-flow references.

RIR snapshots test lowering, but assertions about semantic behavior should not depend exclusively on the precise textual formatting of every temporary register.

### End-to-end program tests

Small Rivet programs compile and execute through the selected backend. The first asserts that `main` returns `42`. Later programs cover arithmetic, locals, calls, branches, memory, modules, errors, and compile-time execution.

Each failure test should verify exit status and structured diagnostics, not only search stderr for a loose substring.

## Diagnostic tests

Diagnostics are a product feature. Golden tests should cover:

- File, line, and column.
- Primary span and caret.
- Message and stable code once codes exist.
- Notes pointing to related declarations.
- Generated-code provenance.
- Import and dependency paths.
- Compile-time execution stacks.

Golden output must remain readable enough that intentional updates can be reviewed rather than accepted blindly.

## Differential backend testing

When both the RIR interpreter and LLVM/native output exist, selected programs should execute through both and compare observable results. This catches mismatches in integer behavior, control flow, calls, layout, and memory operations.

The interpreter acts as a semantic reference only if its own behavior is specified and tested. “They disagree, therefore LLVM is wrong” is not a test strategy.

## Compile-time testing

Compile-time execution requires tests for:

- Pure deterministic evaluation.
- Target reflection while running on a different host.
- Declared file inputs and generated outputs.
- Capability denial.
- Cache hits and invalidation.
- Cycles between generated declarations and binding.
- Diagnostics with generation provenance.

## Target and capability testing

The same source should be compiled under hosted, freestanding, kernel, and Lattice-like configurations as they appear. Tests verify that unavailable APIs fail at the correct phase and that programs which avoid them remain valid.

Provider tests should deliberately omit or mismatch required runtime stubs to ensure compile- or link-time diagnostics are useful.

## Fuzzing and property tests

Once the lexer and parser stabilize enough to survive arbitrary input, fuzzing should assert:

- No crashes or hangs.
- Lexer and parser always make progress.
- Source spans stay within their files.
- Invalid programs produce diagnostics rather than internal failures.
- Printing and reparsing supported syntax preserves relevant structure.

RIR verification and serialization are also good fuzz targets.

## Bootstrap and self-hosting tests

Before replacing C++ implementation paths with Rivet versions, run the same conformance suite against both compilers. Their diagnostics need not initially be byte-identical, but accepted/rejected programs and runtime behavior must agree within the defined language subset.

The eventual staged compiler comparison should record the exact bootstrap compiler, source revision, target, dependencies, and build configuration.

## Initial acceptance checklist

The first vertical slice is complete when:

- A valid single file loads without copying token text.
- Token dumps contain correct kinds and spans.
- Malformed input reports diagnostics without hanging.
- The parser builds the minimal AST.
- The binder validates `int main()` and its return.
- Lowering produces verifiable register RIR.
- The interpreter returns `42`.
- Both GCC and Clang bootstrap builds pass the same tests.
