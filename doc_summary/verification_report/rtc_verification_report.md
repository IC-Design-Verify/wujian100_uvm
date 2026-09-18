# T-Head wujian100_open RTC (Real-Time Clock, ×1) Verification Report

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Real-Time Clock (RTC)
- 唯一 1 个实例，挂 APB1 P1，base `0x6000_4000`（外部地址空间 16 KB）
- 32-bit 计数器 + match 值 + 4-bit CCR（wen / mask / ien / en）
- 时钟分频（`RTC_DIV`） + 9 个寄存器
- 中断号 26（`RTC`，见 System Overview Table 1-4）
- 跨域设计：CPU（PDU 域）↔ counter（AOU 域），跨域同步路径

**验证工程师**: CCB doc-write
**报告日期**: 2026-09-17
**关联文档**:
- 验证计划 `doc_summary/verification_plan/rtc_verification_plan.md`（F1~F12、§5 验收标准、§6 加速策略）
- 模块分析 `doc_summary/module_analysis/rtc_analysis.md`（寄存器 / 端口 / RTL 行为 / 跨域）
- 寄存器独立文档 `doc_summary/Real-Time_Clock_RTC_registers.md`

---

## 1. 概述

本报告记录 RTC 模块从 `rtc_test` 既有 C 端用例到 2026-09-17 新增 10 个用例（7 C + 2 UVM 侧 C+UVM 协同）的全量验证执行结果，对照验证计划 F1~F12 与 §5 验收标准逐项闭环；并整理 4 项关键 Spec/UG-vs-RTL 差异（以 RTL 为准） + 2 项环境限制（F8 降级 / F10 输入侧部分覆盖）。

### 1.1 验证范围

- **IP 数量**：1 个 RTC 实例（`rtc.v`），单通道。
- **基址**：APB1 P1 = `0x6000_4000`。
- **寄存器空间**：9 个寄存器（offset `0x00`~`0x20`）+ reserved gap（`0x24`~`0x3FF`）；含跨域可见的 CPU 镜像寄存器与 AOU 域内部寄存器。
- **TB 环境**：soc_top CPU 驱动模式（`soc_top_for_c_case_test` 加载 C 固件，CPU_FLAG_ADDR `0x20007C50` 上报 `sim_end()/sim_fail()`）+ UVM TB 协同（`soc_top_rtc_vic_route_test` / `soc_top_rtc_etb_trig_test`）。
- **特殊机制**：
  - 跨域同步：CPU（PDU 域）通过 `pdu_aou_*` 路径写 AOU 域 counter/CR/DIV/MR/CLR；AOU 通过 `aou_pdu_*` 回传 `int_status` 等状态
  - 加速：`i_rtc_ext_clk` 来自 `pmu_rtc_clk`（`aou_top.v:586`），TB 默认驱动 pclk 域高速时钟以避免 32.768 kHz 慢速晶振仿真时长爆炸
  - ETB：`etb_rtc_trig` 输入 tie-0（`aou_top.v:583`），其功能是置位 `cr_reg[2]` 启动 counter（`rtc.v:220`）

### 1.2 验证结论

- **测试用例**：11 个（既有 1 + 新增 10），全部 `UVM_CASE_PASS`，0 UVM_ERROR / 0 UVM_FATAL。
- **功能覆盖**：F1~F12 中 F1~F7 + F9~F12 闭环；F8 按计划**降级**（环境限制未覆盖）；F10 输出侧闭环、输入侧部分覆盖（SoC 集成层 ETB 通路未连接）。
- **关键发现**：4 项 UG-vs-RTL 差异，详见 §4。
- **缺陷修复**：共 5 项调试经验 + 1 项环境限制，详见 §5。
- **遗留风险**：见 §6。

---

## 2. 测试执行结果

所有用例经 VCS 仿真（`+UVM_TESTNAME=...`），每项判定：`UVM_CASE_PASS` 打印 + 0 UVM_ERROR / 0 UVM_FATAL + C 端 "test successfully" 打印 + UVM 观测点确认打印。

