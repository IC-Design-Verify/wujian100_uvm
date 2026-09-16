# wujian100_open SoC Real-Time Clock (RTC) — 模块分析

> 本文档基于 `wujian100_open/soc/rtc.v` 单一 RTL 源文件（含 `rtc0_sec_top` / `rtc_aou_apbif` / `rtc_aou_top` / `rtc_cdr_sync` / `rtc_clk_div` / `rtc_clr_sync` / `rtc_cnt` / `rtc_ig` / `rtc_pdu_apbif` / `rtc_pdu_top` 子模块），以及 User Guide 第 7 节「Real-Time Clock (RTC)」（userguide.txt 行 1859–1955）整理而成。寄存器信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

RTC（Real-Time Clock）是 SoC 的实时时钟计数器，提供：

- **32-bit 递增计数器**（`RTC_current_value`，只读）。
- **匹配中断**：当 counter == `RTC_match_value` 时触发中断（可 mask）。
- **加载初值**：`RTC_load_value` 写入后作为下次重新启动 counter 的初值（也用作 wrap 时的回绕值）。
- **Wrap 模式**：`RTC_CCR.rtc_wen` 决定计数器在匹配时是立即 wrap（重新装载 `RTC_load_value`）还是继续递增到最大值。
- **控制位**：`RTC_CCR` 提供 `rtc_wen` / `Rtc_en` / `rtc_mask` / `rtc_ien` 4 个 1-bit 控制。
- **中断**：`RTC_int_status`（masked 后状态）+ `RTC_raw_int_status`（未 mask 原始状态）+ `RTC_int_clr`（读清中断）。
- **组件版本**：`RTC_COMP_VERSION`（只读 32-bit 版本寄存器）。
- **时钟分频**：`RTC_DIV`（20-bit）对 RTC 输入时钟分频；reset = `0x4000`（即 16384 分频）。
- **ETB 触发**：1 路 `rtc_etb_trig` 输出 + 1 路 `etb_rtc_trig` 输入。
- **外部 RTC 时钟**：`i_rtc_ext_clk`（通常来自低速振荡器 `PIN_ELS`）。
- **低功耗/AOU 域特性**：RTC 位于 Always-On（`aou_top`）子系统中，在主电源关闭时仍可保留运行；并具备 PDU 域镜像寄存器供主域 CPU 读访问。

### 1.2 SoC 中的位置 / 总线挂载

依据 System Overview 章节 Peripheral Address Map：

- **RTC 寄存器基地址**：`0x6000_4000` ~ `0x6000_7FFF`（16 KB，APB1 子映射 P6）。
- 总线接入：APB1（P6）→ LS AHB (`apb1_sub_top` 的 S3) → MAIN AHB 总线矩阵。
- **RTL 挂载方式特殊**：RTC 的 APB 接口 (`aortc_paddr/psel/penable/pwrite/pwdata/prdata/rst_n/pprot`) 不直接挂到 `apb1_sub_top`，而是通过 `aou_top` 中的 `rtc0_sec_top` 直接接入 AOU 域；CPU 对 RTC 的访问在 AOU 域内部完成。详见 §2 结构。
- **AOU 域与 PDU 域镜像**：`rtc0_sec_top` 内部同时实例化 `rtc_aou_top`（AOU 域，独立 RTC 时钟运行）与 `rtc_pdu_top`（PDU 域镜像，把 CPU 在 PDU 域的寄存器写入同步到 AOU 域）。
- **中断**：1 bit `rtc0_vic_intr` 送 CLIC/VIC；在 System Overview Table 1-4 中占 1 个 slot（`RTC`，中断号 26）。
- **外部 RTC 时钟源**：`i_rtc_ext_clk` 来自 SoC 的低速振荡器（`PIN_ELS`，即 32.768 kHz 晶振）；`rtc_clk` 在 PDU 域是 APB 时钟（pclk 域）。

### 1.3 RTL 文件与实例关系

- `rtc.v` 包含完整 RTC 实现，由 `rtc0_sec_top`（AOU 域顶层）包裹两个子模块：
    - `rtc_aou_top`：AOU 域 RTC 主体（含 counter、中断、APB slave 等）。
    - `rtc_pdu_top`：PDU 域镜像 wrapper（提供 CPU 访问 RTC 寄存器的 APB 接口，把 PDU 域寄存器写入同步到 AOU 域）。
