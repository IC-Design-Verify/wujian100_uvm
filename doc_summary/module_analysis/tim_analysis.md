# wujian100_open SoC Timer (TIM) — 模块分析

> 本文档基于 `wujian100_open/soc/tim.v`、`tim1.v` ~ `tim7.v` 八个 RTL 源文件，以及 User Guide 第 2 节「Timer (TIM)」（userguide.txt 行 399–518）整理而成。寄存器/地址信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

wujian100_open SoC 内置 8 个 Timer IP（即 TIM0 ~ TIM7，System Overview 章节命名为 TIM×8），每个 Timer IP 内部由 **Timer1** 与 **Timer2** 两个独立 32-bit 计数器通道组成，因此 SoC 共有 16 个独立计数器通道。每个 Timer IP 通过 APB 总线与 CPU 通信，输出 ETB (Event Trigger Bus) 触发信号，并产生两路中断。

- 计数宽度：32-bit。
- 工作模式：free-running / user-defined（由 `TimerN Control Reg[1]` 选择）。
- 中断源：每个 Timer 通道 1 个中断（详见 §5），共 16 个；本 IP 在 SoC VIC 中断表中占 16 个 slot（`TIM0[1:0]` ~ `TIM7[1:0]`）。
- 触发：每个 Timer 通道对外输出 1 bit `timN_etb_trig`，受 `etb_timN_trig_en_on/off` 控制；可通过 `TimerN Control Reg[4]` 打开 hardware trigger。
- 时钟：每个 Timer IP 拥有两条独立 `timer_1_clk` / `timer_2_clk`（在 tim0_sec_top/tim_top 内部由 `pclk` 经门控生成）。
- 复位：`presetn` 全局复位，内部生成 `timer_1_resetn` / `timer_2_resetn`。
- 安全：`tim0_sec_top` ~ `tim7_sec_top` 顶层提供 `tipc_timN_trust` 输入与 `pprot[2:0]`，但 RTL 当前实现未对接 trust 逻辑（仅端口预留，见 §6）。

### 1.2 SoC 中的位置 / 总线挂载

依据 System Overview 章节 Table 1-2（Peripheral Address Map）与 `apb0_sub_top` / `apb1_sub_top` 端口列表：

| TIM IP | 文件 | 所在 APB | 基地址 | P# | 通道 |
|---|---|---|---|---|---|
| TIM0 | `tim.v` | APB0 | `0x5000_0000` | P0 | T1/T2 |
| TIM1 | `tim1.v` | APB1 | `0x6000_0000` | P0 | T1/T2 |
| TIM2 | `tim2.v` | APB0 | `0x5000_0400` | P1 | T1/T2 |
| TIM3 | `tim3.v` | APB1 | `0x6000_0400` | P1 | T1/T2 |
| TIM4 | `tim4.v` | APB0 | `0x5000_0800` | P2 | T1/T2 |
| TIM5 | `tim5.v` | APB1 | `0x6000_0800` | P2 | T1/T2 |
| TIM6 | `tim6.v` | APB0 | `0x5000_0C00` | P3 | T1/T2 |
| TIM7 | `tim7.v` | APB1 | `0x6000_0C00` | P3 | T1/T2 |

APB 桥来自 LS 总线 S2 (APB0) / S3 (APB1)，由 `ls_sub_top` 仲裁。每个 TIM IP 占 1 KB 地址空间，user guide 中每个 timer 通道 5 个寄存器 × 2 = 10 个寄存器（40 字节有效），其余高位地址读取为 0。

### 1.3 RTL 文件与实例关系

- `tim.v`：包含 `tim0_sec_top`（sec 封装顶层）+ `timers_apbif`（APB 接口与寄存器读写）+ `timers_frc`（自由计数/用户计数核心）+ `timers_top`（组合层）+ `tim_top`（tim0 的内部 RTL 顶层）。
- `tim1.v` ~ `tim7.v`：每个文件包含 `timX_sec_top`（sec 封装顶层）+ `timX_tim_top`（等同于 tim_top，但带 X 后缀），其内部直接包含一份与 tim.v 相同的 `timers_apbif` + `timers_frc` 实现。
- 8 个 sec_top 通过 `apb0_sub_top` / `apb1_sub_top` 实例化，挂到 APB0/1 总线 P0 ~ P3。
- 每个 sec_top 内部 `timX_tim_top` 实例化 1 个 `timers_apbif` 与 2 个 `timers_frc`（Timer1 / Timer2）。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次（以 TIM0 为例，其它 7 个完全同构）

