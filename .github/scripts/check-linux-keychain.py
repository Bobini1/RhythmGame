#!/usr/bin/env python3
"""Check the extracted Linux package's QtKeychain/GLib runtime dependencies."""

import os
from pathlib import Path
import re
import subprocess
import sys


def check_package(package_root):
    lib_dir = package_root.resolve() / "lib"
    glib_names = {
        "libglib-2.0.so.0",
        "libgobject-2.0.so.0",
        "libgio-2.0.so.0",
        "libgmodule-2.0.so.0",
    }
    libraries = sorted(glib_names | {"libqt6keychain.so.1", "libsecret-1.so.0"})
    for name in libraries:
        library = lib_dir / name
        if not library.is_file() or not library.resolve().is_relative_to(lib_dir):
            raise RuntimeError(f"Missing bundled library or invalid symlink: {library}")

    glib_functions = {
        "g_error_free",
        "g_hash_table_contains",
        "g_source_attach",
        "g_main_context_default",
    }
    for name in ("libqt6keychain.so.1", "libsecret-1.so.0"):
        symbols = subprocess.check_output(
            ["nm", "-D", "--defined-only", str(lib_dir / name)], text=True
        )
        for line in symbols.splitlines():
            if not line.strip():
                continue
            symbol = line.split()[-1].split("@")[0]
            if symbol in glib_functions:
                raise RuntimeError(f"{name} embeds a GLib symbol: {symbol}")

    env = dict(os.environ, LD_LIBRARY_PATH=str(lib_dir))
    env.pop("LD_PRELOAD", None)
    required_dependencies = {
        "libqt6keychain.so.1": {"libglib-2.0.so.0"},
        "libsecret-1.so.0": {
            "libglib-2.0.so.0", "libgobject-2.0.so.0", "libgio-2.0.so.0"
        },
    }
    for name in libraries:
        dependencies = subprocess.check_output(
            ["ldd", str(lib_dir / name)], env=env, text=True, stderr=subprocess.STDOUT
        )
        if "not found" in dependencies:
            raise RuntimeError(f"Unresolved dependencies for {name}:\n{dependencies}")
        resolved_glib = set()
        for line in dependencies.splitlines():
            match = re.match(r"\s*(\S+) => (.+?) \(0x[0-9a-f]+\)", line)
            if match and match[1] in glib_names:
                resolved_glib.add(match[1])
                if not Path(match[2]).resolve().is_relative_to(lib_dir):
                    raise RuntimeError(f"{name} loads system GLib: {line.strip()}")
        missing = required_dependencies.get(name, set()) - resolved_glib
        if missing:
            raise RuntimeError(f"{name} is missing shared dependencies: {sorted(missing)}")

    # Reproduce the QtKeychain -> dlopen(libsecret) -> GIO task creation path.
    # No credential is read or written, and no GUI or keyring daemon is needed.
    subprocess.run(
        [sys.executable, "-c", """
import ctypes

keychain = ctypes.CDLL("libqt6keychain.so.1", mode=ctypes.RTLD_GLOBAL)
secret = ctypes.CDLL("libsecret-1.so.0", mode=ctypes.RTLD_GLOBAL)
new_task = secret.g_task_new
new_task.argtypes = [ctypes.c_void_p] * 4
new_task.restype = ctypes.c_void_p
unref = secret.g_object_unref
unref.argtypes = [ctypes.c_void_p]
unref.restype = None
task = new_task(None, None, None, None)
if not task:
    raise RuntimeError("GIO task creation returned NULL")
unref(task)
"""],
        env=env,
        check=True,
        timeout=30,
    )
    print("Linux keychain dependencies and GIO task creation passed.")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(f"Usage: {sys.argv[0]} EXTRACTED_PACKAGE_DIRECTORY")
    check_package(Path(sys.argv[1]))
