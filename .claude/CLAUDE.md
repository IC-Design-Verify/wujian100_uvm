# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

wujian100_uvm is a UVM verification environment for the open-source wujian100_open SoC (RISC-V, T-Head XuanTie E902 core). It supports three simulation modes: CPU-driven, VIP-driven, and VIP+DPI.

## Directory Structure

```
dv/simulation/verif_env/soc/
├── soc_top/             # Top-level UVM testbench
│   ├── env/             # Environment (env_config, env, v_sequencer, coverage)
│   ├── sequences/       # Virtual sequences (intr_sequence, reg_sequence)
│   ├── tests/           # UVM tests (test_base, smoke_test, reg_test)
│   ├── tb_top/          # Testbench top + includes (itf_inst, itf_config, interface_assignment)
│   ├── filelist/        # VCS file lists (tb.f, tc.f, rtl.f, ahb.f)
│   ├── reg_model/       # Register model
│   └── headers/         # Additional headers
├── apb0/                # APB0 subsystem (PWM, USI-I2C/SPI/UART, WDT)
├── apb1/                # APB1 subsystem (GPIO, PMU, RTC)
├── ahb_hs/              # AHB high-speed subsystem (DMA)
├── ahb_ls/              # AHB low-speed subsystem
├── top_sim/             # Top-level simulation (address map, HAD)
├── gate_sim/            # Gate-level simulation
├── common/uvc/ahb/      # Custom AHB UVC agent
├── c_case/              # C test cases for CPU-driven simulation
└── script/              # Perl build scripts (make_hex, make_hex_for_vip)

dv/simulation/firmware_ksim/  # Firmware library for C test compilation

wujian100_open/          # DUT RTL submodule (RISC-V SoC)
```

## Three Simulation Modes

| Mode | Make Target | Description |
|------|-------------|-------------|
| CPU-driven | `make all` | CPU runs compiled C test cases |
| VIP-driven | `make all_vip` | UVM VIP drives the SoC directly |
| VIP+DPI | `make all_vip_dpi` | UVM with DPI calls to compiled C tests |

## Common Commands

```bash
# Source environment (run from dv/simulation/verif_env/soc/)
cd dv/simulation/verif_env/soc
source ../path/to/dv.bashrc  # or dv.cshrc for csh/tcsh

# --- CPU-driven simulation ---
make all                          # compile + run (default timer_test)
make all C_TEST=timer/timer_test.c  # specific C test
make comp && make run             # separate compile/run steps
make clean                        # clean artifacts

# --- VIP-driven simulation ---
make all_vip                      # compile + run smoke test

# --- VIP + DPI simulation ---
make all_vip_dpi                  # compile + run with DPI C calls
```

### C_TEST Selection Examples

| Peripheral | C_TEST value |
|------------|-------------|
| Timer | `timer/timer_test.c` |
| GPIO | `gpio/gpio_test.c` |
| DMA | `dma/dma_test.c` |
| PWM | `pwm/pwm_test.c` |
| USI I2C | `usi_i2c/usi_i2c_test.c` |
| USI SPI | `usi_spi/usi_spi_test.c` |
| USI UART | `usi_uart/usi_uart_test.c` |
| WDT | `wdt/wdt_test.c` |
| RTC | `rtc/rtc_test.c` |
| Address Map | `addr_map/map_test.c` |
| HAD | `had_soc/e902_had_test.c` |

### Use-Define Compile Flags (from Makefile)

| Compile Flag | Enables |
|-------------|---------|
| `USE_APB0` | APB0 subsystem verification |
| `USE_APB1` | APB1 subsystem verification |
| `USE_AHB_HS` | AHB high-speed verification |
| `USE_AHB_LS` | AHB low-speed verification |
| `USE_GATE_SIM` | Gate-level simulation |
| `USE_TOP_SIM` | Top-level simulation (address map, HAD) |
| `USE_AHB_VIP_TO_REPLACE` | AHB VIP replaces custom AHB UVC |

### VCS Compilation

```bash
# CPU-driven compile
vcs -debug_access -full64 -sverilog -kdb \
    -l comp.log -timescale=1ns/1ps \
    +define+${DEF} \
    -ntb_opts uvm-1.2 +define+UVM_PACKER_MAX_BYTES=1500000 \
    +define+DEMO_MAKEFILE +define+UVM_EVENT_CALLBACK_FIX \
    -f ./soc_top/filelist/tb.f -f ./soc_top/filelist/tc.f \
    -f ./soc_top/filelist/rtl.f \
    -top tb_top

# VIP-driven compile (adds USE_AHB_VIP_TO_REPLACE)
vcs ... +define+USE_AHB_VIP_TO_REPLACE ...
```

