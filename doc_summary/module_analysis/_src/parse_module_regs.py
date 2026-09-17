#!/usr/bin/env python3
"""Stage-1 of reg-def-gen pipeline: parse register field tables from module
analysis docs into reg-def-gen JSON.

Reads: doc_summary/module_analysis/{tim,dma,usi,wdt,pwm,rtc,gpio}_analysis.md
Writes: /tmp/regdef_work/<mod>_regs.json
"""
from __future__ import annotations

import copy, json, os, re, sys

ROOT = "/home/IC_verify/project/wujian100_uvm"
DOC_DIR = f"{ROOT}/doc_summary/module_analysis"
OUT_DIR = "/tmp/regdef_work"
MODULES = ["tim", "dma", "usi", "wdt", "pwm", "rtc", "gpio"]

# access mapping per schema: R/W->"R/W", RO/R->"R", WO/W->"W", others kept as-is
ACCESS_MAP = {
    "R/W": "R/W", "READ/WRITE": "R/W", "RW": "R/W",
    "RO": "R", "READ": "R", "R": "R",
    "WO": "W", "WRITE": "W", "W": "W",
}


# ---------------------------------------------------------------------------
# low-level parsing helpers (adapted from compare_fields.py)
# ---------------------------------------------------------------------------
def norm_bits(b: str) -> str:
    b = b.replace(" ", "").strip("`")
    b = b.replace("[", "").replace("]", "")
    m = re.match(r"^(\d+):(\d+)$", b)
    if m:
        hi, lo = int(m.group(1)), int(m.group(2))
        return f"{max(hi,lo)}:{min(hi,lo)}"
    if re.match(r"^\d+$", b):
        return str(int(b))
    return b


def strip_backtick(s: str) -> str:
    return s.strip().strip("`").strip()


def parse_access(raw: str) -> str:
    s = raw.strip().upper().replace(" ", "")
    return ACCESS_MAP.get(s, raw.strip())


# ---------------------------------------------------------------------------
# table header / row parsing
# ---------------------------------------------------------------------------
def parse_table_header(hdr_cells: list[str]):
    """Return (bit_col, name_col, acc_col, desc_col) indices or None if not
    a field table."""
    hdr = [c.strip().lower() for c in hdr_cells]
    joined = "|".join(hdr)
    # check if it's a field-table style (Bits/Name/R/W/Description)
    if "bits" in hdr[0] or "field" in hdr[0] or "位域" in hdr[0]:
        bi = 0
        ni = next((k for k, h in enumerate(hdr) if "name" in h or "名称" in h or "字段名" in h), None)
        ai = next((k for k, h in enumerate(hdr) if h in ("r/w", "access", "访问", "访问类型")), None)
        di = len(hdr) - 1  # description = last column
        # DMA 3-col style (Bits|R/W|Description): no name column
        if len(hdr) == 3 and bi == 0 and ai == 1 and di == 2:
            ni = None
        # GPIO 5-col style (Bits|Field Name|R/W|Description|Reset): di=3
        if len(hdr) == 5 and bi == 0 and ai == 2:
            di = 3
        return bi, ni, ai, di
    return None


def _clean_name(name: str) -> str:
    """Clean a field name to a valid C identifier.

    Steps:
    1. Strip [N:M] width decoration (e.g. RPL[2:0] -> RPL)
    2. Lowercase
    3. Replace non-alphanumeric chars (except _) with underscore
    4. Collapse consecutive underscores
    5. Strip leading/trailing underscores
    """
    # Step 1: strip [N:M] decoration
    cleaned = re.sub(r'\[[^\]]*\]', '', name)
    # Step 2: lowercase
    cleaned = cleaned.lower()
    # Step 3: replace non-alphanumeric (except underscore) with underscore
    cleaned = re.sub(r'[^a-z0-9_]', '_', cleaned)
    # Step 4: collapse consecutive underscores
    cleaned = re.sub(r'_+', '_', cleaned)
    # Step 5: strip leading/trailing underscores
    cleaned = cleaned.strip('_')
    return cleaned


