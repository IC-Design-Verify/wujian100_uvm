# empty-gen Usage Guide — Generic Workflow

This reference walks through the complete end-to-end workflow for generating a `.empty.v` blackbox placeholder for any DUT, using its ksim FSDB waveform as the source of stable reset-state default values.

## Use Case

The DUT under verification is a complex block (CPU, DSP, accelerator, third-party IP) whose full RTL is needed for some tests but slows down other tests. For SoC-level testing of the surrounding logic, we want a blackbox placeholder that:

- Satisfies the DUT port list (compile succeeds)
- Drives only constant reset-state values on the outputs (no functional behavior)
- Compiles in seconds instead of minutes

The generated `.empty.v` is a normal SystemVerilog module — the ksim build system compiles it into `lib_dummy` (separate from `lib_rtl`) and the testbench selects which to use via `soc_top_cfg`.

## End-to-End Example

This example uses placeholder names — replace `<DUT_MODULE_NAME>`, `<dut>`, etc. with your actual values.

### Step 1: Set up ksim environment

```bash
cd $PROJ_HOME/dv && source dv.bashrc
export PATH="$PROJ_HOME/.claude/skills/ksim/scripts:$PATH"
export PYTHONPATH="$PROJ_HOME/.claude/skills/ksim/scripts/mypylib/pylib:$PROJ_HOME/.claude/skills/ksim/scripts/mypylib/lib:$PROJ_HOME/.claude/skills/ksim/scripts:$PYTHONPATH"
export KSIM_DB_DIR=$PROJ_HOME/dv
```

### Step 2: Run ksim with FSDB dump

```bash
ksim -t <testbench>::<testlist>.py -n <testname> --w=fsdb
# --w=fsdb triggers full recompile (cannot use -so)
```

Wait for the testbench end marker (`WUJIAN TEST END` and `UVM_CASE_PASS`) in the output.

FSDB output: `${KSIM_SIM_DIR}/<project>/test/<build>.<testname>/novas.fsdb`

### Step 3: Locate the DUT instance scope

```bash
export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
export NPIL1_PATH=$VERDI_HOME/share/NPI/lib/linux64
export DUT_PATTERN=<DUT_MODULE_NAME>
export FSDB_PATH=<path_to_fsdb>
$VERDI_HOME/bin/novas -play $PROJ_HOME/.claude/skills/empty-gen/scripts/find_dut_scope.tcl -batch
```

Sample output:

```
Top scope: tb_top
Pattern:   <DUT_MODULE_NAME>
Searching:
---
FOUND: .tb_top.<parent_path>.x_<dut_leaf> (def=<DUT_MODULE_NAME>)
---
Done.
```

Use the printed path (drop the leading `.`) as `DUT_SCOPE`.

### Step 4: Sample output port values

**Option A — provide an explicit port list file (preferred when ports are known):**

```bash
# $PROJ_HOME/design/<dut>/ports.txt — one signal name per line
sig_a
sig_b
sig_c[31:0]
# comments start with '#'
```

```bash
export DUT_SCOPE=<hierarchical_path_from_step_3>
export OUT_PORTS_FILE=$PROJ_HOME/design/<dut>/ports.txt
$VERDI_HOME/bin/novas -play $PROJ_HOME/.claude/skills/empty-gen/scripts/sample_outputs.tcl -batch
```

**Option B — auto-discover all signals under the DUT scope:**

```bash
export DUT_SCOPE=<hierarchical_path_from_step_3>
export AUTO_DISCOVER=1
$VERDI_HOME/bin/novas -play $PROJ_HOME/.claude/skills/empty-gen/scripts/sample_outputs.tcl -batch
```

Output file (default `/tmp/<dut_leaf>_output_values.txt`, override with `OUT_FILE`):

```
# Output port values for <DUT_SCOPE>
# Format: port<TAB>t1<TAB>t2<TAB>t3<TAB>t4<TAB>t5 (X = unresolved)
Port    200ns   1us     10us    50us    140075ns
sig_a   XXXXXXXX  XXXXXXXX  00000000  00000000  6000fff8
sig_b   3         3         3         3         3
...
```

