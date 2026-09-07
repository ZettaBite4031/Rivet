# Compiler architecture and bootstrap roadmap

## Bootstrap strategy

The first compiler, `rivetc0`, is written in C++23. Its job is not to implement the finished language. Its job is to establish a small, sound architecture and grow until it can compile a useful self-hosting subset of Rivet.

After self-hosting, compiler development can move progressively into Rivet. `rivetc0` should remain buildable long enough to recover or reproduce the first self-hosted compiler.

## Development approach: blitz vertically

The project deliberately prioritizes a complete tiny pipeline over perfect individual subsystems. The first program is:

```rivet
module hello;

int main() {
    return 42;
}
```

Its complete path is:

```text
SourceFile → Lexer → Parser → syntax AST → Binder → bound tree → RIR → interpreter
```

Once that works, the compiler expands horizontally with arithmetic, variables, calls, control flow, types, and additional targets.

The approach still requires clean boundaries and tests. “Blitz” means deferring breadth and polish, not fusing the entire compiler into an irreparable blob.

## Driver and compilation session

The command-line driver reads options, creates a compilation session, loads source files, invokes phases, prints diagnostics, and selects an output action.

The compilation session should own shared-lifetime state:

- Source files.
- Interned names when introduced.
- Diagnostics or phase-specific diagnostic collections.
- Target and project configuration.
- Module and symbol tables.
- Syntax and bound arenas if used.

Compiler phases should not depend directly on command-line printing or process exit.

## Source management

The current bootstrap can load one small file through `SourceFile`. This is enough to start.

The ownership invariant should be:

> `SourceFile` owns immutable source text. Tokens and syntax nodes refer to it through source spans and do not outlive the compilation session.

A source span initially needs a byte offset and length. Human line and column positions can be computed through the source file, with a line-start index added when repeated diagnostics justify it.

The full source manager may later handle stable file IDs, generated virtual files, encodings, package-relative paths, and source maps. None is required before the lexer.

## Diagnostics

Diagnostics are structured data, not strings printed from arbitrary compiler phases. A minimal diagnostic contains severity, source span, message, and eventually a stable diagnostic code.

The driver or a formatter produces terminal output with file, line, column, source excerpt, and caret. Tests can inspect diagnostics without capturing `stderr`.

Phases should recover where practical so one malformed source file can report several meaningful errors. Recovery must always make progress.

## Lexer

The first token set needs:

```text
Invalid, EndOfFile
Identifier, IntegerLiteral
module, int, return
(, ), {, }, ;
```

The lexer skips whitespace and initially supports line comments. It recognizes ASCII identifiers and decimal integers. Unicode identifiers, elaborate literals, nested comments, and preprocessing wait until required.

Every call to `NextToken` must either advance the byte cursor or return EOF. Tokens store source spans instead of copied spellings.

An early `--dump-tokens` mode provides direct observability and snapshot-test input.

## Parser and initial grammar

The first recursive-descent grammar is:

```text
translation-unit:
    module-declaration function-declaration EOF

module-declaration:
    "module" identifier ";"

function-declaration:
    type identifier "(" ")" block

type:
    "int"

block:
    "{" statement* "}"

statement:
    return-statement

return-statement:
    "return" expression ";"

expression:
    integer-literal
```

The initial syntax AST contains translation unit, module declaration, function declaration, block statement, return statement, and integer literal expression nodes.

The parser operates only on syntax. It does not decide whether an identifier names a type or whether `main` has the correct signature beyond what the minimal grammar requires.

## Binder

Binding resolves names and attaches semantic types. Even in the first slice, a distinct binder establishes an important boundary.

Initial checks include:

- The source has one module declaration.
- `int` resolves as a built-in type.
- The module has one valid entry point.
- The `main` function returns `int`.
- Its return expression has type `int`.
- Duplicate declarations produce diagnostics.

As the language grows, the binder handles multiple files, imports, scopes, overloads, implicit members, conversions, generic instantiation, compile-time generated declarations, and framework-provided semantics.

## Bound representation

The first bound tree may resemble the syntax tree. It is still valuable because it contains resolved symbols and types rather than source spellings.

Syntax should remain useful to diagnostics and tooling. Bound nodes should remain useful to semantic analysis and lowering. Avoid storing backend-specific machine choices in either.

## Lowering and RIR

The initial program lowers to:

```text
func @main() -> i32 {
entry:
    %0 = const.i32 42
    ret %0
}
```

An RIR verifier checks the representation before execution. The interpreter executes virtual-register operations and returns the entry result to the driver.

Useful inspection modes are:

```text
--dump-tokens
--dump-ast
--dump-bound
--dump-rir
--run
```

Names may change, but equivalent observability should remain.

## Horizontal growth order

After the first complete slice, a sensible sequence is:

1. Arithmetic and operator precedence.
2. Immutable and mutable local variables.
3. Function calls and multiple functions.
4. Comparison and structured control flow.
5. Additional primitive types and conversions.
6. Multiple files in one module.
7. Imports and multiple modules.
8. Aggregates and user-defined types.
9. Checked and raw pointers.
10. Compile-time constants and basic reflection.
11. LLVM object emission.
12. Enough standard library and language surface to begin self-hosting.

This sequence can change when a motivating program exposes a better dependency order.

## Multiple passes and no headers

The compiler must be designed for source-order independence. It first establishes module contents and declaration shapes, then binds bodies and resolves dependencies. Functions need definitions, but their bodies need not appear before their call sites.

This architecture is required before multiple files and circular module dependencies become practical.

## Self-hosting threshold

Self-hosting requires more than parsing `main`. At minimum, Rivet needs enough support for:

- Strings or byte buffers and source input.
- Collections and allocation.
- Tagged results or another workable error convention.
- Classes or aggregates.
- Enums and variants.
- Loops, branches, functions, and recursion.
- Modules and imports.
- File and command-line I/O on the host.
- RIR construction and interpretation or native emission.
- A standard library subset usable by the compiler.

The first self-hosted compiler does not need every future language feature. Features not used by the compiler can remain implemented later.

## Debt policy

Bootstrap shortcuts are acceptable when they are visible and contained behind the correct interface. Examples include ASCII-only identifiers, linear symbol lookup, one loaded file, and a simple interpreter.

Shortcuts that erase architectural boundaries—printing errors in the lexer, making the parser perform name lookup, coupling Lattice into syntax nodes, or encoding LLVM assumptions throughout binding—are much more expensive and should be avoided even during the blitz.