def parse_field_row(row_cells: list[str], hdr_cells: list[str], bi, ni, ai, di,
                    has_extra_name_col=False) -> dict | None:
    """Parse a field row; returns {bits, name, access, desc, reserved} or None."""
    if len(row_cells) < 2:
        return None
    # Skip separator lines: rows where bits or name cols contain only '-', '|', ' ', ':'
    sep_pat = re.compile(r"^[-| :]+$")
    if row_cells:
        bits_cell = row_cells[bi] if bi < len(row_cells) else ""
        name_cell = row_cells[ni] if ni is not None and ni < len(row_cells) else ""
        if sep_pat.match(bits_cell) or sep_pat.match(name_cell):
            return None

    bits = norm_bits(row_cells[bi]) if bi < len(row_cells) else ""
    name_raw = ""
    if ni is not None and ni < len(row_cells):
        name_raw = strip_backtick(row_cells[ni])
    if has_extra_name_col and not name_raw and di < len(row_cells):
        desc_cell = row_cells[di]
        # Try to extract name from desc's leading backtick token
        # Handle: `NAME` — text, `NAME` alone, `NAME`（text）
        mf = re.match(r"^`([A-Za-z0-9_]+(?:\[[^\]]*\])?)`", desc_cell)
        if mf:
            name_raw = mf.group(1)
    if not name_raw or name_raw.lower() in ("reserved", "n/a", "-", ""):
        if "reserved" in name_raw.lower() or name_raw.lower() in ("reserved", "-", "n/a"):
            name_raw = ""
    access = parse_access(row_cells[ai]) if ai is not None and ai < len(row_cells) else ""
    desc = row_cells[di].strip() if di < len(row_cells) else ""
    reserved = (not name_raw or name_raw.lower() in ("reserved", "n/a", "-"))
    name = _clean_name(name_raw) if name_raw else ""
    return {
        "bits": bits,
        "name": name,
        "access": access,
        "desc": desc,
        "reserved": reserved,
    }


# ---------------------------------------------------------------------------
# section header parsing
# ---------------------------------------------------------------------------
SECTION_RE = re.compile(
    r"^#{3,5}\s+(?:\d+(?:\.\d+)*\s+)?`?([A-Za-z0-9_ ]+?)`?"
    r"(?:\s*[—–\-][^（]*)?（?\s*[Oo]ffset\s*`?(0[xX][0-9a-fA-F]+)`?"
)


def parse_sections(lines):
    """Return list of (name, offset, section_start_line_index) from §4 headers."""
    sections = []
    for i, ln in enumerate(lines):
        m = SECTION_RE.match(ln.strip())
        if m:
            name = strip_backtick(m.group(1)).strip()
            offset = int(m.group(2), 16)
            sections.append((name, offset, i))
    return sections


# ---------------------------------------------------------------------------
# field table extraction: between a section header and the next section header
# ---------------------------------------------------------------------------
def extract_field_table(lines, start, end):
    """Find and parse the field table starting after `start` up to `end`."""
    # find the table header (first line starting with | after start)
    tbl_start = None
    for i in range(start + 1, min(end + 1, len(lines))):
        if lines[i].strip().startswith("|"):
            tbl_start = i
            break
    if tbl_start is None:
        return []

    # collect table lines (consecutive lines starting with |)
    tbl_lines = []
    for i in range(tbl_start, len(lines)):
        if not lines[i].strip().startswith("|"):
            break
        tbl_lines.append(lines[i])
    if len(tbl_lines) < 2:
        return []

    hdr_cells = [c.strip() for c in tbl_lines[0].strip().strip("|").split("|")]
    parsed = parse_table_header(hdr_cells)
    if parsed is None:
        return []
    bi, ni, ai, di = parsed
    has_extra = (len(hdr_cells) == 3 and bi == 0)  # DMA 3-col/4-row style

    fields = []
    for row in tbl_lines[1:]:
        cells = [c.strip() for c in row.strip().strip("|").split("|")]
        f = parse_field_row(cells, hdr_cells, bi, ni, ai, di, has_extra_name_col=has_extra)
        if f and f["bits"]:
            fields.append(f)
    return fields


