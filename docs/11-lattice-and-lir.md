# Lattice, LIR, and UI markup

## Boundary

Lattice is Rivet's flagship application framework and UI ecosystem. It is not the language runtime, the parser, or the owner of Rivet's AST.

Rivet should have the same kind of independence that C# has from any one application framework: strong integration is welcome, dependency is not.

## Intended Lattice responsibilities

Lattice may eventually provide:

- Desktop and graphical application startup.
- Windows, views, controls, layout, styling, and resources.
- A declarative UI markup language inspired by XAML.
- Data binding and observation.
- Events, commands, navigation, and application lifetime.
- Rendering and platform integration.
- Designer, preview, hot-reload, and debugging tools.
- Serialization or resource-generation conventions.

The exact platform scope and rendering architecture remain open.

## Application startup

A raw Rivet program provides a recognized `main` entry. A Lattice application instead declares application and window structure through its project, code, and markup.

Lattice tooling supplies or generates the actual Rivet entry point that:

1. Initializes required target/runtime capabilities.
2. Creates the application object or equivalent state.
3. Loads compiled resources and UI descriptions.
4. Creates the initial window or view.
5. Enters the platform event loop.
6. Performs orderly shutdown.

This setup is inspectable generated behavior, not a secret alternative executable format.

## UI markup

Lattice should support a declarative markup format in the spirit of XAML without copying it mechanically. The name and syntax are not selected.

The markup needs to represent:

- Object or control construction.
- Property assignment.
- Nested visual and logical structure.
- Resources and reusable styles.
- Events and commands.
- Data-binding expressions.
- Templates and composition.
- Names or references used by Rivet code.
- Design-time-only metadata.

The format should compile into typed LIR rather than remain a collection of strings interpreted blindly at runtime.

## Typed integration

Markup names, types, properties, events, and bindings should be checked against Rivet semantic information during the build. Generated code should retain source mapping back to markup so diagnostics point to the correct file and location.

A property typo should be caught by the compiler or Lattice compiler. It should not survive until the user clicks a window three weeks later.

## Observation

Lattice needs an observation mechanism for UI-facing state. Observation affects both ordinary Rivet declarations and Lattice binding metadata, which is why it cannot be treated as a superficial decorator.

The working model uses a general Rivet declaration specifier or semantic attribute:

```rivet
observe mut string title = "Rivet";
```

Lattice's extension provider may generate storage access, change notification, validation, and LIR property metadata. Rivet itself supplies the extension interface, compile-time execution, type information, and generated-declaration support.

Outside Lattice, no observation provider is assumed. `observe` does not become a globally meaningful native-code qualifier.

## LIR

LIR—Lattice Intermediate Representation—is the typed representation of framework-specific application information. It exists so Lattice concepts survive long enough for validation, tooling, generation, and optimization without polluting general RIR.

Potential LIR entities include:

- Applications, windows, views, and component instances.
- Typed properties and assigned values.
- Resource dictionaries and lookup scopes.
- Bindings and dependency paths.
- Observed state and notification edges.
- Events and command targets.
- Templates, styles, and visual trees.
- Generated-code references.
- Platform and design-time metadata.

The name LIR does not require it to resemble low-level machine IR. It may be a graph or typed object representation suited to UI composition.

## Lowering

A likely pipeline is:

```text
Rivet source ──► Rivet syntax/bound declarations ──► RIR
      ▲                    ▲
      │                    │
UI markup ──► typed LIR ───┘
```

LIR validation can use Rivet type information. It then generates ordinary Rivet semantic entities, initialization data, resources, or RIR calls into the Lattice runtime.

The exact ordering must support bindings to declarations that may themselves be generated at compile time.

## Runtime versus compile-time work

Lattice should compile as much structural information as practical:

- Resolve known types and properties at build time.
- Precompute static resources where useful.
- Generate direct accessors rather than string lookup when possible.
- Preserve dynamic binding only where the application requests it.
- Avoid requiring reflection metadata for code paths that can be statically linked.

This serves native performance and makes constrained targets more plausible.

## Platform abstraction

Lattice may expose a coherent application model across targets while allowing platform-specific APIs. It should not claim a feature is universal when the target lacks the needed windowing, rendering, input, or accessibility capability.

Target providers can implement Lattice capabilities and report unsupported features through the same capability model used by Rivet's runtime and standard library.

## Tooling goals

Long-term Lattice tooling may include:

- A visual designer or structural inspector.
- Live preview and hot reload.
- Binding diagnostics.
- Resource and style tracing.
- Generated-code and LIR inspection.
- Accessibility inspection.
- Project templates similar in convenience to polished .NET IDE workflows.

These tools should consume stable compiler and LIR APIs rather than scrape implementation details from the parser.
