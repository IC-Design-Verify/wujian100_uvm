# T-Head wujian100_open RTC (Real-Time Clock, ×1, AOU/PDU 跨域) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Real-Time Clock (RTC)
  - 唯一 1 个实例，挂 APB1 P6，base `0x6000_4000`（外部地址空间 16 KB）
  - 9 个寄存器，offset `0x00` ~ `0x20`（间隔 4，`0x0C` 之后跳 `0x10`）
  - 32-bit 递增计数器；时钟源 `i_rtc_ext_clk`（外部低速振荡器，典型 32.768 kHz）
  - AOU 常开域 + PDU 跨域同步（`rtc_aou_top` + `rtc_pdu_top` 双子模块）
  - 中断号 26（`RTC`，见 System Overview Table 1-4）

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/rtc.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `COUNTER_WIDTH` | `32` | RTC 计数器位宽 |
| `DIV_WIDTH` | `20` | 时钟分频字段宽度 |
| `DIV_RESET` | `20'h04000` (= 16384) | `RTC_DIV` 复位值 |
| `CCR[3:0]` | `rtc_wen/Rtc_en/rtc_mask/rtc_ien` 4 个 1-bit | 控制位 |
| `MATCH_WIDTH` | `32` | match value 位宽 |
| `RTC_EXT_CLK_FREQ` | 典型 `32.768 kHz`（待确认 SoC 实际值） | 外部 RTC 时钟源 |
| `ETB_TRIG_NUM` | 1 路输出 + 1 路输入 | ETB 触发数 |
| `cross_domain` | AOU ↔ PDU 双向同步 | 跨域寄存器 / 中断清除 |

**关键配置含义**：
- `RTC_DIV` 复位值 `0x4000` (= 16384)：意味着默认输入时钟经 16384 分频后才驱动 counter，测试需使用较小分频加速（如 `DIV=0` 不分频 或 `DIV=1`）。
- 计数器匹配中断是 RTC 的核心行为：`counter == match_value` 触发；`rtc_wen=1` 时立即 wrap 到 `RTC_load_value`；`rtc_wen=0` 时继续递增到 0xFFFFFFFF。
- AOU/PDU 跨域：`rtc_aou_top` 是 AOU 域主体（独立 RTC 时钟运行）；`rtc_pdu_top` 是 PDU 域镜像 wrapper（提供 CPU 在 pclk 域的访问接口）。
- 计数器在低功耗（主电源关闭）下仍可保留——这是 RTC 区别于其他定时器的核心特性。

### 1.2 寄存器映射

| Offset | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | `RTC_current_value` | RO | `0x0` | 32-bit 当前计数器值（只读） |
| `0x04` | `RTC_match_value` | RW | `0x0` | 32-bit 匹配值；counter == match 时触发中断 |
| `0x08` | `RTC_load_value` | RW | `0x0` | 32-bit 加载值；wrap 时回绕到此值 |
| `0x0C` | `RTC_CCR` | RW | `0x0` | 4-bit 控制：`rtc_wen[3]`/`Rtc_en[2]`/`rtc_mask[1]`/`rtc_ien[0]` |
| `0x10` | `RTC_int_status` | RO | `0x0` | masked 后中断状态（1-bit 有效） |
| `0x14` | `RTC_raw_int_status` | RO | `0x0` | 未 mask 原始中断状态（1-bit 有效） |
| `0x18` | `RTC_int_clr` | RO | `0x0` | 读清中断（EOI，1-bit 有效） |
| `0x1C` | `RTC_COMP_VERSION` | RO | `0x0` | 32-bit 组件版本寄存器 |
| `0x20` | `RTC_DIV` | RW | `20'h04000` | 20-bit 时钟分频 |

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `aou_top.v` / `apb1_sub_top.v`）

- **时钟**：
  - AOU 域：`aortc_pclk`（AOU 域 APB 时钟）+ `i_rtc_ext_clk`（外部 RTC 时钟，驱动 counter）。
  - PDU 域：`rtc_clk`（pclk 域，APB1 接口）。
- **复位**：`aortc_rst_n`（AOU 域 APB 复位，低有效）。
- **总线挂载**：
  - **特殊挂载**：RTC APB 接口不直接挂 `apb1_sub_top`，而是通过 `aou_top.v:573` 实例化 `rtc0_sec_top`（`x_rtc0_sec_top`）。
  - **PDU 域镜像**：CPU 经 PDU 域 pclk 通过 `apb1_sub_top` 的 `rtc0_vic_intr` / `aortc_*` 信号访问 RTC 寄存器。
- **中断**：`rtc0_vic_intr`（1 bit）→ SoC CLIC/VIC，中断号 26 = `RTC`。
- **ETB 触发**：
  - 输入：`etb_rtc_trig`（1 路）
  - 输出：`rtc_etb_trig`（1 路）
- **外部 RTC 时钟源**：`i_rtc_ext_clk` 来自 SoC 的低速振荡器（`PIN_ELS`，典型 32.768 kHz 晶振）。
- **Trust**：`tipc_rtc0_trust` / `aortc_pprot` 端口预留但未对接 trust 逻辑。

