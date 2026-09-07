# Memory, references, and pointers

## Goals

Rivet must remain useful for operating systems, platform runtimes, engines, drivers, embedded targets, and native libraries. Direct memory access is therefore a normal capability, not a grudging compatibility feature.

At the same time, the language should offer pointer forms that carry useful invariants and are pleasant enough that programmers do not reach for raw pointers by default.

## Three concepts, not one

The current direction distinguishes:

1. **References**, expressed with `ref`-based syntax, for language-level aliasing or parameter passing.
2. **Checked pointers**, provisionally spelled `T&`, for pointer-like values whose invariants are enforced by the compiler and/or runtime.
3. **Raw pointers**, spelled `T*`, for direct machine addresses and foreign or systems work.

The exact relationship between references and checked pointers remains provisional. The design must prevent them from becoming two names for the same thing.

## Checked pointers: `T&`

**Provisional direction:** `T&` is a compiler-intrinsic pointer category rather than a library template such as `Pointer<T>`.

```rivet
Widget& widget;
```

Treating the concept as type syntax has several advantages:

- Diagnostics can explain pointer-specific failures directly.
- The compiler may attach nullability, provenance, bounds, lifetime, or capability information.
- The ABI representation can remain compact where the target permits it.
- The type feels fundamental rather than like a container object.

However, “checked” needs a precise contract. Candidate guarantees include non-nullness, valid lifetime, alignment, object identity, bounds for array access, and restricted arithmetic. Rivet should not promise all of them through a single character until their implementation costs are understood.

Likely baseline behavior:

- Construction from a raw pointer requires a checked operation or proof.
- Dereference verifies whatever invariant the type promises.
- Unrestricted integer arithmetic is not available directly.
- Loss of guarantees when converting to `T*` is explicit.
- Freestanding targets may select compile-time-only checks, runtime checks, or a documented reduced profile.

## Raw pointers: `T*`

Raw pointers expose addresses with minimal assumptions:

```rivet
byte* framebuffer;
```

They are required for memory-mapped hardware, allocators, kernels, foreign interfaces, custom runtimes, and some performance-sensitive structures. Raw pointer operations should have precise language semantics even when the compiler cannot prove them safe.

The language still needs to define:

- Null representation and null literal syntax.
- Pointer arithmetic and comparison.
- Alignment requirements.
- Conversion between object pointers, function pointers, integers, and byte pointers.
- Aliasing and provenance rules.
- Volatile or device-memory access.
- Behavior of invalid dereference.

Avoiding mandatory `unsafe` syntax does not excuse ambiguity in these rules.

## `unsafe` and policy

**Accepted direction:** Rivet will not require every raw-pointer operation to live inside a Rust-style `unsafe` block.

Some operations may remain intrinsically dangerous enough to warrant a marker, but enforcement should largely be selectable through compiler or project policy. Potential modes include:

- **Permissive:** raw operations compile normally, with ordinary warnings.
- **Audited:** operations that cross selected safety boundaries require annotations or emit strong diagnostics.
- **Strict:** designated raw operations require explicit acknowledgement and may be prohibited in selected modules.

The flag names and exact boundaries are open. A policy mode must not silently change executable semantics; it changes acceptance and diagnostics.

## Ownership and lifetimes

No complete ownership model has been selected. Rivet should not adopt Rust's borrow checker by default merely because checked pointers exist. Possible mechanisms include ownership-aware library types, move semantics, region or lifetime metadata, static analysis, and opt-in strict modes.

The design must eventually answer:

- Which values own storage?
- When are destructors run?
- How are moves and copies selected or generated?
- Can `T&` outlive the referenced object?
- How does aliasing interact with `mut`?
- Can checked pointers cross foreign boundaries?
- Which guarantees survive conversion, serialization, or RIR lowering?

These questions should be evaluated using real allocator, container, kernel, and Lattice examples rather than solved entirely in the abstract.

## Allocation is not a language assumption

Core Rivet syntax should not assume that a heap exists. Allocation belongs to libraries, runtimes, or target capability providers. Types and features that require allocation must declare that requirement so freestanding builds can reject or replace them clearly.

The language may still understand construction and destruction independently of heap allocation. Stack storage, static storage, arenas, custom allocators, placement construction, and framework-managed objects all need room in the model.

## Interaction with RIR

RIR must preserve distinctions necessary for correctness and optimization:

- Checked versus raw address values.
- Address spaces when targets require them.
- Alignment and access width.
- Volatile or device accesses.
- Bounds or provenance metadata where available.
- Explicit operations that discard guarantees.

Lowering everything immediately to an unannotated integer address would erase the very information that makes checked pointers useful.
