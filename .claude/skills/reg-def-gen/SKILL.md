---
name: reg-def-gen
description: "Generate uvm register-model companion headers (C .h for BSP and SystemVerilog .svh for testbench) from a Synopsys DesignWare IP databook text extract. Use when you need to produce project-convention RegDef.h/RegDef.svh files that match the layout used by `dv/simulation/verif_env/soc/header_files/` (one typedef-union per register, per-register offset + addr macros). The skill is IP-agnostic across Synopsys IP families (DW_apb_*, DW_axi_*, DWC_*) provided the input text uses the standard 'N.M.K REG_NAME' section markers and 'Table N-M Fields for Register: NAME' field tables."
metadata:
  author: IC_verify
  organization: openclaw
  contact: 78490223@qq.com
---

# reg-def-gen

Generate **two companion headers** for a Synopsys IP from a text-extracted databook:

| Output | Path convention | Used in |
|--------|-----------------|---------|
| `<IP>RegDef.h` | `dv/simulation/verif_env/<area>/header_files/c_header/<IP>RegDef.h` | BSP / firmware C code |
| `<IP>RegDef.svh` | `dv/simulation/verif_env/<area>/header_files/sv_header/<IP>RegDef.svh` | UVM testbench register handles |

Both files expose the same set of register typedefs (`U_<IP>_<REG>`) and per-register offset/addr macros. Field tables in the source are parsed → resolved (variable-width fields expanded) → gap-filled (undocumented bits added as reserved) → emitted.

## When to Use

- After a Synopsys DesignWare IP (e.g. `DW_apb_wdt`, `DWC_ddr_umctl2`, `DW_axi_dmac`) databook PDF has been extracted to a plain-text or JSON form.
- The host project already uses the `header_files/c_header` and `header_files/sv_header` convention for UVM register-handle headers.
- You need both the C and SystemVerilog views from the *same* field-table source so they stay in lockstep.

## When NOT to Use

- The IP is not Synopsys-format (e.g. ARM PrimeCell, Cadence — their databook tables differ).
- The host project uses its own register tooling (e.g. `reg_gen.py` from `light_gui` for Excel-driven register models) — use that instead.
- The databook is far away from the running agent (large, unparseable PDF) — extract it first with a dedicated tool.

## Input Contract

The skill consumes a **pre-parsed JSON** matching `references/json_schema.md`. The JSON is expected to be emitted by an upstream parser (the legacy DDR-specific script `parse_ddr.py` does this for Synopsys `5.X.Y`-style databook text). To extend to other IP text formats, write a new parser that emits the same JSON shape and pass its output here.

Minimum required JSON fields per register:
```jsonc
{
  "section": 1,                         // integer (group number)
  "name":   "MSTR",                     // register name (UPPER_SNAKE)
  "meta": {
    "offset":      "0x00000000",        // hex string, e.g. "0x004"
    "description": "Master",
    "size":        "32"
  },
  "fields": [
    { "bits": "31:30", "name": "device_config", "access": "R/W",
      "desc": "...", "reserved": false },
    { "bits": "x:24",  "name": "active_ranks",  "access": "R/W",
      "desc": "...\nRange Variable[x]: ..." , "reserved": false }
  ]
}
```

`x:N` bit positions are **variable-width** fields; they are resolved by `scripts/resolve_field_width.py` from the `Range Variable[x]:` expression embedded in the description.

## Workflow

### Stage 1 — Parse databook text → JSON

Out of scope for this skill. Reuse the format produced by the legacy `parse_ddr.py` from `/tmp/` (works on Synopsys text with `5.X.Y REG_NAME` markers and `Table N-X Fields for Register: NAME` field tables). For other text formats, write a new parser that emits the JSON defined in `references/json_schema.md`.

```bash
# DDR-style example (Synopsys Enhanced Universal DDR memory controller):
python3 <parser_script> \
    --input  /tmp/ddr_umctl2_databook.txt \
    --output /tmp/ddr_regs.json
```

### Stage 2 — Generate C header

```bash
python3 $SKILL_DIR/scripts/gen_c_header.py \
    --input      /tmp/ddr_regs.json \
    --output     /home/IC_verify/work_for_openclaw/ip_docs/DWC_ddr_umctl2RegDef.h \
    --base-addr  0xC0001000
```

Produces one `typedef union { struct {...}; unsigned int u32; } U_<IP>_<REG>;` per register, plus `#define <IP>_<REG>_REG_OFFSET / _REG_ADDR` macros.

See `references/output_format.md` for the exact emitted layout.

