# Type system and values

## General direction

Rivet's type system begins from a C++-like native model: statically known value types, explicit user-defined types, deterministic layout where required, overloadable behavior, and direct access to platform representations.

It should improve consistency and introspection rather than reproduce every C++ rule. In particular, compile-time reflection and compiler-provided type queries should replace large amounts of library-emulated metaprogramming.

## Primitive and complex type spelling

**Accepted direction:** primitive types are lowercase. Complex types—classes, records, enums, framework types, and structured library types—are conventionally capitalized.

```rivet
int count;
float ratio;
bool ready;
byte flags;
char letter;
void reset();

Result operation_result;
Window main_window;
```

This is intended to give readers an immediate visual distinction between language primitives and richer types.

## Variables and declaration order

**Accepted direction:** Rivet uses the type-first declaration family:

```text
qualifiers type identifier = expression;
```

Examples:

```rivet
int answer = 42;
mut float progress = 0.0;
public static Widget factory = create_widget();
```

Rivet does not simultaneously maintain a general Rust-style `let name: Type` declaration system. Two fully capable declaration grammars would complicate parsing, style, diagnostics, and teaching without a strong semantic benefit.

## Immutability and mutability

**Accepted direction:** variables are immutable by default. Mutability must be requested explicitly.

The working spelling is `mut`:

```rivet
int stable = 12;
mut int changing = 12;

changing += 1;
```

`mut` is provisional until the first grammar is frozen, but default immutability is not. The type system must eventually distinguish:

- Rebinding a variable.
- Mutating the object referred to by a handle or pointer.
- Mutating through aliases.
- Interior mutation implemented by a type.
- Compile-time versus runtime mutation.

Those rules must be expressed coherently rather than inferred from spelling alone.

## Integer types

Rivet should provide convenient native integer types such as `int` while also offering fixed-width types or aliases comparable to C++'s `int8_t`, `uint8_t`, `int16_t`, and related names.

Still open:

- Exact widths of `short`, `int`, and `long`, if all exist.
- Whether fixed-width names are primitives, aliases supplied by the standard library, or compiler-known aliases.
- Whether native-width integers use names such as `isize` and `usize`.
- Overflow behavior in debug, release, checked, and compile-time evaluation.

Whatever names are selected, ABI-dependent integers and guaranteed-width integers must be clearly distinguishable.

## `byte` is not `char`

**Accepted direction:** `byte` and `char` represent different concepts even if both occupy eight bits on a common target.

- `byte` is an eight-bit unit of raw numeric or memory data.
- `char` represents a character code unit and participates in text operations.

The type checker should not silently treat arbitrary bytes as text merely because their storage widths match. Explicit conversion may be permitted when the programmer is intentionally interpreting an encoding.

This distinction improves overload selection, diagnostics, serialization, and memory APIs. A buffer of `byte` says “uninterpreted data”; a buffer of `char` says “textual code units.”

## Character and string direction

A wider character type such as `wchar` or an explicitly sized character type is desired, particularly for sixteen-bit code units. The final design must avoid implying that a fixed-width code unit always represents one complete Unicode character.

Open questions include:

- Whether `char` is UTF-8, a raw narrow code unit, or target-dependent.
- Whether `wchar` is guaranteed to be sixteen bits.
- Names for UTF-8, UTF-16, UTF-32 code units and Unicode scalar values.
- String ownership, slicing, null termination, and encoding guarantees.

These decisions must be made before string literals and foreign interfaces are stabilized.

## `void`

`void` is a lowercase primitive used where a function returns no value or a low-level operation refers to untyped storage. The language should avoid expanding `void` into a universal escape hatch when a more precise byte-oriented type is available.

## Inference

Type inference is compatible with type-first declarations but its syntax is not selected. Rivet may eventually support a placeholder type or a constrained inference form. It should not undermine the visual distinction between immutable and mutable declarations or make public APIs difficult to inspect.

## User-defined types

The complete set is open, but the language is expected to require several categories:

- Value aggregates with predictable layout.
- Classes or reference-oriented objects.
- Enumerations.
- Tagged variants or sum types.
- Generic types and functions.
- Interfaces, traits, concepts, or some combination for constraints and shared behavior.

The standard library's `Result`-like facilities should be expressible with ordinary language capabilities plus compiler-supported ergonomics, rather than existing as an unimplementable magical exception.

## Conversions and casts

Rivet should distinguish safe implicit conversions, checked explicit conversions, bit reinterpretation, and raw address conversion. The exact syntax is open.

The guiding rule is that a conversion which may truncate, reinterpret representation, change ownership meaning, or discard checked guarantees should be visible and diagnosable. Low-level conversion remains possible, especially in freestanding and systems profiles.

## Type metadata and reflection

Elementary type questions—identity, size, alignment, members, qualifiers, generic arguments, and supported operations—should be available through language-level compile-time facilities. A standard library may make those queries prettier, but it should not need to derive them with thousands of specializations.
