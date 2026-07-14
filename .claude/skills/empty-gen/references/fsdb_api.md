# FSDB NPI API Notes for empty-gen

This reference documents the Synopsys Verdi NPI FSDB API used by the empty-gen skill scripts. The same API is also used by `fsdb-analysis` and many other waveform-debugging tools.

## Environment

```bash
export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
export NPIL1_PATH=$VERDI_HOME/share/NPI/lib/linux64
# Library is auto-loaded by viaSetupL1Apps; set NPIL1_PATH only if running without it
```

## Running the Scripts

```bash
$VERDI_HOME/bin/novas -play <script>.tcl -batch
# -batch: non-interactive (exit after script)
# -play <file>: run the .tcl file
# Stdout/stderr show UVM log + script output; everything goes to one stream
```

**Useful env vars (set before invoking `novas`)**:
- `FSDB_PATH` — absolute path to the FSDB
- `DUT_SCOPE` — full hierarchical path of the DUT instance
- `DUT_PATTERN` — substring to match against module definition name
- `OUT_FILE` — output file path

## Time Units

**FSDB timestamps are in picoseconds**, not nanoseconds.

```tcl
# 1 ns = 1000 ps
# 1 us = 1,000,000 ps
# 1 ms = 1,000,000,000 ps

npi_fsdb_goto_time -vct $vct -time 200000   # 200 ns
npi_fsdb_goto_time -vct $vct -time 10000000 # 10 us
```

## NPI FSDB API Cheat Sheet

| API | Purpose | Example |
|---|---|---|
| `npi_fsdb_open` | Open FSDB | `npi_fsdb_open -name "novas.fsdb"` |
| `npi_fsdb_close` | Close FSDB | `npi_fsdb_close -file $hdl` |
| `npi_fsdb_min_time` | Get min time (ps) | `npi_fsdb_min_time -file $hdl` |
| `npi_fsdb_max_time` | Get max time (ps) | `npi_fsdb_max_time -file $hdl` |
| `npi_fsdb_iter_top_scope` | Iterate top scopes | `npi_fsdb_iter_top_scope -file $hdl` |
| `npi_fsdb_iter_child_scope` | Iterate child scopes | `npi_fsdb_iter_child_scope -scope $scope` |
| `npi_fsdb_iter_scope_next` | Get next scope in iter | `npi_fsdb_iter_scope_next -iter $iter` |
| `npi_fsdb_iter_scope_stop` | Stop iteration (free handle) | `npi_fsdb_iter_scope_stop -iter $iter` |
| `npi_fsdb_scope_by_name` | Find scope by hierarchical name | `npi_fsdb_scope_by_name -file $hdl -name "tb.dut" -scope ""` |
| `npi_fsdb_scope_property_str` | Get scope property (name/defName) | `npi_fsdb_scope_property_str -scope $s -type npiFsdbScopeFullName` |
| `npi_fsdb_sig_by_name` | Find signal under scope | `npi_fsdb_sig_by_name -file $hdl -name "data_o" -scope $s` |
| `npi_fsdb_create_vct` | Create value-change-trace for signal | `npi_fsdb_create_vct -sig $sig` |
| `npi_fsdb_goto_time` | Position VCT at/before time | `npi_fsdb_goto_time -vct $vct -time $ps` |
| `npi_fsdb_vct_value` | Read VCT value at current pos | `npi_fsdb_vct_value -vct $vct -format npiFsdbHexStrVal` |
| `npi_fsdb_release_vct` | Free VCT handle | `npi_fsdb_release_vct -vct $vct` |

## Value Format Options

```tcl
-format npiFsdbBinStrVal   ;# "00110000..."
-format npiFsdbHexStrVal   ;# "30000000" (no 0x prefix)
-format npiFsdbDecStrVal   ;# "805306368"
-format npiFsdbBoolVal     ;# 0 or 1
```

**Hex format gotcha**: `npiFsdbHexStrVal` returns raw hex without `0x` prefix (e.g., `"a"` not `"0xa"`). Use `scan $hex_str "%x" int_var` for hex-to-int conversion rather than `expr {0x$val}` which fails on certain strings.

## Common Gotchas

| Problem | Cause | Fix |
|---|---|---|
| `npi_fsdb_top_scope` not found | Function name is wrong | Use `npi_fsdb_iter_top_scope` |
| `-type cannot be recognized` | Using `npiFsdbScopeFullNameType` (with `Type` suffix) | Use `npiFsdbScopeFullName` |
| `-iter has no value` | Iter handle was reset or never created | Check return of `npi_fsdb_iter_*_scope` before using |
| Empty signal handle | `npi_fsdb_sig_by_name` is exact-match only | Verify signal name with `fsdbSigQ` first |
| VCT returns `X` for all values | Time is before signal was driven | Sample after reset deassertion + settling time |
| VCT time drifts between signals | Each VCT has its own cursor | Call `npi_fsdb_goto_time` on each VCT for every sample |

## Verifying Signal Existence

Before sampling, verify the signal exists in the FSDB:

```bash
# Using fsdb-analysis skill
FSDBQ_FILE=/path/to/novas.fsdb \
FSDBQ_SCOPE=<dut_hierarchical_path> \
FSDBQ_SIG="<signal_pattern>*" \
OUTPUT_LOG=/tmp/signals.log \
$VERDI_HOME/bin/novas -play $PROJ_HOME/.claude/skills/fsdb-analysis/scripts/FsdbInvestigation/FsdbSigQ/fsdbSigQ_batch.tcl -batch
```

## Sample Time Selection Strategy

For "default value" extraction, sample at multiple times and pick the first non-X:

| Phase | Time | Why |
|---|---|---|
| Just after reset deassertion | ~100-200 ns | Some signals still X; useful as baseline |
| Settled reset state | ~1-10 us | Most signals stable; **best for defaults** |
| Mid-simulation | ~50 us | Some signals modified by code |
| End of simulation | max_time | Final values; not useful for defaults |

The default in `sample_outputs.tcl` uses 200ns / 1us / 10us / 50us / end-of-sim. The Python generator picks the first non-X value (leftmost column) as the default.

## Reset-Default Cross-Check

After sampling, cross-check non-zero defaults against the DUT's specification or architecture reference manual:

- **CPU cores**: CSR reset values (e.g., RISC-V `mcause`, `mstatus`, `mtvec`) are documented in the ISA manual
- **Bus IPs**: most signals should be 0 (bus idle) at reset
- **Active-low resets** (`*_n` suffix, `*_l` suffix): expect `1'b1` at reset
- **Clock-divider outputs**: expect a default divider ratio or 0 depending on the spec

If a sampled value disagrees with the spec, prefer the spec value (override the first column in the values file before running `gen_empty.py`).
