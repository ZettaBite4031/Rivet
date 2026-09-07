# RIR and code generation

## Purpose

RIR—Rivet Intermediate Representation—is the compiler's language-independent-enough but Rivet-aware representation between semantic analysis and final execution or machine code.

It must support ordinary native programs, freestanding code, compile-time execution, optimization, diagnostics, and eventually several backends. It should preserve useful language guarantees instead of erasing everything into untyped machine operations too early.

## Register-oriented model

**Accepted direction:** RIR is register-oriented rather than stack-based. Instructions read virtual registers and produce virtual-register results.

Conceptual example:

```text
func @main() -> i32 {
entry:
    %0 = const.i32 40
    %1 = const.i32 2
    %2 = add.i32 %0, %1
    ret %2
}
```

This model is familiar to the project, straightforward to interpret, maps naturally to data-flow analysis, and avoids implicit operand-stack state.

Whether RIR is strictly SSA at every stage is open. A useful design may have a construction form with mutable virtual registers and a canonical analysis/optimization form with SSA-like invariants.

## Not an LLVM imitation exercise

RIR should feel familiar to compiler developers, but it is not intended to rename LLVM IR instructions and declare victory. Differences should buy concrete quality of life or preserve Rivet semantics.

Potential Rivet-specific value includes:

- First-class checked and raw pointer distinctions.
- Explicit target capabilities and runtime requirements.
- Better source and generated-code provenance.
- Structured operations before lower-level expansion.
- Reflection and compile-time metadata.
- Stable inspection APIs for Rivet tooling.
- Direct representation of language contracts where optimization can use them.

Conversely, RIR should reuse conventional control-flow graphs, blocks, typed operations, and calling-convention concepts where novelty would only make implementation harder.

## Core entities

The long-term representation likely contains:

- Modules and imported/exported symbols.
- Functions and signatures.
- Basic blocks.
- Virtual registers or values.
- Typed instructions.
- Constants and global data.
- Structured type and layout information.
- Source locations and inlining/generation provenance.
- Target and capability requirements.
- Linkage, visibility, ABI, and calling-convention metadata.

The bootstrap implements only what the current vertical slice needs.

## Initial instruction set

The first executable program needs only:

```text
const.i32
ret
```

The next horizontal slice adds:

```text
add.i32
sub.i32
mul.i32
div.i32
local or stack storage
load
store
call
branch
conditional branch
```

Instructions should be added because a source feature requires them, not because a comprehensive opcode spreadsheet looks satisfying.

## Types in RIR

RIR needs explicit machine-facing types such as fixed-width integers, floating-point values, aggregates, functions, and addresses. It should also retain higher-level distinctions until the pass that intentionally lowers them.

For example, a checked pointer should not immediately become the same unannotated address as a raw pointer if its bounds or non-null guarantee can drive diagnostics and optimization.

Type layout depends on the selected target. Compile-time reflection must query target layout even when RIR is interpreted on the build host.

## Control flow and effects

Functions lower to explicit basic blocks and terminators. Every block must end in a terminator such as return, branch, conditional branch, trap, or unreachable.

RIR should eventually model effects relevant to correctness and optimization:

- Memory reads and writes.
- Volatile and device access.
- Calls and possible failure.
- Allocation.
- Unwinding or termination.
- Atomic ordering.
- Compile-time-only operations.

The effect model need not be elaborate in `rivetc0`, but instruction semantics must be documented as they appear.

## Verification

RIR should have an independent verifier. It checks structural invariants before interpretation or backend lowering:

- Referenced blocks and registers exist.
- Instructions receive compatible operand types.
- Every block is terminated.
- Definitions dominate uses where the selected form requires it.
- Function returns match signatures.
- Capability-restricted operations are legal for the target.
- Checked guarantees are discarded only by explicit operations.

Verifier failures indicate compiler bugs, not user diagnostics, unless RIR is accepted as a deliberate user-facing input format in the future.

## Interpreter and VM

The bootstrap uses an in-process register interpreter to execute early RIR. This provides:

- A rapid end-to-end compiler milestone.
- A compile-time execution engine.
- A semantic reference for backend tests.
- A way to test programs before object emission exists.

The interpreter is not automatically the production runtime. Its purpose can remain tooling and compile-time execution even after native backends mature.

## LLVM as an early backend

**Accepted direction:** LLVM IR is a sensible initial code-generation target while Rivet's language and optimizer are taking shape. It provides object emission, register allocation, optimization, debug information, and broad architecture support.

LLVM is not intended to become an irremovable dependency or the definition of RIR. The compiler should lower RIR deliberately into LLVM IR behind a backend interface.

This makes it possible to:

- Ship native experiments sooner.
- Compare RIR interpreter behavior against native execution.
- Learn which RIR information LLVM can consume.
- Replace or supplement LLVM later without redesigning the front end.

## Long-term native output

Rivet eventually aims to own more of its output pipeline rather than permanently relying on LLVM. Potential stages include:

1. Direct assembly emission for one architecture.
2. Object-file writing for a selected format.
3. Basic register allocation and instruction selection.
4. Debug information and unwind metadata.
5. Linker integration or a dedicated linker.
6. Additional architectures and optimization passes.

This is a long-range goal. A reliable LLVM backend is preferable to a rushed custom backend that blocks language work.

## Textual and binary forms

A textual RIR form is valuable for debugging, tests, bug reports, and learning. A binary or serialized form may later support module caches and tooling.

Neither form should be declared stable prematurely. If external tools begin depending on them, each format needs an explicit version and compatibility rules separate from the Rivet source-language version.

## Relationship to LIR

RIR represents general program semantics. LIR preserves Lattice-specific information such as UI composition, resources, observation, bindings, and generated application setup until Lattice-aware validation and lowering are complete.

LIR ultimately produces or references ordinary Rivet declarations and RIR. Lattice semantics should not require every RIR consumer to become a UI framework.
