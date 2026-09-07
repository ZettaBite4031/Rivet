# Test Harness Contract

The package intentionally does not prescribe Catch2, GoogleTest, or another
host framework. `rivetc0` may implement a small C++ runner or use CTest to invoke
one. The data format remains reusable by the self-hosted compiler.

The runner reads `../manifest.toml` in declaration order. For each case it:

1. Resolves paths relative to the `tests` directory.
2. Invokes the selected in-process compiler mode or CLI command.
3. Normalizes CRLF to LF in compiler-produced textual output.
4. Compares stdout with `expect` when present.
5. Compares structured compact diagnostics with `diagnostics` when present.
6. Verifies the expected status category.
7. Prints a useful unified diff on mismatch.

For `run`, the `.run` file is declarative rather than literal process stdout.
The `exit` field records Rivet's signed interpreter result; an in-process runner
must not truncate it to the host process's eight-bit status representation:

```text
exit = 42
stdout = ""
stderr = ""
```

The harness should prefer calling compiler APIs in-process for speed, while a
small CLI smoke-test group verifies command dispatch and actual process status.
