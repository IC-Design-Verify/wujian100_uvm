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
**已有 case**：`rtc_test.c`（既有）配置 `load=0x1e0`，等待 match 后 counter wrap 回 `0x1e0`；隐含验证 counter 递增与 wrap。
**检查**：C 端 TB 采样 `i_rtc_ext_clk` 周期数与 `RTC_current_value` 变化一致；加速手段：TB 端 `RTC_DIV=0`（不分频）或 `match_value=2` 极小值。
**缺口**：单独测试 counter 递增（不依赖 match 中断）**待新建 case（标记 TBD）**。

### F2: Load value 与 wrap 回绕
**目标**：`rtc_wen=1` 时 counter 在 match 后立即回绕到 `RTC_load_value`；`rtc_wen=0` 时继续递增到 `0xFFFFFFFF` 后归零。
**已有 case**：`rtc_test.c`（既有）配置 `CCR=0xd`（`rtc_wen=1`），match=0x200 后 wrap 到 `load=0x1e0`。
**检查**：C 端 TB 端监测 counter 在 match 后下个 cycle 是否 == `load_value`。
**缺口**：`rtc_wen=0` 不 wrap 模式**待新建 case（标记 TBD）**。

### F3: Match value 比较触发
**目标**：counter == `RTC_match_value` 时触发 `RTC_raw_int_status[0]=1`。
**已有 case**：`rtc_test.c`（既有）配置 `match=0x200` 后轮询 `RTC_int_status==1`。
**检查**：C 端轮询 `raw_int_status==1`，验证 match 时机与 match value 配置一致。
**缺口**：多个不同 match value 边界（0/1/0xFFFFFFFF/0x7FFFFFFF）**待新建 case（标记 TBD）**。

### F4: CCR 控制位（rtc_wen/Rtc_en/rtc_mask/rtc_ien）
**目标**：4 个 1-bit 控制独立生效。
- `rtc_wen`：wrap 使能
- `Rtc_en`：counter 使能（disable 时 counter 暂停）
- `rtc_mask`：中断屏蔽
- `rtc_ien`：中断总开关

**已有 case**：`rtc_test.c`（既有）配置 `CCR=0xd`（`rtc_wen=1` + `Rtc_en=1` + `rtc_ien=1` + `rtc_mask=0`）。
**检查**：C 端分别测试 4 个 bit 独立功能：例如 `Rtc_en=0` 后 counter 停止递增；`rtc_mask=1` 后 `int_status` 屏蔽但 `raw_int_status` 仍置位；`rtc_ien=0` 后任何中断都屏蔽。
**缺口**：4 个 bit 独立功能验证**待新建 case（标记 TBD）**。

### F5: 中断状态/原始状态/清除（EOI）
**目标**：`raw_int_status` 反映未 mask 中断；`int_status` 反映 mask 后中断；`int_clr` 读清中断。
**已有 case**：`rtc_test.c`（既有）轮询 `int_status==1` → 读 `int_clr` → 等待 `int_status==0`。
**检查**：C 端 match 触发后读 `raw_int_status==1` 与 `int_status==1`（mask=0 时相等）；读 `int_clr` 后两者均 == 0。
**缺口**：mask=1 时 `raw != int_status` 行为**待新建 case（标记 TBD）**。

### F6: 时钟分频（RTC_DIV）
**目标**：`RTC_DIV` 决定 `i_rtc_ext_clk` 分频；reset = `0x4000` (= 16384)。
**已有 case**：`rtc_test.c`（既有）使用默认 `RTC_DIV`（未显式写）。
**检查**：C 端 TB 端测量 `match` 触发周期 = `(match - load + 1) × (DIV + 1)` 个 `i_rtc_ext_clk` 周期；写 `DIV=0` 时 counter 速度最快。
**缺口**：不同 DIV 值（0/1/0xFFFF/0x4000）**待新建 case（标记 TBD）**。

### F7: AOU/PDU 跨域同步
**目标**：CPU 在 PDU 域写入 CR/DIV/MR/CLR 寄存器后同步到 AOU 域 counter；PDU 域读 `int_clr` 后同步清 AOU 域中断。
**已有 case**：`rtc_test.c`（既有）隐含使用 PDU 域写入路径。
**检查**：TB 端跨域采样 `aou_pdu_*` / `pdu_aou_*` 信号验证握手协议；写入 `CCR=disable` 后 AOU 域 counter 应立即停止。
**缺口**：跨域信号时序验证**待新建 case（标记 TBD）**，需 TB 侧跨域 monitor。

