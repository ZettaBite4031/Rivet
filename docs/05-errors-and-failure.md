# Errors, failure, and contracts

## Core philosophy

Error handling should be a powerful layer available to Rivet programs, not a mandatory runtime ideology baked so deeply into the language that freestanding use becomes awkward.

**Accepted direction:** ordinary Rivet functions begin closer to C++ semantics. A function may signal errors using return values, structured result types, exceptions if the selected environment supports them, termination for truly unrecoverable conditions, or domain-specific mechanisms.

Rivet should provide strong ergonomics for expected failures without requiring every environment to implement the most elaborate model.

## Categories of failure

The design distinguishes several concepts:

- **Expected failure:** a documented outcome such as a missing file, invalid input, unavailable resource, or declined operation.
- **Recoverable fault:** a failure from which the current program can meaningfully continue.
- **Fatal error with information:** a condition the API designer treats as fatal but still wants to describe or propagate before termination.
- **Contract violation:** the caller or implementation violated a stated precondition, postcondition, or invariant.
- **Unrecoverable runtime failure:** continuing execution cannot be given meaningful semantics.

These categories should not all collapse into process termination, nor should every one require an allocated exception object.

## Structured expected errors

A Rust- or Zig-inspired structured result model is desirable. `Result` is the working vocabulary type:

```rivet
Result<Document, OpenError> open_document(string path) {
    // ...
}
```

The final syntax may differ to give Rivet a cleaner visual identity and better interaction with qualifiers, propagation, and pattern handling. Structured errors should remain implementable as ordinary types with compiler-provided ergonomics rather than an opaque second calling convention unless performance evidence requires one.

Possible ergonomic facilities include:

- Concise propagation.
- Pattern matching or destructuring.
- Compile-time access to the success and error types.
- A way to deliberately discard or terminate on an error.
- Clear conversion rules between error domains.

None of these spellings is frozen.

## Failable functions are not compulsory-handling traps

**Accepted direction:** callers are not universally required to handle a function's declared failure information. The programmer may deliberately ignore, unwrap, propagate, convert, or terminate according to context.

The compiler may provide warnings or stricter project policies for ignored results, but the base language should not make every call site ceremonious.

This policy is particularly important for experiments, kernel paths, tools, and applications whose appropriate response is immediate failure with a diagnostic.

## `nofail`

`nofail` is the current working name for a contract stating that a function does not permit its declared failure mechanism to escape.

```rivet
nofail int calculate_constant() {
    return 42;
}
```

**Provisional semantics:** inside a `nofail` function, calls that may fail must be proven safe, converted into a value, handled locally, or explicitly converted to a fatal operation. The compiler can then use the contract for diagnostics, optimization, API documentation, and Lattice tooling.

`nofail` must remain viable without a particular structured-error runtime. It describes a semantic boundary; it does not force every function to use `Result`.

Open questions include whether `nofail` covers exceptions, allocation failure, hardware traps, cancellation, and contract violations, or only language-level declared failures.

## Exceptions

No final exception model has been selected. If exceptions exist, they must be optional at the target/profile level and cannot be required to express ordinary Rivet errors.

Freestanding or kernel profiles may disable unwinding entirely. Hosted profiles may provide zero-cost unwinding or another implementation. Code requiring exceptions must declare that capability rather than failing mysteriously during linking.

## Fatal termination

Process termination is appropriate only when continuing is genuinely invalid or when the application explicitly chooses termination as its handling strategy.

The core language cannot assume a process exists. On a hosted target, fatal termination may exit a process and report diagnostics. In a kernel, it may panic or enter a platform-defined fatal handler. In an embedded environment, it may reset, halt, or invoke a supplied callback.

Therefore fatal behavior should lower through a target/runtime capability, with a compile-time error if no implementation exists and the path remains reachable.

## Interaction with targets

Error facilities advertise their requirements:

- A value-based `Result` may require no runtime support.
- Formatted diagnostics may require allocation or output.
- Stack unwinding requires unwind tables and runtime personality functions.
- Process termination requires a hosted process or supplied fatal hook.
- Backtraces require platform metadata and services.

This decomposition allows most useful error handling to survive on constrained targets without pretending every convenience is free.
