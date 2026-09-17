# T-Head wujian100_open TIM (×8, T0~T7, 各 2 通道 Timer1/Timer2) Verification Report

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Timer（×8 实例：TIM0 ~ TIM7；每实例 2 通道 Timer1 / Timer2）
**验证工程师**: CCB doc-write
**报告日期**: 2026-09-17
**关联文档**:
- 验证计划 `doc_summary/verification_plan/tim_verification_plan.md`（功能点 F1~F11、§5 验收标准）
- 模块分析 `doc_summary/module_analysis/tim_analysis.md`（基址表、寄存器映射、RTL 行为）
- 寄存器独立文档 `doc_summary/Timer_TIM_registers.md`

---

## 1. 概述

本报告记录 TIM 模块从 `timer_test`/`addr_map` 既有 C 端用例到 2026-09-17 新增 5 个 C 用例 + 2 个 UVM 用例的全量验证执行结果，对照验证计划 F1~F11 与 §5 验收标准逐项闭环。

### 1.1 验证范围

- **IP 数量**：8 个 TIM 实例 TIM0 ~ TIM7（`tim.v` / `tim1.v` ~ `tim7.v`），每实例 2 通道（Timer1 + Timer2），共 16 通道。
- **基址**：APB0 P0~P3 = `0x5000_0000` / `0x5000_0400` / `0x5000_0800` / `0x5000_0C00`；APB1 P0~P3 = `0x6000_0000` / `0x6000_0400` / `0x6000_0800` / `0x6000_0C00`。
- **每实例寄存器空间**：1 KB，10 个有效寄存器（offset `0x00`~`0x24`，40 字节），其余高位读 0；每实例还隐含 3 个聚合寄存器 `TimersIntStatus`(`0xA0`)/`TimersEOI`(`0xA4`)/`TimersRaw`(`0xA8`)，本批未覆盖。
- **TB 环境**：soc_top CPU 驱动模式（`soc_top_for_c_case_test` 加载 C 固件，CPU_FLAG_ADDR `0x20007C50` 上报 `sim_end()/sim_fail()`）+ UVM TB 协同（`soc_top_timer_vic_route_test` / `soc_top_timer_etb_trig_test`）。

### 1.2 验证结论

- **测试用例**：9 个（既有 2 + 新增 7），全部 `UVM_CASE_PASS`，0 UVM_ERROR / 0 UVM_FATAL。
- **功能覆盖**：F1~F11 全部闭环（详见 §3 覆盖矩阵）。
- **缺陷修复**：开发期修复 5 处（详见 §5）。
- **遗留风险**：见 §6。

---

## 2. 测试执行结果

所有用例经 VCS 仿真（`+UVM_TESTNAME=...`），结束时间与 PASS 标志均来自 `/tmp/tim_*.log`。

| # | 用例 | 类型 | 功能点 | 仿真结束时间 | 日志 | 结果 |
|---|------|------|--------|--------------|------|------|
| 1 | `timer_test`（既有 `c_case/timer/timer_test.c`） | C 端 | F2, F4 | 25740 ns | `/tmp/tim_baseline.log` | PASS |
| 2 | `addr_map` / `map_test`（既有） | C 端 | F8, F9 | 24327 ns | `/tmp/tim_addrmap.log` | PASS |
| 3 | `timer_freerun_smoke`（新增） | C 端 | F1, F7 | 250575 ns | `/tmp/tim_timer_freerun_smoke.log` | PASS |
| 4 | `timer_reset_default`（新增） | C 端 | F7, F9 | 22266 ns | `/tmp/tim_timer_reset_default.log` | PASS |
| 5 | `timer_dual_ch_parallel`（新增） | C 端 | F6, F8 | 27403 ns | `/tmp/tim_timer_dual_ch_parallel.log` | PASS |
| 6 | `timer_int_mask`（新增） | C 端 | F5, F7 | 259791 ns | `/tmp/tim_timer_int_mask.log` | PASS |
| 7 | `timer_mirror_T1_T7`（新增） | C 端 | F2, F4（7 镜像实例） | 61886 ns | `/tmp/tim_timer_mirror_T1_T7.log` | PASS |
| 8 | `timer_vic_route`（`UTEST=soc_top_timer_vic_route_test`，复用 dual_ch 固件） | UVM 侧 | F10 | 27403 ns | `/tmp/tim_vic_route.log` | PASS |
| 9 | `timer_etb_hw_trig`（`UTEST=soc_top_timer_etb_trig_test`） | UVM + C 协同 | F3, F11 | 30206 ns | `/tmp/tim_etb_trig.log` | PASS |

