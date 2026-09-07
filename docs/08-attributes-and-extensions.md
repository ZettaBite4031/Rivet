# Attributes, decorators, and semantic extensions

## General attribute syntax

Rivet is intended to support Python-like decorators or attributes, provisionally written with `@`:

```rivet
@serializable
class UserSettings {
    string theme;
    int font_size;
}
```

Attributes attach structured metadata or compile-time behavior to declarations. They should not merely be untyped strings that every tool interprets differently.

Likely targets include modules, imports, types, fields, variables, functions, parameters, statements, and possibly expressions. Each attribute declares where it is valid.

## Built-in and user-defined attributes

Some attributes may be compiler-defined because they affect ABI, layout, optimization, diagnostics, or core semantics. Others should be definable by libraries and frameworks through compile-time extension interfaces.

Examples of likely domains:

- Serialization.
- Foreign calling conventions and symbol names.
- Layout and alignment.
- Deprecation and availability.
- Tests and benchmarks.
- Generated bindings.
- Lattice resources, events, and observation.
- Static-analysis contracts.

The compiler needs to distinguish an unknown attribute from a known but unavailable provider and offer a useful diagnostic in either case.

## Attribute arguments

Attribute arguments should be compile-time values rather than arbitrary token strings:

```rivet
@serializable(format: SerializationFormat.binary)
@deprecated(message: "Use open_v2")
```

Named arguments, positional arguments, repetition, ordering, and attribute composition remain provisional.

## Transformation boundaries

Attributes may have different levels of authority:

1. **Metadata-only:** recorded for tools or runtime reflection.
2. **Validation:** checks the annotated declaration and emits diagnostics.
3. **Generation:** creates related declarations or data.
4. **Semantic modifier:** changes compiler behavior through a deliberately supported extension contract.

Arbitrary plugins should not receive unrestricted ability to mutate the core AST. Structured APIs and generated declarations keep compiler invariants visible.

## `observe`

Observation is more involved than a simple function decorator. It affects storage, mutation, generated notification behavior, binding, Lattice UI updates, and possibly thread or lifetime rules.

**Accepted direction:** `observe` should act like a declaration or variable specifier implemented through Rivet's general semantic extension system, rather than a permanent Lattice-owned keyword embedded into the language grammar.

Illustrative syntax:

```rivet
observe mut string title = "Untitled";
```

The exact spelling is not frozen. A decorator form may participate in the implementation even if the surface reads as a specifier.

Outside a Lattice-capable environment:

- `observe` remains usable as an ordinary identifier where grammar permits.
- Using it in specifier position should produce a precise error that the observation provider or capability is unavailable.
- Raw Rivet's parser and AST remain valid without loading Lattice.

## Possible observation lowering

A Lattice provider could interpret an observed property as a combination of:

- Backing storage.
- Generated get/set behavior.
- Change comparison policy.
- A notification channel or version counter.
- Metadata identifying the property to LIR and UI bindings.
- Optional validation, coercion, or thread-affinity rules.

The generated behavior should be inspectable. Tools should be able to show the expanded declarations or LIR metadata rather than hiding framework magic.

## Serialization example

`@serializable` demonstrates a cleaner attribute case. A compile-time provider could reflect fields, validate supported types, generate encode/decode functions, and optionally emit runtime schema metadata.

This example is conceptual. Rivet has not yet selected serialization formats, field naming rules, compatibility behavior, or whether generated functions are member or module declarations.

## Extension discovery and trust

Semantic extensions must come from explicit package or project configuration. Merely importing a runtime library should not silently grant arbitrary compiler execution.

Because extensions may execute at build time, the package system eventually needs trust, capability, caching, and reproducibility rules consistent with `#run`.
