---
name: fsdb-analysis
description: "Analyze FSDB waveforms, debug simulation errors, explore netlist hierarchy, or run Verdi VIA apps in batch mode."
metadata:
  author: 康耀鹏
  organization: personal
  contact: 78490223@qq.com
---

# fsdb-analysis

Synopsys Verdi VIA (Verdi Interoperable Apps) for FSDB waveform analysis and netlist exploration. Tcl-based apps with Perl batch wrappers + C++ NPI plugins.

## Environment

```bash
export VERDI_HOME=/eda_tools/synopsys/verdi/W-2024.09-SP1
export PATH="$VERDI_HOME/bin:$PATH"
# Optional: export VCS_HOME=/path/to/vcs  # for Coverage UCAPI
```

## Running Batch Apps

```bash
cd $PROJ_HOME/.claude/skills/fsdb-analysis/scripts

# design.f based
./Bin/<app>.pl -f <design.f> -o <output.log> [options]

# DBDIR based
./Bin/<app>.pl -dbdir <simv.daidir> -o <output.log> [options]

# Get help
./Bin/<app>.pl -help
```

## Quick Tool Index

**Waveform/FSDB:** `fsdbSigQ.pl`, `getRegValues.pl`, `npiFsdbFreqDuty`, `wi_merge_fsdb.pl`, custom Tcl via `$VERDI_HOME/bin/novas -play <script.tcl> -batch`

**Netlist:** `getModHier.pl`, `getModIO.pl`, `getConstAssign.pl`, `findInst.pl`

**Design checks:** `detectDanglingPort.pl`, `detectPortZ.pl`, `detectConstPort.pl`

## Detailed References

- `references/architecture.md` — three modes (Tcl Apps / Perl Batch / C++ NPI), RC config system
- `references/tools.md` — all tools by category (DesignComprehension / Validation / Manipulation / FsdbInvestigation)
- `references/fsdb_api.md` — NPI FSDB API cheat sheet, built-in FSDB batch tools, custom Tcl script template
- `references/troubleshooting.md` — Tcl pitfalls, file naming conventions, C++ plugin build guide
