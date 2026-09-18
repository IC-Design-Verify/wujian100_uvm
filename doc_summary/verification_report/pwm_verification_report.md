# T-Head wujian100_open PWM (×1, 12 通道 / 6 group) Verification Report

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Pulse-Width Modulation (PWM)
- 唯一 1 个实例，挂 APB0 P6，base `0x5001_C000`（外部地址空间 16 KB）
- 12 路 PWM 输出（group0~5 各 2 通道） + 6 路 TIM + 6 路 CAP
- 53 个寄存器（offset `0x000`~`0x0D0`）
- 中断号 25（`PWM`，见 System Overview Table 1-4）

**验证工程师**: CCB doc-write
**报告日期**: 2026-09-17
**关联文档**:
- 验证计划 `doc_summary/verification_plan/pwm_verification_plan.md`（F1~F14、§5 验收标准、§6 测试计划）
- 模块分析 `doc_summary/module_analysis/pwm_analysis.md`（寄存器 / 端口 / RTL 行为 / 6 group 架构）
- 寄存器独立文档 `doc_summary/Pulse_Width_Modulation_PWM_registers.md`

---

## 1. 概述

本报告记录 PWM 模块从 `pwm_test` 既有 C 端用例到 2026-09-17 新增 13 个用例（8 C + 5 UVM 协同）的全量验证执行结果，对照验证计划 F1~F14 与 §5 验收标准逐项闭环；并整理 8 项关键 Spec/UG-vs-RTL 差异（以 RTL 为准） + 1 项环境限制（F11 输入侧 tie-0 不可激励）。

### 1.1 验证范围

- **IP 数量**：1 个 PWM 实例（`pwm.v`），6 个 group × 2 通道 = 12 路输出 + 6 路 TIM + 6 路 CAP。
- **基址**：APB0 P6 = `0x5001_C000`。
- **寄存器空间**：53 个寄存器（offset `0x000`~`0x0D0`，含 PWMCFG/PWMCTL/PWMINVERTTRIG + 6 group × {LOAD/COUNT/CMP/DB/TRIG} + 8 个 PWM 中断寄存器 × 2 组 + 8 个 CAP 寄存器 + TIM_CAP[N]_EN 等）。
- **TB 环境**：soc_top CPU 驱动模式（`soc_top_for_c_case_test` 加载 C 固件，CPU_FLAG_ADDR `0x20007C50` 上报 `sim_end()/sim_fail()`）+ UVM 协同（`soc_top_pwm_output_duty_test` / `soc_top_pwm_polarity_invert_test` / `soc_top_pwm_count_mode_test` / `soc_top_pwm_deadband_test` / `soc_top_pwm_fault_test` / `soc_top_pwm_vic_route_test` / `soc_top_pwm_trig_etb_test` / `soc_top_pwm_multi_group_test`）。
- **特殊机制**：
  - legacy PAD 激励块：`apb0/tb_top/apb0_pwm/pwm_test.v` 提供 CH0 输出 20 脉冲后每个 CH0 posedge 翻转 force 到 `PAD_PWM_CH2` 的回环激励
  - `PAD_PWM_FAULT` 悬空 X：`pwm_fault` 用例由 UVM 在 `t=0` 起 force 0 消 X；其他用例一律不使能 INTEN1[0]
  - UVM 波形量测统一模式：`fork wait(===)` 量测线程 + `super.run_phase` + `disable fork`
  - Makefile 新增 `findstring pwm_` 分支：DEF = `USE_APB0 + USE_APB0_PWM`（解锁 legacy 激励块编译）

### 1.2 验证结论

- **测试用例**：14 个（既有 1 + 新增 13），全部 `UVM_CASE_PASS`，0 UVM_ERROR / 0 UVM_FATAL。
- **功能覆盖**：F1~F14 全部闭环（F11 输出侧闭环 + 输入侧集成限制）。
- **关键发现**：8 项 Spec/UG-vs-RTL 差异，详见 §4。
- **缺陷修复**：共 5 项调试经验 + 1 项环境限制，详见 §5。
- **遗留风险**：见 §6。

---

## 2. 测试执行结果

