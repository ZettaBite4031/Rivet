# Tooling, builds, and repository conventions

## Primary environment

`rivetc0` is developed primarily in WSL Ubuntu through Visual Studio Code. This provides a Linux-native toolchain and shell while retaining the user's Windows desktop workflow.

The repository should remain usable from an ordinary Linux installation and avoid depending on WSL-specific paths or Windows-mounted filesystem behavior.

## Bootstrap language and toolchains

The bootstrap is C++23. Both GCC and Clang builds are valuable because they catch different diagnostics, portability problems, and accidental compiler extensions.

The build should select versioned tools or configured paths rather than assume the operating system's unversioned compiler is sufficiently new. Clang on Linux may use GCC's libstdc++; the selected compiler frontend and standard-library installation should be visible in verbose build output.

Warnings should be strong but introduced deliberately. Treating warnings as errors can be appropriate in continuous integration once both supported compilers have clean, stable warning sets.

## SCons

SCons is the chosen build system. Top-level files should handle configuration while subordinate `SConscript` files describe their local products.

Desired build qualities include:

- Automatic source discovery.
- Mirroring source directory structure beneath the build root.
- Separate debug and release variants.
- GCC and Clang selection.
- Central warning and language-standard configuration.
- Dependency tracking supplied by the compiler/SCons.
- Straightforward test discovery and execution.
- A future path for sanitizers, coverage, and compile command export.

## Source and header placement

Headers live beside their corresponding translation units:

```text
src/
├── main.cpp
├── source/
│   ├── source_file.hpp
│   └── source_file.cpp
└── syntax/
    ├── lexer.hpp
    ├── lexer.cpp
    └── token.hpp
```

The project deliberately avoids separate top-level `src/` and `include/` mirrors for internal compiler code. A future public embedding API can receive an explicit exported include surface if needed.

Concepts should remain grouped coherently. The compiler should not fragment every two-field structure into its own translation unit merely to create an impressive directory tree.

## Mirrored build paths

Generated objects preserve their source hierarchy:

```text
src/source/source_file.cpp
    → build/debug/gcc/src/source/source_file.o
```

This prevents same-named source files from colliding and makes build output easy to trace. Build variants and compiler identity may each receive a distinct directory.

No generated object, dependency file, executable, coverage output, or SCons database belongs in the source directories.

## Suggested repository shape

```text
.
├── README.md
├── SConstruct
├── .gitignore
├── docs/
├── src/
│   ├── main.cpp
│   ├── diagnostics/
│   ├── source/
│   ├── syntax/
│   ├── semantic/
│   └── rir/
└── tests/
    ├── unit/
    ├── diagnostics/
    ├── snapshots/
    └── programs/
```

This is directional. Directories should be created when they contain coherent work, not pre-populated with empty architectural promises.

## Command-line interface

The compiler should offer developer-facing inspection from the beginning. Candidate commands and switches include:

```text
rivetc0 source.rivet
rivetc0 --dump-tokens source.rivet
rivetc0 --dump-ast source.rivet
rivetc0 --dump-bound source.rivet
rivetc0 --dump-rir source.rivet
rivetc0 --run source.rivet
```

Later additions may include target selection, profile and capability reports, package graph inspection, emitted files, timing, compile-time action logs, and machine-readable diagnostics.

Exit codes should distinguish successful compilation, source diagnostics, command misuse, and internal compiler failure.

## Formatting and linting

The C++ bootstrap should eventually adopt a checked-in `.clang-format` once the preferred style is stable. Formatting tools should not be introduced with defaults that mechanically rewrite existing intentional style.

Static analysis and sanitizers are valuable after the first vertical slice:

- AddressSanitizer and UndefinedBehaviorSanitizer.
- Clang-Tidy with a curated check set.
- Leak detection where supported.
- Coverage for phase and diagnostic tests.

## Generated and committed files

Source-level RIR/LIR fixtures used by tests should be committed. Generated build RIR and LIR should live beneath ignored build directories.

Package lockfiles should normally be committed once the package system exists. Local caches, environment files, and machine-specific SCons overrides should not be committed.

## Continuous integration direction

An early CI matrix should build and test debug configurations under supported GCC and Clang versions. Later dimensions include sanitizers, release mode, target profiles, bootstrap reproducibility, and self-hosting stages.

A self-hosted compiler eventually needs a staged comparison:

1. Build compiler A with the trusted bootstrap.
2. Build compiler B with compiler A.
3. Build compiler C with compiler B.
4. Compare B and C under a defined reproducibility policy.

Byte-for-byte identity is desirable but may require deterministic timestamps, paths, identifiers, and backend behavior.
