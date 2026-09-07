import os
import subprocess

from SCons.Script import ARGUMENTS, BoolVariable, EnumVariable, Environment, Help, SConscript, Variables

GCC_VERSION="16"
CLANG_VERSION="21"

options = Variables()
options.Add(EnumVariable("mode", "Build Configuration", "debug", allowed_values=("debug", "release")))
options.Add(EnumVariable("compiler", "Host Compiler Toolchain", "clang", allowed_values=("clang", "gcc")))
options.Add(BoolVariable("sanitize", "Enable ASan & UBSan", False))
options.Add(BoolVariable("werror", "Treat compiler warnings as errors", False))
options.Add(BoolVariable("compdb", "Generate compile_commands.json in the active build directory", True))

# Preserve the user's normal executable search path so SCons can find the
# selected host compiler. Other host environment variables are intentionally
# not imported Wholesale.
environment = Environment(variables=options, ENV={"PATH": os.environ.get("PATH", "")})

Help(options.GenerateHelpText(environment))

if environment["compiler"] == "gcc":
    environment.Replace(CC=f"gcc-{GCC_VERSION}", CXX=f"g++-{GCC_VERSION}", LINK=f"g++={GCC_VERSION}")
elif environment["compiler"] == "clang":
    libgcc_path = subprocess.check_output([f"gcc-{GCC_VERSION}", "-print-libgcc-file-name"], text=True).strip()
    gcc_install_dir = os.path.dirname(libgcc_path)
    gcc_install_flag = f"--gcc-install-dir={gcc_install_dir}"
    environment.Replace(CC=f"clang-{CLANG_VERSION}", CXX=f"clang++-{CLANG_VERSION}", LINK=f"clang++-{CLANG_VERSION}")
    environment.AppendUnique(CXXFLAGS=[gcc_install_flag], LINKFLAGS=[gcc_install_flag])

environment.AppendUnique(
    CXXFLAGS=["-std=c++23", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wsign-conversion", "-Wshadow"],
    CPPDEFINES=[("RIVET_HOST_COMPILER_CLANG", 1) if environment["compiler"] == "clang" else ("RIVET_HOST_COMPILER_GCC", 1)]
)

if environment["werror"]:
        environment.AppendUnique(CXXFLAGS=["-Werror"])

if environment["mode"] == "debug":
    environment.AppendUnique(CXXFLAGS=["-O0", "-g3", "-fno-omit-frame-pointer"], CPPDEFINES=[("RIVET_DEBUG", 1)])
else:
    environment.AppendUnique(CXXFLAGS=["-O2"], CPPDEFINES=[("NDEBUG", 1), ("RIVET_RELEASE", 1)])

if environment["sanitize"]:
    sanitizer_flags = ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
    environment.AppendUnique(CXXFLAGS=sanitizer_flags, LINKFLAGS=sanitizer_flags)

project_root = environment.Dir("#").abspath
build_root = environment.Dir(f"#/build/{environment['mode']}").abspath

environment.AppendUnique(CPPPATH=[environment.Dir("#/src")])

Export("environment", "project_root", "build_root")

targets = SConscript("SConscript")
Default(targets["compiler"])