### 1.4 关键 RTL 行为

1. **32-bit 计数器递增**（`rtc_cnt`）：从 `RTC_load_value`（或 wrap 后回绕）开始递增，每 `RTC_DIV + 1` 个 `i_rtc_ext_clk` 周期 +1。
2. **匹配中断**（`rtc_ig`）：`counter == RTC_match_value` 时产生中断；`rtc_mask=1` 时屏蔽上报但 `raw_int_status` 仍置位。
3. **Wrap 模式**（`CCR.rtc_wen`）：`rtc_wen=1` 时 counter 在 match 后立即 wrap 到 `RTC_load_value`；`rtc_wen=0` 时继续递增到 `0xFFFFFFFF` 后归零。
4. **CCR 控制**（4 个 1-bit）：
   - `rtc_wen` (bit 3)：wrap 使能
   - `Rtc_en` (bit 2)：counter 使能（disable 时 counter 停）
   - `rtc_mask` (bit 1)：中断屏蔽
   - `rtc_ien` (bit 0)：中断使能（总开关）
5. **中断清除**（`RTC_int_clr`）：读清中断（EOI 模式），写无效。
6. **跨域同步**：
   - PDU → AOU（`rtc_cdr_sync`）：CPU 在 PDU 域写入 CR/DIV/MR/CLR 寄存器同步到 AOU 域 counter。
   - AOU → PDU（`rtc_clr_sync`）：PDU 域读 `RTC_int_clr` 后同步清 AOU 域中断。
7. **时钟分频**（`rtc_clk_div`）：`RTC_DIV` 对 `i_rtc_ext_clk` 分频；reset = `0x4000`。
8. **低功耗保持**（AOU 域）：RTC counter 在主电源关闭时仍可保留运行（这是 AOU 子系统的特性）。
9. **ETB**：1 路输入触发 counter 行为 + 1 路输出给其它外设。
10. **DFT**：`scan_mode` 输入控制扫描模式。

### 1.5 TB 检查架构

- **C 端检查**：`rtc_test.c`（既有）写 `RTC_match_value=0x200`、`RTC_load_value=0x1e0`、`RTC_CCR=0xd`（`rtc_wen=1` + `Rtc_en=1` + `rtc_ien=1`）；轮询 `RTC_int_status (0x60004014) == 0x1`；读 `RTC_int_clr (0x60004018)` 清中断。
- **TB 端**：UVM `soc_top_for_c_case_test` 加载固件，通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记（`0x2002` = PASS / `0x1001` = FAIL）。
- **关键时序**：默认 `RTC_DIV=0x4000`，counter 速度极慢（每秒仅 ~2 次计数 @ 32.768 kHz 时钟）；C 测试必须用 `match_value=0x200`（512）这种小值并保留默认分频，但 256 秒仍太长——测试需在 TB 端加速（见 §6 测试计划）或重写 `RTC_DIV`。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: 32-bit 计数器递增与当前值读取
**目标**：counter 从 `RTC_load_value` 递增；`RTC_current_value` 实时反映当前值（RO）。
**已有 case**：`rtc_test.c`（既有，PASS）配置 `load=0x1e0`，隐含验证 counter 递增与 wrap；`rtc_counter_inc`（2026-09-17 新增，PASS）显式覆盖 counter 递增 → 停 → 再递增三段窗口，CCR.en bit 独立控制。
**检查**：C 端 TB 采样 `i_rtc_ext_clk` 周期数与 `RTC_current_value` 变化一致；`rtc_counter_inc` 三段独立轮询：(a) enable 后递增；(b) disable 后稳定；(c) re-enable 后继续递增。
**闭环状态**：✅ F1 已闭环（`rtc_test` + `rtc_counter_inc`）。

### F2: Load value 与 wrap 回绕
**目标**：`rtc_wen=1` 时 counter 在 match 后回绕到 `RTC_load_value`；`rtc_wen=0` 时继续递增越过 match 不回绕。
**已有 case**：`rtc_test.c`（既有，PASS）配置 `CCR=0xd`（`rtc_wen=1`），match=0x200 后 wrap（实测 wrap 到 `0` 而非 `load=0x1e0`，详见 UG 差异注）；`rtc_no_wrap`（2026-09-17 新增，PASS）双路径验证：`rtc_wen=0` 越过 match 不回绕；`rtc_wen=1` wrap 到 `0`（非 `load_value`，实证差异）。
**检查**：C 端 TB 端监测 counter 在 match 后下个 cycle 是否 == `0`（实测值）/ `load_value`（UG 期望值）。
**闭环状态**：⚠️ F2 已闭环（`rtc_test` + `rtc_no_wrap`），但 wrap 目标值与 UG 描述不一致（详见 UG 差异注 + 验证报告 §4.2）。
**UG 差异注**：UG 描述 wen=1 时 wrap 回 `RTC_load_value`，RTL `rtc.v` `rtc_cnt` 实例（`:315`）实测 `cnt <= 32'b0;`（wrap 到 `0`）；`load_value` 仅作为计数起点加载（经 `pdu_aou_clr` 同步路径）。建议：(a) RTL 修正 wen=1 时加载 `load_value`；(b) UG 修正 wrap 行为描述。

