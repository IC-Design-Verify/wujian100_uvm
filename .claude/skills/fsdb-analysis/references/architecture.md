# Architecture — Three Execution Modes

## Tcl Apps (VIA Framework)

Synopsys Verdi VIA (Verdi Interoperable Apps) — Tcl-based GUI apps that run inside Verdi's VIA framework.

- Subdirectories: `DesignComprehension/`, `DesignValidation/`, `DesignManipulation/`, `FsdbInvestigation/`
- `.tcl` files contain `*_main` procedures (registered in `via.rc` as entry points)
- Use NPI Tcl API (`npi_*` commands) to traverse netlists
- Output to log files, format compatible with Verdi Smart Log viewer
- Paired `*_rule.rc` defines output column format and hyperlink patterns
- HTML help (`.htm`) + PNG icons

## Batch Mode (Perl)

Scripts in `Bin/` (e.g., `getConstAssign.pl`) wrap Tcl apps for CLI use.

```bash
# Usage pattern
./Bin/<app_name>.pl -f <design.f> -o <output.log> [options]

# Examples (design.f based)
./Bin/getConstAssign.pl -f run.f -modules "TOP" -o getConstAssign.log
./Bin/detectDanglingPort.pl -f run.f -o dangling.log

# DBDIR based
./Bin/getConstAssign.pl -dbdir simv.daidir -modules "TOP" -o getConstAssign.log

# Tool-specific help
./Bin/<app_name>.pl -help
```

`.pl` files must be in the same directory as their corresponding `.tcl`.

Some `Bin/` tools are **native compiled C/C++** (no Tcl dependency).

## C++ NPI Plugins

Located in `LowPower/`, `FsdbInvestigation/`, `Coverage/`. Compiled to `.so`, loaded by Verdi NPI framework.

- Link against `libnpiL1.so` or `libNPI.so` (`$VERDI_HOME/share/NPI/`)
- Each has its own Makefile — no top-level build system
- No compilation needed for Tcl apps and Perl batch scripts

## RC Configuration Files

- **`via.rc`**: main registry — maps app names → Tcl files/procedures/params/icons/help
  - RC syntax: `[Group.App]` sections + key-value pairs
  - Section names escape spaces with `@#`: `Get@#Constant@#Assign`
- **`PredefinedRules/*.rc`**: hyperlink detection rules + Smart Log output column definitions
- **`PredefinedParRules/*.rc`**: log partitioning rules (regex for severity/timestamp/code/scope)
- **`SmartLog/mappingRuleFile`**: routes log file patterns to rule files
- **`*_rule.rc`**: output column format + hyperlink rules for specific apps