- `aou_top.v:573` 实例化 `rtc0_sec_top`（`x_rtc0_sec_top`），把它作为 AOU 子系统的成员；PDU 域寄存器访问通过 `apb1_sub_top` 接到 `rtc0_vic_intr`/`aortc_*` 信号。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次

```
rtc0_sec_top                 (rtc.v:21, AOU 域顶层 wrapper：预留 tipc_rtc0_trust / pprot，未实际接入)
├── rtc_aou_top  x_rtc_aou_top   (rtc.v:241, AOU 域 RTC 主体：counter + APB slave + interrupt + ETB)
│       ├── rtc_aou_apbif   (rtc.v:135, AOU 域 APB 接口解码 + 寄存器读写)
│       ├── rtc_cnt         (rtc.v:564, 32-bit 计数器)
│       ├── rtc_clk_div     (rtc.v:431, RTC 时钟分频)
│       ├── rtc_ig          (rtc.v:619, 中断产生与状态)
│       ├── rtc_cdr_sync    (rtc.v:371, PDU→AOU 跨域寄存器同步)
│       └── rtc_clr_sync    (rtc.v:481, AOU→PDU 跨域中断清除同步)
└── rtc_pdu_top  x_rtc_pdu_top   (rtc.v:1014, PDU 域镜像 wrapper：CPU 经 PDU 域 APB 访问 RTC 寄存器)
        └── rtc_pdu_apbif   (rtc.v:700, PDU 域 APB 接口)
```

### 2.2 各子模块职责（结构层）

| 子模块 | 位置 | 职责 |
|---|---|---|
| `rtc0_sec_top` | `rtc.v:21` | AOU 域顶层 wrapper：把 AOU 域 APB 信号（`aortc_*`）、ETB 触发、故障输入接入；预留 `tipc_rtc0_trust` / `aortc_pprot`；协调 `rtc_aou_top` 与 `rtc_pdu_top` 的跨域信号。 |
| `rtc_aou_top` | `rtc.v:241` | AOU 域 RTC 主体：集成 APB slave、32-bit counter、时钟分频、中断产生、ETB 触发；接收 `i_rtc_ext_clk`（如 32.768 kHz）作为 RTC counter 时钟源。 |
| `rtc_aou_apbif` | `rtc.v:135` | AOU 域 APB 接口：实现 9 个寄存器（见 §4）的 R/W 控制；在 AOU 域内独立响应 CPU 访问。 |
| `rtc_pdu_top` | `rtc.v:1014` | PDU 域镜像 wrapper：CPU 在 PDU 域访问 RTC 寄存器的入口；通过 `aou_pdu_*` 与 `pdu_aou_*` 跨域信号同步到 AOU 域。 |
| `rtc_pdu_apbif` | `rtc.v:700` | PDU 域 APB 接口：CPU 经 PDU 域的 pclk 域访问 RTC 寄存器；产生跨域写入 `pdu_aou_wen_*`。 |
| `rtc_cnt` | `rtc.v:564` | 32-bit 计数器：根据 `RTC_load_value`/`RTC_match_value`/`RTC_CCR.rtc_wen`/`Rtc_en` 工作。 |
| `rtc_clk_div` | `rtc.v:431` | RTC 时钟分频器：根据 `RTC_DIV` 寄存器产生分频时钟。 |
| `rtc_ig` | `rtc.v:619` | 中断产生：mask/ien 控制；产生 `RTC_int_status` 与 `RTC_raw_int_status`。 |
| `rtc_cdr_sync` | `rtc.v:371` | PDU → AOU 跨域寄存器同步（CPU 在 PDU 域写入的 CR/DIV/MR/CLR 同步到 AOU 域 counter）。 |
| `rtc_clr_sync` | `rtc.v:481` | AOU → PDU 跨域中断清除同步（PDU 域读 `RTC_int_clr` 后同步清 AOU 域中断）。 |

> 内部 counter 逻辑、跨域握手协议等具体逻辑按要求不深入展开，详见 `rtc.v` 源文件。

---

## 3. 端口列表