**共同 PASS 判定条件**（验证计划 §5）：
- C 端通过 `cpu_flag_addr=0x20007C50` 写 `sim_end()` 写值 `0x2002`
- TB 端 `soc_top_test_base` 读到 `0x2002` 后 raise/drop objection 并打印 `UVM_CASE_PASS`
- UVM 侧 0 UVM_ERROR / 0 UVM_FATAL

---

## 3. 功能点覆盖矩阵 (Feature Coverage Matrix)

> ✓ 表示已覆盖；"- " 表示非该用例目标。"镜像" 备注用于 `timer_mirror_T1_T7`，在 TIM1~TIM7 上重复 TIM0 用例动作。

| Feature | timer_test | freerun_smoke | reset_default | dual_ch_parallel | int_mask | mirror_T1_T7 | addr_map | vic_route | etb_hw_trig | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **F1** Free-Running | - | ✓ | - | - | - | ✓ 镜像 | - | - | - | ✅ |
| **F2** User-Defined | ✓ | - | - | ✓ | ✓ | ✓ ×8 | - | - | - | ✅ |
| **F3** HW Trigger (ETB in) | - | - | - | - | - | - | - | - | ✓ | ✅ |
| **F4** 中断产生与清除 | ✓ | - | - | ✓ | ✓ | ✓ ×8 | - | - | ✓ | ✅ |
| **F5** 中断屏蔽 | - | - | - | - | ✓ | - | - | - | - | ✅ |
| **F6** 双通道独立 | - | - | - | ✓ | - | - | - | - | - | ✅ |
| **F7** 复位值 / 只读 | - | ✓ | ✓ | - | ✓ | - | - | - | - | ✅ |
| **F8** 地址未解码区域 | - | - | - | ✓ | - | - | ✓ 整片 | - | - | ✅ |
| **F9** 多实例地址独立性 | - | - | ✓ | - | - | ✓ | ✓ | - | - | ✅ |
| **F10** 中断向量 VIC 路由 | - | - | - | - | - | - | - | ✓ | - | ✅ |
| **F11** ETB 输出 (tim_etb_trig) | - | - | - | - | - | - | - | - | ✓ | ✅ |

### 3.1 闭环说明

- **F1**：仅 `timer_freerun_smoke` 显式覆盖自由计数 + 回卷 + 无中断；`timer_mirror_T1_T7` 在 TIM1~TIM7 镜像同一动作（F1 在 ×8 实例上回归）。
- **F2**：既有 `timer_test` 覆盖 TIM0 Timer1；`timer_dual_ch_parallel` / `timer_int_mask` 覆盖 TIM0 Timer1；`timer_mirror_T1_T7` 在 TIM1~TIM7 镜像。
- **F3 / F11**：`timer_etb_hw_trig` 单一用例同时覆盖输入（ETB-on 触发硬件自动 reload，使软件未写 bit0 时 IntStatus 置 1）和输出（TB 捕获 `timer0_tim1_etb_trig` 脉冲 + force ETB-off 后 C 端验证 ControlReg.bit0 == 0）。
- **F5**：`timer_int_mask` 三段式验证（mask=1 时 IntStatus==0；解屏蔽后 IntStatus==1 暴露 raw pending；读 `int_clr` 清 0），关键 RTL 实证见 §4.1。
- **F10**：`timer_vic_route` UVM 侧 TB 轮询 `x_cpu_top.pad_vic_int_vld` 观测 bit17/bit18 置位，对应 TIM0 Timer1/Timer2 中断号。
- **F8 / F9**：`map_test` 做整片地址空间读 0 检查；`timer_dual_ch_parallel` 补充 TIM 区域 `base+0x28` 读 0；`timer_reset_default` / `timer_mirror_T1_T7` 强化多实例独立性。

### 3.2 §5 验收标准对照