# ---------------------------------------------------------------------------
# register parser: combines section headers + field tables + memory map
# ---------------------------------------------------------------------------
def parse_register_sections(lines):
    """Parse all registers from §4 of a document.

    Strategy: find register entries from both the memory map table (in §4.1)
    and section headers with field tables, deduplicate by offset.
    """
    raw_regs = []  # list of {name, offset, fields, description}

    # 1) Find section headers (each register sub-section in §4)
    sections = parse_sections(lines)

    # 2) For each section header, extract field table
    for idx, (name, offset, sec_start) in enumerate(sections):
        end = sections[idx + 1][2] if idx + 1 < len(sections) else len(lines)
        fields = extract_field_table(lines, sec_start, end)
        # description: the prose right after the header (before the table)
        desc = ""
        for j in range(sec_start + 1, min(sec_start + 6, end)):
            l = lines[j].strip()
            if l.startswith("|") or l == "[TABLE]" or l.startswith("注："):
                break
            if l and not l.startswith("###") and not l.startswith("####"):
                desc = l
                break
        raw_regs.append({
            "name": name,
            "offset": offset,
            "fields": fields,
            "description": desc,
        })

    # 3) Also parse memory map table in §4 for any registers not covered by
    #    section headers (e.g. registers that only appear in the map but have
    #    no dedicated field table section)
    #    But typically section headers cover all registers that have fields,
    #    and the memory map table lists them without fields. We need to pick
    #    only the entries with fields (from section headers) and supplement
    #    with map-only entries.

    return raw_regs


# ---------------------------------------------------------------------------
# Deduplication & module-specific transformations
# ---------------------------------------------------------------------------
def _norm_name(name: str) -> str:
    """Normalize register name for comparison: strip spaces, casefold."""
    return re.sub(r"[\s_]+", "", name).lower()


def dedup_by_offset(regs: list[dict]) -> list[dict]:
    """Keep entries per offset; if duplicate offset, prefer entry with fields
    but preserve same-offset registers with different names (e.g. TX_FIFO/RX_FIFO)."""
    seen: dict[int, dict] = {}
    seen_norm: dict[int, str] = {}  # offset -> normalized name of kept entry
    result: list[dict] = []
    for r in regs:
        off = r["offset"]
        nname = _norm_name(r["name"])
        if off not in seen:
            seen[off] = r
            seen_norm[off] = nname
            result.append(r)
        else:
            existing = seen[off]
            existing_norm = seen_norm[off]
            # Same normalized name at same offset (true duplicate): prefer entry with fields
            if existing_norm == nname:
                if r["fields"] and not existing["fields"]:
                    seen[off] = r
                    seen_norm[off] = nname
                    result[-1] = r
            # Different normalized name at same offset (e.g. TX_FIFO vs RX_FIFO): keep both
            else:
                if r["fields"] and not existing["fields"]:
                    seen[off] = r
                    seen_norm[off] = nname
                    result[-1] = r
                result.append(r)
    return result


def transform_register_name(name: str, module: str) -> str:
    """Convert register name to C identifier uppercase per spec."""
    n = re.sub(r"[\s_`]+", "", name).upper()
    return n


def transform_register_name_tim(name: str) -> str:
    """TIM: remove spaces, uppercase."""
    return re.sub(r"[\s]+", "", name).upper()


def transform_register_name_dma(name: str, offset: int) -> str:
    """DMA: channel register names get n→0; globals stay as-is."""
    channel_offsets = {0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20}
    if offset in channel_offsets and "n" in name:
        name = re.sub(r"\bn\b", "0", name)
    return name


def parse_offset_description(lines, reg_offset):
    """Find description text near a register section header."""
    return ""