### 3.1 `rtc0_sec_top` 顶层端口（来自 `rtc.v:21`）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `aortc_pclk` | input | 1 | AOU 域 APB 时钟 |
| `aortc_rst_n` | input | 1 | AOU 域 APB 复位（低有效） |
| `aortc_paddr` | input | 32 | AOU 域 APB 地址 |
| `aortc_psel` | input | 1 | AOU 域 APB 选择 |
| `aortc_penable` | input | 1 | AOU 域 APB 传输使能 |
| `aortc_pwrite` | input | 1 | AOU 域 APB 写控制 |
| `aortc_pwdata` | input | 32 | AOU 域 APB 写数据 |
| `aortc_prdata` | output | 32 | AOU 域 APB 读数据 |
| `aortc_pprot` | input | 3 | APB 保护/特权（预留，未实际使用） |
| `tipc_rtc0_trust` | input | 1 | TIPC trust 信号（预留，未实际使用） |
| `i_rtc_ext_clk` | input | 1 | 外部 RTC 时钟（来自低速振荡器） |
| `etb_rtc_trig` | input | 1 | 来自 ETB 的 RTC 触发 |
| `rtc_etb_trig` | output | 1 | 送 ETB 的 RTC 触发 |
| `rtc0_vic_intr` | output | 1 | RTC 中断（送 CLIC/VIC） |
| `test_mode` | input | 1 | DFT test mode |

### 3.2 `rtc_pdu_top` 内部端口（来自 `rtc.v:1014`，作为 PDU 域镜像 wrapper 端口参考）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pclk` / `presetn` | input | 1 | PDU 域 APB 时钟与复位 |
| `paddr` / `psel` / `penable` / `pwrite` / `pwdata` | input | 32/1 | PDU 域 APB 控制信号 |
| `prdata` | output | 32 | PDU 域 APB 读数据 |
| `rtc_clk` | input | 1 | RTC 时钟（PDU 域时钟，与 AOU 域不同） |
| `test_mode` | input | 1 | DFT test mode |
| `aou_pdu_cnt[31:0]` | input | 32 | AOU 域 → PDU 域 counter 当前值 |
| `aou_pdu_cr_reg[3:0]` | input | 4 | AOU 域 → PDU 域 CCR 寄存器值 |
| `aou_pdu_div_reg[19:0]` | input | 20 | AOU 域 → PDU 域 DIV 寄存器值 |
| `aou_pdu_intr_mask` | input | 1 | AOU 域 → PDU 域 中断 mask |
| `aou_pdu_mr_reg[31:0]` | input | 32 | AOU 域 → PDU 域 MR 寄存器值 |
| `int_flag` | input | 1 | AOU 域 → PDU 域 中断标志 |
| `pdu_aou_clr_reg[31:0]` | output | 32 | PDU 域 → AOU 域 CLR 寄存器写入值 |
| `pdu_aou_int_clr` | output | 1 | PDU 域 → AOU 域 中断清除请求 |
| `pdu_aou_wen_clr_sync` | output | 1 | PDU 域 → AOU 域 CLR 寄存器写使能同步 |
| `pdu_aou_wen_cr` | output | 1 | PDU 域 → AOU 域 CR 寄存器写使能 |
| `pdu_aou_wen_div` | output | 1 | PDU 域 → AOU 域 DIV 寄存器写使能 |
| `pdu_aou_wen_mr` | output | 1 | PDU 域 → AOU 域 MR 寄存器写使能 |

注：`rtc_pdu_top` 不是 SoC 总线顶层，CPU 不直接通过 APB 总线访问 `rtc_pdu_top`；它仅作为 `rtc0_sec_top` 内部 PDU 域镜像 wrapper，与 AOU 域 RTC 共享同一份寄存器布局。

### 3.3 `rtc_aou_top` 内部端口（来自 `rtc.v:241`，作为 AOU 域 RTC 主体端口参考）

包含 PDU/AOU 跨域信号：`aou_pdu_cnt`/`aou_pdu_cr_reg`/`aou_pdu_div_reg`/`aou_pdu_intr_mask`/`aou_pdu_mr_reg`/`pdu_aou_clr_reg`/`pdu_aou_int_clr`/`pdu_aou_pwdata`/`pdu_aou_wen_clr_sync`/`pdu_aou_wen_cr`/`pdu_aou_wen_div`/`pdu_aou_wen_mr`/`rtc_clk`/`rtc_por_rst_n`/`int_flag`/`etb_rtc_trig`/`rtc_etb_trig`/`i_rtc_ext_clk`/`pclk`/`rtc0_vic_intr`/`test_mode`。

---

## 4. 寄存器配置

### 4.1 Register Memory Map（User Guide Table 7-1）