```
tim0_sec_top                 (tim.v, sec 封装层：预留 tipc_tim0_trust / pprot，未实际接入)
└── tim_top  x_tim_top       (tim.v, 内部 RTL 顶层)
    ├── timers_apbif U_TIMERS_APBIF   (tim.v, APB 接口 + 寄存器读写)
    │      ├── 寄存器：Timer1LoadCount/CurrentValue/ControlReg/int_clr/IntStatus
    │      └── Timer2 5 个寄存器（同上）
    ├── timers_frc  U_TIMER0           (tim.v, Timer1 通道：32-bit 自由/用户计数器)
    └── timers_frc  U_TIMER1           (tim.v, Timer2 通道：32-bit 自由/用户计数器)
```

TIM1 ~ TIM7 结构相同，只是顶层文件名/模块名为 `timN_sec_top` / `timN_tim_top`，内部 RTL 直接复制（参数 `TIMER1_WIDTH` / `TIMER2_WIDTH` / `TIMER1_PULSE_EXTD` / `TIMER2_PULSE_EXTD` 等可独立配置）。

### 2.2 各子模块职责

| 子模块 | 位置 | 职责（结构层） |
|---|---|---|
| `timN_sec_top` | `tim.v` / `timN.v` | 顶层 wrapper：把 APB 桥传来的 `paddr/psel/penable/pwrite/pwdata/pclk/presetn`、ETB 控制信号、`tipc_timN_trust` 和 `pprot` 接入；目前仅透传到 `timN_tim_top`，未对 trust 做实际过滤。 |
| `timN_tim_top` (=`tim_top` for TIM0) | `tim.v` / `timN.v` | 内部 RTL 顶层，组合 `timers_apbif` + 2×`timers_frc`；输出 `intr[1:0]`、`prdata[31:0]`、`timN_etb_trig` 组合。 |
| `timers_apbif` | `tim.v` | APB 接口：实现 10 个寄存器（LoadCount/CurrentValue/ControlReg/int_clr/IntStatus ×2）；将 APB `paddr` 解码到对应寄存器；输出 `timer_en[1:0]`、`timer_mode[1:0]`、`timer_hwen[1:0]`、`timer1loadcount`、`timer2loadcount` 等到 `timers_frc`；从 `timers_frc` 收集 `bus_current_value[63:0]`、`bus_interrupts[1:0]` 组成 `prdata`。 |
| `timers_frc` | `tim.v` | 单通道 32-bit 计数器核心：根据 `timer_en`/`timer_mode`/`load_value`/`timerhwen` 实现 free-running / user-defined 计数；输出 `current_value`、`interrupt`、`timertrig` 等。同一模块被实例化两次（U_TIMER0 / U_TIMER1）。 |

> 内部逻辑（具体计数、toggle、reload 时序等）按要求不深入展开，详见 `tim.v` 源文件。

---

## 3. 端口列表

以 `timN_sec_top`（N = 0..7）为顶层 wrapper；以 `tim_top` / `timN_tim_top`（内部 RTL 顶层）作为功能端口参考。两层端口基本一致，`timN_sec_top` 仅多出 `pprot[2:0]` 和 `tipc_timN_trust`（见 §6 差异说明）。

### 3.1 `timN_sec_top` 端口（N=0..7，来自 RTL module 声明）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pclk` | input | 1 | APB 时钟 |
| `presetn` | input | 1 | APB 复位，低有效 |
| `psel` | input | 1 | APB slave 选择信号 |
| `penable` | input | 1 | APB 传输使能 |
| `pwrite` | input | 1 | APB 写控制（1=写） |
| `paddr` | input | 32 | APB 地址 |
| `pwdata` | input | 32 | APB 写数据 |
| `prdata` | output | 32 | APB 读数据 |
| `pprot` | input | 3 | APB 保护/特权（预留，未实际使用） |
| `tipc_timN_trust` | input | 1 | TIPC trust 信号（预留，未实际使用；N = 0..7，按实例命名） |
| `etb_tim1_trig_en_on` | input | 1 | 来自 ETB 的 timer1 trigger enable on |
| `etb_tim1_trig_en_off` | input | 1 | 来自 ETB 的 timer1 trigger enable off |
| `etb_tim2_trig_en_on` | input | 1 | 来自 ETB 的 timer2 trigger enable on |
| `etb_tim2_trig_en_off` | input | 1 | 来自 ETB 的 timer2 trigger enable off |
| `scan_mode` | input | 1 | DFT scan 模式 |
| `intr` | output | 2 | Timer1/Timer2 中断（bit0=timer1, bit1=timer2） |
| `tim1_etb_trig` | output | 1 | Timer1 输出到 ETB 的触发信号 |
| `tim2_etb_trig` | output | 1 | Timer2 输出到 ETB 的触发信号 |

