# Glossary

## Binder

The compiler phase that resolves parsed names into symbols, determines types, validates semantic rules, and produces a bound representation. It is intentionally separate from parsing.

## Bootstrap compiler

The initial C++23 implementation, named `rivetc0`, used to establish enough Rivet to compile a later compiler written in Rivet.

## Capability

A named environmental or runtime facility—such as allocation, file I/O, unwinding, or Lattice UI services—that code may require and a target or provider may supply.

## Checked pointer

A compiler-intrinsic pointer-like type, provisionally spelled `T&`, carrying stronger invariants than a raw machine address. Its exact guarantees remain open.

## Compile-time execution

Rivet code evaluated by the toolchain while compiling or building, including `#run`, reflection, validation, and generation.

## Complex type

Project vocabulary for non-primitive types such as `Result`, `Window`, classes, records, or generic library types. These are conventionally capitalized.

## Freestanding

A target profile that assumes no conventional operating system, process, filesystem, allocator, console, or monolithic language runtime unless explicitly supplied.

## Hosted

A target profile with selected operating-system and runtime services. Hosted does not automatically mean every standard-library capability is enabled.

## Lattice

The optional application and UI framework designed to integrate closely with Rivet while remaining independent from the core language.

## LIR

Lattice Intermediate Representation: a typed, framework-aware representation for application structure, UI markup, resources, observation, bindings, and generated setup.

## Module

A named scope, compiler compilation unit, and dependency unit. Each source file declares one module; several files may contribute to the same module.

## Namespace

A name-organization scope within a module. A module may contain multiple namespaces, but namespaces do not automatically become independent dependency units.

## Observation

A Lattice-oriented semantic facility for state whose changes participate in bindings or notifications. It is intended to use general Rivet extension machinery rather than a permanent core keyword.

## Package

A distributable or workspace unit containing modules, resources, configuration, dependencies, and target implementations. Its manifest and versioning model remain open.

## Primitive type

A compiler-defined foundational type with lowercase spelling, such as `int`, `float`, `byte`, `char`, or `void`.

## Provider

An implementation of one or more capabilities or semantic extensions, supplied by a runtime, target, package, framework, or application.

## Raw pointer

A direct machine-address type spelled `T*`, intended for systems, foreign, allocator, driver, and platform work. Its use is not universally wrapped in mandatory `unsafe` blocks.

## RIR

Rivet Intermediate Representation: the typed, register-oriented representation used for analysis, interpretation, optimization, and backend lowering.

## Rivet

The native, statically typed programming language. Rivet remains useful independently of Lattice.

## `rivetc`

The eventual self-hosted Rivet compiler.

## `rivetc0`

The deliberately constrained bootstrap compiler implemented in C++23.

## Semantic extension

A structured compiler-supported mechanism by which an attribute, package, target, or framework validates or generates program behavior without taking unrestricted ownership of Rivet's AST.

## Source span

A range in a source file, initially represented by byte offset and length, used by tokens, syntax, diagnostics, and source mapping.

## Vertical slice

A minimal feature carried through every compiler phase. The first Rivet slice compiles a module containing `int main() { return 42; }` through RIR execution.