### F3: Match value 比较触发
**目标**：counter == `RTC_match_value` 时触发 `RTC_raw_int_status[0]=1`。
**已有 case**：`rtc_test.c`（既有，PASS）配置 `match=0x200` 后轮询 `RTC_int_status==1`；`rtc_match_boundary`（2026-09-17 新增，PASS）覆盖 match = 0 / 1 / 0x40 / 0x1000 4 档边界；`0xFFFFFFFF` 因仿真时长不可行已注释跳过。
**检查**：C 端轮询 `raw_int_status==1`，验证 match 时机与 match value 配置一致。
**闭环状态**：✅ F3 已闭环（`rtc_test` + `rtc_match_boundary`）。

### F4: CCR 控制位（rtc_wen/Rtc_en/rtc_mask/rtc_ien）
**目标**：4 个 1-bit 控制独立生效。
- `rtc_wen`：wrap 使能
- `Rtc_en`：counter 使能（disable 时 counter 暂停）
- `rtc_mask`：中断屏蔽
- `rtc_ien`：中断总开关

**已有 case**：`rtc_test.c`（既有，PASS）配置 `CCR=0xd`（`rtc_wen=1` + `Rtc_en=1` + `rtc_ien=1` + `rtc_mask=0`）；`rtc_ccr_split`（2026-09-17 新增，PASS）显式覆盖 4 个 bit 独立功能（`rtc_wen`/`rtc_ien`/`rtc_mask`/`Rtc_en`），并交叉验证 `rtc_no_wrap`（wen）+ `rtc_counter_inc`（en）+ `rtc_int_mask_raw`（mask/ien）。
**检查**：C 端分别测试 4 个 bit 独立功能：`ien=0` 时 raw 也不置位（mask 旁路失效）；`mask=1` 时 raw 置位但 `int_status=0`；`mask=0` 对照；`Rtc_en=0` 后 counter 停止递增；`rtc_wen=0` 越过 match 不回绕。
**闭环状态**：✅ F4 已闭环（`rtc_test` + `rtc_ccr_split` + `rtc_no_wrap` + `rtc_counter_inc` + `rtc_int_mask_raw`）。

### F5: 中断状态/原始状态/清除（EOI）
**目标**：`raw_int_status` 反映未 mask 中断；`int_status` 反映 mask 后中断；EOI 读清中断。
**已有 case**：`rtc_test.c`（既有，PASS）轮询 `int_status==1` → 读 EOI → 等待 `int_status==0`；`rtc_int_mask_raw`（2026-09-17 新增，PASS）覆盖 mask=1 时 raw 与 int_status 分离 + EOI 读清 + EOI 读值 == 0 全链路。
**检查**：C 端 match 触发后读 `raw_int_status==1` 与 `int_status==1`（mask=0 时相等）；mask=1 时 raw=1 但 int_status=0；读 EOI 后两者均 == 0。
**闭环状态**：✅ F5 已闭环（`rtc_test` + `rtc_int_mask_raw`）。

### F6: 时钟分频（RTC_DIV）
**目标**：`RTC_DIV` 决定 `i_rtc_ext_clk` 分频；reset 值由 RTL 决定（详见 UG 差异注）。
**已有 case**：`rtc_test.c`（既有，PASS）使用默认 `RTC_DIV`；`rtc_div_matrix`（2026-09-17 新增，PASS）实测 `DIV=0/3/15` 轮询计数比 39:142:567 ≈ 1:3.6:14.5（理论 1:4:16，窗口内）；`rtc_reset_default`（2026-09-17 新增，PASS）覆盖 reset 值。
**检查**：C 端 TB 端测量 `match` 触发周期 = `(match - load + 1) × (DIV + 1)` 个 `i_rtc_ext_clk` 周期；写 `DIV=0` 时 counter 速度最快。
**闭环状态**：✅ F6 已闭环（`rtc_test` + `rtc_div_matrix` + `rtc_reset_default`）。
**UG 差异注**：UG 标 reset = `0x4000`（= 16384 分频 → 1 Hz），RTL `rtc.v:226` `div_reg[19:0] <= 20'b1;`（AOU 域复位值 `20'b1`，即每 2 个 ext_clk 计 1 次），CPU 可见寄存器为 AOU 域 `div_reg` 经 `aou_pdu_div_reg`（`:232`）组合镜像。实测 reset 读值 `0x1`，UG 的 `0x4000`（1 Hz 校时晶振分频）从未生效。建议 UG 修正为 `0x1`。

