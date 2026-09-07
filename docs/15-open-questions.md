# Open design questions

This document keeps unresolved choices visible so provisional examples do not quietly become accidental language law.

## Syntax and declarations

- Final spelling and placement of the explicit mutability qualifier.
- Type-inference syntax, if any.
- Full literal syntax, suffixes, digit separators, and overflow rules.
- Entry-point parameters, alternate return types, async entry, and static semantics of `Main.main`.
- Generic parameter and constraint syntax.
- Overload-resolution rules and conversions.
- Whether declaration specifiers and `@attributes` share one semantic mechanism.
- Complete reserved-keyword policy versus contextual keywords.

## Object and value model

- Categories of aggregate, class, interface, trait, concept, enum, and tagged variant.
- Copy, move, destruction, and initialization rules.
- Inheritance and virtual dispatch.
- Stable layout controls and ABI guarantees.
- Interior mutability and aliasing.
- Constant and compile-time value models.

## Text

- Encoding and precise meaning of `char`.
- Name and width guarantees for `wchar` or explicit character types.
- Unicode scalar versus code-unit types.
- String ownership, slicing, literals, interpolation, and null termination.

## Memory

- Exact guarantees carried by `T&`.
- Exact distinction between `ref` references and checked pointers.
- Nullability syntax and behavior.
- Ownership and lifetime analysis.
- Pointer provenance, aliasing, arithmetic, and integer conversion.
- Strict raw-access policy modes and which operations they govern.
- Address-space and device-memory syntax.

## Errors

- Final structured result spelling and propagation syntax.
- Whether ignored failable values warn by default.
- Precise scope of `nofail`.
- Exception existence, syntax, ABI, and profile availability.
- Panic/fatal hooks and formatting on constrained targets.
- Contract syntax and debug/release behavior.

## Modules and packages

- Nested module spelling and its relationship to directories.
- Namespace syntax inside modules.
- Fine-grained visibility levels.
- Selective imports, aliases, and selective re-exports.
- Conflict resolution for multiple public imports.
- Package manifest and lockfile format.
- Versioning scheme and source/ABI compatibility policy.
- Registry, cache, workspace, Git, and path discovery.
- Handling multiple dependency versions.
- Runtime initialization across module cycles.

## Compile time

- Final `#run` and conditional-compilation grammar.
- Reflection API shape and stability.
- Structured generation API.
- Hygiene and name capture.
- Host capability permissions and sandboxing.
- Cache dependency tracking.
- Phase ordering for generated imports and declarations.
- Whether compile-time execution uses only RIR or may also run trusted native helpers.

## Extensions and Lattice

- Registration and trust model for semantic extension providers.
- Final surface spelling of observed properties.
- Observation change, threading, lifetime, and equality policies.
- UI markup name and grammar.
- LIR schema and versioning.
- Static versus dynamic binding behavior.
- Rendering and platform architecture.
- Hot reload and generated-state migration.

## Runtime and targets

- Project configuration syntax.
- Standard capability taxonomy.
- Required-provider declaration syntax.
- Reachability rules for unavailable code.
- Allocation failure behavior.
- Runtime component ABI and versioning.
- Official initial hosted, kernel, embedded, and architecture targets.

## RIR and backends

- SSA requirements and construction form.
- Precise type and effect system.
- Checked-pointer representation.
- Text and binary syntax.
- Optimization-pass architecture.
- Debug information and source provenance format.
- First production LLVM target and first custom native backend target.
- Stable embedding and tooling APIs.

## Project governance

- Open-source license.
- Version numbering before and after self-hosting.
- Compatibility and deprecation schedule.
- Proposal/design-record format.
- Official style guide and formatter behavior.
- Release artifact and bootstrap trust policy.

## How to close an open question

Prefer a concrete motivating program and a narrow prototype. A decision should document syntax, semantics, diagnostics, target impact, compiler phases, and rejected alternatives. Questions should not be closed merely because one spelling looks pleasing in isolation.
