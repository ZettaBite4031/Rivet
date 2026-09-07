# Modules, imports, and packages

## Module responsibilities

**Accepted direction:** a Rivet module serves three closely related roles:

1. A named scope for declarations.
2. A compilation unit visible to the compiler and incremental build system.
3. A dependency unit used by imports and package resolution.

A module may contain more than one nested namespace or logical API area. “Module” and “namespace” are therefore related but not identical concepts.

## One module per file

Every Rivet source file declares one module:

```rivet
module lattice.graphics;
```

A file cannot contribute to unrelated modules. Several files may declare the same module, allowing a large module to be organized without C-style header/source separation.

This rule supports:

- Predictable dependency ownership.
- Straightforward source discovery.
- Clear diagnostic and tooling boundaries.
- Parallel parsing and compilation.
- A clean answer to “which module owns this declaration?”

## Nested modules and namespaces

Modules can be nested. The precise spelling and whether nesting implies directory layout remain provisional.

A module may also contain multiple namespaces. Namespaces organize names inside the dependency boundary; they do not independently become packages or compiler dependency nodes unless promoted to modules.

## Privacy

**Accepted direction:** declarations are private by default.

```rivet
module network.http;

int parse_internal_state() {
    // private to its visibility scope
}

public Response send(Request request) {
    // exported API
}
```

The exact distinction among file-private, module-private, package-private, protected, and public visibility remains open. Defaults should minimize accidental API surfaces.

## Imports

Rivet uses `import`, not textual C/C++ `#include` and not the keyword `use`:

```rivet
import core.collections;
```

Imports resolve semantic declarations. They do not copy source text into the importing file.

This permits the compiler to understand dependency structure, cache module products, diagnose cycles meaningfully, and avoid preprocessing behavior that changes tokens before parsing.

## Public imports and re-exports

Imports are not transitive by default. A module's dependency does not automatically become visible to its consumers.

```rivet
public import graphics.color;
```

`public import` re-exports an imported API through the current module. This is the preferred way to construct façade modules rather than manually repeating aliases for every declaration.

The detailed re-export model remains open:

- Whether the entire imported module is re-exported or selected declarations can be listed.
- How renaming and aliasing work.
- How conflicts between multiple public imports are diagnosed.
- Whether a consumer sees the original symbol identity or a façade-owned alias.
- How documentation and semantic versioning attribute ownership.

## Circular dependencies

**Accepted direction:** module imports may form cycles. The compiler must not require developers to split natural abstractions merely to satisfy a single-pass loader.

Allowing cycles does not mean every initialization cycle is valid. The toolchain should separate:

- Declaration and type dependency cycles, which multiple semantic passes can often resolve.
- Compile-time execution cycles.
- Constant initialization cycles.
- Runtime module initialization order.

Invalid cycles should be diagnosed at the semantic layer where they become impossible, with the complete dependency path shown.

## Headers and forward declarations

Rivet does not use C-style header files. Ordinary functions cannot be declared without definitions simply to make later source visible.

The compiler gathers declarations and definitions across module files in multiple passes. Public interfaces may be serialized into compiler-generated module artifacts for dependency compilation, but those artifacts are not handwritten headers.

Foreign functions, framework-generated declarations, compiler intrinsics, and abstract requirements need explicit constructs because they truly have a different source of implementation.

## Packages

A package groups modules, resources, build configuration, dependencies, and potentially target-specific implementations. The manifest format is not selected.

The eventual package system needs to answer:

- Package identity and naming.
- Semantic or alternative version schemes.
- Version constraints and lockfiles.
- Local path, registry, Git, and workspace dependencies.
- Source versus precompiled distribution.
- Feature and capability selection.
- Target-specific sources.
- Reproducibility and integrity verification.
- Multiple versions of one dependency in one process.
- ABI compatibility and compiler-version constraints.

## Compiler discovery

How the compiler locates packages is an open question. The search must eventually be deterministic and visible through diagnostic or inspection commands.

A likely precedence model would include workspace members, explicitly configured paths, a package cache, and registries recorded in project configuration. Environment-variable-only discovery should not silently determine reproducible builds.

## Build graph

Because a module is a dependency unit, the compiler or build coordinator can construct a graph before full code generation. Cycles form strongly connected groups that may require joint semantic analysis. Public API changes should invalidate dependents; private implementation changes should ideally avoid doing so.

This model is a longer-term goal, not a requirement for the single-file bootstrap slice.