# ---------------------------------------------------------------------------
# Field inheritance & cleanup helpers
# ---------------------------------------------------------------------------
def _inherit_fields(registers: list[dict], targets: list[str], source: str) -> bool:
    """Copy fields from one register into grouped registers."""
    source_reg = next((r for r in registers if r["name"] == source), None)
    if source_reg is None or not source_reg["fields"]:
        return False
    for target in targets:
        target_reg = next((r for r in registers if r["name"] == target), None)
        if target_reg is not None:
            target_reg["fields"] = copy.deepcopy(source_reg["fields"])
    return True


def _fix_dma_chsr_field_names(registers: list[dict]) -> None:
    """Correct the UG typo for CHSR bit 13 before generic duplicate handling."""
    chsr = next((r for r in registers if r["name"] == "CHSR"), None)
    if chsr is None:
        return
    bit13 = next((f for f in chsr["fields"] if f["bits"] == "13"), None)
    if bit13 is None:
        return
    bit13["name"] = "ch13bsy"
    bit13["desc"] = f"{bit13['desc']}；注：UG 原文误拼为 ch12bsy，按 bit13 修正为 ch13bsy"


def _deduplicate_field_names(registers: list[dict]) -> None:
    """Make field names unique within each register."""
    for register in registers:
        used: set[str] = set()
        for field in register["fields"]:
            name = field["name"]
            if not name:
                continue
            if name not in used:
                used.add(name)
                continue
            field["name"] = f"{name}_b{field['bits']}"
            used.add(field["name"])


def _inherit_grouped_register_fields(module: str, registers: list[dict]) -> None:
    """Inherit fields documented only in representative grouped registers."""
    if module == "tim":
        tim_mappings = [
            (["TIMER2LOADCOUNT"], "TIMER1LOADCOUNT"),
            (["TIMER2CURRENTVALUE"], "TIMER1CURRENTVALUE"),
            (["TIMER2CONTROLREG"], "TIMER1CONTROLREG"),
            (["TIMER2_INT_CLR"], "TIMER1_INT_CLR"),
            (["TIMER2INTSTATUS"], "TIMER1INTSTATUS"),
        ]
        for targets, source in tim_mappings:
            _inherit_fields(registers, targets, source)
        for register in registers:
            if not register["name"].startswith("TIMER2"):
                continue
            for field in register["fields"]:
                if field["name"]:
                    field["name"] = re.sub(r"^timer1_?", lambda m: "timer2_" if m.group(0).endswith("_") else "timer2", field["name"])
                if field.get("desc"):
                    field["desc"] = field["desc"].replace("Timer1", "Timer2")
    elif module == "pwm":
        pwm_mappings = [
            (["PWM01LOAD", "PWM23LOAD"], "PWM45LOAD"),
            (["PWM01COUNT", "PWM23COUNT"], "PWM45COUNT"),
            (["PWM01DB", "PWM23DB"], "PWM45DB"),
            (["PWM0CMP", "PWM1CMP", "PWM2CMP", "PWM3CMP", "PWM4CMP"], "PWM5CMP"),
            (["CAP01T", "CAP23T"], "CAP45T"),
            (["CAP01MATCH", "CAP23MATCH"], "CAP45MATCH"),
            (["TIM01LOAD", "TIM23LOAD"], "TIM45LOAD"),
            (["TIM01COUNT", "TIM23COUNT"], "TIM45COUNT"),
            (["CNT01VAL", "CNT23VAL"], "CNT45VAL"),
        ]
        for targets, source in pwm_mappings:
            _inherit_fields(registers, targets, source)