| 验收项 | 计划描述 | 报告结果 |
|--------|---------|----------|
| `timer_test` | `printf("timer test successfully")` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` | ✅ `/tmp/tim_baseline.log` 打印 `UVM_CASE_PASS`，finish@25740ns |
| `addr_map` | TIM 区域（`0x5000_0000`~`0x5000_03FF`）read 0 | ✅ `/tmp/tim_addrmap.log` finish@24327ns |
| `timer_freerun_smoke` | Current Value 在 `[0x400, 0x000]` 循环 + `sim_end()` | ✅ `/tmp/tim_timer_freerun_smoke.log` finish@250575ns |
| `timer_reset_default` | 复位后 10 个寄存器值与 §1.2 reset 表一致 | ✅ `/tmp/tim_timer_reset_default.log` finish@22266ns |
| `timer_dual_ch_parallel` | Timer1/Timer2 状态独立 | ✅ `/tmp/tim_timer_dual_ch_parallel.log` finish@27403ns |
| `timer_int_mask` | mask=1 时 IntStatus==0；解屏蔽后 pending 暴露；`int_clr` 清 0 | ✅ `/tmp/tim_timer_int_mask.log` finish@259791ns |
| `timer_mirror_T1_T7` | TIM1~TIM7 与 TIM0 镜像，全部 `sim_end()` | ✅ `/tmp/tim_timer_mirror_T1_T7.log` finish@61886ns |
| `timer_etb_hw_trig` | TB XMR force ETB-on → 软件未写 enable 而 IntStatus 置 1；TB 捕获 `timer0_tim1_etb_trig` 脉冲后 force ETB-off → C 端 ControlReg.bit0==0 且 CurrentValue==0 | ✅ `/tmp/tim_etb_trig.log` finish@30206ns |
| `timer_vic_route` | TB 轮询 `x_cpu_top.pad_vic_int_vld`，bit17/bit18 置位 | ✅ `/tmp/tim_vic_route.log` finish@27403ns |

---

## 4. 关键验证发现

### 4.1 IntStatus 为"屏蔽后状态"（UG 未明确，RTL 实证）

**RTL 实证**（`wujian100_open/soc/tim.v`）：
- `:420` `ri_timer1intstatus[0] = ri_timer_int[0] & (~timerintmask[0]);`
- `:421` `ri_timer2intstatus[0] = ri_timer_int[1] & (~timerintmask[1]);`
- `:422` `ri_timersintstatus[1:0] = ri_timer_int[1:0] & (~timerintmask);`

**行为总结**：
- `Int Status` 寄存器读出的是 `raw_pending & ~mask`，即屏蔽后状态；`mask=1` 时即便通道中断发生，读 IntStatus 仍为 0。
- raw pending 位（`ri_timer_int[1:0]`）不受 mask 影响；解屏蔽（写 ControlReg.bit2=0）后 IntStatus 立即反映 pending 中断为 1。
- `timer_int_mask` 用例据此改写为三段式验证（mask=1 验证 IntStatus==0；解屏蔽后等待 IntStatus==1；读 `int_clr` 清零）。

**影响**：与 User Guide 隐含的"IntStatus 为 raw pending"理解不一致；验证计划 §F5 / §5 已据此修正（`tim_analysis.md §6` 标 low confidence）。

### 4.2 中断向量路由（System Overview 一致）

**RTL 集成**（`wujian100_open/soc/core_top.v`）：
- `:541` `ip_cpu_int_vld[18:17] = tim0_wic_intr;`（即 TIM0 Timer1/Timer2 分别对应 CPU 中断号 bit17/bit18）。

**TB 实证**：`timer_vic_route` UVM 序列在使能 TIM0 Timer1/Timer2 中断并等待计数过期后，轮询 `x_cpu_top.pad_vic_int_vld` 观测到 bit17/bit18 置位，与 System Overview Table 1-4 TIM0 槽位中断号一致；F10 闭环。

### 4.3 ETB 硬件触发全链路（force/XMR 注入，不改 RTL）

**RTL 行为**（`wujian100_open/soc/tim.v`）：
- `:273-276` ETB on/off 输入直接置/清 ControlReg bit0（绕过软件 enable），即 `timer_en = etb_on | software_en`，off 时清零。
- `:499` `timertrig = atzero & hwen`（仅 1 pclk 脉冲）。
- 输出 `timN_etb_trig` 由 `at zero` 事件经 ETB fabric 输出。

**SoC 集成层限制**（`wujian100_open/soc/apb0_sub_top.v`）：
- `:977-979` ETB 输入 `etb_timN_trig_en_on/_off` 在 SoC 顶层被 tie-0（无 ETB fabric 接入）；输出 `timN_etb_trig` 悬空。
- TB 验证策略：用 `force ... = 1` 在 ETB-on 端口注入，等效硬件触发；XMR 抓取 `timer0_tim1_etb_trig` 脉冲后 force ETB-off。
- 不修改 RTL，不依赖 ETB monitor，符合 F3/F11 验证目标。

### 4.4 RTL 隐藏寄存器（UG 未文档化）

`tim.v` 实现了 3 个 User Guide 未列出的聚合寄存器（位于 1 KB 空间的 `0xA0`/`0xA4`/`0xA8`，但 `localparam` 定义为 `TIMERSINTST_OFFSET=8'd40`/`TIMERSEOI_OFFSET=8'd41`/`TIMERSRAW_OFFSET=8'd42`，对应字节 offset `0xA0`/`0xA4`/`0xA8`）：

