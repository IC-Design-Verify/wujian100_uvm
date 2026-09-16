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
├── env_gen.config       # Master config for UVM env generation (clocks, agents, DUT connects)
└── script/              # Perl build scripts (make_hex, make_hex_for_vip)

dv/simulation/firmware_ksim/  # Firmware library for C test compilation

doc_summary/           # Peripheral register documentation (Markdown, per-IP: DMA, GPIO, PWM, RTC, TIM, USI, WDT)
                       # Primary spec input for the spec-to-testplan skill and reference models

wujian100_open/          # DUT RTL submodule (RISC-V SoC)
```

## Three Simulation Modes

| Mode | Make Target | Description |
|------|-------------|-------------|
| CPU-driven | `make all` | CPU runs compiled C test cases |
| VIP-driven | `make all_vip` | UVM VIP drives the SoC directly |
| VIP+DPI | `make all_vip_dpi` | UVM with DPI calls to compiled C tests |

## Environment Setup

```bash
# Clone with the DUT submodule
git clone --recurse-submodules https://github.com/IC-Design-Verify/wujian100_uvm.git

# CPU-driven tests need the RISC-V gcc toolchain (riscv64-elf-x86_64-20210512);
# default path is /common/riscv-toolchain — otherwise edit the tool path in dv/dv.cshrc

# Source environment from the dv/ directory (csh; dv.bashrc also provided)
cd dv && source dv.cshrc   # or: source dv.bashrc
cd simulation/verif_env/soc
```

## Common Commands

```bash
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

## Multi-Agent Collaboration (CCB)

This project uses CCB for visible multi-agent collaboration.

### Collaboration

- You are one agent in a CCB-managed project team.
- Use CCB `ask` for project-level collaboration with configured agents.
- Delegate with the goal, scope/files, assumptions, expected output, and verification needs.
- Reply concisely with findings, changes, verification, blockers, and risks when relevant.

### Role Assignment

抽象角色映射到具体的 CCB 代理。协作时引用角色，实际调用时按下表解析为 `/ask <agent>`。

| Role | Agent | Provider | Description |
|------|-------|----------|-------------|
| `coding` | `worker1` / `worker2` / `worker3` / `worker4` | `codex` | 代码编写 — RTL/UVM 验证环境、测试用例、脚本等所有编码任务的实现与修改（多 worker 可并行承接独立编码任务；`coding` 代理自 2026-09-16 起暂停接受任务） |
| `doc_review` | `doc_review` | `codex` | 审查 — 文档审查与代码审查（质量门：验证计划、测试点、代码 diff 的评审） |
| `doc-write` | `doc-write` | `codex` | 文档编写 — 验证计划、验证报告、Spec 分析文档、注释说明等文档产出 |

调用方式：`/ask worker1 "..."`（或 worker2/worker3/worker4）/ `/ask doc_review "..."` / `/ask doc-write "..."`（或 shell `command ask <agent>`）。

### 分工规则

- **编码任务**（RTL、UVM 组件、sequence/test、脚本）一律委派 `worker1`~`worker4`，其他角色不代写代码；相互独立的编码任务可并行分给不同 worker。`coding` 代理暂不接受任务。
- **审查任务**（代码 diff 评审、文档评审）一律委派 `doc_review`；代码合入前必须通过审查。
- **文档任务**（验证计划、报告、说明文档）一律委派 `doc-write`。
- 委派时给出：目标、涉及文件/范围、假设、期望输出、验证要求。
- 某代理不可用时，在任务描述中注明「降级接管」后再委派替代代理，便于追溯。
- 注意：代理报"无权限/无法访问文件"时先查证工具调用是否真实执行——2026-09-16 曾发生 doc-write tool-call 失败并误诊为权限问题，重启+重新投影配置后恢复。API 分配（2026-09-16 更新）：doc-write=MiniMax-M3 @ api.minimaxi.com，doc_review=glm-5.3 @ api.mlxz.cc（当日切换，见 .ccb/incidents/doc_review-reasoning-fragment-replies-20260916.md），worker1~4=glm-5.3 @ api.mlxz.cc；各代理 pane 相互独立。provider/API 变更后应对该 agent 执行 `ccb clear <agent>` 再验证，旧会话不会自动继承新配置。