# ---------------------------------------------------------------------------
# Main parsing per module
# ---------------------------------------------------------------------------
def parse_module(module: str) -> dict:
    doc_path = f"{DOC_DIR}/{module}_analysis.md"
    lines = open(doc_path, encoding="utf-8").read().split("\n")

    # Find the start of §4 (寄存器章节)
    sec4_start = None
    for i, ln in enumerate(lines):
        if re.match(r"^## 4\b", ln.strip()):
            sec4_start = i
            break
    if sec4_start is None:
        return {"registers": [], "errors": [f"{module}: no §4 found"]}

    # Parse all register sections within §4
    # Restrict parsing to lines within §4 (before §5 or end)
    sec4_end = len(lines)
    for i in range(sec4_start + 1, len(lines)):
        if re.match(r"^## 5\b", lines[i].strip()):
            sec4_end = i
            break

    section_lines = lines[sec4_start:sec4_end]

    # Use the section header parser
    raw_regs = parse_register_sections(section_lines)

    # For modules where section headers don't capture everything (like PWM
    # where the memory-map lists registers that later get field sections),
    # we need a different approach: scan all register sections within §4
    # and build registers from those with field tables only.
    if module == "pwm":
        raw_regs = _parse_pwm(section_lines)
    elif module == "tim":
        raw_regs = _parse_tim(section_lines)

    # Deduplicate
    regs = dedup_by_offset(raw_regs)

    # Apply module-specific transforms
    errors = []
    result_regs = []
    for i, r in enumerate(regs):
        name = r["name"]
        offset = r["offset"]
        fields = r["fields"]
        description = r["description"]

        if module == "tim":
            name = transform_register_name_tim(name)
        elif module == "dma":
            name = transform_register_name_dma(name, offset)
        else:
            name = transform_register_register_name(name)

        # Clean up description (remove backticks, trim)
        description = strip_backtick(description)
        if not description:
            # Try to get from register section title
            for sec_name, sec_off, _ in parse_sections(section_lines):
                if sec_off == offset:
                    description = strip_backtick(sec_name)
                    break

        # Normalize fields
        norm_fields = []
        for f in fields:
            nf = {
                "bits": f["bits"],
                "name": f["name"] if f["name"] else "",
                "access": f["access"],
                "desc": f["desc"],
                "reserved": f["reserved"],
            }
            norm_fields.append(nf)

        result_regs.append({
            "section": _guess_section(module, offset),
            "name": name,
            "meta": {
                "offset": f"0x{offset:08x}",
                "description": description,
                "size": "32",
            },
            "fields": norm_fields,
        })

    _fix_dma_chsr_field_names(result_regs)
    _inherit_grouped_register_fields(module, result_regs)
    _deduplicate_field_names(result_regs)

    return {"registers": result_regs, "errors": errors}


def transform_register_register_name(name: str) -> str:
    """Default: uppercase, no spaces."""
    return re.sub(r"[\s_`]+", "", name).upper()


def _guess_section(module, offset):
    return 4


# ---------------------------------------------------------------------------
# Module-specific parsers for edge cases
# ---------------------------------------------------------------------------
def _parse_tim(lines):
    """TIM: find registers by scanning all register sections in §4."""
    regs = []
    # Use parse_register_sections but with improved description extraction
    raw = parse_register_sections(lines)
    # Also parse the memory map table in §4.1
    raw.extend(_parse_memory_map_table(lines))
    return raw


def _parse_pwm(lines):
    """PWM: deduplicated from both memory map and field sections."""
    raw = parse_register_sections(lines)
    # Add memory map entries (they may have no fields but list registers)
    raw.extend(_parse_memory_map_table(lines))
    return raw


