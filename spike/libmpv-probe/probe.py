#!/usr/bin/env python3
"""P0 S1b probe: confirm a libmpv DLL loads and its core C API is callable.

Used by the GitHub Actions Windows runner, which downloads the mpv dev package
from GitHub releases (fast CI network + MSVC runtime). This does NOT verify
native surface rendering into a transparent window -- that still needs a GUI
run.

    python probe.py --dll path/to/libmpv-2.dll
"""

import argparse
import ctypes
import sys


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dll", required=True)
    args = ap.parse_args()

    print(f"loading: {args.dll}")
    try:
        lib = ctypes.CDLL(args.dll)
    except OSError as exc:
        print(f"FAIL: cannot load DLL: {exc}")
        return 1

    checks = []
    try:
        lib.mpv_client_api_version.restype = ctypes.c_ulong
        version = lib.mpv_client_api_version()
        checks.append(("mpv_client_api_version", version != 0, hex(version)))
    except AttributeError as exc:
        checks.append(("mpv_client_api_version", False, str(exc)))

    try:
        lib.mpv_create.restype = ctypes.c_void_p
        handle = lib.mpv_create()
        checks.append(("mpv_create", bool(handle), hex(handle or 0)))
        if handle:
            lib.mpv_terminate_destroy.argtypes = [ctypes.c_void_p]
            lib.mpv_terminate_destroy(handle)
            checks.append(("mpv_terminate_destroy", True, "ok"))
    except AttributeError as exc:
        checks.append(("mpv_create", False, str(exc)))

    for name, ok, detail in checks:
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}: {detail}")
    failed = sum(1 for _, ok, _ in checks if not ok)
    print(f"=== {len(checks) - failed}/{len(checks)} passed ===")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