## UVM Test Package Architecture

The verification environment follows a modular subsystem-based architecture:

1. **tb_top.sv** (`soc_top/tb_top/tb_top.sv`) — Top module, uses `USE_*` defines to conditionally include subsystem TBs
2. **env_pkg** (`soc_top/env/soc_top_env_pkg.sv`) — Environment package with env_config, env, v_sequencer, coverage
3. **filelist** — VCS compile file lists (tb.f for TB/UVC, tc.f for subsystem includes, rtl.f for DUT RTL, filelist.f for soc RTL)
4. **Subsystem TBs** — Each subsystem (apb0, apb1, ahb_hs, ahb_ls, top_sim, gate_sim) has its own:
   - sequence (virtual sequences, scenario sequences)
   - tb_top (interface connections)
   - test (UVM tests)

Each subsystem follows a consistent file naming pattern:
- `soc_subsys_<name>_seq_pkg.svh` — Sequence package
- `soc_subsys_<name>_sequence.svh` — Base sequence
- `soc_subsys_<name>_vseq.svh` — Virtual sequence
- `subsys_<name>_testcase.svh` — Test case
- `subsys_<name>_testcase_pkg.svh` — Test case package

## Custom AHB UVC Agent

Located at `common/uvc/ahb/`:
- `ahb_if.sv` — AHB interface
- `ahb_pkg.sv` — UVC package (agent, driver, monitor, sequencer, sequences, config)
- `ahb_mst_rand_seq.svh` — Master random sequence
- `ahb_slv_resp_seq.svh` — Slave response sequence

## .claude Skills

The project includes these Claude Code skills for UVM verification workflows:

| Skill | Directory | Purpose |
|-------|-----------|---------|
| ksim | `.claude/skills/ksim/` | Run UVM simulation with ksim, manage jobs |
| fsdb-analysis | `.claude/skills/fsdb-analysis/` | FSDB waveform debug, Verdi batch apps |
| spec-to-testplan | `.claude/skills/spec-to-testplan/` | Spec analysis and testplan generation |
| testcase-build | `.claude/skills/testcase-build/` | UVM test case and vseq generation |
| reg-def-gen | `.claude/skills/reg-def-gen/` | Register model header generation |
| empty-gen | `.claude/skills/empty-gen/` | Placeholder module generation from FSDB |

Key rules files (`.claude/rules/`):
- `coverage_workflow.md` — Coverage collection and URG reporting
- `skill_usage_specifications.md` — 5-stage UVM verification flow with 7 root-cause rules
- `svt_ahb_integration_usage_guide.md` — SVT AHB VIP integration patterns
- `svt_ahb_interface_connection_specification.md` — AHB interface wiring spec
- `uvm_debug_experience.md` — Scoreboard patterns, driver protocol, RTL bug patterns
- `uvm_verification_common_issues.md` — Compile/sim troubleshooting and template fixes

## Key Verification Patterns

- **TB top VCS ordering**: class definitions (wrapped in `all_testcases.svh`) must come before module items (wire/assign). Multi-bit DUT ports must be explicitly declared as `wire[N-1:0]`.
- **Scoreboard**: Use queue+run_phase pattern with `clone()`-before-push to handle timing offsets in multi-operation tests.
- **APB VIP**: Both `apb_master_vif` and `apb_slave_vif` must be connected with clock/reset and signal assignments.
- **AHB VIP**: In AHB-Lite mode, disable VIP internal `*_bus` drivers via `assign xxx_vif.haddr_bus = '0` to avoid multi-driver conflicts.
- **Register model**: Add `_reg` suffix to avoid name conflicts (e.g., `CTRL_reg` not `CTRL`). Use `get_mirrored_value()` instead of `uvm_reg::read()` in functions.
- **Parameterized classes**: Use inline method definitions (parameter scope issue in VCS).
- **PH_TIMEOUT fix**: Add `event test_done` with `post_main_phase` trigger and `run_phase` objection in test_base.

## RTL Sources

The DUT is the `wujian100_open` submodule (XuanTie open-source SoC). RTL files are in `wujian100_open/soc/` and include:
- E902 CPU core, AHB matrix, SMU, clock gen, reset gen
- APB0 peripherals: PWM, USI×3, WDT, timers
- APB1 peripherals: GPIO, PMU, RTC
- AHB peripherals: DMA
- Memory: FPGA SRAM models