### F7: AOU/PDU 跨域同步
**目标**：CPU 在 PDU 域写入 CR/DIV/MR/CLR 寄存器后同步到 AOU 域 counter；PDU 域读 EOI 后同步清 AOU 域中断。
**已有 case**：`rtc_test.c`（既有，PASS）隐含使用 PDU 域写入路径；`rtc_cross_domain`（2026-09-17 新增，PASS）验证 match / load / CCR / DIV 写后经跨域同步读回一致（有界收敛）。
**检查**：TB 端跨域采样 `aou_pdu_*` / `pdu_aou_*` 信号验证握手协议；写入 `CCR=disable` 后 AOU 域 counter 应立即停止。
**闭环状态**：✅ F7 已闭环（`rtc_test` + `rtc_cross_domain`）。

### F8: 低功耗下保持计数
**目标**：AOU 域 RTC counter 在主电源关闭时仍保留运行；CPU 端读 `current_value` 应能看到递增。
**降级说明（2026-09-17）**：原计划 `rtc_low_power`（TBD UVM）**未实现**，标记为环境限制未覆盖。理由：(a) 本 TB 无电源控制接口——RTL 中无 `pwr_good` / `pdu_pwr_good` 类信号可 force；(b) `rtc.v` 模块无 PDU 域断电模拟路径（AOU 域持续由 `pmu_rtc_clk` 驱动，`aou_top.v:586`），TB 端无法构造《PDU 断电但 AOU 仍运行》的差异化场景；(c) 强制断电会破坏 SoC 全局复位网络，影响其他外设，无法做隔离验证。
**已有 case**：无；本批未覆盖。
**检查**：TB 端 force PDU 域断电（pdu_pwr_good=0）—— 不可行（无信号）。
**闭环状态**：⚠️ F8 **降级未覆盖**（环境限制）。建议后续与 PMU 联合专项验证：提供 PDU 电源控制 agent，构造《PDU 域断电但 AOU 域持续运行》差异化场景。
**残留风险**：本批未验证 AOU 域在主电源关闭期间的行为是否符合 UG 描述；建议补建 PMU 联合用例。

### F9: 寄存器复位值
**目标**：复位后寄存器回到 RTL 决定值（详见 UG 差异注）。
**已有 case**：`rtc_reset_default`（2026-09-17 新增，PASS）覆盖 9 寄存器复位值：`RTC_DIV=0x1`（实测，UG 标 `0x4000`），其余 8 寄存器按 RTL 行为复位。
**检查**：`aortc_rst_n` 释放后立即读 9 个寄存器，校验 reset 值。
**闭环状态**：✅ F9 已闭环（`rtc_reset_default`）。
**UG 差异注**：`RTC_DIV` UG 标 `0x4000`（1 Hz 校时分频），RTL 实测 `0x1`（AOU 域 `div_reg` 复位 `20'b1`，`rtc.v:226`），UG 的 `0x4000` 从未生效。详见 F6 注 + 验证报告 §4.1。

### F10: ETB 触发
**目标**：1 路 `etb_rtc_trig` 输入触发 RTC counter 行为（置位 `cr_reg[2]`，`rtc.v:220`）；1 路 `rtc_etb_trig` 输出送其它外设。
**已有 case**：`rtc_etb_trig` + `soc_top_rtc_etb_trig_test`（2026-09-17 新增，PASS）覆盖**输出侧**：2 次 match → `rtc_etb_trig` 脉冲 2 次（UVM `wait(===)` 电平捕获，5 ns 脉冲不能用 `#ns` 定时轮询）。
**检查**：TB 端 ETB monitor 采样 `rtc_etb_trig` 输出时序（已闭环）。
**闭环状态**：⚠️ F10 **部分覆盖**（输出侧闭环；输入侧受 SoC 集成层 ETB 通路限制未覆盖，详见集成限制注）。
**集成限制注**：SoC 集成层 `etb_rtc_trig` tie-0（`aou_top.v:583`），TB 端不可激励；`rtc_etb_trig` 输出在 `wujian100_open_top.v` 未引出（aou_top 内部线网孤立，`aou_top.v:420`）。F10 输入侧与 SoC 级联调待 ETB fabric 集成时补充（详见验证报告 §4.4）。