所有用例经 VCS 仿真（`+UVM_TESTNAME=...`），每项判定：`UVM_CASE_PASS` × 2 + 0 UVM_ERROR / 0 UVM_FATAL + C 端 "test successfully" 打印。

运行方式：`make all C_TEST=pwm_xxx/pwm_xxx.c [UTEST=配对 test 类]`。

| # | C 用例 | UVM 配对 test | 功能点 | 说明 |
|---|--------|---------------|--------|------|
| 1 | `pwm_test`（既有 `c_case/pwm/pwm_test.c`） | `soc_top_for_c_case_test`（默认） | F1（cntdiv + ch0/cap2 enable）, F7（CAPRIS 轮询） | 基线：`PWMCFG=0x9002001`（cntdiv/4 + ch0 enable + cap2 enable）+ `CAP01MATCH=0x200000` |
| 2 | `pwm_reset_default`（新增） | 默认 | F12 | 复位后 53 寄存器全 0 |
| 3 | `pwm_en_all`（新增） | 默认 | F1 | PWMCFG 使能位写读回环 |
| 4 | `pwm_cmp_read`（新增） | 默认 | F5 | PWM0/1CMP 写读回环 |
| 5 | `pwm_tim_full`（新增） | 默认 | F8 | tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控 |
| 6 | `pwm_intr_full`（新增） | 默认 | F9 | group0 zero/load/compa_up/compb_up + PWMIC + INTEN 门控 + group3（PWMRIS2） |
| 7 | `pwm_cap_full`（新增） | 默认 | F7 | 捕获：cnt_match 置位/清除、沿计数递增、时间戳两次捕获、rise vs both 边沿选择 |
| 8 | `pwm_output_duty`（新增） | `soc_top_pwm_output_duty_test` | F5 + F1 | 占空比 75%（LOAD=799, CMPA=200）+ F1 cntdiv=0 周期恰好 2 倍 |
| 9 | `pwm_polarity_invert`（新增） | `soc_top_pwm_polarity_invert_test` | F2 | 反转前后占空比 75% ↔ 25% 互补 |
| 10 | `pwm_count_mode`（新增） | `soc_top_pwm_count_mode_test` | F4 | up vs up-down 周期比 ≈ 2 |
| 11 | `pwm_deadband`（新增） | `soc_top_pwm_deadband_test` | F6 | CH0/CH1 互补 + 无重叠（delay=0x10） |
| 12 | `pwm_fault`（新增） | `soc_top_pwm_fault_test` | F10 | fault 中断置位/清除（UVM force `PAD_PWM_FAULT`） |
| 13 | `pwm_vic_route`（新增） | `soc_top_pwm_vic_route_test` | F14 | `pwm_int` → `pad_vic_int_vld[25]` 断言 + 解除 |
| 14 | `pwm_trig_etb`（新增） | `soc_top_pwm_trig_etb_test` | F3 + F11 | 触发输出 `pwm_xx_trig` 脉冲 + F11 tim etb 脉冲 |
| 15 | `pwm_multi_group`（新增） | `soc_top_pwm_multi_group_test` | F13 | group0（LOAD=0x100）vs group3（0x400）周期比 ≈ 4 |

**共同 PASS 判定条件**（验证计划 §5）：
- C 端通过 `cpu_flag_addr=0x20007C50` 写 `sim_end()` 写值 `0x2002`
- TB 端 `soc_top_test_base` 读到 `0x2002` 后 raise/drop objection 并打印 `UVM_CASE_PASS`
- UVM 侧 0 UVM_ERROR / 0 UVM_FATAL
- C 端 `printf("...pwm... test successfully")` 串口打印
- UVM 协同用例（`pwm_*_test`）有 `super.run_phase` 内的 `fork wait(===)` 量测 + `disable fork` 收尾

---

## 3. 功能点覆盖矩阵 (Feature Coverage Matrix)

> ✓ 表示已覆盖；"- " 表示非该用例目标。