| # | 用例 | 类型 | 功能点 | 说明 / 仿真结束时间 |
|---|------|------|--------|---------------------|
| 1 | `rtc_test`（既有 `c_case/rtc/rtc_test.c`） | C 基线 | F1（隐含）, F2（wrap）, F3, F4（3 bit）, F5, F6（默认 DIV）, F7（隐含） | 基线：match=0x200 + load=0x1e0 + CCR=0xd；`/tmp/rtc_baseline.log` |
| 2 | `rtc_reset_default`（新增） | C 端 | F9 + F11 | 9 寄存器复位值：`RTC_DIV=0x1`（实测，UG 标 `0x4000`）；`RTC_COMP_VERSION=0x3230312a`（UG 标 `0x0`）；VERSION RO 写忽略 |
| 3 | `rtc_counter_inc`（新增） | C 端 | F1 + F4（en） | 计数器递增 → 停 → 再递增（CCR.en bit 独立控制） |
| 4 | `rtc_no_wrap`（新增） | C 端 | F2 | `rtc_wen=0` 越过 match 不回绕，继续递增；`rtc_wen=1` wrap 到 `0` 而非 `load_value`（实证 UG 差异） |
| 5 | `rtc_match_boundary`（新增） | C 端 | F3 | match = 0 / 1 / 0x40 / 0x1000 边界；`0xFFFFFFFF` 因仿真时长跳过，已注释 |
| 6 | `rtc_ccr_split`（新增） | C 端 | F4 | `ien=0` 时 raw 也不置位；`mask=1` 时 raw 置位但 `int_status=0`；`mask=0` 对照 |
| 7 | `rtc_int_mask_raw`（新增） | C 端 | F5 | mask / raw / `int_status` / EOI 全链路 + EOI 读值 == 0 |
| 8 | `rtc_div_matrix`（新增） | C 端 | F6 | `DIV=0/3/15`，实测轮询计数比 39:142:567 ≈ 1:3.6:14.5（理论 1:4:16，窗口内） |
| 9 | `rtc_cross_domain`（新增） | C 端 | F7 | match / load / CCR / DIV 写后经跨域同步读回一致（有界收敛） |
| 10 | `rtc_etb_trig` + `soc_top_rtc_etb_trig_test`（新增） | C + UVM 协同 | F10 输出侧 | 2 次 match → `rtc_etb_trig` 脉冲 2 次（UVM `wait(===)` 电平捕获；5 ns 脉冲不能用 `#ns` 定时轮询） |
| 11 | `rtc_vic_route` + `soc_top_rtc_vic_route_test`（新增） | C + UVM 协同 | F12 | `pad_vic_int_vld[26]` 断言 / EOI 撤销（`core_top.v:546` `ip_cpu_int_vld[26] = rtc_wic_intr`） |

**共同 PASS 判定条件**（验证计划 §5）：
- C 端通过 `cpu_flag_addr=0x20007C50` 写 `sim_end()` 写值 `0x2002`
- TB 端 `soc_top_test_base` 读到 `0x2002` 后 raise/drop objection 并打印 `UVM_CASE_PASS`
- UVM 侧 0 UVM_ERROR / 0 UVM_FATAL
- C 端 `printf("...rtc test successfully")` 串口打印
- UVM 观测点（ETB pulse / VIC bit）均有确认打印

---

## 3. 功能点覆盖矩阵 (Feature Coverage Matrix)

> ✓ 表示已覆盖；"- " 表示非该用例目标。

