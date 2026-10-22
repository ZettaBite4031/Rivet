from pathlib import Path
import subprocess

from SCons.Script import Action, Alias, AlwaysBuild, Import, Return

Import("environment", "project_root", "build_root")

env = environment.Clone()

if env["compdb"]:
    # Load the tool before object builders are invoked so it can observe every
    # compile action added below.
    env.Tool("compilation_db")

project_path = Path(project_root)
source_root = project_path / "src"
test_root = project_path / "tests" / "cpp"
build_path = Path(build_root)
object_root = build_path / "obj"
binary_root = build_path / "bin"

def discover_cpp(directory):
    """Returns a stable list of every C++ translation unit below directory."""
    if not directory.is_dir():
        return []
    return sorted(path for path in directory.rglob("*.cpp") if path.is_file())

def object_target(source, category=None):
    """Map a source file to a mirrored object target below build/<mode>/obj"""
    if category is None:
        relative = source.relative_to(project_path)
    else:
        relative = Path(category) / source.relative_to(test_root)

    # SCOns adds the platform object suffix. Omitting it here keeps the mapping
    # portable between .o and .obj hosts
    return object_root / relative.with_suffix("")

def compile_objects(build_environment, sources, category=None):
    objects = []
    for source in sources:
        target = object_target(source, category)
        objects.extend(build_environment.Object(target=str(target), source=str(source)))
    return objects

sources = discover_cpp(source_root)
entry_source = source_root / "main.cpp"

if not entry_source.is_file():
    raise RuntimeError("rivetc0 requires its executable entry point at src/main.cpp")

core_sources = [source for source in sources if source != entry_source]
core_objects = compile_objects(env, core_sources)
entry_objects = compile_objects(env, [entry_source])

compiler = env.Program(target=str(binary_root / "rivetc0"), source=core_objects + entry_objects)
Alias("compiler", compiler)


# All tests below tests/cpp are linked into one bootstrap test executable.
# Keeping src/main.cpp separate prevents a duplicate entry point, while every
# other compiler object is reused by the test binary.
test_sources = discover_cpp(test_root)
test_binary = None

if test_sources:
    test_objects = compile_objects(env, test_sources, category="tests")
    test_binary = env.Program(target=str(binary_root / "rivetc0-tests"), source=core_objects + test_objects)
    Alias("tests", test_binary)
    def run_golden(target, source, env):
        return subprocess.call([
            "python3", str(project_path / "tests" / "golden.py"),
            str(compiler[0]),
        ], cwd=str(project_path))

    test_alias = Alias("test", list(test_binary) + list(compiler), [
        Action('"$SOURCE"', cmdstr="Running rivetc0 tests"),
        Action(run_golden, cmdstr="Running rivetc0 golden fixtures"),
    ])
    AlwaysBuild(test_alias)
else:
    # The alias remains valid before the first C++ test exists.
    Alias("tests", [], Action(lambda target, source, env: print("No C++ tests found under tests/cpp") or 0, cmdstr="Checking rivetc0 tests"))
    Alias("test", "tests")

compilation_database = None
if env["compdb"]:
    compilation_database = env.CompilationDatabase(str(build_path / "compile_commands.json"))
    Alias("compdb", compilation_database)

targets = {
    "compiler": compiler, 
    "tests": test_binary,
    "compdb": compilation_database
}

Return("targets")