def _parse_memory_map_table(lines):
    """Parse memory map tables (Register|Offset|...|Description tables)."""
    regs = []
    lines_text = [l.strip() for l in lines]
    i = 0
    while i < len(lines_text):
        if not lines_text[i].startswith("|"):
            i += 1
            continue
        # Collect table
        tbl = []
        while i < len(lines_text) and lines_text[i].startswith("|"):
            tbl.append(lines_text[i])
            i += 1
        if len(tbl) < 2:
            continue
        hdr_cells = [c.strip() for c in tbl[0].strip().strip("|").split("|")]
        hdr_joined = "|".join(hdr_cells).lower()
        if not ("offset" in hdr_joined and ("name" in hdr_joined or "寄存器" in hdr_joined or "register" in hdr_joined)):
            continue
        # Parse header indices
        hc = [h.strip().lower() for h in hdr_cells]
        ni = next((k for k, h in enumerate(hc) if h in ("name", "寄存器", "register", "寄存器名")), None)
        oi = next((k for k, h in enumerate(hc) if "offset" in h), None)
        if ni is None or oi is None:
            continue
        for row in tbl[1:]:
            cells = [c.strip() for c in row.strip().strip("|").split("|")]
            if len(cells) <= max(ni, oi):
                continue
            name = strip_backtick(cells[ni])
            mo = re.search(r"0[xX][0-9a-fA-F]+", cells[oi])
            if name and mo:
                regs.append({
                    "name": name,
                    "offset": int(mo.group(0), 16),
                    "fields": [],
                    "description": "",
                })
    return regs


# ---------------------------------------------------------------------------
# Main entry
# ---------------------------------------------------------------------------
def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    expected = {"tim": 10, "dma": 11, "usi": 29, "wdt": 6, "pwm": 53, "rtc": 9, "gpio": 11}
    counts = {}
    errors_summary = {}

    for mod in MODULES:
        result = parse_module(mod)
        regs = result["registers"]
        errs = result.get("errors", [])
        counts[mod] = len(regs)
        errors_summary[mod] = errs

        out_path = f"{OUT_DIR}/{mod}_regs.json"
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(result, f, indent=2, ensure_ascii=False)
        print(f"{mod}: {len(regs)} registers -> {out_path}")

    # Print comparison
    print("\n=== Register Count Comparison ===")
    all_ok = True
    for mod in MODULES:
        exp = expected[mod]
        act = counts[mod]
        status = "OK" if act == exp else "MISMATCH"
        if act != exp:
            all_ok = False
        print(f"  {mod}: expected={exp} actual={act} [{status}]")

    # Validation
    print("\n=== Offset Uniqueness Check ===")
    for mod in MODULES:
        result = json.load(open(f"{OUT_DIR}/{mod}_regs.json"))
        regs = result["registers"]
        offset_counts: dict[str, int] = {}
        for r in regs:
            off = r["meta"]["offset"]
            offset_counts[off] = offset_counts.get(off, 0) + 1
        dups = {k: v for k, v in offset_counts.items() if v > 1}
        print(f"  {mod}: {len(dups)} duplicate offsets {dups if dups else ''}")

    print("\n=== Bit Range Validation ===")
    for mod in MODULES:
        result = json.load(open(f"{OUT_DIR}/{mod}_regs.json"))
        regs = result["registers"]
        bit_errors = []
        for r in regs:
            covered = set()
            for f in r["fields"]:
                bits = f["bits"]
                if ":" in bits:
                    hi, lo = bits.split(":")
                    hi, lo = int(hi), int(lo)
                    for b in range(min(hi, lo), max(hi, lo) + 1):
                        if b > 31 or b < 0:
                            bit_errors.append(f"{r['name']}[{bits}]: bit {b} OOB")
                        if b in covered:
                            bit_errors.append(f"{r['name']}[{bits}]: bit {b} overlap")
                        covered.add(b)
                else:
                    b = int(bits)
                    if b > 31 or b < 0:
                        bit_errors.append(f"{r['name']}[{bits}]: bit {b} OOB")
                    if b in covered:
                        bit_errors.append(f"{r['name']}[{bits}]: bit {b} overlap")
                    covered.add(b)
        if bit_errors:
            all_ok = False
            print(f"  {mod}: {len(bit_errors)} errors")
            for e in bit_errors[:5]:
                print(f"    {e}")
        else:
            print(f"  {mod}: OK ({sum(len(r['fields']) for r in regs)} fields across {len(regs)} regs)")

    print("\n=== Summary ===")
    print(f"  All counts match: {'PASS' if all_ok else 'FAIL'}")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