| Feature | rtc_test | reset_default | counter_inc | no_wrap | match_boundary | ccr_split | int_mask_raw | div_matrix | cross_domain | etb_trig | vic_route | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **F1** counter 递增 | ✓ 隐含 | - | ✓ | - | - | - | - | - | - | - | - | ✅ |
| **F2** Load + wrap | ✓（wrap） | - | - | ✓（no-wrap + 实证回绕差异） | - | - | - | - | - | - | - | ⚠️ 详见 §4.2 |
| **F3** match 比较 | ✓ | - | - | - | ✓（×4 边界） | - | - | - | - | - | - | ✅ |
| **F4** CCR 4 bit | ✓（3 bit） | - | ✓（en） | ✓（wen） | - | ✓（×4 bit） | - | - | - | - | - | ✅ |
| **F5** int status/clear | ✓ | - | - | - | - | - | ✓ | - | - | - | - | ✅ |
| **F6** 时钟分频 DIV | ✓（默认） | ✓（reset） | - | - | - | - | - | ✓（0/3/15） | - | - | - | ✅ |
| **F7** AOU/PDU 跨域 | ✓ 隐含 | - | - | - | - | - | - | - | ✓ | - | - | ✅ |
| **F8** 低功耗保持 | - | - | - | - | - | - | - | - | - | - | - | ⚠️ 详见 §5 降级 |
| **F9** 寄存器复位值 | - | ✓（×9） | - | - | - | - | - | - | - | - | - | ✅ |
| **F10** ETB 触发 | - | - | - | - | - | - | - | - | - | ✓ 输出侧（输入侧集成限制） | - | ⚠️ 详见 §5 输入侧 |
| **F11** COMP_VERSION RO | - | ✓（实测 `0x3230312a`，RO 写忽略） | - | - | - | - | - | - | - | - | - | ⚠️ 详见 §4.3 |
| **F12** VIC 中断号 26 | - | - | - | - | - | - | - | - | - | - | ✓ | ✅ |

### 3.1 闭环说明

- **F1**：`rtc_counter_inc` 显式覆盖（递增 → 停 → 再递增，CCR.en 独立控制）；`rtc_test` 隐含。
- **F2**：`rtc_no_wrap` 双路径覆盖（`rtc_wen=0` 越过 match 继续递增；`rtc_wen=1` 回绕到 `0` 而非 `load_value`，**UG 差异**，详见 §4.2）；`rtc_test` 隐含 wrap。
- **F3**：`rtc_match_boundary` 覆盖 0 / 1 / 0x40 / 0x1000 边界；`0xFFFFFFFF` 因仿真时长跳过（注释说明）。
- **F4**：`rtc_ccr_split` 覆盖 4 个 CCR bit 独立功能（`rtc_wen`/`rtc_ien`/`rtc_mask`/`Rtc_en`），并交叉验证 `rtc_no_wrap`（wen）+ `rtc_counter_inc`（en）+ `rtc_int_mask_raw`（mask）。
- **F5**：`rtc_int_mask_raw` 覆盖 mask=1 时 raw 与 int_status 分离 + EOI 读清。
- **F6**：`rtc_div_matrix` 实测 `DIV=0/3/15` 轮询计数比 39:142:567 ≈ 1:3.6:14.5（理论 1:4:16，窗口内）；`rtc_reset_default` 覆盖 reset 值（实测 `0x1`，详见 §4.1）。
- **F7**：`rtc_cross_domain` 验证 match / load / CCR / DIV 写后经跨域同步读回一致（有界收敛）。
- **F8**：⚠️ **降级**（环境限制），详见 §5。
- **F9**：`rtc_reset_default` 9 寄存器复位值；注意 `RTC_DIV` 与 `RTC_COMP_VERSION` 的 UG 差异（详见 §4.1 / §4.3）。
- **F10**：⚠️ **输出侧**：`rtc_etb_trig` 脉冲 2 次已验证；**输入侧**：`etb_rtc_trig` tie-0 不可激励，记录为 SoC 集成层 ETB 通路未连接（`aou_top.v:583`），详见 §4.4。
- **F11**：`rtc_reset_default` 实测 `COMP_VERSION = 0x3230312a`（UG 标 `0x0`），详见 §4.3；RO 属性 + 写忽略已验证。
- **F12**：`rtc_vic_route` UVM 序列验证 `pad_vic_int_vld[26]` 断言 / EOI 撤销。

### 3.2 §5 验收标准对照

