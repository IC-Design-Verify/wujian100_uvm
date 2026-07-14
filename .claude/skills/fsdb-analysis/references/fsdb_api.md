# NPI FSDB API Quick Reference

## Custom Tcl Script via NPI FSDB API

When built-in tools are insufficient, write a Tcl script directly using NPI FSDB API:

```bash
export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
cd <sim_dir>   # contains novas.fsdb and simv.daidir
$VERDI_HOME/bin/novas -play /tmp/my_analysis.tcl -batch
```

**Script template**:

```tcl
# my_analysis.tcl
viaSetupL1Apps                         ;# Initialize VIA L1 environment

set fsdb_hdl [npi_fsdb_open -name "novas.fsdb"]
set scope [npi_fsdb_scope_by_name -file $fsdb_hdl -name "tb_top.dut" -scope ""]
set sig_hdl [npi_fsdb_sig_by_name -file $fsdb_hdl -name "clk" -scope $scope]

# Create VCT to traverse signal value changes
set vct [npi_fsdb_create_vct -sig $sig_hdl]
npi_fsdb_goto_time -vct $vct -time "0"
while { [npi_fsdb_goto_next -vct $vct] != 0 } {
    set t [npi_fsdb_vct_time -vct $vct]
    set v [npi_fsdb_vct_value -vct $vct -format npiFsdbBinStrVal]
    puts "time=$t val=$v"
}

npi_fsdb_release_vct -vct $vct
npi_fsdb_close -file $fsdb_hdl
debExit
```

## NPI FSDB API Cheat Sheet

| API | Purpose |
|-----|---------|
| `npi_fsdb_open -name <file>` | Open FSDB |
| `npi_fsdb_close -file <hdl>` | Close FSDB |
| `npi_fsdb_scope_by_name -file -name -scope` | Get scope handle |
| `npi_fsdb_sig_by_name -file -name -scope` | Get signal handle |
| `npi_fsdb_create_vct -sig <hdl>` | Create value change tracer (VCT) |
| `npi_fsdb_goto_time -vct -time <time>` | Jump to specified time |
| `npi_fsdb_goto_next -vct` | Jump to next change point; returns 0 when done |
| `npi_fsdb_vct_time -vct` | Get current time |
| `npi_fsdb_vct_value -vct -format npiFsdbBinStrVal` | Get current value (binary string) |
| `npi_fsdb_release_vct -vct` | Release VCT |

## Built-in Batch FSDB Tools

```bash
cd <sim_dir>   # contains novas.fsdb and simv.daidir

# Search FSDB signals
./Bin/fsdbSigQ.pl -dbdir simv.daidir -fsdb novas.fsdb \
    -sig "*in_en* *out_en*" -scope "tb_top.dut" -o result.log

# Export FSDB scope hierarchy
./Bin/getFsdbScopeHier.pl -dbdir simv.daidir -fsdb novas.fsdb -o hier.log

# Read register values at specific time
./Bin/getRegValues.pl -dbdir simv.daidir -fsdb novas.fsdb \
    -time 573000 -unit ns -o regs.log

# Clock frequency / duty cycle
./Bin/npiFsdbFreqDuty -fsdb novas.fsdb -clk "clk" -scope "tb_top"

# Minimum pulse detection
./Bin/npiFsdbMinPulse -fsdb novas.fsdb -sig "enable_pulse"

# Force statement report
./Bin/npiFsdbReportForce -dbdir simv.daidir -fsdb novas.fsdb

# MDA signal report
./Bin/npiFsdbReportMDA -dbdir simv.daidir -fsdb novas.fsdb

# FSDB merge
./Bin/wi_merge_fsdb.pl -fsdb "fsdb1.fsdb fsdb2.fsdb" -o merged.fsdb
```