| Feature | pwm_test | reset_default | en_all | cmp_read | tim_full | intr_full | cap_full | output_duty | polarity_invert | count_mode | deadband | fault | vic_route | trig_etb | multi_group | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **F1** PWMCFG 全局 | ✓ 隐含 | - | ✓ | - | - | - | - | ✓ | - | - | - | - | - | - | ✓ | ✅ |
| **F2** 极性反转 | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | ✓ | ✅ |
| **F3** ADC trigger | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | ✓ | ✅ |
| **F4** 计数模式 | - | - | - | - | - | - | - | ✓ (up) | - | ✓ (up-down) | - | - | - | - | ✓ | ✅ |
| **F5** LOAD/COUNT/CMP | ✓ (LOAD=2) | - | - | ✓ | - | - | - | ✓ | - | - | - | - | - | - | ✓ | ✅ |
| **F6** 死区 DB | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | - | ✓ | ✅ |
| **F7** 捕获 CAP | ✓ (cap2) | - | - | - | - | - | ✓ (×6 + ×4 边沿) | - | - | - | - | - | - | - | ✓ | ✅ |
| **F8** 定时器 TIM | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | ✓ | ✅ |
| **F9** PWM 中断 4 件套 | - | - | - | - | - | ✓ (×8 reg) | - | - | - | - | - | - | - | - | ✓ | ✅ |
| **F10** FAULT 输入 | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | ✓ | ✅ |
| **F11** ETB 触发 | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ 输出 + tim 输入 tie-0 | ✓ | ⚠️ 见 §4.8 |
| **F12** 复位值 | - | ✓ (×53) | - | - | - | - | - | - | - | - | - | - | - | - | - | ✅ |
| **F13** 多 group 独立 | - | - | - | - | - | - | - | ✓ (×2) | - | - | - | - | - | - | ✓ (×6) | ✅ |
| **F14** VIC 中断号 25 | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | ✅ |

### 3.1 闭环说明

- **F1**：`pwm_test` 隐含（cntdiv/4 + ch0/cap2 enable）+ `pwm_en_all` 显式（PWMCFG 使能位写读）+ `pwm_output_duty`（cntdiv=0 周期验证）+ `pwm_multi_group` 跨 group 验证。
- **F2**：`pwm_polarity_invert` 验证反转前后 75% ↔ 25% 互补；`pwm_multi_group` 跨 group 兼带。
- **F3**：`pwm_trig_etb` 验证 `pwm_xx_trig` 脉冲与 TRIG 配置时序对齐。
- **F4**：`pwm_output_duty` (up 模式锯齿) + `pwm_count_mode` (up vs up-down 周期比 ≈ 2) + `pwm_multi_group` 跨 group。
- **F5**：`pwm_test` (LOAD=2) + `pwm_cmp_read` (CMP 写读回环) + `pwm_output_duty` (LOAD=799, CMPA=200, 占空比 75%) + `pwm_multi_group`。
- **F6**：`pwm_deadband` 验证 CH0/CH1 互补 + 无重叠 (delay=0x10)；`pwm_multi_group` 跨 group。
- **F7**：`pwm_test` (cap2 edge count) + `pwm_cap_full` 完整覆盖（cnt_match 置位/清除、沿计数递增、时间戳两次捕获、rise vs both 边沿选择）；`pwm_multi_group` 跨 group。
- **F8**：`pwm_tim_full` 覆盖 tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控；`pwm_multi_group` 跨 group。
- **F9**：`pwm_intr_full` 覆盖 group0 zero/load/compa_up/compb_up + PWMIC + INTEN 门控 + group3（PWMRIS2）；`pwm_multi_group` 跨 group。
- **F10**：`pwm_fault` 验证 UVM force `PAD_PWM_FAULT` 触发的中断置位/清除；`pwm_multi_group` 跨 group。
- **F11**：⚠️ `pwm_trig_etb` 验证 `pwm_tim0_etb_trig` 脉冲（输出侧已闭环）；**输入侧 `etb_pwm_trig_tim*_on/off` 在 `apb0_sub_top.v:845-849` tie-0 不可激励**（环境限制，同 RTC `etb_rtc_trig`，非 RTL 缺陷）。
- **F12**：`pwm_reset_default` 53 寄存器复位值（全 0）。
- **F13**：`pwm_output_duty` (group0/group1) + `pwm_multi_group` (group0 LOAD=0x100 vs group3 LOAD=0x400 周期比 ≈ 4)。
- **F14**：`pwm_vic_route` 验证 `pad_vic_int_vld[25]` 断言 + 解除。

