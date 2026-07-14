# Troubleshooting & Build Guide

## Tcl Script Common Pitfalls

| Symptom | Cause | Fix |
|---------|-------|-----|
| `[N]` treated as command | In Tcl `"..."`, `[...]` is command substitution | Wrap strings containing `[...]` with `{...}` |
| `expr` with `\|` errors | `\|` is Tcl bitwise OR operator | Use `expr {$a \| $b}` |
| `expr $i-1` mis-parsed | Subtraction precedence issue | Use `expr {$i - 1}` |
| Non-interactive `novas` prompt | `$VERDI_HOME` not set | `export VERDI_HOME=/path/to/verdi` |

## File Naming Conventions

| Type | Convention | Example |
|------|-----------|---------|
| Tcl procedure | `snake_case` | `get_const_assign_main` |
| Tcl filename | `camelCase` | `getConstAssign.tcl` |
| RC section name | `PascalCase`, space escaped with `@#` | `Get@#Constant@#Assign` |
| Perl batch wrapper | Same as Tcl name, `.pl` suffix | `getConstAssign.pl` |
| Rule config | `*_rule.rc` | `getConstAssign_rule.rc` |
| Resource directories | `*_files/` | icons + HTML docs |

## Building C++ Plugins

No top-level build system — each plugin builds independently:

```bash
# LowPower C++ plugins (need $VERDI_HOME and $PLATFORM)
cd LowPower/SupplySet && make clean && make
cd LowPower/IsoStrategy && make clean && make
cd LowPower/PowerSwitch && make clean && make

# FSDB C++ plugins
cd FsdbInvestigation/LogToTransFsdb && make
cd FsdbInvestigation/SerialSig2Parallel/C_SHARE && make

# Coverage toggle test table generator (needs $VCS_HOME)
cd Coverage/UCAPI/GenTglTestTable && make
```

Tcl apps and Perl batch scripts require no compilation — run directly.