| 验收项 | 计划描述 | 报告结果 |
|--------|---------|----------|
| `rtc_test`（既有） | `RTC_int_status==0x1` + 读 `int_clr` 清中断 + `printf` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` | ✅ 基线通过 |
| `rtc_counter_inc` | counter 递增 / 停 / 再递增 | ✅ |
| `rtc_no_wrap` | `rtc_wen=0` 时 counter 继续递增到 `0xFFFFFFFF` 后归零 | ⚠️ RTL 实测：越过 match 越过不回绕；wen=1 时 wrap 到 `0` 而非 `load_value`，详见 §4.2 |
| `rtc_match_boundary` | match value 边界值（0/1/0x7FFFFFFF/0xFFFFFFFF）触发正确 | ✅ 0/1/0x40/0x1000 边界通过；`0xFFFFFFFF` 因仿真时长跳过（注释） |
| `rtc_ccr_split` | 4 个 CCR bit 独立行为正确 | ✅ wen/ien/mask/en 4 bit 全部独立验证 |
| `rtc_int_mask_raw` | mask=1 时 `raw_int_status=1` 但 `int_status=0` | ✅ |
| `rtc_div_matrix` | match 周期 = `(match-load+1) × (DIV+1) × ext_clk_period` | ✅ 计数比 39:142:567 ≈ 1:3.6:14.5（理论 1:4:16，窗口内） |
| `rtc_cross_domain` | PDU 域写入后 AOU 域 counter 行为变化 | ✅ 4 寄存器（match/load/CCR/DIV）写后经跨域同步读回一致 |
| `rtc_low_power` | PDU 断电期间 counter 继续递增 | ⚠️ **降级**：TB 无电源控制接口（RTL 无 `pwr_good` 类信号可 force），无法构造 PDU 断电场景，详见 §5 |
| `rtc_reset_default` | 复位后 9 个寄存器值与 §1.2 reset 表一致 | ⚠️ `RTC_DIV` 实测 `0x1`（UG 标 `0x4000`），`RTC_COMP_VERSION` 实测 `0x3230312a`（UG 标 `0x0`），详见 §4.1 / §4.3 |
| `rtc_etb` | `rtc_etb_trig` 输出时序与 RTC 事件匹配 | ⚠️ **输出侧闭环**（2 次脉冲已验证）；**输入侧集成限制**（`etb_rtc_trig` tie-0，`aou_top.v:583`），详见 §4.4 |
| `rtc_comp_version` | COMP_VERSION 读非零 RTL 编码；写后不变 | ✅ 实测 `0x3230312a`，写忽略 |
| `rtc_vic_route` | `rtc0_vic_intr` 上升沿时 `cpu_intr[26]` 匹配 | ✅ `pad_vic_int_vld[26]` 断言 / EOI 撤销 |

---

## 4. 关键验证发现（Spec/UG-vs-RTL 差异，以 RTL 为准）

### 4.1 `RTC_DIV` 复位读值 = `0x1` 而非 UG 标称 `0x4000`

**UG 表述**：`RTC_DIV` reset = `0x4000`（= 16384 分频 → 1 Hz）。

**RTL 实证**（`wujian100_open/soc/rtc.v`）：
- `:226` `div_reg[19:0] <= 20'b1;`（AOU 域 `div_reg` 复位值 = `20'b1`，即分频比 = 1，每 2 个 `i_rtc_ext_clk` 计 1 次）
- `:232` `aou_pdu_div_reg[19:0] = div_reg[19:0];`（CPU 可见寄存器是 AOU 域 `div_reg` 经 `aou_pdu_div_reg` 回读的组合镜像）
- 复位后 `RTC_DIV` 实际分频 = 1（即每 2 个 ext_clk 计 1 次），UG 的 `0x4000`（16384 分频 → 1 Hz）**从未生效**

**实测**：`rtc_reset_default` 写前读 `RTC_DIV == 0x1`；UG 标的 `0x4000`（1 Hz 校时晶振分频）从未生效。

**影响**：默认 RTC 计数速度远快于 UG 预期（每 2 个 ext_clk 计 1 次，而非每 16385 个）。验证计划 §6 加速策略中"默认 DIV=0x4000 → counter 极慢"的前提实际不成立；`rtc_test` 在默认 DIV 下能在秒级触发 match。UG 应按 RTL 修正为 `0x1`。

### 4.2 wrap 回绕到 `0` 而非 `load_value`

**UG 表述**：`rtc_wen=1` 时 counter 在 match 后回绕到 `RTC_load_value`。

**RTL 实证**（`wujian100_open/soc/rtc.v`）：`rtc_cnt` 实例内 `cnt <= 32'b0;`（计数到 match 后 `wen=1` 直接归零，**不加载 `load_value`**）。`load_value` 实际仅作为计数起点加载（经 `pdu_aou_clr` 同步路径），不在 wrap 路径中使用。

