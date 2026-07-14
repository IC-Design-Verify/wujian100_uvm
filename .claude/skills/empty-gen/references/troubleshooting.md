# empty-gen Troubleshooting

Common errors and fixes for the empty-gen workflow.

## ksim Stage

### `database busy, retry...` infinite loop

**Cause**: `KSIM_DB_DIR` not set or points to a non-existent directory.

**Fix**:
```bash
export KSIM_DB_DIR=$PROJ_HOME/dv
ls -d $KSIM_DB_DIR   # must exist
```

### `--w=fsdb` does not generate FSDB

**Cause**: Using `-so` (simulate only) instead of full recompile. The `+fsdb+all` flag is a compile-time option.

**Fix**: Run full compile+sim:
```bash
ksim -t <testbench>::<testlist>.py -n <testname> --w=fsdb
# NOT: ksim -t soc::soc_top.py -so --w=fsdb
```

### ksim command not found

**Cause**: PATH not set, or ksim scripts dir not in PATH.

**Fix**:
```bash
export PATH="$PROJ_HOME/.claude/skills/ksim/scripts:$PATH"
which ksim   # should print a path
```

## FSDB Stage

### `npi_fsdb_open` returns empty handle

**Cause**: FSDB path is wrong or FSDB is corrupt.

**Fix**:
```bash
ls -la $FSDB_PATH    # file must exist
file $FSDB_PATH      # should say "data" (FSDB is binary)
```

### `npi_fsdb_scope_by_name` returns empty

**Cause**: Scope path is wrong, or scope doesn't exist in FSDB.

**Fix**: Run `find_dut_scope.tcl` first to confirm the exact path:
```bash
DUT_PATTERN=<module_substring> FSDB_PATH=... \
  novas -play find_dut_scope.tcl -batch
# Use the output path exactly (drop leading dot)
```

### `npi_fsdb_sig_by_name` returns empty

**Cause**: Signal name doesn't match exactly. NPI sig_by_name is exact-match, not glob.

**Fix**: Use `fsdb-analysis` `fsdbSigQ` to list all signals under the scope:
```bash
FSDBQ_FILE=$FSDB_PATH \
FSDBQ_SCOPE=$DUT_SCOPE \
FSDBQ_SIG="*" \
OUTPUT_LOG=/tmp/sigs.log \
novas -play $PROJ_HOME/.claude/skills/fsdb-analysis/scripts/FsdbInvestigation/FsdbSigQ/fsdbSigQ_batch.tcl -batch
grep "<signal_pattern>" /tmp/sigs.log   # find exact signal name
```

### VCT values are all `X`

**Cause**: Sample time is too early — the signal hasn't been driven yet (DUT in reset, signal not initialized).

**Fix**: Sample at a later time. Use at least 1us, preferably 10us. Set `SAMPLE_TIMES_PS` before invoking the script:
```bash
export SAMPLE_TIMES_PS="200000 1000000 10000000 50000000"
#                              ^^^^^^^^ add 1us, 10us
```

### Time appears wrong (off by 1000x)

**Cause**: Confusing ps and ns.

**Fix**: FSDB times are in **picoseconds**. 1 ns = 1000 ps. The Python generator's header is in ns (for human readability) but Tcl always uses ps.

## gen_empty.py Stage

### `Cannot find module header in <file>`

**Cause**: `--source` file doesn't have a standard `module name(...);` header, or the header spans multiple lines incorrectly.

**Fix**: Check the source file has a clear `module <dut_name>(...);` declaration. Try `--ports` mode with a hand-written port list instead.

### Wrong bit-width on output assignment

**Cause**: The width parser doesn't understand complex width expressions (e.g., `[WIDTH-1:0]` parameterized widths).

**Fix**: Edit `gen_empty.py` `width_to_int()` to handle the case, or hardcode the width in `--ports` mode.

### Generated file has compile errors

**Cause**: The module header order doesn't match the parent module's expectation, or port directions are wrong.

**Fix**: Compare the generated file's `module name(...);` header line-by-line with the source. The order must match exactly. The Python script preserves the source's port order via `extract_module_ports`.

## Integration Stage

### New empty file ignored by build

**Cause**: `blackbox.file` still references the old path.

**Fix**: Edit `dv/simulation/verif_env/soc/build/blackbox.file` and update the path. Re-run `ksim -t ... -co` to recompile (or full run).

### Build picks up real RTL instead of empty

**Cause**: `soc_top_cfg` defaults to `design DEFAULT.tb_top`, which resolves the DUT to `lib_rtl` (the real RTL) instead of `lib_dummy` (the empty).

**Fix**: Check the build's selection logic. In the build file:
```
design rtlLib.<top>      ;# uses lib_rtl (real RTL)
design DEFAULT.tb_top    ;# uses work lib (whatever is built last)
```
The blackbox mechanism requires that `lib_dummy/<dut>` is compiled and the config picks it. Verify by checking `synopsys_sim.setup` and `soc_top_cfg.sv` in the build dir.

## Performance

### ksim with FSDB is much slower than without

**Cause**: FSDB dumping is a heavy I/O operation.

**Mitigation**:
- Use shorter tests
- Use selective FSDB dump (only the DUT and immediate signals)
- Use `--w=fsdb` only when needed; default runs are faster
- For long regressions, batch FSDB dumps across tests

### Empty file generation fails on huge DUTs (>1000 ports)

**Cause**: Some Tcl functions have arg-list length limits.

**Fix**: Split the port list across multiple `sample_outputs.tcl` runs, or use a Python-based FSDB reader instead (e.g., `verdi_pa_extensions` Python API).