The first non-X column gives the reset default. For most ports this is the 10us column.

### Step 5: Generate the empty file

```bash
python3 $PROJ_HOME/.claude/skills/empty-gen/scripts/gen_empty.py \
    --module <DUT_MODULE_NAME> \
    --source $PROJ_HOME/design/<dut>/<dut>.v \
    --values /tmp/<dut_leaf>_output_values.txt \
    --output $PROJ_HOME/design/blackbox/<dut>.empty.v
```

The `--source` flag reuses the port list and declarations from the existing RTL module (no need to hand-write them). The output file is a complete SystemVerilog module with only `assign` statements for each output.

### Step 6: Verify the generated file

```bash
wc -l $PROJ_HOME/design/blackbox/<dut>.empty.v
# Should be ~10 lines per output port plus a small header

grep -c "^assign " $PROJ_HOME/design/blackbox/<dut>.empty.v
# Should equal the number of output ports in the source module

grep -c "^wire" $PROJ_HOME/design/blackbox/<dut>.empty.v
# Should be 0 (no internal wires)
```

## Integration with ksim Blackbox Flow

The empty file is used by the SoC build's blackbox mechanism. The build flow:

1. The build template always calls `process_file(blackbox.file)`
2. `blackbox.file` lists the empty file(s) — typically `$KSIM_PWA_ROOT/design/blackbox/<dut>.empty.v`
3. The empty file is compiled into `lib_dummy`; the real RTL into `lib_rtl`
4. `soc_top_cfg` selects which lib's module to use

To use the new generated file, either:

- **Replace** the existing empty: `mv <dut>.empty.v <dut>.empty.v.bak && mv <dut>.generated.v <dut>.empty.v`
- **Add** as alternate build: modify `blackbox.file` to include both, and gate with `+define+USE_EMPTY_<DUT>`

### Conditional blackbox pattern

`dv/simulation/verif_env/<top>/build/blackbox.file`:

```
<%include file='rtl_vlogan_opts.file'/>
% if 'EMPTY_<DUT>' in ENV:
$KSIM_PWA_ROOT/design/blackbox/<dut>.empty.v
% else:
$KSIM_PWA_ROOT/design/<dut>/<dut>.v
% endif
```

Then run with `ksim -t ... KSIM__DEF_EMPTY_<DUT>=1` to use the generated file.

## Adapting for Different DUTs

The workflow is identical regardless of DUT — only the input parameters change:

| Parameter | Source |
|---|---|
| `DUT_PATTERN` | The DUT's module definition name (or a substring) |
| `FSDB_PATH` | The ksim test's FSDB output |
| `DUT_SCOPE` | Output of `find_dut_scope.tcl` |
| `OUT_PORTS_FILE` | Hand-written port list (one signal per line) — see the DUT's module declaration in RTL |
| `--module` | The DUT's module name (must match the `module <name>(...)` declaration in RTL) |
| `--source` | Path to the DUT's RTL module file |

## Common Variations

### Multi-clock reset settling

If the DUT has outputs gated by multiple clocks, sample at a time after all clocks have toggled. Use `SAMPLE_TIMES_PS`:

```bash
export SAMPLE_TIMES_PS="200000 1000000 10000000 100000000 1000000000"
$VERDI_HOME/bin/novas -play $PROJ_HOME/.claude/skills/empty-gen/scripts/sample_outputs.tcl -batch
```

### Custom default overrides

If a sampled default is wrong (e.g., X for a signal that should be 1), edit the values file before running `gen_empty.py`:

```
# /tmp/<dut_leaf>_output_values.txt — manual override
sig_active_low   1
sig_status_reg   32'h0
```

Put the override as the first column — the Python generator picks the first non-X column as the default.

### Verifying against a second test

Generate the empty file using test A, then run ksim on test B with `--w=fsdb`. Compare the actual DUT outputs in test B against the empty file's defaults. They should match at the stable reset state (before user code executes).