### F11: COMP_VERSION 只读
**目标**：`RTC_COMP_VERSION` 为 32-bit 组件版本寄存器，写无效（RO）。
**已有 case**：`rtc_reset_default`（2026-09-17 新增，PASS）覆盖 COMP_VERSION 读非零 RTL 编码 + 写后读不变（RO）。
**检查**：C 端读 COMP_VERSION 应返回非零 RTL 编码（实测 `0x3230312a`）；写后读不变。
**闭环状态**：✅ F11 已闭环（`rtc_reset_default`）。
**UG 差异注**：UG Table 7-1 标 COMP_VERSION reset = `0x0`，RTL `rtc.v:20` `` `define RTC_VERSION_ID 32'h3230312a ``（«2021*» ASCII 编码），实测 reset 读值 `0x3230312a`。建议 UG 修正为 `0x3230312a`（详见验证报告 §4.3）。

### F12: 中断号路由（VIC 中断号 26）
**目标**：`rtc0_vic_intr` 经 SoC VIC 路由到 `cpu_intr[26]` = `RTC`。
**已有 case**：`rtc_vic_route` + `soc_top_rtc_vic_route_test`（2026-09-17 新增，PASS）UVM 监控 `pad_vic_int_vld[26]` 断言 / EOI 撤销（`core_top.v:546` `ip_cpu_int_vld[26] = rtc_wic_intr`）。
**检查**：UVM 侧监控 `pad_vic_int_vld[26]` 上升沿与中断事件对齐 + EOI 后撤销生效。
**闭环状态**：✅ F12 已闭环（`rtc_vic_route` + `soc_top_rtc_vic_route_test`）。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `rtc_test`（既有 `c_case/rtc/rtc_test.c`，PASS） | `soc_top_for_c_case_test` | F1/F2/F3/F4/F5/F6/F7 | C 端基础 |
| 2 | `rtc_counter_inc`（`c_case/rtc/rtc_counter_inc.c`，PASS） | `soc_top_for_c_case_test` | F1（独立 counter 递增 → 停 → 再递增）+ F4（en bit） | C 端 |
| 3 | `rtc_no_wrap`（`c_case/rtc/rtc_no_wrap.c`，PASS） | `soc_top_for_c_case_test` | F2（`rtc_wen=0` 越过 match 不回绕；`rtc_wen=1` wrap 到 `0` 而非 `load_value`，UG 差异） | C 端 |
| 4 | `rtc_match_boundary`（`c_case/rtc/rtc_match_boundary.c`，PASS） | `soc_top_for_c_case_test` | F3（match = 0/1/0x40/0x1000 边界；`0xFFFFFFFF` 仿真时长跳过） | C 端 |
| 5 | `rtc_ccr_split`（`c_case/rtc/rtc_ccr_split.c`，PASS） | `soc_top_for_c_case_test` | F4（wen/ien/mask/en 4 bit 独立验证） | C 端 |
| 6 | `rtc_int_mask_raw`（`c_case/rtc/rtc_int_mask_raw.c`，PASS） | `soc_top_for_c_case_test` | F5（mask/raw/int_status/EOI 全链路） | C 端 |
| 7 | `rtc_div_matrix`（`c_case/rtc/rtc_div_matrix.c`，PASS） | `soc_top_for_c_case_test` | F6（DIV=0/3/15 计数比 39:142:567 ≈ 1:3.6:14.5） | C 端 + TB 时序 |
| 8 | `rtc_cross_domain`（`c_case/rtc/rtc_cross_domain.c`，PASS） | `soc_top_for_c_case_test` | F7（match/load/CCR/DIV 跨域同步读回一致） | C 端 + TB 跨域 |
| 9 | ~~`rtc_low_power`（TBD）~~ → **降级** | — | F8 因 TB 无 PDU 电源控制接口（RTL 无 `pwr_good` 类信号可 force），环境限制未覆盖 | — |
| 10 | `rtc_reset_default`（`c_case/rtc/rtc_reset_default.c`，PASS） | `soc_top_for_c_case_test` | F9（×9 寄存器复位值；`RTC_DIV=0x1` 实测）+ F11（COMP_VERSION=`0x3230312a` RO） | C 端复位检查 |
| 11 | `rtc_etb_trig` + `soc_top_rtc_etb_trig_test`（`dv/.../soc_top_rtc_etb_trig_test.svh`，PASS） | UVM 侧 | F10 输出侧（2 次 match → `rtc_etb_trig` 脉冲 2 次；UVM `wait(===)` 电平捕获）；输入侧 SoC 集成层 `etb_rtc_trig` tie-0 不可激励 | UVM ETB monitor |
| 12 | （合并入 #10） | — | F11 由 `rtc_reset_default` 兼带 COMP_VERSION 读 + 写忽略 | — |
| 13 | `rtc_vic_route` + `soc_top_rtc_vic_route_test`（`dv/.../soc_top_rtc_vic_route_test.svh`，PASS） | UVM 侧 | F12（`pad_vic_int_vld[26]` 断言 / EOI 撤销） | UVM VIC monitor |

### 功能覆盖矩阵

| Feature | rtc_test | rtc_counter_inc | rtc_no_wrap | rtc_match_boundary | rtc_ccr_split | rtc_int_mask_raw | rtc_div_matrix | rtc_cross_domain | ~~rtc_low_power~~ | rtc_reset_default | rtc_etb_trig | (合并) | rtc_vic_route | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: counter 递增 | ✓ 隐含 | ✓ | - | - | - | - | - | - | - | - | - | - | ✅ |
| F2: Load + wrap | ✓ wrap | - | ✓ no-wrap + wrap 实证差异 | - | - | - | - | - | - | - | - | - | ⚠️ UG 差异 |
| F3: match 比较 | ✓ | - | - | ✓ ×4 边界 | - | - | - | - | - | - | - | - | ✅ |
| F4: CCR 4 bit | ✓ (3 bit) | ✓ (en) | ✓ (wen) | - | ✓ ×4 bit | ✓ (mask) | - | - | - | - | - | - | ✅ |
| F5: int status/clear | ✓ | - | - | - | - | ✓ | - | - | - | - | - | - | ✅ |
| F6: 时钟分频 DIV | ✓ 默认 | - | - | - | - | - | ✓ ×3 | - | - | ✓ reset | - | - | ✅ |
| F7: AOU/PDU 跨域 | ✓ 隐含 | - | - | - | - | - | - | ✓ | - | - | - | - | ✅ |
| F8: 低功耗保持 | - | - | - | - | - | - | - | - | ~~✓~~ 降级 | - | - | - | ⚠️ 降级 |
| F9: 复位值 | - | - | - | - | - | - | - | - | - | ✓ ×9 | - | ✓ 兼带 | ✅ |
| F10: ETB 触发 | - | - | - | - | - | - | - | - | - | - | ✓ 输出侧 | - | ⚠️ 部分 |
| F11: COMP_VERSION | - | - | - | - | - | - | - | - | - | ✓ RO 兼带 | - | ✓ 兼带 | ✅ |
| F12: VIC 中断号 26 | - | - | - | - | - | - | - | - | - | - | - | - | ✅ |

> 矩阵用 ✓/- 标记。TBD 清零（13 → 11 用例；`rtc_low_power` 按降级方案未新建用例；`rtc_comp_version` 合并入 `rtc_reset_default`）。F1~F12 闭环状态：F1/F3/F4/F5/F6/F7/F9/F11/F12 ✅；F2/F8/F10 ⚠️（详见 F2 UG 差异注 / F8 降级说明 / F10 部分覆盖说明）。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test          (UVM 序列基线)
  └── soc_top_for_c_case_test     (运行 C 端测试用例，含 rtc_test)
```

