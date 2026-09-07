# Targets, runtime, and standard library

## Portability objective

Rivet should make as many language features as practical available across every target. Features should disappear only when their semantics genuinely require an unavailable capability, not because the compiler assumes all non-desktop targets are primitive.

This goal intentionally makes compiler and library engineering harder. The payoff is one language model spanning zOS kernel work, native applications, embedded systems, tools, and Lattice software.

## Target description

A target is more than a CPU triple. It may describe:

- Architecture and instruction set.
- Operating system or absence of one.
- ABI and calling conventions.
- Object format and linker.
- Pointer width, alignment, and endianness.
- Available runtime and standard-library capabilities.
- Exception and unwinding support.
- Threading and atomics.
- Memory model or address spaces.
- Application framework profile.

The exact configuration-file format is open.

## Hosted and freestanding profiles

**Accepted direction:** projects may mark compilation as freestanding. A freestanding build disables assumptions that require an operating-system environment.

```text
profile = freestanding
```

Conceptually, profiles include:

- **Hosted native:** an operating system, process model, conventional allocation, files, and other selected services.
- **Freestanding:** no assumed OS, process, filesystem, standard allocator, or console.
- **Kernel:** freestanding plus platform-specific privileges and restrictions suitable for zOS or another kernel.
- **Lattice application:** hosted capabilities plus Lattice providers and generated startup.

Profiles are collections of capabilities and defaults, not entirely separate languages.

## Capabilities

The compiler and package configuration should express which services are available. Candidate capability groups include:

- Raw memory operations.
- Dynamic allocation.
- File and stream I/O.
- Console or diagnostic output.
- Threads and synchronization.
- Time and clocks.
- Process and environment access.
- Networking.
- Exceptions and unwinding.
- Runtime type metadata.
- Lattice UI services.

A standard-library API that requires a capability declares it. The compiler can reject an unavailable call during compilation when it is statically reachable or require the program to supply an implementation.

## Standard-library selection

Project configuration should be able to state what is permitted from the standard library. This serves several purposes:

- Prevent accidental hosted dependencies in a kernel.
- Reduce runtime and binary footprint.
- Allow platform-specific or custom implementations.
- Make unavailable functionality fail early.
- Support gradual bring-up of a new target.

The system should operate above crude “stdlib on/off” selection. A kernel might have memory operations, collections backed by a custom allocator, and formatting into buffers while lacking files and processes.

## Host stubs and providers

For native or custom targets, Rivet can expose defined interfaces for essential services such as allocation, file I/O, fatal handling, clocks, and output. A target runtime or application supplies marked implementations.

The toolchain may verify providers at several points:

- **Compile time:** required declarations and signatures exist.
- **Link time:** required symbols have implementations.
- **Load or startup time:** dynamically supplied capabilities are registered.
- **Runtime:** an optional capability is queried before use.

Compile-time checking is preferred for mandatory capabilities. Runtime checks are appropriate only where availability is genuinely dynamic.

## Reachability and unavailable code

A library package may contain implementations for capabilities unavailable on the current target. Merely parsing the package should not always fail. The compiler needs rules for conditional declarations and unreachable code.

However, hiding missing dependencies until a rare runtime branch executes would be unpleasant. The intended balance is:

- Compile-time selection removes target-inapplicable declarations.
- Binding rejects references to unavailable APIs.
- Final reachability verifies required providers for emitted code.
- Dynamic capability APIs make optional runtime availability explicit.

## Runtime components

Rivet should not assume one monolithic runtime. Potential components include:

- Startup and termination.
- Allocation hooks.
- Exception unwinding.
- Thread-local storage.
- Reflection metadata.
- Dynamic casts or interface lookup.
- Sanitizer/check handlers.
- Lattice application services.

A freestanding program can omit unused components or supply its own. Language features must document which component, if any, they require.

## Cross-compilation

Compile-time code runs on the build host while runtime code targets another environment. Reflection about size, layout, calling convention, and availability must use the target model, not the host's accidental properties.

Host-dependent `#run` actions must not confuse host files or native handles with target values. RIR and the compiler's compile-time API should keep that distinction explicit.

## Policy versus semantics

Compiler flags may strengthen diagnostics for raw pointers, ignored errors, unavailable capabilities, allocation, or framework-specific constructs. Such flags should not unpredictably change core expression meaning.

Target configuration describes reality. Policy configuration describes what the project permits. Keeping them separate allows the same hardware target to support both permissive experiments and audited production builds.