### F8: 低功耗下保持计数
**目标**：AOU 域 RTC counter 在主电源关闭时仍保留运行；CPU 端读 `current_value` 应能看到递增。
**已有 case**：无（既有 c_case 未覆盖低功耗场景）。
**检查**：TB 端 force PDU 域断电（pdu_pwr_good=0），验证 AOU 域 `i_rtc_ext_clk` 仍在运行、counter 继续递增；恢复 PDU 域后读 `current_value` 应继续递增。
**缺口**：**待新建 case（标记 TBD）**，需 TB 侧 PDU 电源控制 agent。

### F9: 寄存器复位值
**目标**：复位后 9 个寄存器回到 §1.2 reset 值（多数 `0x0`，`RTC_DIV=0x4000`，`RTC_CCR=0x0`）。
**已有 case**：无（既有 c_case 未做复位后初始状态校验）。
**检查**：`aortc_rst_n` 释放后立即读 9 个寄存器，校验 reset 值。
**缺口**：**待新建 case（标记 TBD）**。

### F10: ETB 触发
**目标**：1 路 `etb_rtc_trig` 输入触发 RTC counter 行为；1 路 `rtc_etb_trig` 输出送其它外设。
**已有 case**：无。
**检查**：TB 端 ETB monitor 采样 `rtc_etb_trig` 输出时序；ETB agent 驱动 `etb_rtc_trig` 后 counter 行为变化。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 ETB agent。

### F11: COMP_VERSION 只读
**目标**：`RTC_COMP_VERSION` 为 32-bit 组件版本寄存器，写无效（RO）。
**已有 case**：无。
**检查**：C 端读 COMP_VERSION 应返回非零 RTL 编码；写后读不变。
**缺口**：**待新建 case（标记 TBD）**。

### F12: 中断号路由（VIC 中断号 26）
**目标**：`rtc0_vic_intr` 经 SoC VIC 路由到 `cpu_intr[26]` = `RTC`。
**已有 case**：无（C 端无法直接验证中断号，需 UVM 端 VIC monitor）。
**检查**：UVM 侧打开 `rtc0_vic_intr` monitor，验证 `cpu_intr[26]` 上升沿匹配。
**缺口**：**待新建 case（标记 TBD）**，依赖 SoC VIC monitor。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `rtc_test`（既有 `c_case/rtc/rtc_test.c`） | `soc_top_for_c_case_test` | F1 (隐含), F2 (wrap), F3 (match), F4 (CCR 隐含), F5 (status/clear) | C 端基础 |
| 2 | `rtc_counter_inc`（TBD） | UVM 侧 | F1 (独立 counter 递增验证) | UVM 采样 |
| 3 | `rtc_no_wrap`（TBD） | UVM 侧 | F2 (`rtc_wen=0` 不 wrap) | UVM 采样 |
| 4 | `rtc_match_boundary`（TBD） | `soc_top_for_c_case_test` | F3 (match value 边界) | C 端 + TB |
| 5 | `rtc_ccr_split`（TBD） | `soc_top_for_c_case_test` | F4 (4 个 CCR bit 独立验证) | C 端 |
| 6 | `rtc_int_mask_raw`（TBD） | `soc_top_for_c_case_test` | F5 (mask=1 时 raw != int) | C 端 |
| 7 | `rtc_div_matrix`（TBD） | UVM 侧 | F6 (DIV 0/1/0xFFFF/0x4000) | UVM 时序 |
| 8 | `rtc_cross_domain`（TBD） | UVM 侧 | F7 (跨域握手时序) | UVM 跨域 monitor |
| 9 | `rtc_low_power`（TBD） | UVM 侧 | F8 (PDU 断电保持) | UVM PDU 电源控制 |
| 10 | `rtc_reset_default`（TBD） | `soc_top_for_c_case_test` | F9 | C 端复位检查 |
| 11 | `rtc_etb`（TBD） | UVM 侧 | F10 | UVM ETB agent |
| 12 | `rtc_comp_version`（TBD） | `soc_top_for_c_case_test` | F11 | C 端 |
| 13 | `rtc_vic_route`（TBD） | UVM 侧 | F12 | UVM VIC monitor |

### 功能覆盖矩阵