| Name | Address Offset | Width | Access | Reset Value | Description |
|---|---|---|---|---|---|
| `RTC_current_value` | `0x00` | 32 | R | `0x0` | Current Counter Value Register |
| `RTC_match_value` | `0x04` | 32 | R/W | `0x0` | Counter Match Register |
| `RTC_load_value` | `0x08` | 32 | R/W | `0x0` | Counter Load Register |
| `RTC_CCR` | `0x0C` | 4 (有效位) | R/W | `0x0` | Counter Control Register |
| `RTC_int_status` | `0x10` | 32 (1-bit 有效) | R | `0x0` | Interrupt Status Register |
| `RTC_raw_int_status` | `0x14` | 32 (1-bit 有效) | R | `0x0` | Interrupt Raw Status Register |
| `RTC_int_clr` | `0x18` | 32 (1-bit 有效) | R | `0x0` | End of Interrupt Register |
| `RTC_COMP_VERSION` | `0x1C` | 32 | R | `0x0` | Component Version Register |
| `RTC_DIV` | `0x20` | 20 (有效位) | R/W | `0x4000` | RTC clock divider value |

注：`0x24` 及以后保留。

### 4.2 `RTC_current_value`（Offset `0x00`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | Current Counter Value | R | 32-bit 内部 counter 当前值，读取保持一致。Reset: `0x0` |

### 4.3 `RTC_match_value`（Offset `0x04`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | Counter Match | R/W | 匹配值寄存器；当 counter 与之匹配时产生中断（需 `rtc_ien=1` 且 `rtc_mask=0`）。仅当 4 字节全部写入后才生效。Reset: `0x0` |

### 4.4 `RTC_load_value`（Offset `0x08`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | Counter Load | R/W | 加载到 counter 的初值（coherently written）。Reset: `0x0` |

### 4.5 `RTC_CCR` — Counter Control Register（Offset `0x0C`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:4` | Reserved | — | Reserved, read as 0 |
| `3` | `rtc_wen` | R/W | Wrap 控制：`0` = 匹配时不立即 wrap（继续到 max）；`1` = 匹配时立即 wrap 回 `RTC_load_value`。Reset: `0x0` |
| `2` | `Rtc_en` | R/W | counter 使能：`0` = disabled; `1` = enabled. Reset: `0x0` |
| `1` | `rtc_mask` | R/W | interrupt mask：`0` = unmasked; `1` = masked. Reset: `0x0` |
| `0` | `rtc_ien` | R/W | interrupt enable：`0` = disabled; `1` = enabled. Reset: `0x0` |

注：`RTC_CCR` 整体 reset = `0x0`（4 个 bit 全 0）。

### 4.6 `RTC_int_status` — Interrupt Status Register（Offset `0x10`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as 0 |
| `0` | `rtc_stat` | R | Masked 后中断状态：`0` = inactive; `1` = active. Reset: `0x0` |

### 4.7 `RTC_raw_int_status` — Interrupt Raw Status Register（Offset `0x14`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as 0 |
| `0` | `rtc_rstat` | R | 原始中断状态（未 mask）：`0` = inactive; `1` = active. Reset: `0x0` |

### 4.8 `RTC_int_clr` — End of Interrupt Register（Offset `0x18`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as 0 |
| `0` | `rtc_int_clr` | R | 读此寄存器清除匹配中断；读完成时（end of read）清除中断。Reset: `0x0` |

### 4.9 `RTC_COMP_VERSION`（Offset `0x1C`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | Component Version | R | 组件版本号（只读）。Reset: `0x0` |

### 4.10 `RTC_DIV`（Offset `0x20`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:20` | Reserved | — | Reserved, read as 0 |
| `19:0` | `rtc_div` | R/W | RTC 时钟分频值（基于此寄存器的分频）。Reset: `0x4000` (=16384) |

---

## 5. 工作流程（User Guide Work Flow）

User Guide 给出的标准编程顺序：

