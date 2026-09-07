# Document status and decision process

Rivet is still being designed while `rivetc0` is being built. Documentation must therefore communicate both ambition and uncertainty without confusing one for the other.

## Status labels

Documents use the following labels where a distinction matters:

| Label | Meaning |
| --- | --- |
| **Accepted direction** | The project intends to preserve this behavior unless implementation reveals a serious flaw. Syntax may still receive minor refinement before the first language version is frozen. |
| **Provisional** | The concept belongs in Rivet, but its spelling, detailed semantics, or implementation strategy remains open. |
| **Exploratory** | A candidate worth testing rather than a promise. |
| **Open question** | The project has intentionally not selected an answer. |
| **Bootstrap subset** | Behavior required by `rivetc0` before the broader language is implemented. |

An unimplemented accepted direction is still unimplemented. It should not be advertised as current compiler functionality.

## Design records

Important decisions should eventually receive short design records containing:

1. The problem and relevant constraints.
2. Representative syntax and behavior.
3. Alternatives that were seriously considered.
4. Effects on parsing, typing, lowering, tooling, and portability.
5. The final decision and its current status.

The purpose is not bureaucracy. It is to prevent old questions from reopening because nobody remembers why the current answer exists.

## Compatibility expectations

The bootstrap language has no stability guarantee. Before a numbered Rivet release, syntax and IR may change freely when the improvement justifies migration.

Eventually, compatibility should be stated independently for:

- Rivet source language versions.
- Standard-library APIs and ABI.
- RIR textual and binary formats.
- Lattice and UI markup versions.
- Package manifests and lockfiles.
- Compiler extension interfaces.

Those surfaces should not accidentally inherit one another's version number or compatibility policy.

## How proposals are judged

A feature should answer more than “is this neat?” Fun is a valid goal for Rivet, but a lasting feature should also improve at least one of clarity, capability, performance, safety, portability, diagnostics, or tooling.

Particular scrutiny belongs on features that:

- Create contextual keywords or phase-dependent parsing.
- Require runtime support on freestanding targets.
- Add implicit allocation, ownership, or control flow.
- Change ABI or cross-module compilation.
- Allow compile-time code to access the host environment.
- Give frameworks influence over core language semantics.

The preferred experiment is the smallest implementation that can expose the real tradeoff.