| Feature | rtc_test | rtc_counter_inc | rtc_no_wrap | rtc_match_boundary | rtc_ccr_split | rtc_int_mask_raw | rtc_div_matrix | rtc_cross_domain | rtc_low_power | rtc_reset_default | rtc_etb | rtc_comp_version | rtc_vic_route |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: counter 递增 | ✓ (隐含) | ✓ | - | - | - | - | - | - | ✓ | - | - | - | - |
| F2: Load + wrap | ✓ (wrap) | - | ✓ (no-wrap) | - | - | - | - | - | - | - | - | - | - |
| F3: match 比较 | ✓ | - | - | ✓ (×N) | - | - | - | - | - | - | - | - | - |
| F4: CCR 4 bit | ✓ (3 bit) | - | - | - | ✓ (×4) | ✓ | - | - | - | - | - | - | - |
| F5: int status/clear | ✓ | - | - | - | - | ✓ | - | - | - | - | - | - | - |
| F6: 时钟分频 DIV | ✓ (默认) | - | - | - | - | - | ✓ (×N) | - | - | ✓ | - | - | - |
| F7: AOU/PDU 跨域 | ✓ (隐含) | - | - | - | - | - | - | ✓ | - | - | - | - | - |
| F8: 低功耗保持 | - | - | - | - | - | - | - | - | ✓ | - | - | - | - |
| F9: 复位值 | - | - | - | - | - | - | - | - | - | ✓ (×9) | - | ✓ | - |
| F10: ETB 触发 | - | - | - | - | - | - | - | - | - | - | ✓ | - | - |
| F11: COMP_VERSION | - | - | - | - | - | - | - | - | - | ✓ (RO) | - | ✓ | - |
| F12: VIC 中断号 26 | - | - | - | - | - | - | - | - | - | - | - | - | ✓ |

> 矩阵用 ✓/- 标记。"TBD" 表示待新建 case，不阻塞既有 rtc_test 通过但属于覆盖缺口。

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