1. **禁用 RTC**：写 `RTC_CCR` 复位值（`0x0`），先禁用 RTC。
2. **设置匹配值**：写 `RTC_match_value`（4 字节对齐写入）。
3. **设置加载值**：写 `RTC_load_value`。
4. **配置 CCR 并启用**：写 `RTC_CCR` 设定 `rtc_wen`/`Rtc_en`/`rtc_mask`/`rtc_ien`。
5. **等待中断**：当 counter 递增到 `RTC_match_value` 时，若 `rtc_ien=1` 且 `rtc_mask=0`，则 `rtc_stat` 与 `rtc_rstat` 置 1，`rtc0_vic_intr` 触发。
6. **清除中断**：读 `RTC_int_clr`（end-of-read 清中断）。
7. **可选读当前值**：任何时候可读 `RTC_current_value` 获取 counter 当前状态。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| 实例数 | "RTC connects to APB" (System Overview) | `rtc.v` 单文件 + `rtc0_sec_top` 单实例 | ✅ |
| 寄存器数量 | 9 个 | `rtc_aou_apbif` / `rtc_pdu_apbif` 内部 reg 实现 9 个寄存器 | ✅ |
| 寄存器 offset | `0x00` ~ `0x20`，间隔 4 字节（`0x0C` 之后跳 `0x10`） | RTL offset 解码一致 | ✅ |
| `RTC_CCR` 字段 | `[3] rtc_wen` / `[2] Rtc_en` / `[1] rtc_mask` / `[0] rtc_ien` | `rtc_aou_apbif` / `rtc_pdu_apbif` 中 `cr`/`ccr` reg 字段一致 | ✅ |
| `RTC_DIV` reset | `0x4000`（20-bit 有效） | RTL `rtc_div` 寄存器 reset = `0x4000` | ✅ |
| `RTC_current_value` 宽度 | 32-bit | RTL counter 32-bit | ✅ |
| `RTC_match_value` 匹配中断 | "When the internal counter matches this register, an interrupt is generated" | RTL `rtc_ig` 匹配逻辑 | ✅ |
| `rtc_int_clr` 读清 | "Performing read-to-clear on interrupts, the interrupt is cleared at the end of the read" | RTL 实现读完成清中断 | ✅ |
| 中断输出 | 1 bit `rtc0_vic_intr` | sec_top 输出 1 bit | ✅ |
| ETB 触发 | 1 in + 1 out | `etb_rtc_trig` + `rtc_etb_trig` | ✅ |
| 外部 RTC 时钟 | `i_rtc_ext_clk` | sec_top 输入 1 bit | ✅ |
| 跨域镜像 | — | `rtc_pdu_top` ↔ `rtc_aou_top` 通过 `aou_pdu_*` / `pdu_aou_*` 跨域信号握手 | ✅ |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | sec_top 的 `aortc_pprot` / `tipc_rtc0_trust` | User Guide 未提 | `rtc0_sec_top` 顶层预留 2 个端口 | 当前 RTL 仅透传 `aortc_pclk`/`aortc_rst_n`，未对接 trust 逻辑 | high |
| 2 | `RTC_CCR` Width | Table 7-1 标 "4~2bits" | RTL 寄存器总线宽度 32-bit，有效位仅 `[3:0]` | spec 文本不规范：实际有效位宽 4 bit | low |
| 3 | `RTC_int_status` / `RTC_raw_int_status` / `RTC_int_clr` Width | Table 7-1 标 `32` | RTL 32-bit 总线，有效位 `[ `[0]`，其余 reserved | 与 spec 一致 | high |
| 4 | `RTC_COMP_VERSION` | spec 仅说 "Component Version Register"，未给字段编码 | RTL 32-bit R | spec 描述笼统 | medium |
| 5 | `RTC_DIV` Width | Table 7-1 标 `20` | RTL `div` reg 32-bit 总线，有效位 `[19:0]` | 与 spec 一致 | high |
| 6 | `aou_pdu_cnt` 等跨域信号位宽 | spec 未提 | RTL 中 `aou_pdu_cnt[31:0]` / `aou_pdu_cr_reg[3:0]` / `aou_pdu_div_reg[19:0]` / `aou_pdu_mr_reg[31:0]` | 跨域信号带宽合理 | high |
| 7 | PDU 域寄存器访问 | spec 未提及 | RTL 提供 `rtc_pdu_top` + `rtc_pdu_apbif`，CPU 经 PDU 域 APB 写入 → 跨域同步到 AOU 域 | RTL 实现细节，spec 未明确描述跨域 | medium |
| 8 | Wrap 行为 | `rtc_wen=1` → "wrap when a match occurs instead of waiting until the maximum count is reached" | RTL `rtc_cnt` 实现 wrap | ✅ | high |
| 9 | 中断 mask 优先级 | spec 表述"rtc_mask=1 mask interrupt"，但 `rtc_int_clr` 行为描述未明确是否受 mask 影响 | RTL 中 `rtc_ig` 模块结合 mask + ien + clr 处理 | 与 spec 一致 | medium |
| 10 | `RTC_COMP_VERSION` 默认值 | spec Reset Value 列写 `0x0` | RTL 寄存器 reset 行为 `0x0` | ✅ | high |
| 11 | `RTC_DIV` default = `0x4000` 含义 | "Rtc clock divider value" | 20-bit，16384 分频；具体对应 32.768 kHz 输入到 1 Hz counter | spec 描述笼统，分频计算需结合 `rtc_clk_div` 实现确认 | low |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 1859–1955。
- 寄存器交叉参考：`doc_summary/Real-Time_Clock_RTC_registers.md`（与 userguide.txt 内容一致）。
- RTL 源码：`wujian100_open/soc/rtc.v`（含 `rtc0_sec_top` / `rtc_aou_apbif` / `rtc_aou_top` / `rtc_cdr_sync` / `rtc_clk_div` / `rtc_clr_sync` / `rtc_cnt` / `rtc_ig` / `rtc_pdu_apbif` / `rtc_pdu_top`）。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- `tipc_rtc0_trust` 与 `aortc_pprot` 端口预留但未实际使用，可能对应未来 trustzone 接入。
- `RTC_CCR` Width 字段在 spec 中标 "4~2bits"（含义模糊），实际 RTL 实现为 32-bit 总线 + 4-bit 有效位。
- `RTC_COMP_VERSION` 的版本号格式/含义在 spec 中未给出；RTL 实现为 32-bit R，reset = 0；实际版本号内容由具体 IP 版本决定（当前 RTL 可能 hardcode 为固定值）。
- PDU 域镜像 wrapper `rtc_pdu_top` 的引入是 SoC 设计中的细节（CPU 经 PDU 域访问 RTC 寄存器时，需经过跨域同步）；User Guide 未明确描述跨域握手协议，建议结合 `rtc_cdr_sync` / `rtc_clr_sync` 实现进一步分析。
- `RTC_DIV` 默认 `0x4000`（16384）分频，推测配合 32.768 kHz 外部晶振得到 1 Hz counter；具体分频公式需结合 `rtc_clk_div` 实际实现确认。
- `rtc0_sec_top` 的 APB slave 接口 (`aortc_*`) 与 `apb1_sub_top` 的 P6 接入关系：`aou_top.v:573` 把 `rtc0_sec_top` 实例化为 AOU 子系统成员；`apb1_sub_top` 内部存在 `apb1_rtc_psel_s6` 等端口（见 `apb1_sub_top.v` 与 `aou_top.v:573-577`），CPU 对 RTC 的访问通过 `apb1_sub_top` 的 `apb1_rtc_psel_s6` 路由到 `aou_top` 内的 `rtc0_sec_top` 的 APB 端口。
- `rtc0_vic_intr` 中断接到 `core_top`/`aou_top` 的中断网络，具体路由需在 CLIC/VIC 章节进一步分析。
- `etb_rtc_trig` / `rtc_etb_trig` 的具体触发源/目标未在 spec 中定义，推测为 RTC 匹配事件触发其它外设（如或定时）。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.1 Memory Map 共 9 个寄存器（`RTC_current_value` / `RTC_match_value` / `RTC_load_value` / `RTC_CCR` / `RTC_int_status` / `RTC_raw_int_status` / `RTC_int_clr` / `RTC_COMP_VERSION` / `RTC_DIV`，offset `0x00`/`0x04`/`0x08`/`0x0C`/`0x10`/`0x14`/`0x18`/`0x1C`/`0x20`），与 User Guide Table 7-1 及 `Real-Time_Clock_RTC_registers.md` 完全一致。
- **字段描述**：§4.2 ~ §4.10 中每个字段的位域、访问类型、复位值均与 User Guide Tables 7-2 ~ 7-9 一致；`RTC_CCR` 4 个控制位、`RTC_DIV` reset = `0x4000`、匹配中断读清等关键行为已记录。
- **端口列表**：§3.1 `rtc0_sec_top` 端口集合（15 个：AOU 域 APB 9 + trust 预留 2 + ext_clk + ETB in + ETB out + 中断 + test_mode）与 RTL `module rtc0_sec_top(...);` 声明逐项核对一致；§3.2 / §3.3 内部 `rtc_pdu_top` / `rtc_aou_top` 端口集合亦与 RTL 一致。
- **结构挂载**：§1.2 中 RTC 在 APB1 P6（`0x6000_4000`~`0x6000_7FFF`）的分配与 Peripheral Address Map 一致；RTC 通过 AOU 域 `rtc0_sec_top` + PDU 域镜像 `rtc_pdu_top` 的双域架构实现，已在 §2 中明确。
- **存疑项**：§6.2 与 §7.2 已逐项列出 trust/pprot 预留、`RTC_CCR` Width 文本疑误、`RTC_COMP_VERSION` 版本含义、跨域同步细节、`RTC_DIV` 默认值含义等差异，未在文档中掩盖。