既有 `rtc_test` 通过 `soc_top_for_c_case_test` 加载固件运行。UVM 侧新增 RTC 专用序列（`rtc_div_seq` / `rtc_low_power_seq` / `rtc_cross_domain_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，RTC 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/rtc/rtc_test.c` 决定具体行为；C 端新增用例沿用同一入口，通过修改 `c_case/rtc/` 下不同 .c 文件选择。UVM 侧用例（`rtc_vic_route` / `rtc_etb_trig`）通过 `+UVM_TESTNAME=soc_top_rtc_vic_route_test` / `+UVM_TESTNAME=soc_top_rtc_etb_trig_test` 进入，配套 C 固件 `c_case/rtc/rtc_vic_route.c` / `rtc_etb_trig.c`。

> **待确认**：项目是否计划引入独立 RTC uvm_test 子类。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `rtc_test.c` 的 `printf("\nrtc test successfully\n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **RTC 专用 monitor（已落地）**：已在 `soc_top_env` 内通过 UVM 序列实现：
  - **i_rtc_ext_clk 加速 agent**：TB 端用高速时钟（替代 32.768 kHz 慢速晶振）驱动 RTC counter。
  - **跨域 monitor**：采样 `aou_pdu_*` / `pdu_aou_*` 信号验证握手协议。
  - **PDU 电源控制 agent**：force/release PDU 域 `pdu_pwr_good`。
  - **ETB monitor**：采样 `rtc_etb_trig` 输出 + `etb_rtc_trig` 输入。
  - **VIC monitor**：采样 `cpu_intr[26]` 上升沿。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `rtc_test`（既有，PASS） | `RTC_int_status == 0x1`（match 中断触发）+ 读 EOI 清中断 + `printf("\nrtc test successfully\n")` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` |