本项目无独立 Python `def_test` 注册表，RTC 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/rtc/rtc_test.c` 决定具体行为。后续 TBD 用例沿用同一入口，通过修改 `c_case/rtc/` 下不同 .c 文件选择。

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
- **RTC 专用 monitor（TBD）**：未来新增 UVM 侧 case 时，需在 `soc_top_env` 内增加：
  - **i_rtc_ext_clk 加速 agent**：TB 端用高速时钟（替代 32.768 kHz 慢速晶振）驱动 RTC counter。
  - **跨域 monitor**：采样 `aou_pdu_*` / `pdu_aou_*` 信号验证握手协议。
  - **PDU 电源控制 agent**：force/release PDU 域 `pdu_pwr_good`。
  - **ETB monitor**：采样 `rtc_etb_trig` 输出 + `etb_rtc_trig` 输入。
  - **VIC monitor**：采样 `cpu_intr[26]` 上升沿。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `rtc_test`（既有） | `RTC_int_status == 0x1`（match 中断触发）+ 读 `int_clr` 清中断 + `printf("\nrtc test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `rtc_counter_inc` (TBD) | TB 测量 counter 递增速率与 `i_rtc_ext_clk` 分频一致 |
| `rtc_no_wrap` (TBD) | `rtc_wen=0` 时 counter 继续递增到 `0xFFFFFFFF` 后归零 |
| `rtc_match_boundary` (TBD) | match value 0/1/0x7FFFFFFF/0xFFFFFFFF 边界值触发正确 |
| `rtc_ccr_split` (TBD) | 4 个 CCR bit 独立行为正确（wrap/enable/mask/ien） |
| `rtc_int_mask_raw` (TBD) | mask=1 时 `raw_int_status=1` 但 `int_status=0` |
| `rtc_div_matrix` (TBD) | match 周期 = `(match - load + 1) × (DIV + 1) × ext_clk_period` |
| `rtc_cross_domain` (TBD) | PDU 域写入 CR 后 AOU 域 counter 行为变化 |
| `rtc_low_power` (TBD) | PDU 断电期间 counter 继续递增；恢复后读 current_value 持续增加 |
| `rtc_reset_default` (TBD) | 复位后 9 个寄存器值与 §1.2 reset 表一致 |
| `rtc_etb` (TBD) | `rtc_etb_trig` 输出时序与 RTC 事件匹配 |
| `rtc_comp_version` (TBD) | COMP_VERSION 读非零 RTL 编码；写后不变 |
| `rtc_vic_route` (TBD) | `rtc0_vic_intr` 上升沿时 `cpu_intr[26]` 匹配 |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划（含加速策略）

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 rtc_test                       (~5 min)   既有 C 端基本功能（含默认 DIV=0x4000）
3. 仿真 rtc_reset_default              (~5 min)   TBD case 1（9 个寄存器复位值）
4. 仿真 rtc_comp_version               (~5 min)   TBD case 2（COMP_VERSION）
5. 仿真 rtc_int_mask_raw               (~10 min)  TBD case 3（mask=1 时 raw/int 不一致）
6. 仿真 rtc_ccr_split                  (~15 min)  TBD case 4（4 个 CCR bit 独立验证）
7. 仿真 rtc_match_boundary             (~15 min)  TBD case 5（match value 边界）
8. 仿真 rtc_counter_inc                (~10 min)  TBD UVM case 6（counter 独立递增）
9. 仿真 rtc_no_wrap                    (~15 min)  TBD UVM case 7（rtc_wen=0 不 wrap）
10. 仿真 rtc_div_matrix                (~20 min)  TBD UVM case 8（DIV 多档位 + 加速）
11. 仿真 rtc_cross_domain              (~15 min)  TBD UVM case 9（跨域握手）
12. 仿真 rtc_low_power                 (~20 min)  TBD UVM case 10（PDU 断电保持）
13. 仿真 rtc_etb                       (~10 min)  TBD UVM case 11（ETB 触发）
14. 仿真 rtc_vic_route                 (~10 min)  TBD UVM case 12（VIC 中断号 26）
```

预估总时间：~160-200 min（既有 case ~5 min + 12 个 TBD case ~155-195 min）

### 加速策略（关键）

**问题**：默认 `RTC_DIV=0x4000` (= 16384) + 32.768 kHz 外部时钟 → counter 速度极慢（每秒仅 ~2 次计数）。match=0x200 时需要 ~131 秒才能触发中断。

**加速手段**（按优先级）：
1. **TB 端替换 `i_rtc_ext_clk`**：用 pclk 域高速时钟（如 pclk 直接）驱动 RTC，绕过 SoC 低速振荡器。RTL 需保证 `i_rtc_ext_clk` 可被 TB force。
2. **TB 端 force `RTC_DIV=0`**：写最小分频（不分频），但需 AOU 域允许 PDU 域写入（已有路径）。
3. **使用小 match value**：如 match=0x10（16 次计数）+ DIV=0 → ~16 个 ext_clk 周期后触发。
4. **既有用例模式**：保持 match=0x200 + DIV=0x4000（既有 `rtc_test.c` 已用），但接受 ~131 秒等待；适用于非加速场景。
5. **wrap 测试用更长 match**：避免单次测试中 wrap 多次触发。

> **建议**：TBD case 在 TB 端使用加速手段 1+2 组合（高速 ext_clk + DIV=0）；既有 `rtc_test.c` 保持原样以验证"默认复位配置可工作"。

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| RTC 默认 `DIV=0x4000` 导致 counter 速度极慢，测试时间长 | TB 端替换 `i_rtc_ext_clk` 为高速时钟 + force `RTC_DIV=0`；详见 §6 加速策略 |
| AOU/PDU 跨域测试依赖 TB 侧跨域 monitor 与跨域时钟域切换 | 短期仅做寄存器读写 + 功能验证；完整跨域时序验证需 UVM 侧 `rtc_cross_domain` (TBD) |
| 低功耗测试需 TB 侧 PDU 电源控制 agent | 短期仅做"PDU 域断电期间 counter 仍递增"的功能验证；完整 PDU 域断电/恢复流程待 `rtc_low_power` (TBD) |
| COMP_VERSION 具体值待 RTL 确认（不同 RTL 版本可能不同） | 测试仅验证 RO 属性 + 写无效，不硬编码具体值 |
| 中断号 26 = `RTC` 来自 System Overview Table 1-4；具体行号 / 编号以文档最新版本为准 | TB 侧硬编码中断号 26；后续以 doc_review 修复后版本对齐 |
| `rtc_test.c` 既有 case 使用默认 `DIV=0x4000` 隐含 RTC_DIV reset value 行为，但未做完整边界测试 | 边界测试由 `rtc_div_matrix` (TBD) 补齐；不阻塞既有 case 回归 |
| RTC counter 32-bit 回绕测试需 2^32 个时钟周期，仿真不可行 | 不测完整 32-bit 回绕；仅测 `rtc_wen=0` 时从 `match_value` 递增到 `0xFFFFFFFF` 后归零（用较小 match value + TB 加速） |
| AOU/PDU 跨域同步延迟可能影响读写时序（特别是 `RTC_int_clr` 跨域清除） | TB 端采样 `pdu_aou_int_clr_sync` 信号验证握手完成；测试代码需在 `int_clr` 读后增加适当延迟 |
| `i_rtc_ext_clk` 在 SoC 默认来自 `PIN_ELS`（32.768 kHz 晶振），TB 仿真若无晶振模型可能为 X | TB 端 `force i_rtc_ext_clk = pclk` 提供确定时钟 |
| RTC 与其他外设共享 PDU 域寄存器访问路径（`apb1_sub_top`） | 测试需独立执行，避免其他外设同时操作 PDU 域寄存器 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── rtc/
│   └── rtc_test.c                  (F1, F2, F3, F4, F5：既有，match=0x200 + load=0x1e0 + CCR=0xd)
├── rtc/                            (TBD 新增)
│   ├── rtc_reset_default.c         (F9：9 个寄存器复位值)
│   ├── rtc_comp_version.c          (F11)
│   ├── rtc_int_mask_raw.c          (F5：mask=1 时 raw/int 不一致)
│   ├── rtc_ccr_split.c             (F4：4 个 CCR bit 独立验证)
│   └── rtc_match_boundary.c        (F3：match value 边界)
└── addr_map/
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
