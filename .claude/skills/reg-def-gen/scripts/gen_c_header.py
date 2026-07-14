#!/usr/bin/env python3
"""Emit a C-language register-definition header from a parsed JSON.

The header is organised as follows (matches the convention used in
``dv/simulation/verif_env/soc/header_files/c_header``):

  /* file-header comment */
  #define <IP>_REG_BASE_ADDR                0xXXXXXXXX

  /* ===== Section N.M = heading ===== */
  /* REG_NAME (offset) - description */
  typedef union {
      struct {
          unsigned int name /* name, access, comment */ : W;
          ...
      }bits;
      unsigned int u32;
  } U_<IP>_<REG>;

Each documented field is emitted as ``unsigned int name : W;``; each
reserved field is emitted as ``unsigned int /* rsvd_hi_lo */ : W;``.

For every register a matching pair of macros is emitted::

  #define <IP>_<REG>_REG_OFFSET  0x<OFFSET>
  #define <IP>_<REG>_REG_ADDR    (<IP>_REG_BASE_ADDR + <IP>_<REG>_REG_OFFSET)

Variable-width fields (``x:N``) have their width resolved by
``resolve_field_width.resolve_field_width``; the description's
``Range Variable[x]:`` clause is parsed to compute the maximum legal width.

Gap-fill: after all documented fields are emitted, any contiguous run of
bits that has no documented field is emitted as a reserved field with name
``/* rsvd_HI_LO */`` so the struct sums to exactly 32 bits.

Usage::

    python3 gen_c_header.py --input regs.json --output RegDef.h --base-addr 0xC0001000

Pass ``--ip-prefix DWC_ddr_umctl2`` to use a different prefix in the macro
and typedef names; defaults to deriving the prefix from the input JSON's
first register name with the "section/title" pattern, or to ``DW_<ip>``.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

# Local import (works whether invoked directly or as a module).
sys.path.insert(0, str(Path(__file__).resolve().parent))
from resolve_field_width import resolve_field_width  # noqa: E402


# ---------------------------------------------------------------------------
# Bit-range parser
# ---------------------------------------------------------------------------
def parse_bits(bits: str) -> tuple[int, int, int, bool] | None:
    """Return ``(hi, lo, width, is_variable)`` or ``None`` on bad input."""
    if ":" in bits:
        hi_s, lo_s = bits.split(":")
        is_var = hi_s == "x"
        try:
            hi = int(hi_s) if hi_s != "x" else 0
            lo = int(lo_s) if lo_s != "x" else 0
        except ValueError:
            return None
        width = 4 if is_var else hi - lo + 1
        return (hi, lo, width, is_var)
    try:
        bit = int(bits)
    except ValueError:
        return None
    return (bit, bit, 1, False)


# ---------------------------------------------------------------------------
# Gap-fill augmentation
# ---------------------------------------------------------------------------
def fill_field_gaps(fields: list[dict[str, Any]]) -> list[tuple[int, int, int, dict[str, Any], bool]]:
    """Augment field list with reserved fields for undocumented bit positions.

    Returns a list of ``(effective_hi, lo, width, field_dict, is_variable)``
    tuples sorted by descending ``effective_hi`` (MSB-first), suitable for
    emitting the bitfield declaration from MSB to LSB.
    """
    annotated: list[tuple[int, int, int, dict[str, Any], bool]] = []
    for f in fields:
        bp = parse_bits(f["bits"])
        if bp is None:
            continue
        hi, lo, width, is_var = bp
        if is_var:
            w = resolve_field_width(f["bits"], f.get("desc", ""))
            if w is not None:
                width = w
        effective_hi = (lo + width - 1) if is_var else hi
        annotated.append((effective_hi, lo, width, f, is_var))

    covered = {b for hi, lo, _w, _f, _v in annotated for b in range(lo, hi + 1)}
    gaps: list[tuple[int, int]] = []
    b = 31
    while b >= 0:
        if b not in covered:
            ghi = b
            while b >= 0 and b not in covered:
                b -= 1
            glo = b + 1
            gaps.append((ghi, glo))
        else:
            b -= 1

    merged = list(annotated)
    for ghi, glo in gaps:
        w = ghi - glo + 1
        merged.append(
            (
                ghi,
                glo,
                w,
                {
                    "bits": f"{ghi}:{glo}" if w > 1 else str(ghi),
                    "name": "",
                    "access": "",
                    "desc": "Reserved Field: Yes (gap-fill)",
                    "reserved": True,
                },
                False,
            )
        )
    merged.sort(key=lambda x: x[0], reverse=True)
    return merged


# ---------------------------------------------------------------------------
# Header rendering
# ---------------------------------------------------------------------------
def render_c_header(registers: list[dict[str, Any]], base_addr: int,
                    ip_prefix: str, source_label: str) -> str:
    """Produce the full source text of the C header."""
    # Deduplicate by (section, name, offset) so shadow registers are not
    # emitted twice when the same register is described in two chapters.
    seen: dict[tuple, bool] = {}
    unique: list[dict[str, Any]] = []
    for r in registers:
        k = (r.get("section", 0), r.get("name", ""), r.get("meta", {}).get("offset", ""))
        if k in seen:
            continue
        seen[k] = True
        unique.append(r)

    def sort_key(r: dict[str, Any]):
        try:
            off = int(r["meta"]["offset"], 16)
        except (ValueError, KeyError):
            off = 0
        return (r.get("section", 0), off, r.get("name", ""))

    unique.sort(key=sort_key)
    sections = sorted({r.get("section", 0) for r in unique if r.get("section")})

    guard = f"__{ip_prefix.upper().replace('-', '_')}_REG_DEF_H__"
    lines: list[str] = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("/*")
    lines.append(" * ============================================================================")
    lines.append(f" * {ip_prefix} Register Definition (C / BSP Header)")
    lines.append(" * ---------------------------------------------------------------------------")
    lines.append(f" * Auto-generated by reg-def-gen skill from {source_label}")
    lines.append(" * Layout: APB/AHB/AXI byte-addressable, 32-bit data width.")
    lines.append(" * Naming follows the project convention used by")
    lines.append(" *     dv/simulation/verif_env/soc/header_files/c_header/<ip>RegDef.h")
    lines.append(" *")
    lines.append(f" * Base address: 0x{base_addr:08X}.")
    lines.append(" * ============================================================================")
    lines.append(" */")
    lines.append("")

    current_section: int | None = None
    typedefs: list[str] = []
    offsets: list[str] = []
    seen_names: set[str] = set()

    for r in unique:
        sec = r.get("section", 0)
        if sec != current_section:
            current_section = sec
            section_label = f"Section {sec}" if sec else "Common"
            typedefs.append("")
            typedefs.append(f"/* ===== {section_label} ===== */")
            typedefs.append("")

        name = r["name"]
        if name in seen_names:
            continue
        seen_names.add(name)

        offset = r["meta"].get("offset", "0x0")
        typedef_name = f"U_{ip_prefix.upper()}_{name}"

        typedefs.append(f"/* {name} ({offset}) - {r['meta'].get('description', '')} */")
        typedefs.append("typedef union")
        typedefs.append("{")
        typedefs.append("\tstruct")
        typedefs.append("\t{")

        for hi, lo, w, f, is_var in fill_field_gaps(r["fields"]):
            comment_parts: list[str] = []
            if f.get("name"):
                comment_parts.append(f["name"])
            if f.get("access"):
                comment_parts.append(f["access"])
            if is_var:
                comment_parts.append(f"variable width (x:N), default={w} bits")
            if f.get("desc"):
                first_line = f["desc"].split("\n")[0].strip()[:60]
                if first_line and "Reserved Field" not in first_line and "gap-fill" not in first_line:
                    comment_parts.append(first_line)
            comment = " /* " + ", ".join(comment_parts) + " */" if comment_parts else ""

            if f.get("reserved"):
                typedefs.append(f"\t\tunsigned int /* rsvd_{hi}_{lo} */{comment} : {w};")
            else:
                typedefs.append(f"\t\tunsigned int {f['name']}{comment} : {w};")

        typedefs.append("\t}bits;")
        typedefs.append("\tunsigned int u32;")
        typedefs.append(f"}} {typedef_name};")
        typedefs.append("")

        offsets.append(f"#define {ip_prefix.upper()}_{name}_REG_OFFSET\t\t\t\t\t{offset}")

    body: list[str] = []
    body.append("\n".join(typedefs))
    body.append("")
    body.append("/* Address Base & Per-Register OFFSET / ADDR Macros */")
    body.append("")
    body.append(f"#define {ip_prefix.upper()}_REG_BASE_ADDR\t\t\t\t\t\t0x{base_addr:08X}")
    body.append("")
    body.extend(offsets)
    body.append("")
    for r in unique:
        nm = r["name"]
        if nm not in seen_names:
            continue
        body.append(
            f"#define {ip_prefix.upper()}_{nm}_REG_ADDR\t\t\t\t\t\t"
            f"({ip_prefix.upper()}_REG_BASE_ADDR + {ip_prefix.upper()}_{nm}_REG_OFFSET)"
        )
    body.append("")
    body.append(f"#endif //{guard}")

    lines.append("\n".join(body))
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--base-addr", required=True, type=lambda s: int(s, 16))
    p.add_argument("--ip-prefix", default=None,
                   help="Override the macro/typedef prefix (default: derive from JSON)")
    p.add_argument("--source-label", default="<unknown>",
                   help="Label for the file-header comment (e.g. Databook version)")
    args = p.parse_args()

    data = json.loads(args.input.read_text())
    registers = data.get("registers", [])
    if not registers:
        print("ERROR: input JSON contains no registers", file=sys.stderr)
        return 2

    # Derive prefix from JSON when not given: use the longest common uppercase
    # leading substring of all register-name keys, then strip the trailing
    # underscore.  Falls back to "DW_<ip>" if the regex gives nothing.
    ip_prefix = args.ip_prefix
    if not ip_prefix:
        names = [r["name"] for r in registers]
        # Heuristic: count split-at-underscore prefixes that match all names.
        parts = names[0].split("_")
        common = []
        for idx, part in enumerate(parts):
            if all(n.split("_")[idx] == part for n in names if len(n.split("_")) > idx):
                common.append(part)
            else:
                break
        if common:
            ip_prefix = "_".join(common).rstrip("_")
        else:
            ip_prefix = "DW_IP"

    body = render_c_header(registers, args.base_addr, ip_prefix, args.source_label)
    args.output.write_text(body)
    print(f"Wrote {args.output} ({len(body)} chars, "
          f"{body.count(chr(10))} lines, {body.count('typedef union')} typedefs)",
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