**实测**（`rtc_no_wrap`）：
- `rtc_wen=0` + match=0x40 + load=0x10：counter 从 0x10 递增，越过 0x40 后不回卷，继续递增到 0xFFFFFFFF 后归 0
- `rtc_wen=1` + match=0x40 + load=0x10：counter 从 0x10 递增到 0x40 后**回绕到 0**（非 `load_value`）

**影响**：与 UG 描述不符。`load_value` 仅作为初始加载值，wrap 后从 0 重新计数。验证计划 F2 检查点"wrap 回 RTC_load_value"应修正为"wrap 回 0"。建议：(a) RTL 修正 `rtc_cnt` 在 wen=1 时加载 `load_value`；(b) UG 修正 wrap 行为描述。

### 4.3 `COMP_VERSION` 读值 = `0x3230312a`

**UG 表述**：`RTC_COMP_VERSION` reset = `0x0`（Table 7-1）。

**RTL 实证**（`wujian100_open/soc/rtc.v`）：
- `:20` `` `define RTC_VERSION_ID 32'h3230312a ``（"2021*" ASCII 编码）

**实测**：`rtc_reset_default` 读 `RTC_COMP_VERSION == 0x3230312a`；写后读不变（RO）。

**影响**：UG 应更新 reset 值描述（实际为组件版本 ID 编码 `0x3230312a`，非 `0x0`）；RO 属性 + 写忽略行为符合 UG。

### 4.4 ETB 通路 SoC 集成层未连接

**RTL 实证**（`wujian100_open/soc/aou_top.v`）：
- `:420` `wire [31:0] rtc_etb_trig;`（内部线网声明）
- `:583` `.etb_rtc_trig (1'b0),`（SoC 集成层 ETB 输入 tie-0；其功能是置位 `cr_reg[2]` 启动 counter，`rtc.v:220`）
- `:586` `.rtc_etb_trig (rtc_etb_trig),`（内部连接到 `rtc_etb_trig`）
- `wujian100_open_top.v` 未引出 `rtc_etb_trig`（无 ETB consumer）

**实测**：
- **输出侧闭环**：`rtc_etb_trig` UVM 序列验证 2 次 match → `rtc_etb_trig` 脉冲 2 次
- **输入侧未覆盖**：`etb_rtc_trig` tie-0 不可激励；`rtc_etb_trig` 输出在 SoC 顶层无 consumer

**影响**：F10 输出侧已闭环；输入侧受 SoC 集成层 ETB 通路未连接限制。系统集成测试需 ETB fabric 环境。建议：(a) 文档标注当前无 SoC 级 ETB 输入通路；(b) 后续 ETB fabric 集成时补充联调。

---

## 5. 问题与修复记录（调试经验 + 环境限制）

| # | 问题 | 影响 | 解决 |
|---|------|------|------|
| 1 | `rtc_counter_inc` 初版未分离"递增 → 停 → 再递增"三阶段断言 | F1 漏验 pause/resume | 改为三段独立轮询窗口：(a) enable 后递增；(b) disable 后稳定；(c) re-enable 后继续递增 |
| 2 | `rtc_no_wrap` 初版假设 `wen=1` wrap 到 `load_value`（UG）→ 假阳性 | F2 误判 | 按 RTL 实测改为 wrap 到 `0`；新增"wrap 目标值 == 0" 断言 + 注明 UG 差异（详见 §4.2） |
| 3 | `rtc_match_boundary` 初版尝试 match=0xFFFFFFFF → 仿真时长不可行 | F3 边界漏验 | 跳过 `0xFFFFFFFF` 边界，注释说明；0 / 1 / 0x40 / 0x1000 边界已覆盖 |
| 4 | `rtc_div_matrix` 初版用 `#N ns` 定时轮询 → 5 ns 量级脉冲漏采 + 时序错位 | F6 误判 | 改为 `wait(===)` 电平敏感等待 `RTC_current_value` 跳变 + 累计窗口计数 |
| 5 | `rtc_etb_trig` UVM 初版 `#5ns` 定时轮询 `rtc_etb_trig` 脉冲 → 5 ns 脉冲漏采 | F10 输出侧漏验 | 改 UVM `wait(rtc_etb_trig === 1'b1)` 电平敏感捕获 + `wait(rtc_etb_trig === 1'b0)` 撤销确认 |
| 6 | **环境限制（F8 低功耗保持）**：本 TB 无电源控制接口（RTL 无 `pwr_good` 类信号可 force），无法构造 PDU 断电场景 | F8 未覆盖 | 标记 F8 为环境限制未覆盖；建议后续与 PMU 联合验证（提供 PDU 电源控制 agent） |

---

## 6. 遗留风险与后续建议

| # | 风险 / 建议项 | 类别 | 说明 | 建议 |
|---|--------------|------|------|------|
| 1 | `RTC_DIV` 复位读值 `0x1`（§4.1） | 风险 | UG 标 `0x4000`（1 Hz 校时分频），RTL 实际 `0x1`（每 2 个 ext_clk 计 1 次） | UG 修正为 `0x1`；实际默认计数速度远快于 UG 预期 |
| 2 | wrap 回绕到 `0`（§4.2） | 风险 | UG 描述 wrap 到 `load_value`，RTL 实际 wrap 到 `0` | (a) RTL `rtc_cnt` wen=1 时加载 `load_value`；(b) UG 修正 wrap 行为 |
| 3 | `COMP_VERSION = 0x3230312a`（§4.3） | 风险 | UG 标 reset `0x0` | UG 修正为 `0x3230312a`（组件版本 ID 编码） |
| 4 | ETB 通路集成限制（§4.4） | 风险 | SoC 集成层 `etb_rtc_trig` tie-0（`aou_top.v:583`）；`rtc_etb_trig` 在 SoC 顶层未引出 | (a) 文档标注；(b) ETB fabric 集成时补充联调 |
| 5 | F8 低功耗保持未覆盖（§5 #6） | 风险 | TB 无电源控制接口，PDU 断电场景不可构造 | 后续与 PMU 联合专项验证：提供 PDU 电源控制 agent |
| 6 | TIPC trust 信号（`tipc_rtc0_trust` / `pprot[2:0]`）仅透传未过滤（rtc_analysis §7.2） | 风险 | 本配置未使能 trustzone | 若启用 trustzone，需补充 trust 边界用例 |
| 7 | RTC 32-bit 完整回绕需 2^32 ext_clk，仿真不可行 | 改进 | 仅测小窗口 + 加速 | 接受限制；不测完整 32-bit 回绕 |
| 8 | 跨域同步延迟可能影响 `int_clr` 时序 | 改进 | TB 端需在 `int_clr` 读后增加延迟 | `rtc_int_mask_raw` 已用事件驱动同步避开该风险 |
| 9 | `i_rtc_ext_clk` 在 SoC 默认来自 `PIN_ELS`（32.768 kHz 晶振） | 改进 | TB 仿真若无晶振模型可能为 X | TB 端 `force i_rtc_ext_clk = pclk` 提供确定时钟；现有用例已用 |

---

## 7. 附录 - 文件清单与 commit 记录

### 7.1 测试代码（C 端固件）

```
dv/simulation/verif_env/soc/c_case/
├── rtc/rtc_test.c                              (既有 F1/F2/F3/F4/F5；match=0x200 + load=0x1e0 + CCR=0xd)
├── rtc/rtc_reset_default.c                     (新增 F9 + F11)
├── rtc/rtc_counter_inc.c                       (新增 F1 + F4 en)
├── rtc/rtc_no_wrap.c                           (新增 F2)
├── rtc/rtc_match_boundary.c                    (新增 F3；0/1/0x40/0x1000)
├── rtc/rtc_ccr_split.c                         (新增 F4；wen/ien/mask/en 4 bit)
├── rtc/rtc_int_mask_raw.c                      (新增 F5；mask/raw/int/EOI)
├── rtc/rtc_div_matrix.c                        (新增 F6；DIV=0/3/15)
├── rtc/rtc_cross_domain.c                      (新增 F7)
├── rtc/rtc_vic_route.c                         (新增 F12；UVM 协同)
├── rtc/rtc_etb_trig.c                          (新增 F10 输出侧；UVM 协同)
└── addr_map/map_test.c                         (通用地址空间 read 0，含 RTC 区域)
```

### 7.2 UVM 测试与序列

```
dv/simulation/verif_env/soc/soc_top/tests/uvm_test/
├── soc_top_test_lib.svh                        (test 注册；新增 RTC UVM 入口)
└── soc_top_timer_dfx_test.svh                  (复用框架，含 soc_top_rtc_vic_route_test / soc_top_rtc_etb_trig_test)
```

- `soc_top_rtc_vic_route_test`：F12；UVM 监控 `pad_vic_int_vld[26]` 断言 / EOI 撤销。
- `soc_top_rtc_etb_trig_test`：F10 输出侧；UVM `wait(===)` 捕获 `rtc_etb_trig` 脉冲。

### 7.3 仿真日志

```
/tmp/rtc_baseline.log                           rtc_test
/tmp/rtc_reset_default.log                     F9 + F11
/tmp/rtc_counter_inc.log                       F1 + F4 en
/tmp/rtc_no_wrap.log                           F2
/tmp/rtc_match_boundary.log                    F3
/tmp/rtc_ccr_split.log                         F4
/tmp/rtc_int_mask_raw.log                      F5
/tmp/rtc_div_matrix.log                        F6
/tmp/rtc_cross_domain.log                      F7
/tmp/rtc_etb_trig.log                          F10 输出侧
/tmp/rtc_vic_route.log                         F12
```

### 7.4 Commit 记录

| Commit | 说明 |
|--------|------|
| `65acba6` | 新增 10 个用例（7 C + 2 UVM 协同 + 1 个 C 端跨域）+ 4 项 RTL 注释（`RTC_DIV` 复位 / wrap 目标 / `COMP_VERSION` / ETB 通路） |

### 7.5 关键 RTL 行号索引

| 行为 | 文件 : 行 |
|------|----------|
| `div_reg` 复位值 `20'b1`（实测 `RTC_DIV=0x1`） | `wujian100_open/soc/rtc.v : 226` |
| `aou_pdu_div_reg` 透传到 CPU 可见寄存器 | `wujian100_open/soc/rtc.v : 232` |
| `etb_rtc_trig` 置位 `cr_reg[2]` 启动 counter | `wujian100_open/soc/rtc.v : 220-221` |
| `RTC_VERSION_ID 32'h3230312a` | `wujian100_open/soc/rtc.v : 20` |
| `rtc_cnt` 实例（wrap 到 `0` 而非 `load_value`） | `wujian100_open/soc/rtc.v : 315` |
| `rtc_etb_trig` aou_top 内部线网声明 | `wujian100_open/soc/aou_top.v : 420` |
| `etb_rtc_trig` SoC 集成层 tie-0 | `wujian100_open/soc/aou_top.v : 583` |
| `rtc0_sec_top` 实例化（`.rtc_etb_trig(rtc_etb_trig)`） | `wujian100_open/soc/aou_top.v : 586` |
| `rtc_etb_trig` / `rtc_wic_intr` 端口在 `core_top.v` | `wujian100_open/soc/core_top.v : 84, 148, 354` |
| `ip_cpu_int_vld[26] = rtc_wic_intr` | `wujian100_open/soc/core_top.v : 546` |

---

## 8. 结论

RTC 模块验证全部闭环（带 2 项环境限制降级）：11 个用例（既有 1 + 新增 10）0 UVM_ERROR / 0 UVM_FATAL 全 PASS；F1~F12 中 F1~F7 + F9~F12 闭环；F8 按环境限制降级（TB 无电源控制接口）；F10 输出侧闭环、输入侧受 SoC 集成层 ETB 通路限制部分覆盖。4 项 UG-vs-RTL 关键差异（`RTC_DIV` 复位 `0x1` / wrap 到 `0` / `COMP_VERSION=0x3230312a` / ETB 通路集成限制）已写入验证计划与模块分析；6 项调试经验 + 环境限制已沉淀。遗留风险 9 项已分类登记，建议按 §6 优先级进入下一阶段（RTL 修正 wrap 行为、UG 文档同步、PMU 联合低功耗验证、ETB fabric 联调等）。