### 3.2 `tim_top`（TIM0 内部 RTL 顶层）/ `timN_tim_top`（N=1..7）端口

与上表相比**仅缺 `pprot` 与 `tipc_*_trust`**，其余端口命名/方向/宽度完全一致。

---

## 4. 寄存器配置

### 4.1 Register Memory Map（User Guide Table 2-2）

每个 TIM IP 占 1 KB 地址空间，含 10 个 32-bit 对齐寄存器：

| 寄存器名 | Address Offset | Width (bits) | Access | Reset Value | Description |
|---|---|---|---|---|---|
| `Timer1LoadCount` | `0x00` | 32 | R/W | `32'b0` | Value to be loaded into Timer1 |
| `Timer1CurrentValue` | `0x04` | 32 | R | `32'b0` | Current Value of Timer1 |
| `Timer1Control Reg` | `0x08` | 4 (有效位 3, 见下) | R/W | `4'b0` | Control Register for Timer1 |
| `Timer1_int_clr` | `0x0C` | 1 (有效位 1) | R | `1'b0` | Clears the interrupt from Timer1 |
| `Timer1Int Status` | `0x10` | 1 (有效位 1) | R | `1'b0` | Contains the interrupt status for Timer1 |
| `Timer2LoadCount` | `0x14` | 32 | R/W | `32'b0` | Value to be loaded into Timer2 |
| `Timer2CurrentValue` | `0x18` | 32 | R | `32'b0` | Current Value of Timer2 |
| `Timer2Control Reg` | `0x1C` | 4 (有效位 3) | R/W | `4'b0` | Control Register for Timer2 |
| `Timer2_int_clr` | `0x20` | 1 (有效位 1) | R | `1'b0` | Clears the interrupt from Timer2 |
| `Timer2Int Status` | `0x24` | 1 (有效位 1) | R | `1'b0` | Contains the interrupt status for Timer2 |

注：`0x28` ~ `0x3FF` 为保留区间（每个 Timer IP 总 1 KB）。

### 4.2 Timer1 Load Count（Offset `0x00`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | Timer1 Load Count Register | R/W | Value to be loaded into Timer1. This is the value from which counting commences. Any value written to this register is loaded into the associated timer. Reset Value: `32'b0` |

### 4.3 Timer1 Current Value（Offset `0x04`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | Timer1 Current Value Register | R | Current Value of Timer1. Reset Value: `32'b0` |

### 4.4 Timer1 Control Reg（Offset `0x08`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:5` | Reserved | — | Reserved, read as zero |
| `4` | Timer Hardware Trigger enable | R/W | `0` = timer hardware trigger disable; `1` = timer hardware trigger enable |
| `3` | Reserved | — | Reserved, read as zero |
| `2` | Timer Interrupt Mask | R/W | `0` = timer interrupt not masked; `1` = timer interrupt masked. Reset Value: `1'b0` |
| `1` | Timer Mode Select | R/W | `0` = free-running; `1` = user-defined running. Reset Value: `1'b0` |
| `0` | Timer Enable Select | R/W | `0` = disabled; `1` = enabled. Reset Value: `1'b0` |

### 4.5 Timer1_int_clr（Offset `0x0C`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as zero |
| `0` | Timer1 int clr Register | R | Reading from this register returns all zeros (`0`) and clears the interrupt from Timer1 |

### 4.6 Timer1Int Status（Offset `0x10`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as zero |
| `0` | Timer1Int Status Register | R | Contains the interrupt status for Timer1 |

### 4.7 Timer2 寄存器组（Offset `0x14` ~ `0x24`）

字段定义与 Timer1 完全相同，仅 offset 替换：

- `Timer2LoadCount` @ `0x14`：32-bit R/W，复位 `32'b0`。
- `Timer2CurrentValue` @ `0x18`：32-bit R，复位 `32'b0`。
- `Timer2Control Reg` @ `0x1C`：4-bit R/W，复位 `4'b0`；bitfield 与 Timer1 一致。
- `Timer2_int_clr` @ `0x20`：1-bit R，复位 `1'b0`。
- `Timer2Int Status` @ `0x24`：1-bit R，复位 `1'b0`，含 Reset Value `32'b0`（即 status bit = 0）。

---

## 5. 工作流程（User Guide Work Flow 节）

User Guide 列出的 TIM 标准编程顺序：