### Stage 3 — Generate SystemVerilog header

```bash
python3 $SKILL_DIR/scripts/gen_sv_header.py \
    --input      /tmp/ddr_regs.json \
    --output     /home/IC_verify/work_for_openclaw/ip_docs/DWC_ddr_umctl2RegDef.svh \
    --base-addr  0xC0001000
```

Produces one `typedef union { struct packed {...}; int unsigned u32 = {concat}; } U_<IP>_<REG>;` per register (MSB-first, `const bit[N:0]` for reserved), plus `` `define <IP>_<REG>_REG_OFFSET / _REG_ADDR `` macros.

### Stage 4 — Validate

```bash
python3 $SKILL_DIR/scripts/validate_headers.py \
    --c-header  /home/IC_verify/work_for_openclaw/ip_docs/DWC_ddr_umctl2RegDef.h \
    --sv-header /home/IC_verify/work_for_openclaw/ip_docs/DWC_ddr_umctl2RegDef.svh
```

Re-parses each header and verifies (a) every struct field width sums to **exactly 32 bits** (b) brace balance (c) every register has matching `_OFFSET` and `_ADDR` macros in both C and SV (d) `int unsigned u32 = {...}` initializer in SV uses widths that also sum to 32 bits. Non-zero exit code if any error.

### One-shot pipeline

```bash
$SKILL_DIR/scripts/run_all.sh \
    --json      /tmp/ddr_regs.json \
    --base-addr 0xC0001000 \
    --out-dir   /home/IC_verify/work_for_openclaw/ip_docs \
    --ip-prefix DWC_ddr_umctl2
```

Runs Stages 2 → 3 → 4 in sequence; exits non-zero if validation fails.

## Scripts

| Script | Purpose |
|--------|---------|
| `scripts/parse_summary.py` | Convert Synopsys-style databook text (`N.M.K REG_NAME` markers + `Table N-K Fields` tables) → JSON. Reusable across Synopsys IPs. |
| `scripts/resolve_field_width.py` | Compute max width for `x:N` variable-width fields from `Range Variable[x]:` expressions in the field description. |
| `scripts/gen_c_header.py` | Emit `<IP>RegDef.h` from JSON. |
| `scripts/gen_sv_header.py` | Emit `<IP>RegDef.svh` from JSON. |
| `scripts/validate_headers.py` | Verify both headers sum to 32 bits per register, brace balance, macro consistency. |
| `scripts/run_all.sh` | Drive parse → gen_c → gen_sv → validate. |

## Detailed References

- `references/json_schema.md` — input JSON contract per register
- `references/output_format.md` — exact emitted layouts for C and SV (typedefs, macros, sections)
- `references/variable_width.md` — how `Range Variable[x]:` expressions get evaluated, PARAM_MAX coverage
- `references/troubleshooting.md` — common parse/gen/validate failures and their fixes

## Quick Start

```bash
SKILL=$PROJ_HOME/.claude/skills/reg-def-gen

# 1. Parse databook text (one-off — depends on the source PDF/text format)
python3 $SKILL/scripts/parse_summary.py \
    --input  /tmp/<ip>_databook.txt \
    --output /tmp/<ip>_regs.json

# 2. Generate both headers
python3 $SKILL/scripts/gen_c_header.py  --input /tmp/<ip>_regs.json --output $OUT/DWC_ddr_umctl2RegDef.h  --base-addr 0xC0001000
python3 $SKILL/scripts/gen_sv_header.py --input /tmp/<ip>_regs.json --output $OUT/DWC_ddr_umctl2RegDef.svh --base-addr 0xC0001000

# 3. Validate
python3 $SKILL/scripts/validate_headers.py \
    --c-header  $OUT/DWC_ddr_umctl2RegDef.h \
    --sv-header $OUT/DWC_ddr_umctl2RegDef.svh
```

Or simply:
```bash
$SKILL/scripts/run_all.sh \
    --json /tmp/<ip>_regs.json \
    --base-addr 0xC0001000 \
    --out-dir $OUT \
    --ip-prefix <IP>
```

## Project Integration

Generated headers belong in `dv/simulation/verif_env/<area>/header_files/c_header` and `dv/simulation/verif_env/<area>/header_files/sv_header` to match the light_gui output layout. From the project `CLAUDE.md`:

> Naming follows the project convention used by
> `dv/simulation/verif_env/soc/header_files/c_header/<ip>RegDef.h`

`headers_files/` is one of the directories light_gui's `include` path points at, so the files are picked up automatically by `env_gen.config` builds that reference them.
