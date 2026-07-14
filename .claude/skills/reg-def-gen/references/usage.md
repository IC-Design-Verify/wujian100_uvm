# Usage Walkthrough

End-to-end recipe for generating a register-definition header for a
Synopsys IP from a databook text extract.

## Prerequisites

- Python 3.8+
- A databook text extract at e.g. `/tmp/<ip>_databook.txt` whose format
  matches what `parse_summary.py` expects:
  - section anchors: `N.M.K  REGISTER_NAME` at start of line
  - field tables: `Table N-K Fields for Register: REG_NAME`
  - field rows: `<bits>  <name>  <access>  <description-start>`
  - reserved rows: `<bits>  Reserved Field: Yes`
- A desired base address (e.g. `0xC0001000`)

## Step-by-step

### 1. Parse databook text → JSON

```bash
python3 $SKILL_DIR/scripts/parse_summary.py \
    --input  /tmp/dwc_ddr_umctl2_databook.txt \
    --output /tmp/dwc_ddr_umctl2_regs.json
```

The parser recognises the section-marker and field-table conventions used
across Synopsys DW_apb_*, DW_axi_*, DWC_* databooks.

### 2. Generate both headers

Easiest: use the orchestrator.

```bash
$SKILL_DIR/scripts/run_all.sh \
    --json      /tmp/dwc_ddr_umctl2_regs.json \
    --base-addr 0xC0001000 \
    --out-dir   /home/IC_verify/work_for_openclaw/ip_docs \
    --ip-prefix DWC_ddr_umctl2 \
    --source-label "DWC_ddr_umctl2 Databook 3.91a"
```

Two files appear in `out-dir`:

    DWC_ddr_umctl2RegDef.h
    DWC_ddr_umctl2RegDef.svh

The validator runs as Stage 4 and exits non-zero if either header is bad.

### 3. (Optional) run individually

```bash
python3 $SKILL_DIR/scripts/gen_c_header.py \
    --input      /tmp/dwc_ddr_umctl2_regs.json \
    --output     /tmp/out/DWC_ddr_umctl2RegDef.h \
    --base-addr  0xC0001000 \
    --ip-prefix  DWC_ddr_umctl2

python3 $SKILL_DIR/scripts/gen_sv_header.py \
    --input      /tmp/dwc_ddr_umctl2_regs.json \
    --output     /tmp/out/DWC_ddr_umctl2RegDef.svh \
    --base-addr  0xC0001000 \
    --ip-prefix  DWC_ddr_umctl2

python3 $SKILL_DIR/scripts/validate_headers.py \
    --c-header  /tmp/out/DWC_ddr_umctl2RegDef.h \
    --sv-header /tmp/out/DWC_ddr_umctl2RegDef.svh
```

### 4. Install the headers

Drop the two files into the project layout:

```
dv/simulation/verif_env/<area>/header_files/c_header/<ip>RegDef.h
dv/simulation/verif_env/<area>/header_files/sv_header/<ip>RegDef.svh
```

These will be picked up by light_gui's `env_gen.config` builds that include
`header_files/c_header` and `header_files/sv_header` on the search path.

## Worked example

See `examples/DWC_ddr_umctl2/`.  The example uses a tiny JSON describing
only two registers (`MSTR` and `STAT`) so the pipeline can be exercised
quickly without a 9 MB databook text.

## End-to-end with real DDR databook

```bash
# 1. Extract the databook PDF to plain text (e.g. with pdftotext or a
#    proprietary tool).  Save as /tmp/dwc_ddr_umctl2_databook.txt.

# 2. Parse
python3 $SKILL_DIR/scripts/parse_summary.py \
    --input  /tmp/dwc_ddr_umctl2_databook.txt \
    --output /tmp/dwc_ddr_umctl2_regs.json

# 3. Generate + validate
$SKILL_DIR/scripts/run_all.sh \
    --json      /tmp/dwc_ddr_umctl2_regs.json \
    --base-addr 0xC0001000 \
    --out-dir   /home/IC_verify/work_for_openclaw/ip_docs \
    --ip-prefix DWC_ddr_umctl2 \
    --source-label "DWC_ddr_umctl2 Databook 3.91a"

# 4. Inspect outputs
ls -la /home/IC_verify/work_for_openclaw/ip_docs/DWC_ddr_umctl2RegDef.*
```

The validator prints a summary like:

    === C Header: ... ===
      typedefs:        207
      struct==32:      207
      errors:          0

If your count matches the number of registers in the databook, you're done.