1. **禁用并配置工作模式**：向 `TimerN Control Reg` 写入新值，确保 Timer 被禁用（bit 0 = 0）并选择工作模式（bit 1 = 0 = free-running / 1 = user-defined）。
2. **加载初始计数值**：向 `TimerN Load Count` 寄存器写入初始值。
3. **使能 Timer**：再次写 `TimerN Control Reg`，将 bit 0 置 1，启动计数；如需屏蔽中断可在写 1 前将 bit 2 置 1。
4. **硬件触发（可选）**：将 `TimerN Control Reg[4]` 置 1 启用 hardware trigger；外部 ETB 通过 `etb_timN_trig_en_on/off` 控制是否生效，并可通过 `timN_etb_trig` 把 Timer 事件回灌到 ETB。
5. **中断处理**：Timer 计数到 0 时产生中断；读取 `TimerNInt Status` 判中断源；读 `TimerN_int_clr` 自动清中断。

> 注：User Guide 仅给出 3 步基础流程；步骤 4 / 5 系根据 RTL `etb_*_trig_en_*`、`intr`、`timN_etb_trig` 端口与 §4 寄存器定义补充。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| Timer IP 数量 | "TIM (×8)"（System Overview） | `tim.v` ~ `tim7.v` 共 8 个文件 | ✅ |
| 每 IP 通道数 | "two timers" | `timers_top` / `timN_tim_top` 内部实例 2 个 `timers_frc`（U_TIMER0/U_TIMER1） | ✅ |
| 计数宽度 | "32-bit count width" | `timers_frc` `TIMER_WIDTH` 参数；`current_value` 输出 `32` bit | ✅ |
| 工作模式 | "free-running and user-defined" | `timers_frc` 接收 `timer_mode`；`timers_apbif` 暴露 `TimerN Control Reg[1]` Mode Select | ✅ |
| 中断 | 每 timer 1 个中断 | `intr[1:0]` 输出（bit0=timer1, bit1=timer2） | ✅ |
| 寄存器数量 | 10 个（每个 timer 5 个） | `timers_apbif` 中包含 10 个寄存器地址映射 | ✅ |
| 寄存器 offset | `0x00/0x04/0x08/0x0C/0x10` 与 `0x14/0x18/0x1C/0x20/0x24` | RTL 中 `paddr` 解码到上述偏移 | ✅ |
| 寄存器访问类型 | LoadCount/Control: R/W；其余 R | `timers_apbif` 输出 `timer1loadcount`/`timer2loadcount`/`timer_en`/`timer_mode`/`timer_hwen`，其余只读 | ✅ |
| Timer1 Control Reg 字段 | bit4 HW trigger en、bit2 int mask、bit1 mode、bit0 enable | `timers_apbif` 内对应字段，`timer_en`/`timer_mode`/`timer_hwen` 输出到 `timers_frc` | ✅ |
| ETB 触发 | （User Guide 未明说） | `timN_etb_trig` 输出 + `etb_timN_trig_en_on/off` 输入 | ✅（RTL 提供） |
| Timer 通道 ↔ 中断编号 | `TIM0[1:0]` ~ `TIM7[1:0]`（System Overview Table 1-4） | `intr[1:0]` 中 bit0/1 对应通道 1/2 | ✅ |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | `tipc_timN_trust` 与 `pprot[2:0]` | User Guide 未提 | `tim0_sec_top` ~ `tim7_sec_top` 顶层均有该端口 | sec_top 当前实现**未把这两个信号接到内部 `timN_tim_top`**（仅在 wrapper 中预留 wire），属于预留扩展（如未来 trustzone 接入），当前寄存器访问无 trust 过滤 | high |
| 2 | TIM IP 与外部总线关系 | User Guide "TIM (×8) connects to APB1 and APB0" | APB0 挂 TIM0/2/4/6（P0/P1/P2/P3），APB1 挂 TIM1/3/5/7 | 与 User Guide 描述一致；但 System Overview 中 TIM 总线挂载的具体分配（奇偶号分别在 APB0/1）需依赖 Peripheral Address Map 推得，User Guide 文字未明说 | medium |
| 3 | Timer1Int Status Reset Value | Table 2-7 描述中未显式给出 | user guide 表格单独行注明 "Reset Value: 32'b0"（仅 Timer2Int Status）；Timer1Int Status 在另一行只标 "Reset Value: 1'b0" | 两个状态寄存器位宽均为 1 bit（其余 bit reserved read as zero），实际有效 reset = `1'b0`，User Guide 文字差异不影响 RTL 行为 | high |
| 4 | `scan_mode` | User Guide 未提 | `timN_sec_top` / `timN_tim_top` 顶层均有 `scan_mode` 输入 | DFT 扫描模式控制 | high |
| 5 | `timer_1_clk` / `timer_2_clk` | User Guide 未提 | tim_top 内通过 `timer_1_clk`/`timer_2_clk`/`timer_1_resetn`/`timer_2_resetn` 区分两通道时钟复位 | sec_top 当前**未将 `timer_1_clk`/`timer_2_clk` 显式接入**（仅接 `pclk`/`presetn`），意味着实际两通道共享 pclk；独立时钟为内部预留接口 | medium |
| 6 | 中断 mask | "Timer Interrupt Mask" 在 Control Reg bit2 | RTL `timers_apbif` 暴露 `timer_int_mask` 类信号控制 `interrupt` 输出是否屏蔽 | 行为一致；user guide 未给出 mask 与 `intr` 输出之间的对应关系 | low |
| 7 | Hardware Trigger enable 行为 | "0 disable / 1 enable" | RTL 中 `timer_hwen` 信号送 `timers_frc` 控制 hardware trigger，与 spec 字段对齐 | ✅ 一致 |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 399–518。
- 寄存器交叉参考：`doc_summary/Timer_TIM_registers.md`（与 userguide.txt 内容一致，本文档以 userguide.txt 为准）。
- RTL 源码：`wujian100_open/soc/tim.v`、`tim1.v`、`tim2.v`、`tim3.v`、`tim4.v`、`tim5.v`、`tim6.v`、`tim7.v`。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- `tipc_timN_trust` 与 `pprot` 虽在 sec_top 端口列表出现，但 RTL 实现层未实际使用，可能对应未来安全扩展（trustzone-like 隔离）；当前所有 APB 访问均视为 trust，未做访问过滤。
- `timer_1_clk` / `timer_2_clk` / `timer_1_resetn` / `timer_2_resetn` 在 `timers_top` 内部声明，但 sec_top 未从外部接入；外部仅提供单一 `pclk` / `presetn`。两个 timer 通道的独立时钟门控能力未被使用，与 User Guide "All of timers are 32-bit count width" 的描述无直接矛盾。
- `Timer1Control Reg` Width 字段标 `4`（user guide Table 2-2），但实际有效位只有 bit[2:0]、bit[4]，bit[3] 与 bit[31:5] 均为 reserved。RTL 中该寄存器宽度由 `paddr` 译码为 32-bit 总线但仅低 5 位有意义；此处不影响软件访问。
- `TimerN_int_clr` 描述为「Reading from this register returns all zeros and clears the interrupt」——user guide 仅声明读清中断，未提及是否存在写访问；RTL `timers_apbif` 中该寄存器访问类型为只读。
- `intr` 输出在 RTL 中为 2 bit（timer1 + timer2），但 System Overview 中每个 timer 通道对应 1 个中断向量（`TIMx[1:0]` 即 2 个 slot）；`pclk` 时钟门控（`pmu_apb0_s3clk` / `pmu_apb1_s3clk`）与各 TIM IP 的 `pclk` 关联见 `apb0_sub_top` / `apb1_sub_top` 端口命名（`pmu_tim0_p0clk` ~ `pmu_tim7_p1rst_b` 等），属于 SoC 电源管理范畴。
- 8 个 TIM IP 在 `tim.v` ~ `tim7.v` 中重复实现相同的 `timers_apbif` + `timers_frc`，未走 `include` 共享，因此 RTL 文本有较多重复；每个文件参数（如 `TIMER_WIDTH`、`TIMER_PULSE_EXTD`）理论上可独立配置，本文档未深入参数配置差异分析。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.1 Memory Map 共 10 个寄存器（5 × 2 通道），offset 为 `0x00/0x04/0x08/0x0C/0x10` 与 `0x14/0x18/0x1C/0x20/0x24`，与 User Guide Table 2-2 及 `Timer_TIM_registers.md` 完全一致。
- **字段描述**：§4.2 ~ §4.7 中每个字段的位域、访问类型、复位值均与 User Guide Table 2-3 ~ 2-12 一致（Timer2 字段与 Timer1 完全镜像）。
- **端口列表**：§3.1 `timN_sec_top` 端口集合与 RTL `module timN_sec_top(...)` 声明一致；`pprot[2:0]` 与 `tipc_timN_trust` 仅出现在 sec_top 层（未在内部 `timN_tim_top` / `tim_top` 中），已在 §6 明确标注。
- **结构挂载**：§1.2 TIM 0~7 在 APB0/APB1 上的分配（T0/2/4/6 → APB0，T1/3/5/7 → APB1；P0/P1/P2/P3）与 `apb0_sub_top` / `apb1_sub_top` 实例化端口命名一致。
- **存疑项**：§6.2 与 §7.2 已逐项列出 trust/pprot 端口未使用、独立时钟未接入等差异，未在文档中掩盖。