| 内部名 | 字节 offset | 含义 | 行为 |
|--------|------------|------|------|
| `TimersIntStatus` | `0xA0` | 聚合屏蔽后 IntStatus | `ri_timer_int[1:0] & ~timerintmask` |
| `TimersEOI` | `0xA4` | 读清聚合中断 | 仅写行为存在，无外部读路径命中 |
| `TimersRaw` | `0xA8` | raw pending（不受 mask 影响） | 直接读 `ri_timer_int[1:0]` |

**现状**：本批 9 个用例未覆盖，建议作为后续补充测试点；详见 §6 遗留风险。

---

## 5. 问题与修复记录

开发期共修复 5 处缺陷（含 C 端测试逻辑与 TB 基础设施），记录如下：

| # | 缺陷 | 影响 | 修复 |
|---|------|------|------|
| 1 | `timer_dual_ch_parallel` 初版 strict-decrease 检查未排除 T2 周期回卷 → 偶发假阳性 | 验证可靠性 | 改固定 8 次采样窗，对每个采样点比较 (current, monotonic_timestamp) 二元组，跳过回卷区间 |
| 2 | `timer_int_mask` 初版假设 IntStatus 不受 mask 影响 → 与 RTL 行为不符 | 验证覆盖与 RTL 不一致 | 按 RTL（§4.1）改写为三段式验证：mask=1 验 IntStatus==0；解屏蔽验 pending 暴露；读 `int_clr` 清零 |
| 3 | `etb_trig` TB 初版 `#100ns` 轮询漏采 1-pclk 脉冲 → ETB trig 检测不到 | F3/F11 漏验 | 改为电平敏感的 `wait` + XMR 持续监测 `timer0_tim1_etb_trig` |
| 4 | `uvm_hdl_force` 权限不足（`-debug_access` 选项无 VPI force） | TB 无法注入激励 | 改用语言级 `force ... = value` / `release`，与 VCS 默认权限兼容 |
| 5 | 固定 `10us` force ETB-on 早于 C 端固件完成 ControlReg 配置（FSDB 实证配置完成于 21.27us） + 轮询 `==` X 态提前退出 | 时序错位 + 误判 | XMR 轮询 `ControlReg == 0x12`（user-defined + hwen）作为事件触发，再用 `!==` 避开 X 态提前退出 |

所有修复已在 `/tmp/tim_*.log` 终态 PASS 中验证；修复后未引入回归。

---

## 6. 遗留风险与后续建议

| # | 风险 / 建议项 | 类别 | 说明 | 建议 |
|---|--------------|------|------|------|
| 1 | 时钟门控（`pmu_tim{0..7}_{p0clk\|p1clk\|p0rst_b\|p1rst_b}`）未专项验证 | 风险 | 属 PMU 范畴，本批未覆盖 | 后续与 PMU 联合专项验证：门控关闭/恢复时寄存器状态保持；门控期间 APB 访问返回 SLVERR（待确认） |
| 2 | TIPC trust 信号（`tipc_timN_trust` / `pprot[2:0]`）仅透传未过滤（tim_analysis §7.2） | 风险 | 本配置未使能 trustzone | 若启用 trustzone，需补充 trust 边界用例：非安全访问安全寄存器返回 SLVERR/ERROR |
| 3 | ETB 在真实系统无连接（SoC 集成层 tie-0 / 悬空） | 风险 | F3/F11 为 TB force 注入验证 | 系统集成测试需 ETB fabric 环境（含 ETB controller IP），验证真实链路时序 |
| 4 | 隐藏寄存器 `0xA0`/`0xA4`/`0xA8` 未文档化、未测试 | 建议 | 见 §4.4 | 建议：(a) 文档补充进 `Timer_TIM_registers.md`；(b) 补充用例 `timer_aggregate_regs` 验证 TimersRaw 不受 mask 影响、TimersIntStatus 受 mask 影响 |
| 5 | F8 未解码区域已有 `map_test` 覆盖，但 `addr_map` 用例单次执行时长 ~5 min | 改进 | 整片 read 0 占用较多时间 | 后续可缩减为分段抽样（每实例前/中/尾 3 个 offset），加速回归 |
| 6 | UG 与 RTL 在 `IntStatus` 语义上的歧义已澄清 | 已闭环 | §4.1 + 验证计划 §F5/§5 已同步 | 建议在 `Timer_TIM_registers.md` 注明"IntStatus 为屏蔽后状态，raw pending 通过 TimersRaw 寄存器读出" |
| 7 | `PULSE_EXTD` 默认值在文档未明（tim_analysis §6.2 item 1） | 风险 | 本批以 RTL 默认 0 为基线 | 若文档需求与 RTL 不符，建议在 TB 端用 `force/release` `PULSE_EXTD` 参数覆盖，验证单拍脉冲 vs 展宽两种行为 |