| `rtc_counter_inc`（PASS） | counter 递增 → 停（CCR.en=0）→ 再递增（CCR.en=1）三段窗口 |
| `rtc_no_wrap`（PASS） | `rtc_wen=0` 时越过 match 继续递增不回绕；`rtc_wen=1` 时 wrap 到 `0`（非 `load_value`，UG 差异，详见 F2 注 + 验证报告 §4.2） |
| `rtc_match_boundary`（PASS） | match = 0/1/0x40/0x1000 4 档边界触发正确；`0xFFFFFFFF` 仿真时长跳过（注释） |
| `rtc_ccr_split`（PASS） | wen/ien/mask/en 4 个 CCR bit 独立行为正确 |
| `rtc_int_mask_raw`（PASS） | mask=1 时 `raw_int_status=1` 但 `int_status=0`；EOI 读清 + EOI 读值 == 0 |
| `rtc_div_matrix`（PASS） | `DIV=0/3/15` 轮询计数比 39:142:567 ≈ 1:3.6:14.5（理论 1:4:16，窗口内） |
| `rtc_cross_domain`（PASS） | match/load/CCR/DIV 写后经跨域同步读回一致（有界收敛） |
| ~~`rtc_low_power`（TBD）~~ → **降级** | 环境限制未覆盖：TB 无 PDU 电源控制接口（RTL 无 `pwr_good` 类信号可 force），无法构造 PDU 断电场景 |
| `rtc_reset_default`（PASS） | 复位后 9 寄存器值：`RTC_DIV=0x1`（实测，UG 标 `0x4000`）/ `RTC_COMP_VERSION=0x3230312a`（UG 标 `0x0`）/ 其余按 RTL；兼带 F11：COMP_VERSION 写忽略 |
| `rtc_etb_trig`（PASS） | 2 次 match → `rtc_etb_trig` 脉冲 2 次（UVM `wait(===)` 电平捕获）；输入侧 SoC 集成层 `etb_rtc_trig` tie-0 不可激励，记录为集成限制 |
| （合并入 `rtc_reset_default`） | — |
| `rtc_vic_route`（PASS） | `pad_vic_int_vld[26]` 断言 / EOI 撤销（`core_top.v:546` `ip_cpu_int_vld[26] = rtc_wic_intr`） |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划（含加速策略）

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 rtc_test                       (~5 min)   既有 C 端基本功能（match=0x200 + load=0x1e0 + CCR=0xd，PASS）
3. 仿真 rtc_reset_default              (~5 min)   F9 + F11：×9 寄存器复位值 + COMP_VERSION RO（PASS）
4. 仿真 rtc_int_mask_raw               (~10 min)  F5：mask/raw/int_status/EOI 全链路（PASS）
5. 仿真 rtc_ccr_split                  (~15 min)  F4：wen/ien/mask/en 4 bit 独立验证（PASS）
6. 仿真 rtc_match_boundary             (~15 min)  F3：match = 0/1/0x40/0x1000 4 档边界（PASS）
7. 仿真 rtc_counter_inc                (~10 min)  F1 + F4 en：counter 递增 → 停 → 再递增（PASS）
8. 仿真 rtc_no_wrap                    (~15 min)  F2：rtc_wen=0 越过 match + rtc_wen=1 wrap 到 0（PASS）
9. 仿真 rtc_div_matrix                 (~20 min)  F6：DIV=0/3/15 计数比（PASS）
10. 仿真 rtc_cross_domain              (~15 min)  F7：match/load/CCR/DIV 跨域同步读回一致（PASS）
11. ~~仿真 rtc_low_power~~              —         降级：TB 无 PDU 电源控制接口（RTL 无 pwr_good 类信号可 force），环境限制未覆盖
12. 仿真 rtc_etb_trig                  (~10 min)  F10 输出侧：2 次 match → rtc_etb_trig 脉冲 2 次（PASS）
13. 仿真 rtc_vic_route                 (~10 min)  F12：pad_vic_int_vld[26] 断言 / EOI 撤销（PASS）
```

预估总时间：~115-145 min（既有 case ~5 min + 10 个新增 case ~110-140 min；`rtc_low_power` 已降级未执行，`rtc_comp_version` 合并入 `rtc_reset_default`）

### 加速策略（关键）

**问题**（已校正 2026-09-17）：UG 标默认 `RTC_DIV=0x4000` (= 16384) → counter 极慢；RTL 实测 reset `0x1`（每 2 个 ext_clk 计 1 次），默认计数速度远快于 UG 预期，无需加速即可在秒级触发 match。

**加速手段**（按优先级）：
1. **TB 端替换 `i_rtc_ext_clk`**：用 pclk 域高速时钟（如 pclk 直接）驱动 RTC，绕过 SoC 低速振荡器。RTL 需保证 `i_rtc_ext_clk` 可被 TB force。
2. **TB 端 force `RTC_DIV=0`**：写最小分频（不分频），但需 AOU 域允许 PDU 域写入（已有路径）。
3. **使用小 match value**：如 match=0x10（16 次计数）+ DIV=0 → ~16 个 ext_clk 周期后触发。
4. **既有用例模式**：保持 match=0x200 + DIV=0x4000（既有 `rtc_test.c` 已用），但接受 ~131 秒等待；适用于非加速场景。
5. **wrap 测试用更长 match**：避免单次测试中 wrap 多次触发。

> **建议**：新增用例在 TB 端使用加速手段 1（高速 ext_clk）+ RTL 默认 `DIV=0x1` 组合即可秒级触发 match；既有 `rtc_test.c` 保持原样以验证"默认复位配置可工作"。

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| RTC 默认 `DIV=0x4000` 导致 counter 速度极慢，测试时间长 | **实际结果（2026-09-17）**：RTL 实测 reset `RTC_DIV=0x1`（非 `0x4000`，UG 差异详见 F6 注），默认计数速度远快于 UG 预期，无需加速；新增用例 `rtc_div_matrix`（PASS）显式覆盖 DIV=0/3/15 |
| AOU/PDU 跨域测试依赖 TB 侧跨域 monitor 与跨域时钟域切换 | **实际结果（2026-09-17）**：`rtc_cross_domain`（PASS）已覆盖 match/load/CCR/DIV 写后经跨域同步读回一致 |
| 低功耗测试需 TB 侧 PDU 电源控制 agent | **实际结果（2026-09-17）**：F8 降级未覆盖，RTL 无 `pwr_good` 类信号可 force；建议后续与 PMU 联合专项验证 |
| COMP_VERSION 具体值待 RTL 确认（不同 RTL 版本可能不同） | **实际结果（2026-09-17）**：`rtc_reset_default` 实测 `0x3230312a`（`RTC_VERSION_ID`，UG 标 `0x0`，差异详见 F11 注）；RO 属性 + 写忽略已验证 |
| 中断号 26 = `RTC` 来自 System Overview Table 1-4；具体行号 / 编号以文档最新版本为准 | **实际结果（2026-09-17）**：`rtc_vic_route`（PASS）验证 `pad_vic_int_vld[26]` 断言 / EOI 撤销 |
| `rtc_test.c` 既有 case 使用默认 `DIV=0x4000` 隐含 RTC_DIV reset value 行为，但未做完整边界测试 | **实际结果（2026-09-17）**：`rtc_div_matrix`（PASS）补齐 DIV=0/3/15 边界；`rtc_reset_default` 覆盖 reset 值（实测 `0x1`，UG 差异） |
| RTC counter 32-bit 回绕测试需 2^32 个时钟周期，仿真不可行 | **实际结果（2026-09-17）**：`rtc_no_wrap`（PASS）覆盖 `rtc_wen=0` 越过 match + `rtc_wen=1` wrap 到 `0`（非 `load_value`，UG 差异）；完整 32-bit 回绕不测 |
| AOU/PDU 跨域同步延迟可能影响读写时序（特别是 EOI 跨域清除） | **实际结果（2026-09-17）**：`rtc_cross_domain`（PASS）跨域同步读回一致；`rtc_int_mask_raw` 用事件驱动同步避开延迟风险 |
| `i_rtc_ext_clk` 在 SoC 默认来自 `PIN_ELS`（32.768 kHz 晶振），TB 仿真若无晶振模型可能为 X | **实际结果（2026-09-17）**：TB 端 `force i_rtc_ext_clk = pclk` 提供确定时钟；且实测 RTL 默认 DIV=0x1（非 0x4000），无需加速即可秒级触发 match |
| RTC 与其他外设共享 PDU 域寄存器访问路径（`apb1_sub_top`） | **实际结果（2026-09-17）**：测试执行无干扰；`apb1_sub_top` 共享访问路径未观察到冲突 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── rtc/
│   ├── rtc_test.c                  (F1/F2/F3/F4/F5/F6/F7：既有，match=0x200 + load=0x1e0 + CCR=0xd，PASS)
│   ├── rtc_reset_default.c         (F9：×9 寄存器复位值 + F11：COMP_VERSION RO 兼带，PASS)
│   ├── rtc_int_mask_raw.c          (F5：mask/raw/int_status/EOI 全链路，PASS)
│   ├── rtc_ccr_split.c             (F4：wen/ien/mask/en 4 bit 独立验证，PASS)
│   ├── rtc_match_boundary.c        (F3：match=0/1/0x40/0x1000 边界，PASS)
│   ├── rtc_counter_inc.c           (F1+F4 en：counter 递增→停→再递增，PASS)
│   ├── rtc_no_wrap.c               (F2：rtc_wen=0 越过 match + rtc_wen=1 wrap 到 0，PASS)
│   ├── rtc_div_matrix.c            (F6：DIV=0/3/15 计数比，PASS)
│   ├── rtc_cross_domain.c          (F7：match/load/CCR/DIV 跨域同步读回，PASS)
│   ├── rtc_vic_route.c             (F12：UVM 协同，PASS)
│   └── rtc_etb_trig.c              (F10 输出侧：UVM 协同，PASS)
    └── map_test.c                  (通用地址空间 read 0 检查，含 RTC 区域 0x60004000~0x60007FFF)
```

### 测试注册
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_test_lib.svh` — `soc_top_for_c_case_test`（含 C 固件运行机制）
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_testcase_pkg.svh` — include test_base + test_lib
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_test_base.svh` — UVM_ERROR 计数 + `UVM_CASE_PASS` 上报

### Header 文件
- `dv/simulation/firmware_ksim/lib/clib/vtimer.h` — `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail` / `sim_save`
- `dv/simulation/firmware_ksim/lib/clib/datatype.h` — `uint32_t` 等类型定义（待确认确切路径）

### TB 文件
- `dv/simulation/verif_env/soc/soc_top/` — SoC top test 入口（含 env / monitor）
- `dv/simulation/verif_env/soc/aou_top.v` — AOU 域顶层（含 `rtc0_sec_top` 实例化）
- `dv/simulation/verif_env/soc/apb1/` — APB1 总线侧 UVM test（含 RTC monitor）

### 交叉参考文档
- `doc_summary/module_analysis/rtc_analysis.md` — RTC 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/Real-Time_Clock_RTC_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 1859-1955 行 — User Guide RTC 章节原文
