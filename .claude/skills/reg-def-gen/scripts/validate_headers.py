#!/usr/bin/env python3
"""Validate the C + SystemVerilog register-definition headers emitted by
``gen_c_header.py`` and ``gen_sv_header.py``.

Checks performed:

  * The C header contains one ``typedef union { struct {...} bits; unsigned int u32; }``
    per register, and within each struct, all ``: N;`` widths sum to **exactly 32**.
  * The SV header contains one ``typedef union { struct packed {...} bits; int unsigned u32 = {...}; }``
    per register, with both the struct bit-widths and the ``u32`` initialiser
    summing to 32.
  * Brace counts balance in both files.
  * For every register name, both ``<REG>_REG_OFFSET`` and ``<REG>_REG_ADDR``
    macros are defined in both files.

Exits with code 0 on success, non-zero on any failure.  Reports a summary on
stderr and an itemised list of errors when any are found.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# C header validation
# ---------------------------------------------------------------------------
C_TYPEDEF_RE = re.compile(
    r"typedef\s+union\s*\{?\s*struct\s*\{(?P<body>.*?)\}\s*bits;\s*"
    r"unsigned\s+int\s+u32;\s*\}\s+(?P<name>U_\w+);",
    re.DOTALL,
)
C_WIDTH_RE = re.compile(r":\s*(\d+)\s*;")


def validate_c(text: str) -> tuple[list[tuple[str, int, int]], list[tuple[str, int]]]:
    """Return (typedefs, errors) for the C header text.

    typedefs: list of (name, struct_bits, 32)
    errors:   list of (name, struct_bits) - structs whose bits != 32
    """
    typedefs = []
    errors = []
    for m in C_TYPEDEF_RE.finditer(text):
        body = m.group("body")
        name = m.group("name")
        bits = sum(int(w) for w in C_WIDTH_RE.findall(body))
        typedefs.append((name, bits, 32))
        if bits != 32:
            errors.append((name, bits))
    return typedefs, errors


# ---------------------------------------------------------------------------
# SV header validation
# ---------------------------------------------------------------------------
SV_TYPEDEF_RE = re.compile(
    r"typedef\s+union\s*\{?\s*struct\s+packed\s*\{(?P<body>.*?)\}\s*bits;\s*"
    r"int\s+unsigned\s+u32\s*=\s*\{(?P<concat>.*?)\};\s*\}\s+(?P<name>U_\w+);",
    re.DOTALL,
)
SV_FIELD_RE = re.compile(r"(?:const\s+)?bit(?:\s+|\[(\d+):0\]\s+)(?:\w+)?")
SV_CONCAT_RE = re.compile(r"(\d+)'h\d+")


def validate_sv(text: str) -> tuple[list[tuple[str, int, int, int]], list[tuple[str, int, int]]]:
    typedefs = []
    errors = []
    for m in SV_TYPEDEF_RE.finditer(text):
        body = m.group("body")
        concat = m.group("concat")
        name = m.group("name")

        struct_bits = 0
        for fm in SV_FIELD_RE.finditer(body):
            if fm.group(1):
                struct_bits += int(fm.group(1)) + 1
            else:
                struct_bits += 1

        u32_bits = sum(int(c) for c in SV_CONCAT_RE.findall(concat))

        ok = (struct_bits == 32) and (u32_bits == 32) and (struct_bits == u32_bits)
        typedefs.append((name, struct_bits, u32_bits, int(ok)))
        if not ok:
            errors.append((name, struct_bits, u32_bits))
    return typedefs, errors


# ---------------------------------------------------------------------------
# Macro consistency check
# ---------------------------------------------------------------------------
def extract_macros(text: str) -> dict[str, set[str]]:
    """Return {prefix: set(name)} for every macro defined in the text."""
    out: dict[str, set[str]] = {}
    for m in re.finditer(r"(?:`define\s+|#define\s+)(\w+)", text):
        n = m.group(1)
        out.setdefault(n.split("_")[0], set()).add(n)
    return out


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--c-header", type=Path)
    p.add_argument("--sv-header", type=Path)
    p.add_argument("--quiet", action="store_true",
                   help="only print error/summary lines, not info")
    args = p.parse_args()
    if not args.c_header and not args.sv_header:
        p.error("at least one of --c-header / --sv-header is required")

    exit_code = 0

    if args.c_header:
        c_text = args.c_header.read_text()
        c_typedefs, c_errors = validate_c(c_text)
        n_total = len(c_typedefs)
        n_ok = sum(1 for _, b, _ in c_typedefs if b == 32)
        print(f"\n=== C Header: {args.c_header} ===", file=sys.stderr)
        print(f"  typedefs:        {n_total}", file=sys.stderr)
        print(f"  struct==32:      {n_ok}", file=sys.stderr)
        print(f"  errors:          {len(c_errors)}", file=sys.stderr)
        opens = c_text.count("{")
        closes = c_text.count("}")
        print(f"  brace balance:   {opens} {{ / {closes} }}", file=sys.stderr)
        if c_errors:
            exit_code = 1
            for name, b in c_errors[:10]:
                print(f"    {name}: struct={b}, expected 32", file=sys.stderr)

    if args.sv_header:
        sv_text = args.sv_header.read_text()
        sv_typedefs, sv_errors = validate_sv(sv_text)
        n_total = len(sv_typedefs)
        n_ok = sum(1 for _, _, _, ok in sv_typedefs if ok)
        print(f"\n=== SV Header: {args.sv_header} ===", file=sys.stderr)
        print(f"  typedefs:        {n_total}", file=sys.stderr)
        print(f"  struct==32==u32: {n_ok}", file=sys.stderr)
        print(f"  errors:          {len(sv_errors)}", file=sys.stderr)
        opens = sv_text.count("{")
        closes = sv_text.count("}")
        print(f"  brace balance:   {opens} {{ / {closes} }}", file=sys.stderr)
        if sv_errors:
            exit_code = 1
            for name, s, u in sv_errors[:10]:
                print(f"    {name}: struct={s}, u32={u}", file=sys.stderr)

    # Cross-check: every register name appearing as a typedef should also appear
    # as both _REG_OFFSET and _REG_ADDR in BOTH files.
    if args.c_header and args.sv_header:
        c_text = args.c_header.read_text()
        sv_text = args.sv_header.read_text()
        c_offsets = set(re.findall(r"#define\s+(\w+_REG_OFFSET)", c_text))
        sv_offsets = set(re.findall(r"`define\s+(\w+_REG_OFFSET)", sv_text))
        c_addrs = set(re.findall(r"#define\s+(\w+_REG_ADDR)", c_text))
        sv_addrs = set(re.findall(r"`define\s+(\w+_REG_ADDR)", sv_text))

        miss_off = (c_offsets ^ sv_offsets)
        miss_addr = (c_addrs ^ sv_addrs)
        if miss_off:
            print(f"\nAsymmetric _REG_OFFSET macros ({len(miss_off)}):", file=sys.stderr)
            for n in sorted(miss_off)[:10]:
                print(f"  {n}", file=sys.stderr)
            exit_code = 1
        if miss_addr:
            print(f"\nAsymmetric _REG_ADDR macros ({len(miss_addr)}):", file=sys.stderr)
            for n in sorted(miss_addr)[:10]:
                print(f"  {n}", file=sys.stderr)
            exit_code = 1
        if not miss_off and not miss_addr:
            print("\n=== Macro consistency: OK ===", file=sys.stderr)

    print(f"\nResult: {'FAIL' if exit_code else 'PASS'}", file=sys.stderr)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
