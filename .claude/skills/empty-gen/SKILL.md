---
name: empty-gen
description: "Generate a minimal blackbox/empty placeholder module (.empty.v) for a DUT by sampling its output port default values from a ksim FSDB waveform. Use when a DUT needs to be replaced by a stub (e.g., a CPU, accelerator, or DSP replaced by a non-functional placeholder) for SoC-level testing. The skill is DUT-agnostic: it works with any SystemVerilog/Verilog module whose FSDB waveform is available."
metadata:
  author: IC_verify
  organization: openclaw
  contact: 78490223@qq.com
---

# empty-gen

Generate a minimal SystemVerilog blackbox placeholder module (`.empty.v`) for any DUT instance. The generated module has only port declarations and assigns each output to its sampled default value, with no internal logic, no internal wires, and no behavioral code.

## When to Use

- Replace a complex DUT (CPU, DSP, accelerator, third-party IP) with a stub for SoC-level testing
- Build a blackbox library that satisfies the DUT's port list while driving only constant default values
- Reduce compile time / simulation runtime by avoiding full DUT elaboration
- Provide a stable signal environment for downstream SoC verification

## When NOT to Use

- DUT has runtime data-dependent outputs (the empty stub will be incorrect)
- Need bit-accurate functional behavior of the DUT
- DUT is small enough that the empty version saves no compile time

## Workflow (5 stages)

### Stage 1: Run ksim with FSDB dump

```bash
cd $PROJ_HOME/dv && source dv.bashrc
ksim -t <testbench>::<testlist>.py -n <testname> --w=fsdb
# --w=fsdb triggers full recompile (cannot use -so)
```

FSDB output: `${KSIM_SIM_DIR}/<project>/test/<build>.<testname>/novas.fsdb`

### Stage 2: Locate the DUT instance scope in FSDB

```bash
export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
export NPIL1_PATH=$VERDI_HOME/share/NPI/lib/linux64
export DUT_PATTERN=<module_substring>     # e.g. the module's def name
export FSDB_PATH=<path_to_fsdb>
$VERDI_HOME/bin/novas -play $SKILL_DIR/scripts/find_dut_scope.tcl -batch
```

The script recursively traverses the FSDB scope hierarchy and prints all instances whose definition name matches `DUT_PATTERN`. Use the printed hierarchical path (drop the leading `.`) as `DUT_SCOPE` in the next stage.

### Stage 3: Sample output port default values

Provide a port list via one of two methods:

**Option A — explicit port list file (preferred when ports are known):**

```bash
# ports.txt — one signal name per line; # for comments
clk_out
data_out[31:0]
intr
```

```bash
export DUT_SCOPE=<hierarchical_path_from_stage_2>
export OUT_PORTS_FILE=$PROJ_HOME/design/<dut>/ports.txt
$VERDI_HOME/bin/novas -play $SKILL_DIR/scripts/sample_outputs.tcl -batch
```

**Option B — auto-discover all signals under the DUT scope:**

```bash
export DUT_SCOPE=<hierarchical_path_from_stage_2>
export AUTO_DISCOVER=1
$VERDI_HOME/bin/novas -play $SKILL_DIR/scripts/sample_outputs.tcl -batch
```

The script samples each signal at 5 timestamps (200ns, 1us, 10us, 50us, end-of-sim, configurable via `SAMPLE_TIMES_PS`) and writes a whitespace-separated values file:

```
Port    200ns   1000ns  10000ns 50000ns 140075ns
sig1    X       X       0       0       6000fff8
...
```

**Why sample at multiple times**: At reset deassertion (~100ns), many output ports are still X. The DUT initializes them over the first few microseconds. The "default" value is the stable value reached after the DUT has set its reset state but before user code modifies it. Pick the smallest timestamp where the signal is no longer X — typically 1–10 µs for most IPs.

### Stage 4: Generate the empty file

Use the Python generator to render a SystemVerilog module from the sampled values.

**Option A — generate from an existing module file (recommended):**

```bash
python3 $SKILL_DIR/scripts/gen_empty.py \
    --module <DUT_MODULE_NAME> \
    --source $PROJ_HOME/design/<dut>/<dut>.v \
    --values /tmp/<dut_leaf>_output_values.txt \
    --output $PROJ_HOME/design/blackbox/<dut>.empty.v
```

The `--source` flag reuses the port list and port declarations from the existing module file, so the generated stub is guaranteed to match the original module's port order and widths.

**Option B — generate from a hand-written port list:**

```bash
# ports_list.txt — one port name per line, in module header order
clk_out,
data_out,
intr,

python3 $SKILL_DIR/scripts/gen_empty.py \
    --module <DUT_MODULE_NAME> \
    --ports $PROJ_HOME/design/<dut>/ports_list.txt \
    --values /tmp/<dut_leaf>_output_values.txt \
    --output $PROJ_HOME/design/blackbox/<dut>.empty.v
```

The generated file contains:
- Module header + port list (verbatim from sampling or source)
- Input and output port declarations (widths preserved)
- `assign <output> = <default>;` for each output
- No internal wires, no `&Regs`/`&Wires` tool comments, no behavioral logic

### Stage 5: Verify (optional)

For each output port, manually inspect the generated default. Cross-check against the DUT spec or RISC-V reset-state documentation (e.g., for CPU cores). For signals whose sampled value was X at all timestamps, the default is 0 (override manually if a non-zero reset value is required).

## Scripts

| Script | Purpose |
|---|---|
| `scripts/find_dut_scope.tcl` | NPI Tcl: recursively search FSDB for DUT instance scope |
| `scripts/sample_outputs.tcl` | NPI Tcl: sample signal values at multiple timestamps |
| `scripts/gen_empty.py` | Python: render `.empty.v` from sampled values and a port source |

## Detailed References

- `references/fsdb_api.md` — NPI FSDB API cheat sheet, time-unit gotchas, common gotchas
- `references/usage.md` — generic end-to-end workflow walkthrough
- `references/troubleshooting.md` — common errors and fixes

## Quick Start (one-liner)

```bash
SKILL=$PROJ_HOME/.claude/skills/empty-gen
export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
export FSDB_PATH=<path_to_fsdb>
export DUT_PATTERN=<module_substring>
export OUT_PORTS_FILE=<path_to_port_list>     # or AUTO_DISCOVER=1

# 1. Locate DUT
DUT_SCOPE=$($VERDI_HOME/bin/novas -play $SKILL/scripts/find_dut_scope.tcl -batch 2>/dev/null \
            | grep -m1 "^FOUND:" | awk '{print $2}' | sed 's/^\.//')
export DUT_SCOPE

# 2. Sample output ports
$VERDI_HOME/bin/novas -play $SKILL/scripts/sample_outputs.tcl -batch

# 3. Render empty module
python3 $SKILL/scripts/gen_empty.py \
    --module <DUT_MODULE_NAME> \
    --source <path_to_dut_rtl> \
    --values /tmp/<dut_leaf>_output_values.txt \
    --output $PROJ_HOME/design/blackbox/<dut>.empty.v
```

## Integration with ksim Blackbox Flow

ksim's `vcs_*.build` template always processes `<KSIM_TB_ROOT>/build/blackbox.file`. To use a generated empty file, add the path to that file:

```
$KSIM_PWA_ROOT/design/blackbox/<dut>.empty.v
```

The empty file is compiled into `lib_dummy` (separate from `lib_rtl`); the `soc_top_cfg` selects which lib's module to use. See `references/usage.md` for the full integration example.