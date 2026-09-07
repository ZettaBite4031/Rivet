# Vision and design principles

## The short pitch

Rivet is a native, statically typed language that combines systems-level control with modern application development and unusually capable compile-time programming. It can stand alone from freestanding kernel code through ordinary desktop software, while integrating naturally with the optional Lattice framework.

## The longer pitch

Rivet is intended for programmers who like the directness, performance, and mechanical honesty of C++ but want a language designed around a coherent modern compiler rather than decades of accumulated compatibility. It borrows the strongest ideas from systems and application languages—explicit native control, checked abstractions, rich modules, compile-time execution, structured metadata, and productive UI integration—then asks how those ideas can coexist without making one philosophy dominate every program.

The result should be comfortable in two very different places. In a freestanding zOS component, Rivet should permit precise memory and platform work without dragging in an operating-system runtime. In a Lattice desktop application, the same language should support declarative UI, observation, resources, tooling, and generated application setup. Neither environment defines the language; each selects capabilities from it.

Rivet is inspired by the Windows and .NET ecosystem in its desire for a complete, polished developer experience, but it is not a re-skinned C# or Java. Its surface language is closer in spirit to C++ and Rust, and its lower layers remain native and explicit.

## Desired character

Rivet should feel:

- **Direct.** The programmer can see types, mutability, control flow, and important costs.
- **Native.** Low-level targets and platform APIs are first-class use cases.
- **Capable.** High-level applications should not require abandoning the language for a more productive ecosystem.
- **Playful.** Compile-time execution, reflection, and generation should make ambitious experiments enjoyable.
- **Coherent.** Borrowed ideas must fit Rivet's own model rather than remain visible seams between source languages.
- **Toolable.** Diagnostics, source spans, deterministic builds, and inspectable IR are architectural features.

## Core principles

### Rivet is independent

Rivet must be useful without Lattice. The language cannot require UI concepts, an application object, a garbage collector, or a hosted operating system. Lattice-specific semantics should enter through general extension mechanisms such as attributes, generated code, metadata, libraries, and compiler-recognized capability providers.

### Safety is a facility, not a religion

Rivet should provide excellent checked types and diagnostics, but it should not label all direct memory access as morally suspect or wrap ordinary systems work in pervasive `unsafe` blocks. The programmer may select stricter enforcement through compiler or project policy.

This is not permission for silent chaos. Operations with unverifiable invariants still need precise semantics, good diagnostics, and optional enforcement. The distinction is that policy should be selectable rather than fused to the grammar wherever possible.

### Compile-time programming belongs in the language

Rivet should not require a huge traits library to ask elementary questions about a type. Reflection, type queries, conditional generation, and build-time execution should be supported by the compiler's semantic model.

The standard library may provide friendly wrappers, algorithms, and vocabulary, but it should not have to simulate the compiler.

### Portability means expressing missing capabilities

“Available on every target” cannot mean pretending every platform has files, threads, virtual memory, or a display. It means preserving the language feature wherever its semantics make sense, describing environmental requirements explicitly, and producing an early useful error when requirements cannot be met.

### Familiarity has value

Rivet does not need a novel spelling for every familiar operation. `import`, type-first declarations, `this`, braces, and conventional operators are useful precisely because programmers already understand them. Novel syntax should earn its cost.

### The whole pipeline should remain inspectable

The compiler should expose tokens, syntax trees, bound representations, RIR, LIR when applicable, and backend output. This improves debugging, tests, metaprogramming, tooling, and the language designer's own understanding.

## Non-goals

Rivet is not intended to be:

- A source-compatible replacement for C++, Rust, C#, or any other language.
- A language whose identity depends on a mandatory garbage collector.
- A thin syntax layer over LLVM IR.
- A UI language with a native subset attached.
- A compiler that relies permanently on C-style header declarations.
- A safety experiment that makes low-level programming secondary.
- Different merely so its syntax can be called unique.

## Success in a shippable state

A shippable Rivet should be able to compile itself, build useful native programs across hosted and freestanding profiles, provide stable modules and packages, emit through at least one production-quality backend, offer understandable diagnostics, and integrate with Lattice without compromising raw Rivet.

Its ecosystem should make a small program easy while leaving enough exposed structure to build an operating system, engine, compiler, or platform runtime without fighting hidden assumptions.
