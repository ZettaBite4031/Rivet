# Rivet documentation

This directory records the current design of Rivet, its compiler pipeline, RIR, Lattice, and LIR. It distinguishes decisions already accepted for the project from directional ideas and unresolved questions.

## Reading order

| Document | Subject |
| --- | --- |
| [00 — Document status](00-document-status.md) | Meaning of specification labels, compatibility expectations, and the design process. |
| [01 — Vision and principles](01-vision-and-principles.md) | What Rivet is trying to become and how design choices are evaluated. |
| [02 — Language and program structure](02-language-and-program-structure.md) | Surface character, source files, entry points, functions, classes, and declaration rules. |
| [03 — Type system and values](03-type-system-and-values.md) | Primitive and user-defined types, declarations, immutability, characters, bytes, and conversions. |
| [04 — Memory and pointers](04-memory-and-pointers.md) | Checked pointers, raw pointers, references, ownership direction, and low-level access policy. |
| [05 — Errors and failure](05-errors-and-failure.md) | C++-like core behavior, optional structured errors, `nofail`, fatal failure, and exceptions. |
| [06 — Modules and packages](06-modules-and-packages.md) | One-module-per-file rule, privacy, imports, re-exports, cycles, packages, and discovery. |
| [07 — Compile time and metaprogramming](07-compile-time-and-metaprogramming.md) | `#run`, conditional compilation, reflection, generation, and phase boundaries. |
| [08 — Attributes and extensions](08-attributes-and-extensions.md) | Python-like decorator syntax, semantic attributes, `observe`, and framework independence. |
| [09 — Targets, runtime, and standard library](09-targets-runtime-and-standard-library.md) | Hosted and freestanding profiles, capabilities, host stubs, checks, and portability. |
| [10 — RIR and code generation](10-rir-and-code-generation.md) | Register-oriented IR, execution, optimization, LLVM bootstrapping, and native backends. |
| [11 — Lattice and LIR](11-lattice-and-lir.md) | Framework boundary, application startup, UI markup, observation, binding, and LIR. |
| [12 — Compiler architecture and bootstrap](12-compiler-architecture-and-bootstrap.md) | `rivetc0` stages, ownership, diagnostics, minimal grammar, self-hosting, and roadmap. |
| [13 — Tooling, builds, and repository](13-tooling-builds-and-repository.md) | WSL, C++23, SCons, mirrored build paths, debugging surfaces, and developer workflow. |
| [14 — Verification strategy](14-verification-strategy.md) | Unit, diagnostic, snapshot, integration, backend, and conformance testing. |
| [15 — Open questions](15-open-questions.md) | Decisions that are deliberately not frozen yet. |
| [16 — Glossary](16-glossary.md) | Shared terminology for the language and ecosystem. |

## Scope

These documents describe the intended shippable language as well as the deliberately narrower bootstrap. They are not generated API documentation, and they do not claim that every described feature has been implemented.

The source tree and tests are authoritative about current behavior. These documents are authoritative about accepted design intent unless a newer design decision explicitly supersedes them.
