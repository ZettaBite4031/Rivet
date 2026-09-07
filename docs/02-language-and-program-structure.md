# Language and program structure

## Surface direction

**Accepted direction:** Rivet uses a brace-delimited, C-family surface with type-first declarations. It should feel immediately navigable to a C++ programmer without preserving C++ syntax solely for compatibility.

Keywords and primitive type names are lowercase. User-defined and other complex type names conventionally begin with an uppercase letter.

```rivet
module sample;

int double_value(int value) {
    return value * 2;
}
```

Exact naming conventions beyond the primitive/user-defined distinction remain a style-guide concern rather than core semantics.

## Source files and modules

Every source file declares exactly one module. Several files may contribute declarations to the same module. A file cannot switch between multiple modules because that makes dependencies, ownership, tooling, and file-level reasoning unnecessarily muddy.

```rivet
module graphics.rendering;
```

Whether dotted module spelling is final remains provisional, but nested modules are an accepted direction. Module behavior is described fully in [Modules and packages](06-modules-and-packages.md).

## Raw Rivet entry points

Raw Rivet supports a free function entry point:

```rivet
int main() {
    return 0;
}
```

It also supports organizing the same logical entry point in the conventional default class `Main`:

```rivet
class Main {
    int main() {
        return 0;
    }
}
```

**Accepted direction:** these are organizational forms of the same program entry contract, not two unrelated startup systems. A program may use one form, and the compiler must diagnose ambiguous multiple entries.

The detailed rules still need to settle parameter types, asynchronous entry, return types other than `int`, visibility, and whether the class form's `main` is implicitly static.

## Lattice application startup

Lattice applications do not normally declare `main`. The framework and its build tooling synthesize or select startup based on application metadata, markup, and generated setup.

This is a Lattice convention, not a different Rivet grammar. The generated or framework-supplied entry ultimately lowers through ordinary Rivet compilation. See [Lattice and LIR](11-lattice-and-lir.md).

## Functions

Rivet functions use type-first return and parameter declarations:

```rivet
Result open_document(string path) {
    // ...
}
```

**Accepted direction:** ordinary function declarations without definitions are not part of normal Rivet source. The compiler performs multiple passes over modules to discover definitions, eliminating the need for C-style header files and forward declarations.

This has several consequences:

- Source order should not determine whether a function can be referenced.
- Mutually recursive functions are resolved across compiler passes.
- Public API surfaces are derived from definitions and visibility.
- External or platform-provided symbols need an explicit foreign-interface mechanism rather than a definitionless ordinary function.
- Abstract, virtual, intrinsic, generated, and imported functions require distinct semantics and should not masquerade as forgotten definitions.

Default arguments, overloading, generics, variadics, and calling-convention syntax remain future design work.

## Classes and aggregate types

Classes are optional program-organization tools, not a container every Rivet program must inhabit. Free functions and module-level declarations remain normal.

Inside a class implementation, members are accessible without a mandatory `self.` prefix:

```rivet
class Counter {
    mut int value = 0;

    void increment() {
        value += 1;
    }

    Counter& identity() {
        return this;
    }
}
```

`this` explicitly denotes the current instance. Unqualified lookup should prefer well-defined lexical rules and diagnose ambiguous shadowing. Requiring a qualification prefix everywhere is deliberately rejected.

The exact object model—inheritance, interfaces or traits, virtual dispatch, layout control, constructors, destructors, and move/copy generation—is not yet frozen.

## Statements and expressions

The bootstrap begins with blocks, `return`, and integer literals. The intended full language includes conventional structured control flow, operators, calls, member access, construction, and pattern-like handling where it genuinely improves error and variant types.

Rivet should avoid special statement syntax that cannot lower cleanly across hosted and freestanding targets. High-level constructs may still require libraries or capabilities even when their language semantics are target-independent.

## Name lookup

The compiler should separate parsing from binding. Parsing determines syntactic structure; binding resolves names, types, members, imports, overloads, and contextual meanings.

This separation is especially important for:

- Implicit member access inside implementations.
- Contextual framework specifiers such as `observe`.
- Imports and re-exports.
- Compile-time-generated declarations.
- Multiple files contributing to one module.

The parser should not need knowledge of the symbol table merely to construct a valid syntax tree.

## Keywords and contextual vocabulary

Core language words such as `module`, `import`, primitive types, and control-flow words may be reserved keywords. Framework vocabulary should not automatically become globally reserved.

For example, `observe` has no inherent meaning in raw native Rivet. The design direction is to interpret it through a general declaration-specifier or attribute mechanism when the active environment provides that semantic extension. Outside that context, it should remain available as an ordinary identifier or produce a targeted “provider unavailable” diagnostic only when used in specifier position.

This requirement prevents Lattice from permanently occupying names or controlling the core AST.