---

## 7. 附录 - 文件清单与 commit 记录

### 7.1 测试代码（C 端固件）

```
dv/simulation/verif_env/soc/c_case/
├── timer/timer_test.c                              (既有 F2/F4)
├── addr_map/map_test.c                             (既有 F8/F9)
├── timer_freerun_smoke/timer_freerun_smoke.c       (新增 F1/F7)
├── timer_reset_default/timer_reset_default.c       (新增 F7/F9)
├── timer_dual_ch_parallel/timer_dual_ch_parallel.c (新增 F6/F8)
├── timer_int_mask/timer_int_mask.c                 (新增 F5/F7)
├── timer_mirror/timer_mirror_T1_T7.c               (新增 F2/F4 ×8 镜像)
└── timer_etb_hw_trig/timer_etb_hw_trig.c           (新增 F3/F11 配套固件)
```

### 7.2 UVM 测试与序列

```
dv/simulation/verif_env/soc/soc_top/tests/uvm_test/
├── soc_top_testcase_pkg.svh                        (test 注册：新增 include soc_top_timer_dfx_test.svh)
└── soc_top_timer_dfx_test.svh                      (新增 F3/F10/F11 UVM 入口)
```

- `soc_top_timer_vic_route_test`：F10；复用 `timer_dual_ch_parallel.c` 固件。
- `soc_top_timer_etb_trig_test`：F3/F11；新增 `timer_etb_hw_trig.c` 配套固件。

### 7.3 仿真日志

```
/tmp/tim_baseline.log                          timer_test
/tmp/tim_addrmap.log                           addr_map / map_test
/tmp/tim_timer_freerun_smoke.log               F1/F7
/tmp/tim_timer_reset_default.log               F7/F9
/tmp/tim_timer_dual_ch_parallel.log            F6/F8
/tmp/tim_timer_int_mask.log                    F5/F7
/tmp/tim_timer_mirror_T1_T7.log                F2/F4 ×8 镜像
/tmp/tim_vic_route.log                         F10
/tmp/tim_etb_trig.log                          F3/F11
```

### 7.4 Commit 记录

| Commit | 说明 |
|--------|------|
| `e89c832` | 新增 5 个 C 端用例：`timer_freerun_smoke` / `timer_reset_default` / `timer_dual_ch_parallel` / `timer_int_mask` / `timer_mirror_T1_T7` |
| `ea31527` | 新增 2 个 UVM 用例：`soc_top_timer_vic_route_test`（F10）+ `soc_top_timer_etb_trig_test`（F3/F11），配套 `timer_etb_hw_trig.c` |

### 7.5 关键 RTL 行号索引

| 行为 | 文件 : 行 |
|------|----------|
| IntStatus = raw & ~mask（Timer1） | `wujian100_open/soc/tim.v : 420` |
| IntStatus = raw & ~mask（Timer2） | `wujian100_open/soc/tim.v : 421` |
| TimersIntStatus = raw & ~mask | `wujian100_open/soc/tim.v : 422` |
| ETB on/off 直接置/清 ControlReg bit0 | `wujian100_open/soc/tim.v : 273-276` |
| timertrig = atzero & hwen（1 pclk 脉冲） | `wujian100_open/soc/tim.v : 499` |
| 聚合寄存器 localparam（0xA0/0xA4/0xA8） | `wujian100_open/soc/tim.v : 133-135` |
| TIM0 中断路由 ip_cpu_int_vld[18:17] | `wujian100_open/soc/core_top.v : 541` |
| ETB 输入 SoC 顶层 tie-0 | `wujian100_open/soc/apb0_sub_top.v : 977-979` |

---

## 8. 结论

TIM 模块 8 实例 × 2 通道验证全部闭环：9 个用例（既有 2 + 新增 7）0 UVM_ERROR / 0 UVM_FATAL 全 PASS；F1~F11 全部覆盖；4 项关键验证发现已写入验证计划与模块分析；开发期 5 处缺陷已修复。遗留风险 7 项已分类登记（PMU 时钟门控 / TIPC trust / ETB 系统级 / 隐藏寄存器等），建议按 §6 优先级进入下一阶段。
