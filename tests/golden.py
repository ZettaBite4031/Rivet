"""Run the repository's bootstrap source-to-output fixtures.

Usage: python3 tests/golden.py path/to/rivetc0
"""

from __future__ import annotations

import difflib
import json
import pathlib
import subprocess
import sys


def load_test_manifest(path: pathlib.Path) -> dict:
    """Read the small TOML subset used by tests/manifest.toml.

    Supported values are quoted strings and decimal integers. Keeping this
    intentionally narrow avoids a third-party dependency for the test runner.
    """
    manifest = {"case": []}
    current = manifest
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        entry = line.strip()
        if not entry or entry.startswith("#"):
            continue
        if entry == "[exit_status]":
            current = manifest["exit_status"] = {}
            continue
        if entry == "[[case]]":
            current = {}
            manifest["case"].append(current)
            continue
        if "=" not in entry:
            raise ValueError(f"{path}:{line_number}: unsupported manifest entry: {entry}")
        key, value = (part.strip() for part in entry.split("=", 1))
        try:
            current[key] = json.loads(value) if value.startswith('"') else int(value, 10)
        except (ValueError, json.JSONDecodeError) as error:
            raise ValueError(f"{path}:{line_number}: unsupported manifest value: {value}") from error
    return manifest


def normalize(text: str) -> str:
    return text.replace("\r\n", "\n")


def compare(label: str, expected: str, actual: str) -> bool:
    expected = normalize(expected)
    actual = normalize(actual)
    if actual == expected:
        return True
    print(f"  {label} differs:")
    print("".join(difflib.unified_diff(expected.splitlines(keepends=True),
        actual.splitlines(keepends=True), fromfile="expected", tofile="actual")), end="")
    return False


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: golden.py <rivetc0>", file=sys.stderr)
        return 2

    root = pathlib.Path(__file__).resolve().parent
    compiler = pathlib.Path(sys.argv[1]).resolve()
    manifest = load_test_manifest(root / "manifest.toml")
    exits = manifest["exit_status"]
    failures = 0
    for case in manifest["case"]:
        completed = subprocess.run([str(compiler), case["mode"], str(root / case["input"])],
            capture_output=True, text=True, check=False)
        wanted = exits[case.get("exit", "success")]
        passed = completed.returncode == wanted
        if not passed:
            print(f"  status: expected {wanted}, got {completed.returncode}")
        if "expect" in case:
            passed = compare("stdout", (root / case["expect"]).read_text(encoding="utf-8"),
                completed.stdout) and passed
        else:
            passed = compare("stdout", "", completed.stdout) and passed
        if "diagnostics" in case:
            passed = compare("diagnostics", (root / case["diagnostics"]).read_text(encoding="utf-8"),
                completed.stderr) and passed
        else:
            passed = compare("diagnostics", "", completed.stderr) and passed
        print(f"[{'PASS' if passed else 'FAIL'}] {case['name']}")
        failures += not passed

    print(f"{len(manifest['case'])} golden cases, {failures} failures")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
