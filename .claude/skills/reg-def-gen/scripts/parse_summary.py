#!/usr/bin/env python3
"""Parse Synopsys DesignWare IP databook text into structured JSON.

Designed to work across the Synopsys IP families (DW_apb_*, DW_axi_*, DWC_*).
Recognises two conventions found in their databooks:

  1. Section headers of the form "N.M.K  REGISTER_NAME" (N/M/K integers,
     REGISTER_NAME is UPPER_SNAKE).  Example: "5.1.23  MSTR".

  2. Field tables introduced by "Table N-K Fields for Register: REG_NAME"
     with rows like:
        Bits    Name    Access    Description-start
        31:30   foo     R/W       First line
                                ... continued description

Both single-bit fields ("31") and ranges ("31:30") are supported.  Fields
whose bit position uses an "x" high bit ("x:24") are emitted as-is; their
width is resolved downstream by `resolve_field_width.py`.

Outputs a JSON document whose top-level shape is:

    {
      "registers": [
        { "section": int, "number": int, "name": str,
          "meta":    { "offset": str, "description": str, "size": str },
          "fields":  [ { "bits": str, "name": str, "access": str,
                         "desc": str, "reserved": bool }, ... ] },
        ...
      ],
      "errors":  [ "REG: message", ... ]
    }
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


# Access-token patterns observed in Synopsys databooks (ordered longest-first
# so that "R/RW" wins over "R" and "W/R" wins over "W").
ACCESS_PATTERN = r"(R/RW|R/W1C|R/WL|RS|RWC|RCW1|R/W|R/W1S|RC|RW|HW|W/R|W|R)"

# Field row: bits TAB? name TAB? access TAB* description-first-line.
# `bits` matches digits/x, ":", digits   e.g. 31:30, 31, x:0, x:24
FIELD_ROW_RE = re.compile(
    rf"^\s*([\dx]+(?::[\dx]+)?)\s+(\S*)\s+{ACCESS_PATTERN}(?:\s+|\s*$)(.*)$"
)

# Reserved-only row: bits "Reserved Field: Yes" (no name, no access).
RESERVED_ROW_RE = re.compile(r"^\s*([\dx]+(?::[\dx]+)?)\s+Reserved Field:\s*Yes")

# Section header (a.k.a. "register anchor"): "<digits>.<digits>.<digits><spaces><REG_NAME>"
SECTION_HEADER_RE = re.compile(
    r"^(\d+)\.(\d+)\.(\d+)\s+([A-Z][A-Z0-9_]+)\s*$", re.MULTILINE
)

# Field-table title.
FIELD_TABLE_RE = re.compile(r"Table\s+\d+-(\d+)\s+Fields for Register:\s+(\S+)")

# Register-meta lines: "■  <Key>:  <value>"  (key list is finite in Synopsys doc).
META_KEYS = ("Name", "Description", "Size", "Offset", "Exists")


def find_register_anchors(text: str) -> list[dict[str, Any]]:
    """Return the list of section anchors in the document order."""
    return [
        {
            "section": int(m.group(1)),
            "subsection": int(m.group(2)),
            "number": int(m.group(3)),
            "name": m.group(4),
            "start": m.start(),
        }
        for m in SECTION_HEADER_RE.finditer(text)
    ]


def parse_register_meta(text: str, start: int, end: int) -> dict[str, str]:
    """Extract the metadata block that follows a register anchor.

    The block is delimited by the `META_KEYS` markers and runs until the next
    section anchor (or end of text).
    """
    meta: dict[str, str] = {}
    chunk = text[start:end]
    for key in META_KEYS:
        m = re.search(r"■\s+{0}:\s*(.+?)$".format(re.escape(key)), chunk, re.MULTILINE)
        if m:
            meta[key.lower()] = m.group(1).strip()
    return meta


def _strip_noise(lines: list[str]) -> list[str]:
    """Drop headers, footers, blank lines, page-feeds from a per-register chunk."""
    noise_re = re.compile(
        r"^(Bits\s+Name|Register Descriptions|DesignWare|"
        r"Version\s+\d+\.\d+|October\s+\d{4}|Synopsys, Inc\.|"
        r"Enhanced Universal|\d+\s+SolvNetPlus|Memory\b|Access\b|"
        r"Description\b|Name\b)"
    )
    cleaned: list[str] = []
    for line in lines:
        if not line.strip():
            cleaned.append(line)
            continue
        if line.startswith("\x0c"):
            continue
        if noise_re.match(line):
            continue
        cleaned.append(line)
    return cleaned


def parse_field_table(text: str, anchor_start: int, anchor_end: int) -> list[dict[str, Any]]:
    """Return the list of field dicts from this register's field table.

    Tables start at ``Table N-X Fields for Register: <NAME>`` and end at the
    next register anchor or any noise marker.  Each row is parsed; subsequent
    non-row lines are concatenated as continuation of the description.
    """
    chunk = text[anchor_start:anchor_end]
    title = FIELD_TABLE_RE.search(chunk)
    if not title:
        return []
    table_start = title.end()
    rest = chunk[table_start:]

    # Trim trailing junk: next section anchor wins.
    end_markers = [
        r"\n\d+\.\d+\.\d+\s+[A-Z][A-Z0-9_]+\b",
        r"^Chapter\s+\d+",
    ]
    end_pos = len(rest)
    for em in end_markers:
        m = re.search(em, rest, re.MULTILINE)
        if m and m.start() < end_pos:
            end_pos = m.start()
    table_text = rest[:end_pos]

    rows: list[dict[str, Any]] = []
    lines = _strip_noise(table_text.split("\n"))
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        if not line.strip():
            i += 1
            continue
        m_res = RESERVED_ROW_RE.match(line)
        if m_res:
            row = {
                "bits": m_res.group(1),
                "name": "",
                "access": "",
                "desc": "Reserved Field: Yes",
                "reserved": True,
            }
            i += 1
            rows.extend(_extend_row_desc(lines, i, row))
            continue
        m = FIELD_ROW_RE.match(line)
        if m:
            row = {
                "bits": m.group(1),
                "name": m.group(2),
                "access": m.group(3),
                "desc": m.group(4).strip(),
                "reserved": m.group(2) == "",
            }
            i += 1
            rows.extend(_extend_row_desc(lines, i, row))
            continue
        i += 1
    return rows


def _is_new_row_start(line: str) -> bool:
    return bool(
        FIELD_ROW_RE.match(line)
        or RESERVED_ROW_RE.match(line)
        or re.match(r"^\s*Bits\s+Name", line)
        or line.startswith("\x0c")
    )


def _extend_row_desc(lines: list[str], start: int, row: dict[str, Any]) -> list[dict[str, Any]]:
    """Accumulate continuation lines onto ``row.desc`` until the next row or noise."""
    i = start
    while i < len(lines):
        cont = lines[i].rstrip()
        if not cont.strip():
            i += 1
            continue
        if _is_new_row_start(cont):
            break
        row["desc"] += "\n" + cont.strip()
        i += 1
    return [row]


def parse(text: str) -> tuple[list[dict[str, Any]], list[str]]:
    """Parse the entire text into a list of register dicts and an error list."""
    anchors = find_register_anchors(text)
    errors: list[str] = []
    if not anchors:
        errors.append("No section headers of form 'N.M.K REG_NAME' found")

    sections = []
    for idx, a in enumerate(anchors):
        end = anchors[idx + 1]["start"] if idx + 1 < len(anchors) else len(text)
        sections.append({**a, "end": end})

    registers: list[dict[str, Any]] = []
    for s in sections:
        meta = parse_register_meta(text, s["start"], s["end"])
        if "offset" not in meta:
            errors.append(f"{s['name']}: missing Offset meta")
            continue
        registers.append(
            {
                "section": s["section"],
                "subsection": s["subsection"],
                "number": s["number"],
                "name": s["name"],
                "meta": meta,
                "fields": parse_field_table(text, s["start"], s["end"]),
            }
        )
    return registers, errors


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input", required=True, type=Path, help="databook text extract")
    p.add_argument("--output", required=True, type=Path, help="output JSON path")
    args = p.parse_args()

    text = args.input.read_text()
    regs, errs = parse(text)
    args.output.write_text(
        json.dumps({"registers": regs, "errors": errs}, indent=2)
    )
    print(f"parsed {len(regs)} registers from {args.input} -> {args.output}",
          file=sys.stderr)
    if errs:
        print(f"warnings ({len(errs)}):", file=sys.stderr)
        for e in errs[:5]:
            print(f"  {e}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