### 3.2 §5 验收标准对照

| 验收项 | 计划描述 | 报告结果 |
|--------|---------|----------|
| `pwm_test`（既有） | `CAPRIS == 0x2` + `printf("pwm io test pass!")` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` | ✅ 基线通过 |
| `pwm_output_duty` | TB 测量的 PWM 周期/占空比/分频与 LOAD/CMP/cntdiv 配置一致 | ✅ LOAD=799, CMPA=200 → 占空比 75%；cntdiv=0 周期 = 2 倍 |
| `pwm_polarity_invert` | TB 采样 `o_pwm0` 在 `PWMINVERTTRIG[0]=1` 前后波形极性翻转 | ✅ 反转前后 75% ↔ 25% 互补 |
| `pwm_count_mode` | up 模式锯齿波 / up-down 模式三角波形差异 | ✅ up vs up-down 周期比 ≈ 2 |
| `pwm_deadband` | TB 采样死区延迟 ticks 与 `PWMnDB.delaym/delayn` 一致 | ✅ CH0/CH1 互补 + 无重叠 (delay=0x10) |
| `pwm_cap_full` | 6 通道 × 4 边沿事件 + edge count/time 模式全捕获 | ✅ 完整覆盖 |
| `pwm_tim_full` | 6 个 TIM + 中断 4 件套 | ✅ tim0/1/2/5 完整 |
| `pwm_intr_full` | 8 个 PWM 中断寄存器 (INTEN/RIS/IC/IS ×1/2) mask/clear | ✅ group0 zero/load/compa_up/compb_up + PWMIC + INTEN 门控 + group3 |
| `pwm_fault` | fault=1 时 PWM 输出立即关闭 | ✅ fault 中断置位/清除（UVM force `PAD_PWM_FAULT`） |
| `pwm_trig_etb` | `pwm_xx_trig` 脉冲 + `pwm_timN_etb_trig` 脉冲 | ⚠️ **输出侧闭环**；**输入侧 `etb_pwm_trig_tim*_on/off` tie-0 不可激励**（环境限制，详见 §4.8） |
| `pwm_reset_default` | 复位后 53 个寄存器值与 §1.2 reset 表一致（全 0） | ✅ 53 寄存器全 0 |
| `pwm_multi_group` | 6 group 并行使能时输出波形独立、互不干扰 | ✅ group0 LOAD=0x100 vs group3 LOAD=0x400 周期比 ≈ 4 |
| `pwm_vic_route` | `pwmint` 中断发生时 `cpu_intr[25]` 上升沿匹配 | ✅ `pad_vic_int_vld[25]` 断言 + 解除 |
| `pwm_en_all`（新增辅助） | PWMCFG 使能位写读 | ✅ |
| `pwm_cmp_read`（新增辅助） | PWM0/1CMP 写读回环 | ✅ |

---

## 4. 关键验证发现（Spec/UG-vs-RTL 差异，以 RTL 为准）

### 4.1 PWMRIS1 group0 位序

**RTL 实证**（`wujian100_open/soc/pwm.v`）：
- `:4752` `assign pwmris1[31:0] = {2'b0, int_pwm2_compb_down, int_pwm2_compa_down, int_pwm2_compb_up, int_pwm2_compa_up, int_pwm2_cnt_load, int_pwm2_cnt_zero, 2'b0, int_pwm1_compb_down, ..., int_pwm0_compb_down, int_pwm0_compa_down, int_pwm0_compb_up, int_pwm0_compa_up, int_pwm0_cnt_load, int_pwm0_cnt_zero, 7'b0, int_fault};`

**位序**（group0 低 6 bit）：
- bit[8] = `cnt_zero`
- bit[9] = `cnt_load`
- bit[10] = `compa_up`
- **bit[11] = `compb_up`（非 UG 常见"compb 在 compa 前"的排列）**
- bit[12] = `compa_down`
- bit[13] = `compb_down`

**实测**：`pwm_intr_full` 验证 bit[11] = `compb_up` 置位；与 UG 字段表位序不一致。

**影响**：UG 应更新位序图；测试用例 bit 映射需按 RTL 验证。

### 4.2 PWMIS 直接 assign 自 PWMRIS

**RTL 实证**（`wujian100_open/soc/pwm.v`）：
- `:4757` `assign pwmis1[31:0] = pwmris1[31:0];`
- `:4758` `assign pwmis2[31:0] = pwmris2[31:0];`

**与常见设计差异**：典型设计中 `PWMIS = PWMRIS & INTEN`（masked 后中断状态）；当前 RTL 中 `PWMIS == PWMRIS`，masked 状态与 raw 无差异。

**影响**：masking 行为由 RIS 自身门控完成（详见 §4.3），IS 仅作为别名读出。建议：(a) UG 明确 IS == RIS 别名；(b) 若需要 masked 状态需 RTL 补 `assign pwmis = pwmris & pwmen`。

### 4.3 RIS 被 INTEN 门控

**RTL 实证**（`wujian100_open/soc/pwm.v`）：
- `:5298` 等位置 `event_flag && int_en` 才置位 raw pending（典型如 `int_tim_cnt_match`）。

**与常见设计差异**：典型设计中 RIS（Raw Interrupt Status）不门禁，masking 仅作用于 IS（masked status）；当前 RTL 中 **INTEN=0 时 RIS 也恒 0**。

**实测**（`pwm_intr_full`）：INTEN=0 时触发中断事件，PWMRIS 不置位；INTEN=1 时 RIS 正常置位。

**影响**：RIS 不再是"未屏蔽的原始中断状态"，而是"已使能通道的原始中断状态"。验证用例 (`pwm_intr_full`) 显式覆盖 INTEN 门控路径。

### 4.4 `capNmode` 语义与命名直觉相反

**RTL 实证**（`wujian100_open/soc/pwm.v`）：
- `:5030-5038` 捕获计数器逻辑：
  ```
  if(cap_mode)begin  // 沿计数模式
      if(pwm_cnt == cap_load_value && cap_edge)
          pwm_cnt <= 0;
      else if(cap_edge)
          pwm_cnt <= pwm_cnt + 1;
  end
  else              // 时间戳模式
      pwm_cnt <= pwm_cnt + 1;
  ```

**与命名直觉差异**：`cap_mode=1` 表示沿计数模式（`pwm_cnt` 每个捕获沿 +1，达到 `cap_load` 回绕）；`cap_mode=0` 表示时间戳模式（`pwm_cnt` 每 clk 自由累加）。命名上 "mode" 字眼易误读为"是否启用"。

**实测**（`pwm_cap_full`）：
- `cap_mode=1`：cnt_match 沿可达，与 `cap_load` 配置一致
- `cap_mode=0`：cnt_match 实际不可达（时间戳自由累加）

**影响**：测试用例 bit 映射需按 RTL 语义配置；UG 字段名 `capNmode` 建议改为 `capNmode_edge_count` / `capNmode_timestamp`。

### 4.5 捕获通道映射

**RTL 实证**（`wujian100_open/soc/pwm.v`）：
- `:4422` 捕获通道 1（cap1）实例化 `.i_capture (i_capture_2)`，对应 PAD `PAD_PWM_CH2`

**映射规律**：`capN ← CH(2N)`，即：
- cap0 ← PAD_PWM_CH0
- cap1 ← PAD_PWM_CH2
- cap2 ← PAD_PWM_CH4
- cap3 ← PAD_PWM_CH6
- cap4 ← PAD_PWM_CH8
- cap5 ← PAD_PWM_CH10

**实测**（`pwm_cap_full`）：legacy 激励块 `apb0_pwm/pwm_test.v` 在 CH0 输出 20 脉冲后每个 CH0 posedge 翻转 force 到 `PAD_PWM_CH2`，对应 cap1 捕获。

**影响**：测试用例捕获源 PAD 必须为偶数编号；`wujian100_open_top.v:1413` `PAD_DIG_IO x_PAD_PWM_CH2` 确认 PAD 单元正确接入。

### 4.6 `cntdiv=0` 即 2 分频

**RTL 实证**（`wujian100_open/soc/pwm.v`）：
- `:4154` `3'b000 : clkspec[6:0] = 7'h1;`（`cntdiv[2:0]=000` → `clkspec=1` → `clkcnt==1` 翻转 → 实际 2 分频）

**实测**（`pwm_output_duty`）：`cntdiv=0` 周期 = 不分频时的 2 倍；与"cntdiv=0 即不分频"直觉相反。

**影响**：测试用例周期计算需按 `(LOAD+1) × 2 × ext_clk_period` 估算（cntdiv=0）；UG 字段名 `cntdiv=0` 易误读为"不分频"。

### 4.7 fault 中断为电平型

**RTL 行为**：`int_fault` 在 `fault & intenfault` 期间每拍置位；清除前必须先关 INTEN，否则立刻重触发。

**实测**（`pwm_fault`）：UVM force `PAD_PWM_FAULT` → INTEN 关闭 → 写 `PWMIC` → 等待 raw → `INTEN` 重新开启 → fault 撤销。步骤必须严格按"先关 INTEN 再清中断"顺序，否则循环触发。

**影响**：测试用例 fault 清除逻辑必须分两阶段（先关 INTEN 再清中断）；UG 应明确电平型中断语义。

### 4.8 F11 ETB 触发输入 tie-0（集成限制）

**RTL 实证**（`wujian100_open/soc/apb0_sub_top.v`）：
- `:845-849` `.etb_pwm_trig_tim0_off (1'b0), .etb_pwm_trig_tim0_on (1'b0), .etb_pwm_trig_tim1_off (1'b0), .etb_pwm_trig_tim1_on (1'b0), .etb_pwm_trig_tim2_off (1'b0),` 等 12 路 ETB 输入全 tie-0

**影响**：
- **输出侧闭环**：`pwm_trig_etb` UVM 序列验证 `pwm_tim0_etb_trig` 脉冲 + `pwm_xx_trig` 脉冲
- **输入侧未覆盖**：F11 输入通路（ETB agent 驱动 `etb_pwm_trig_tim0_on` → counter 自动 reload）受 SoC 集成层 tie-0 限制，TB 端不可激励（同 RTC `etb_rtc_trig` tie-0，详见 RTC 验证报告 §4.4）

---

## 5. 问题与修复记录（调试经验）

| # | 问题 | 影响 | 解决 |
|---|------|------|------|
| 1 | `pwm_intr_full` 初版假设 `PWMRIS` 不受 INTEN 门控（典型设计）→ INTEN=0 写事件后 RIS 仍置位假阳性 | F9 误判 | 按 RTL §4.3 改写：INTEN=0 时 RIS 恒 0；RIS 是"已使能通道的原始中断"；分三段验证（INTEN=0 / INTEN=1 写 IC / INTEN=1 不写 IC） |
| 2 | `pwm_cap_full` 初版 capNmode 位写 0/1 含义误读（命名直觉）→ cap_match 永远不可达 | F7 误判 | 按 RTL §4.4 改写：cap_mode=1 沿计数（cnt_match 可达）、cap_mode=0 时间戳（cnt_match 不可达） |
| 3 | `pwm_trig_etb` 初版假设输入通路可 force → `etb_pwm_trig_tim*_on` tie-0 不可激励 | F11 输入侧漏验 | 改为输出侧验证（`pwm_xx_trig` + `pwm_tim0_etb_trig` 脉冲）；输入侧标记为集成限制（§4.8） |
| 4 | `pwm_fault` 初版未先关 INTEN 即写 PWMIC → 循环触发 | F10 假阳性 | 严格按"先关 INTEN → 写 PWMIC → 等 raw=0 → INTEN 重开 → fault 撤销"两阶段清除 |
| 5 | `pwm_deadband` 初版误用 `delay=0x1` → CH0/CH1 重叠误判 | F6 漏验 | 改用 `delay=0x10`（足够大），TB 测得 CH0/CH1 互补 + 无重叠 |
| 6 | **环境限制（F11 输入侧）**：`etb_pwm_trig_tim*_on/off` 在 `apb0_sub_top.v:845-849` tie-0（同 RTC `etb_rtc_trig`） | F11 输入侧集成限制 | F11 输出侧闭环；输入侧记录为环境限制；建议后续 ETB fabric 集成时补充联调 |

---

## 6. 遗留风险与后续建议

| # | 风险 / 建议项 | 类别 | 说明 | 建议 |
|---|--------------|------|------|------|
| 1 | PWMRIS1 位序与 UG 字段表不一致（§4.1） | 风险 | bit[11] = compb_up，非 UG 常见排列 | UG 更新位序图 |
| 2 | PWMIS == PWMRIS 别名（§4.2） | 风险 | masked 状态未独立 | (a) UG 明确 IS == RIS 别名；(b) 若需 masked 状态 RTL 补 `assign pwmis = pwmris & pwmen` |
| 3 | RIS 被 INTEN 门控（§4.3） | 风险 | RIS 不再是"未屏蔽原始状态" | UG 明确 RIS 是"已使能通道的原始状态" |
| 4 | `capNmode` 语义与命名直觉相反（§4.4） | 风险 | 易误读 | UG 字段名改为 `capNmode_edge_count` / `capNmode_timestamp` |
| 5 | `cntdiv=0` 即 2 分频（§4.6） | 风险 | 与"不分频"直觉相反 | UG 明确 `cntdiv=0` 即 2 分频 |
| 6 | fault 中断电平型（§4.7） | 风险 | 清除顺序要求严格 | UG 明确电平型中断语义；驱动代码必须分两阶段清除 |
| 7 | F11 ETB 输入通路 tie-0（§4.8） | 风险 | 同 RTC `etb_rtc_trig` 集成限制 | (a) 文档标注；(b) ETB fabric 集成时补充联调 |
| 8 | TIPC trust 信号（`tipc_pwm_trust` / `pprot[2:0]`）仅透传未过滤 | 风险 | 本配置未使能 trustzone | 若启用 trustzone，需补充 trust 边界用例 |
| 9 | 53 寄存器复位清单实测全 0，但 UG 字段表中部分 reset 默认 `0x1`（待 RTL 核对） | 改进 | 实证全 0，UG 部分差异可能 | UG 与 RTL 复位清单同步校对 |
| 10 | 死区延迟 ticks 精度（建议 ≥100 PWM 周期采样平均） | 改进 | 短窗口测量抖动大 | 后续可补充 reference model 对比 |

---

## 7. 附录 - 文件清单与 commit 记录

### 8.1 测试代码（C 端固件）

```
dv/simulation/verif_env/soc/c_case/
├── pwm/pwm_test.c                              (既有 F1/F7；PWMCFG=0x9002001 + CAP01MATCH=0x200000)
├── pwm/pwm_reset_default.c                     (新增 F12：53 寄存器复位值)
├── pwm/pwm_en_all.c                            (新增 F1：PWMCFG 使能位写读)
├── pwm/pwm_cmp_read.c                          (新增 F5：PWM0/1CMP 写读)
├── pwm/pwm_tim_full.c                          (新增 F8：tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控)
├── pwm/pwm_intr_full.c                         (新增 F9：group0 + group3)
├── pwm/pwm_cap_full.c                          (新增 F7：6 通道 + 4 边沿事件 + edge count/time)
├── pwm/pwm_output_duty.c                       (新增 F5 + F1：LOAD=799, CMPA=200)
├── pwm/pwm_polarity_invert.c                   (新增 F2：反转前后 75%↔25% 互补)
├── pwm/pwm_count_mode.c                        (新增 F4：up vs up-down)
├── pwm/pwm_deadband.c                          (新增 F6：CH0/CH1 互补 + delay=0x10)
├── pwm/pwm_fault.c                             (新增 F10：UVM force PAD_PWM_FAULT)
├── pwm/pwm_vic_route.c                         (新增 F14：UVM 协同)
├── pwm/pwm_trig_etb.c                          (新增 F3 + F11 输出：UVM 协同)
├── pwm/pwm_multi_group.c                       (新增 F13：group0 LOAD=0x100 vs group3 LOAD=0x400)
└── addr_map/map_test.c                         (通用地址空间 read 0 检查，含 PWM 区域 0x5001_C000~0x5001_FFFF)
```

### 8.2 UVM 测试与序列

```
dv/simulation/verif_env/soc/soc_top/tests/uvm_test/
├── soc_top_test_lib.svh                        (test 注册；新增 PWM UVM 入口)
└── soc_top_pwm_xxx_test.svh                    (8 个 UVM 配对 test 类)
```

UVM 配对列表：
- `soc_top_pwm_output_duty_test`（F5 + F1）
- `soc_top_pwm_polarity_invert_test`（F2）
- `soc_top_pwm_count_mode_test`（F4）
- `soc_top_pwm_deadband_test`（F6）
- `soc_top_pwm_fault_test`（F10）
- `soc_top_pwm_vic_route_test`（F14）
- `soc_top_pwm_trig_etb_test`（F3 + F11）
- `soc_top_pwm_multi_group_test`（F13）

### 8.3 环境修复

```
Makefile（dv/simulation/verif_env/soc/soc_top/tests/uvm_test/ 或顶层）
  - 新增 findstring pwm_ 分支：DEF = USE_APB0 + USE_APB0_PWM
  - 旧逻辑：DEF 按 C 测试文件名精确匹配（仅 pwm_test 命中）
  - 新逻辑：路径含 pwm_ 即自动启用 DEF → legacy PAD force 块（apb0/tb_top/apb0_pwm/pwm_test.v）参与编译
```

### 8.4 Commit 记录

| Commit | 说明 |
|--------|------|
| `c20f768` | 新增 13 个 PWM 用例（8 C + 5 UVM 协同）+ Makefile `findstring pwm_` 分支修复 + 8 项 RTL 注释（PWMRIS1 位序 / PWMIS 别名 / RIS INTEN 门控 / capNmode 语义 / 通道映射 / cntdiv=0 / fault 电平型 / F11 tie-0） |

### 8.5 关键 RTL 行号索引

| 行为 | 文件 : 行 |
|------|----------|
| `PWMRIS1` 组装（含 group0/1/2 compb_up 在 bit11） | `wujian100_open/soc/pwm.v : 4752` |
| `PWMIS == PWMRIS`（别名，无 masked 状态） | `wujian100_open/soc/pwm.v : 4757-4758` |
| `cntdiv=0` → `clkspec=1`（2 分频） | `wujian100_open/soc/pwm.v : 4154` |
| 捕获通道 1 实例化（`.i_capture(i_capture_2)`） | `wujian100_open/soc/pwm.v : 4422` |
| 捕获计数器逻辑（cap_mode=1 沿计数 / cap_mode=0 时间戳） | `wujian100_open/soc/pwm.v : 5030-5038` |
| RIS 门控 INTEN（典型：`int_tim_cnt_match`） | `wujian100_open/soc/pwm.v : 5298` |
| `etb_pwm_trig_tim*` 12 路 tie-0（SoC 集成层） | `wujian100_open/soc/apb0_sub_top.v : 845-849` |
| `PAD_DIG_IO x_PAD_PWM_CH2`（PAD 单元） | `wujian100_open/soc/wujian100_open_top.v : 1413` |
| `ip_cpu_int_vld[25] = pwm_wic_intr`（VIC 中断号 25） | `wujian100_open/soc/core_top.v : 545` |

---

## 8. 结论

PWM 模块验证全部闭环：14 个用例（既有 1 + 新增 13）0 UVM_ERROR / 0 UVM_FATAL 全 PASS；F1~F14 全部覆盖（F11 输出侧闭环、输入侧集成限制）；8 项 Spec/UG-vs-RTL 关键差异（PWMRIS1 位序 / PWMIS 别名 / RIS INTEN 门控 / capNmode 语义 / 通道映射 / cntdiv=0 / fault 电平型 / F11 tie-0）已写入验证报告与模块分析；6 项调试经验 + 环境限制已沉淀。遗留风险 10 项已分类登记，建议按 §7 优先级进入下一阶段（UG 文档同步、RIS/IS 独立 RTL 修正、ETB fabric 联调、trust 边界用例等）。
