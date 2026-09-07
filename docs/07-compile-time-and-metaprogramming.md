# Compile-time execution and metaprogramming

## Goal

Rivet should provide a rich compile-time environment as a defining capability. The desired experience is influenced strongly by Jai's direct build-time execution and by the practical needs currently served by C/C++ preprocessing and template metaprogramming.

The result should permit generation, reflection, validation, configuration, and build orchestration without forcing programmers into textual macros or a massive type-traits library.

## `#run`

**Accepted direction:** Rivet includes a `#run`-style directive that evaluates code during compilation or building.

```rivet
#run generate_protocol_tables();
```

The end goal is broad enough to cover the useful capabilities associated with Jai's directive, while allowing Rivet to choose different syntax or boundaries where that produces a more coherent toolchain.

Compile-time code should be able to perform ordinary computation and interact with compiler-provided reflection and generation APIs. Environmental access—files, commands, network, clocks, randomness—requires explicit policy because it affects reproducibility and security.

## Conditional compilation

Rivet needs to include or exclude code based on compile-time values, covering the legitimate use cases of C/C++ `#if` without reducing the source to untyped token substitution.

Illustrative, not final, syntax:

```rivet
#if target.os == Os.windows {
    import platform.windows;
} #else {
    import platform.posix;
}
```

Conditions should be typed compile-time expressions. Undefined macro names should not quietly become zero, and discarded branches should still have well-defined lexical and syntactic behavior.

Typical inputs include:

- Target architecture, operating system, ABI, and endianness.
- Hosted or freestanding profile.
- Package features and build modes.
- Availability of standard-library capabilities.
- User-defined compile-time constants.

## Reflection

The compiler should expose structured semantic information about types and declarations. Expected queries include:

- Type identity and category.
- Size and alignment for a selected target.
- Fields, methods, visibility, and attributes.
- Generic parameters and arguments.
- Supported construction, copying, moving, and destruction.
- Interfaces, constraints, and conversions.
- Source location when available.

Reflection values should be usable by compile-time Rivet code. They should not depend on runtime type information unless a program explicitly emits runtime metadata.

## Generation

Compile-time code may need to generate declarations, functions, data, or resources. The exact mechanism is open.

Possible models include:

- Returning compiler syntax or semantic objects.
- Emitting declarations through a controlled builder API.
- Producing source fragments that are parsed with precise provenance.
- Generating RIR for specialized low-level cases.

Structured generation is preferred over arbitrary string substitution because it preserves hygiene, source attribution, type checking, and tooling. Text generation may still be valuable for external files or interoperability.

## Phase model

Compile-time power requires visible phase rules. At minimum, the compiler must distinguish:

1. Parsing source.
2. Establishing declarations and imports.
3. Binding enough semantics to execute compile-time code.
4. Running compile-time actions.
5. Integrating generated declarations or data.
6. Completing binding and lowering runtime code.

Cycles between these phases must be diagnosed rather than resolved through unpredictable source order.

## Execution environment

Compile-time code may initially run in the bootstrap compiler process or through the RIR interpreter. The long-term environment should have explicit capabilities:

- Pure computation and allocation.
- Compiler reflection and diagnostics.
- Reading declared input files.
- Writing declared generated outputs.
- Invoking approved tools.
- Optional environment, time, randomness, and network access.

A build should be able to report which capabilities were used. Sandboxing and reproducibility modes can restrict them without removing `#run` from the language.

## Caching and determinism

Compile-time results can be cached only when their inputs are known. The compiler should track source, target configuration, imported semantic interfaces, declared files, environment values, and tool versions used by a compile-time action.

Pure actions are naturally cacheable. Impure actions may opt out or declare dependencies. Silent access to arbitrary host state would make incremental builds unreliable.

## Diagnostics

Errors from compile-time execution should show both:

- The location in compile-time Rivet code where failure occurred.
- The source directive or generated declaration that requested the work.

Generated code needs provenance so later type errors can point back through the generation chain. “Error somewhere in generated output” is not an acceptable final developer experience.

## Relationship to the standard library

The language provides primitive reflection and compile-time execution. The standard library provides algorithms, builders, formatting, collection types, and friendly wrappers over those primitives.

This division avoids repeating C++'s situation where elementary semantic queries are expressed through extensive template machinery, while still keeping the compiler surface small enough to evolve deliberately.
