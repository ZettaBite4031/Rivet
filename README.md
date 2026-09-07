<div align="center">

# Rivet

## A native language for building close to the machine without living at its mercy.

Rivet combines systems-level ocntrol, a modern compile-time environment, and an approachable application model. It stands alone as a general-purpose language, while fitting naturally into a larger ecosystem.

![Status](https://img.shields.io/badge/status-bootstrap%20in%20progress-f2a65a?style=flat-square)
![Bootstrap](https://img.shields.io/badge/rivetc0-C%2B%2B23-5f7adb?style=flat-square)
![Build](https://img.shields.io/badge/build-SCons-9b72cf?style=flat-square)

</div>

> [!IMPORTANT]
> Rivet is currently in its bootstrap phase. The repository is building `rivetc0`, the deliberately small C++ compiler that will establish the language well enough for Rivet to eventually compile itself.

## What is Rivet?

Rivet is a compiled, statically typed language designed to cover both ordinary native applications and low-level freestanding software. Its syntax leans toward the clarity and directness of C++ and Rust without trying to impersonate either one. Its long-term character comes from three ideas:

- Native control should remain available instead of being hidden behind a managed runtime.
- Safety and checked abstractions should help the programmer without turning every low-level operation into a ritual of apology.
- Compile-time programming should be a first-class part of the language, not an accidental maze built out of templates and library tricks.

Rivet is the language. **Lattice** is an optional application and UI framework. **RIR** is Rivet's general-purpose intermediate representation. **LIR** is the lowered, target-specific intermediate representation. None of those layers should blur the fact that raw Rivet remains useful by itself.

## A taste of the intended language

The following examples illustrate the direction of the design; they are not all accepted by `rivetc0` yet.

```rivet
module hello;

int main() {
    string message = "Hello from Rivet";
    Console.WriteLine(message);
    return 0;
}
```

Declarations use type-first syntax and local values are immutable unless mutability is requested explicitly:

```rivet
int answer = 42;
mut int attempts = 0;
```

Rivet distringuishes checked pointer-like access from raw addresses:

```rivet
Widget& checked_widget;
Widget* raw_widget;
```

And compile-time execution is intended to be direct enough to enjoy using:

```rivet
#run generate_lookup_tables();
```

The exact syntax of several advanced features remains provisional. The documentation marks those boundaries rather than pretending the paint has already dried.

## The Lattice ecosystem

| Piece | Responsibility | 
| --- | --- | 
| **Rivet** | The language: syntax, type system, native semantics, compile-time facilities, and platform model. | 
| **rivetc0** | The C++23 bootstrap compiler used to establish the first executable language slice. |
| **rivetc** | The eventual self-hosted Rivet compiler. | 
| **RIR** | Typed, register-oriented IR for Rivet programs and general optimization/code generation. |
| **LIR** | Lowered intermediate representation meant to be more target-specific. | 
| **Lattice** | Lattice-aware representation for UI composition, bindings, resources, generated setup, and framework metadata. |

Lattice should feel like an excellant framework written for Rivet-not the hidden reason Rivet exists. A console tool, kernel component, game engine, compiler, or embedded program should never feel like a Lattice application with the furnishings removed. 

## Current bootstrap target

The first vertical slice is intentionally tiny: 

```rivet
module hello

int main() {
    return 42;
}
```

The goal is to carry that program through the entire architecture:

```text
SourceFile -> Lexer -> Parser -> Binder -> RIR -> RIR Interpreter
```

Returning an integer proves the pipeline without prematurely requiring strings, file I/O, a runtime library, or native object emission.

## Development environment

The bootstrap compiler is developed primarily under WSL Ubuntu with GCC and Clang. It uses C++23 and SCons. Headers live beside their translation units, and the source directory hierarchy is mirrored beneath the build directory.

Typical commands will resemble:

```bash
scons compiler=gcc mode=debug
scons compiler=clang mode=debug
scons test
```

The precise options supported by the repository's `SConstruct` remain authoritative.

## Design compass

Rivet is guided by a handful of durable principles:

1. **Prove a vertical slice before expanding horizontally.** A complete tiny compiler teaches more than twelve immaculate but disconnected subsystems.
2. **Pay for what the program uses.** Hosted convenience must not prevent freestanding or kernel development.
3. **Prefer explicit power over compulsory ceremony.** Checked facilities should be excellent; raw facilities should remain available.
4. **Keep the language independent from its flagship framework.** Lattice consumes Rivet extension points rather than owning Rivet's AST.
5. **Make compile-time work feel like ordinary engineering.** Reflection, generation, and conditional compilation belong in the language model.
6. **Use familiarity where it helps.** Rivet may borrow successful ideas, but should only diverge when the difference buys clarity, capability, or coherence.
7. **Keep diagnostics and tooling in the architecture from the beginning.** A compiler that can explain itself is easier to build and substantially nicer to use.

## Documentation

the [documentation index](docs/README.md) provides the complete map. Useful starting points include:

- [Vision and design principles](docs/01-vision-and-principles.md)
- [Language structure and syntax](docs/02-language-and-program-structure.md)
- [Types, values, and declarations](docs/03-type-system-and-values.md)
- [Memory, references, and pointers](docs/04-memory-and-pointers.md)
- [Modules, packages, and imports](docs/06-modules-and-packages.md)
- [Compile-time execution and metaprogramming](docs/07-compile-time-and-metaprogramming.md)
- [RIR and backend strategy](docs/10-rir-and-code-generation.md)
- [Lattice, LIR, and UI markup](docs/11-lattice-and-lir.md)
- [`rivetc0` architecture and bootstrap roadmap](docs/12-compiler-architecture-and-bootstrap.md)
- [Open design questions](docs/15-open-questions.md)

## Project status

The toolchain can currently load a single source file through `SourceFile`. The immediate implementation work is source spans and diagnostics followed by tokenization of the first executable grammar. Features described elsewhere are the intended language direction, not a claim that the bootstrap compiler implements them today.

## Contributing and experimentation

Rivet is young enough that small experiments can still change important decisions. Proposals are most useful when they include:

- The concrete problem being solved.
- A few representative code examples
- Interaction with freestanding targets and compile-time execution.
- Parser, type-system, IR, and tooling consequences.
- A reason the difference improves Rivet rather than merely making it unusual.

See [Decision process and specification status](docs/00-document-status.md) before treating provisional syntax as a compatibility promise.

## License

No license has been selected yet. Until one is added, the source is not automatically granted an open-source license merely because it is visible on GitHub. Selecting a license is tracked as an explicit pre-reelease decision.

<div align="center">

- built one small, stubbornly complete slice at a time - 

</div>
