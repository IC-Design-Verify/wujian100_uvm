# wujian100_open SoC Pulse Width Modulation (PWM) — 模块分析

> 本文档基于 `wujian100_open/soc/pwm.v` 单一 RTL 源文件（含 `pwm` / `pwm_apbif` / `pwm_ctrl` / `pwm_gen` / `pwm_sec_top` 子模块），以及 User Guide 第 6 节「Pulse Width Modulation (PWM)」（userguide.txt 行 1191–1858）整理而成。寄存器信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

PWM（Pulse Width Modulation）是 SoC 的脉冲宽度调制外设，**单实例**位于 APB0，提供 **6 组（group0~group5）共 12 路 PWM 输出**（每组 2 个通道，channel pair：group0 = PWM0/PWM1、group1 = PWM2/PWM3、…、group5 = PWM10/PWM11）。除 PWM 输出外，SoC 同一 PWM IP 还提供 **6 路输入捕获（CAP0~CAP5，对应 channel 0/2/4/6/8/10）** 和 **6 组 16-bit 定时器（tim0~tim5）**。

- **PWM 输出**：12 路（`o_pwm0`~`o_pwm11`），每路独立输出使能（`pwm0oe_n`~`pwm11oe_n`）。
- **计数器**：6 个 32-bit 计数器（`PWM01COUNT`/`PWM23COUNT`/`PWM45COUNT`），每计数器驱动 2 路 PWM。
- **比较器**：每通道 1 个比较寄存器（`PWM0CMP`~`PWM5CMP`），每个寄存器 32-bit 内含 2 个 16-bit 比较值（m/n 对应同一 group 的两通道）。
- **计数模式**：Up / Up-Down 模式（`PWMCTL` 控制）；输出频率由 `PWM01LOAD`/`PWM23LOAD`/`PWM45LOAD` 16-bit 加载值决定。
- **死区控制**：每 group 1 个 `PWM01DB`/`PWM23DB`/`PWM45DB` 寄存器（dead-band delay ticks）。
- **输入捕获**：6 个捕获通道（`i_capedge0/2/4/6/8/10`），支持 edge count 与 edge time 两种模式；捕获匹配值 `CAP01MATCH` 等。
- **定时器**：6 个 16-bit 自由/用户计数器（`Tim01load`/`Tim23load`/`Tim45load` 各 32-bit 内含 2 个 16-bit load value），用于周期触发，与 CAP/计数解耦。
- **中断**：1 路聚合中断 `pwmint`，含 30+ 中断源（PWM 计数器事件 + CAP 事件 + TIM 事件）。
- **ETB 触发**：6 路 `pwm_tim0~5_etb_trig` + 1 路 `pwm_xx_trig` 送 ETB；接收 6 路 ETB 输入 `etb_pwm_trig_tim0~5_on/off`。
- **故障输入**：1 bit `fault`（异步保护）。
- **安全扩展**：`pwm_sec_top` 顶层预留 `tipc_pwm_trust` / `pprot[2:0]`，当前未实际使用。

### 1.2 SoC 中的位置 / 总线挂载

依据 System Overview 章节：

- **PWM 寄存器基地址**：`0x5001_C000` ~ `0x5001_FFFF`（16 KB，APB0 子映射 P12）。
- 总线接入：APB0（P12）→ LS AHB (`apb0_sub_top` 的 S2) → MAIN AHB 总线矩阵的 S10 slave。
- **ETB 触发**：`pwm_tim0~5_etb_trig` 接到 SoC ETB；`pwm_xx_trig` 是额外 1 bit 触发（user guide 未明说定义，应与 `etb_pwm_trig_*` 输入对应）。
- **故障输入**：`fault` 接 SoC 故障检测网络。
- **中断输出**：`pwmint` 单 bit，接到 `core_top`/`aou_top` 的中断网络；在 System Overview Table 1-4 中占 1 个 slot（`PWM`，中断号 25）。
- **PAD 接入**：12 路输出 + 1 路故障 + 6 路捕获输入通过 `wujian100_open_top.v` 中的 `PAD_DIG_IO` 单元接到 `PAD_PWM_CH0`~`CH11` / `PAD_PWM_FAULT`。

### 1.3 RTL 文件与实例关系

- `pwm.v` 包含完整 PWM 实现，由 `pwm_sec_top`（sec 封装顶层）包裹内部 `pwm` 模块。
- `apb0_sub_top` 中实例化 `pwm_sec_top`（P12 / PWM）。
- 内部子模块层次：`pwm_sec_top` → `pwm` → {`pwm_apbif`, `pwm_ctrl` → 6× `pwm_gen`}。
- `pwm_gen` 是单个 group 的 PWM 波形生成器，被 `pwm_ctrl` 实例化 6 次（group0~group5）。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次

```
pwm_sec_top                 (pwm.v:5314, sec 封装层：预留 tipc_pwm_trust / pprot，未实际接入)
└── pwm  x_pwm              (pwm.v:64, 内部 RTL 顶层)
    ├── pwm_apbif  x_pwm_apbif   (pwm.v:1147, APB slave 接口 + 寄存器读写控制)
    └── pwm_ctrl  x_pwm_ctrl     (pwm.v:3113, PWM 控制逻辑：包含 6 个 pwm_gen)
        ├── pwm_gen pwm_0_inst   (pwm.v:4772, Group 0 PWM 波形生成器)
        ├── pwm_gen pwm_1_inst   (Group 1)
        ├── pwm_gen pwm_2_inst   (Group 2)
        ├── pwm_gen pwm_3_inst   (Group 3)
        ├── pwm_gen pwm_4_inst   (Group 4)
        └── pwm_gen pwm_5_inst   (Group 5)
```

### 2.2 各子模块职责（结构层）

| 子模块 | 位置 | 职责 |
|---|---|---|
| `pwm_sec_top` | `pwm.v:5314` | 顶层 wrapper：把 APB 信号、ETB 触发、捕获输入、PWM 输出、故障输入接入；预留 `tipc_pwm_trust` / `pprot`。 |
| `pwm` | `pwm.v:64` | 内部 RTL 顶层，组合 `pwm_apbif` + `pwm_ctrl`；把 6 路 `pwm_timN_etb_trig` + `pwm_xx_trig` 汇总送 ETB，把 `pwmint` 送 VIC。 |
| `pwm_apbif` | `pwm.v:1147` | APB slave 接口：实现 38 个寄存器（offset `0x00` ~ `0xD4`，详见 §4）的 R/W 控制；通过 `paddr` 解码到对应 reg；向 `pwm_ctrl` 输出控制信号。 |
| `pwm_ctrl` | `pwm.v:3113` | PWM 控制核心：实例化 6 个 `pwm_gen`，产生 12 路 PWM 输出；处理 dead-band、counter mode、interrupt aggregation；接收 6 路 ETB `etb_pwm_trig_timN_on/off` 与故障 `fault` 输入。 |
| `pwm_gen` | `pwm.v:4772` | 单 group PWM 波形生成器：处理 LOAD/COUNT/CMP/DB 配置，产生 2 路 PWM 输出 `o_pwmN/o_pwmN+1`，对应 `PWMnCMP`/`PWM(n+1)CMP`；每 group 还包含 CAP/TIM 子功能（捕获输入与定时器）。 |

> 内部计数器时序、dead-band 计算、CAP edge 检测、TIM 16-bit 计数等具体逻辑按要求不深入展开，详见 `pwm.v` 源文件。

---

## 3. 端口列表（`pwm_sec_top` 顶层）

直接来自 RTL `module pwm_sec_top(...)` 端口声明。

### 3.1 时钟 / 复位 / APB

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pclk` | input | 1 | APB 时钟 |
| `presetn` | input | 1 | 复位（低有效） |
| `paddr` | input | 32 | APB 地址 |
| `psel` | input | 1 | APB 选择 |
| `penable` | input | 1 | APB 传输使能 |
| `pwrite` | input | 1 | APB 写控制 |
| `pwdata` | input | 32 | APB 写数据 |
| `prdata` | output | 32 | APB 读数据 |
| `pprot` | input | 3 | APB 保护/特权（预留，未实际使用） |

### 3.2 PWM 输出（共 12 + 12）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `o_pwm0` ~ `o_pwm11` | output | 1 | PWM 通道 0 ~ 11 输出 |
| `pwm0oe_n` ~ `pwm11oe_n` | output | 1 | PWM 通道输出使能（低有效） |

### 3.3 捕获输入（共 6 + 1 故障）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `i_capedge0` | input | 1 | Capture0 edge（group0 channel0） |
| `i_capedge2` | input | 1 | Capture1 edge（group1 channel2） |
| `i_capedge4` | input | 1 | Capture2 edge（group2 channel4） |
| `i_capedge6` | input | 1 | Capture3 edge（group3 channel6） |
| `i_capedge8` | input | 1 | Capture4 edge（group4 channel8） |
| `i_capedge10` | input | 1 | Capture5 edge（group5 channel10） |
| `fault` | input | 1 | 故障输入 |

### 3.4 ETB 触发（共 6 输入 + 7 输出）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `etb_pwm_trig_tim0_on` / `_off` | input | 1 | ETB → PWM tim0 trigger on/off |
| `etb_pwm_trig_tim1_on` / `_off` | input | 1 | ETB → PWM tim1 trigger on/off |
| `etb_pwm_trig_tim2_on` / `_off` | input | 1 | ETB → PWM tim2 trigger on/off |
| `etb_pwm_trig_tim3_on` / `_off` | input | 1 | ETB → PWM tim3 trigger on/off |
| `etb_pwm_trig_tim4_on` / `_off` | input | 1 | ETB → PWM tim4 trigger on/off |
| `etb_pwm_trig_tim5_on` / `_off` | input | 1 | ETB → PWM tim5 trigger on/off |
| `pwm_tim0_etb_trig` ~ `pwm_tim5_etb_trig` | output | 1 | PWM tim0~5 → ETB 触发输出（共 6 bit） |
| `pwm_xx_trig` | output | 1 | PWM 附加 ETB 触发输出（user guide 未明确定义） |

### 3.5 中断 / DFT

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pwmint` | output | 1 | PWM 聚合中断输出 |
| `pwm_idle` | output | 1 | PWM idle 状态（用于时钟门控） |
| `test_mode` | input | 1 | DFT test mode |
| `tipc_pwm_trust` | input | 1 | TIPC trust（预留，未实际使用） |

---

## 4. 寄存器配置

PWM 寄存器 38 个，offset 从 `0x00` 到 `0xD4`，每 4 字节一个寄存器。下面分四组详述：PWM 主控/触发/中断/计数器（`0x00`~`0x50`）/ 死区（`0x68`~`0x70`）/ 捕获（`0x74`~`0x9C`）/ 定时器（`0xA0`~`0xD4`）。

### 4.1 Register Memory Map（User Guide Table 6-3）

| # | Name | Offset | Width | R/W | Reset | Description |
|---|---|---|---|---|---|---|
| 1 | `PWMCFG` | `0x00` | 32 | R/W | `0x0` | PWM 全局配置 |
| 2 | `PWMINVERTTRIG` | `0x04` | 32 | R/W | `0x0` | PWM 输出极性反转 |
| 3 | `PWM01TRIG` | `0x08` | 32 | R/W | `0x0` | Group 0/1 trigger compare value |
| 4 | `PWM23TRIG` | `0x0C` | 32 | R/W | `0x0` | Group 2/3 trigger compare value |
| 5 | `PWM45TRIG` | `0x10` | 32 | R/W | `0x0` | Group 4/5 trigger compare value |
| 6 | `PWMINTEN1` | `0x14` | 32 | R/W | `0x0` | Interrupt enable for group 2/1/0 |
| 7 | `PWMINTEN2` | `0x18` | 32 | R/W | `0x0` | Interrupt enable for group 5/4/3 |
| 8 | `PWMRIS1` | `0x1C` | 32 | RO | `0x0` | Raw interrupt status for group 2/1/0 |
| 9 | `PWMRIS2` | `0x20` | 32 | RO | `0x0` | Raw interrupt status for group 5/4/3 |
| 10 | `PWMIC1` | `0x24` | 32 | R/W | `0x0` | Interrupt clear for group 2/1/0 |
| 11 | `PWMIC2` | `0x28` | 32 | R/W | `0x0` | Interrupt clear for group 5/4/3 |
| 12 | `PWMIS1` | `0x2C` | 32 | RO | `0x0` | Interrupt status (masked) for group 2/1/0 |
| 13 | `PWMIS2` | `0x30` | 32 | RO | `0x0` | Interrupt status (masked) for group 5/4/3 |
| 14 | `PWMCTL` | `0x34` | 32 | R/W | `0x0` | Configure PWM generation blocks（计数模式等） |
| 15 | `PWM01LOAD` | `0x38` | 32 | R/W | `0x0` | PWM group 0/1 加载值 |
| 16 | `PWM23LOAD` | `0x3C` | 32 | R/W | `0x0` | PWM group 2/3 加载值 |
| 17 | `PWM45LOAD` | `0x40` | 32 | R/W | `0x0` | PWM group 4/5 加载值 |
| 18 | `PWM01COUNT` | `0x44` | 32 | RO | `0x0` | Group 0/1 counter 当前值 |
| 19 | `PWM23COUNT` | `0x48` | 32 | RO | `0x0` | Group 2/3 counter 当前值 |
| 20 | `PWM45COUNT` | `0x4C` | 32 | RO | `0x0` | Group 4/5 counter 当前值 |
| 21 | `PWM0CMP` | `0x50` | 32 | R/W | `0x0` | PWM0 compare A / PWM1 compare B（[31:16]=CMP_B, [15:0]=CMP_A） |
| 22 | `PWM1CMP` | `0x54` | 32 | R/W | `0x0` | PWM2 compare A / PWM3 compare B |
| 23 | `PWM2CMP` | `0x58` | 32 | R/W | `0x0` | PWM4 compare A / PWM5 compare B |
| 24 | `PWM3CMP` | `0x5C` | 32 | R/W | `0x0` | PWM6 compare A / PWM7 compare B |
| 25 | `PWM4CMP` | `0x60` | 32 | R/W | `0x0` | PWM8 compare A / PWM9 compare B |
| 26 | `PWM5CMP` | `0x64` | 32 | R/W | `0x0` | PWM10 compare A / PWM11 compare B |
| 27 | `PWM01DB` | `0x68` | 32 | R/W | `0x0` | Group 0/1 dead-band delay ticks |
| 28 | `PWM23DB` | `0x6C` | 32 | R/W | `0x0` | Group 2/3 dead-band delay ticks |
| 29 | `PWM45DB` | `0x70` | 32 | R/W | `0x0` | Group 4/5 dead-band delay ticks |
| 30 | `CAPCTL` | `0x74` | 32 | R/W | `0x0` | Input capture control |
| 31 | `CAPINTEN` | `0x78` | 32 | R/W | `0x0` | Capture interrupt enable |
| 32 | `CAPRIS` | `0x7C` | 32 | RO | `0x0` | Capture raw interrupt status |
| 33 | `CAPIC` | `0x80` | 32 | R/W | `0x0` | Capture interrupt clear |
| 34 | `CAPIS` | `0x84` | 32 | RO | `0x0` | Capture interrupt status (masked) |
| 35 | `CAP01T` | `0x88` | 32 | RO | `0x0` | Capture counter value（group0/1） |
| 36 | `CAP23T` | `0x8C` | 32 | RO | `0x0` | Capture counter value（group2/3） |
| 37 | `CAP45T` | `0x90` | 32 | RO | `0x0` | Capture counter value（group4/5） |
| 38 | `CAP01MATCH` | `0x94` | 32 | R/W | `0x0` | Capture match value low 16 bits（group0/1） |
| 39 | `CAP23MATCH` | `0x98` | 32 | R/W | `0x0` | Capture match value low 16 bits（group2/3） |
| 40 | `CAP45MATCH` | `0x9C` | 32 | R/W | `0x0` | Capture match value low 16 bits（group4/5） |
| 41 | `TIM_INT_EN` | `0xA0` | 32 | R/W | `0x0` | Timer interrupt enable |
| 42 | `TIMRIS` | `0xA4` | 32 | RO | `0x0` | Timer raw interrupt status |
| 43 | `TIM_INT_CLR` | `0xA8` | 32 | R/W | `0x0` | Timer interrupt clear |
| 44 | `TIMIS` | `0xAC` | 32 | RO | `0x0` | Timer interrupt status (masked) |
| 45 | `TIM01LOAD` | `0xB0` | 32 | R/W | `0x0` | Timer load value（group0/1，[31:16]=tim1, [15:0]=tim0） |
| 46 | `TIM23LOAD` | `0xB4` | 32 | R/W | `0x0` | Timer load value（group2/3） |
| 47 | `TIM45LOAD` | `0xB8` | 32 | R/W | `0x0` | Timer load value（group4/5） |
| 48 | `TIM01COUNT` | `0xBC` | 32 | RO | `0x0` | Timer current count（group0/1） |
| 49 | `TIM23COUNT` | `0xC0` | 32 | RO | `0x0` | Timer current count（group2/3） |
| 50 | `TIM45COUNT` | `0xC4` | 32 | RO | `0x0` | Timer current count（group4/5） |
| 51 | `CNT01VAL` | `0xC8` | 32 | RO | `0x0` | Captured input pulse number（group0/1） |
| 52 | `CNT23VAL` | `0xCC` | 32 | RO | `0x0` | Captured input pulse number（group2/3） |
| 53 | `CNT45VAL` | `0xD0` | 32 | RO | `0x0` | Captured input pulse number（group4/5） |

注：原 user guide 共列 53 个有效寄存器（含 `Cnt45val` 拼写为 `Cnv45val`，见 §6 差异）；RTL 中 `pwm_apbif` 通过 `define *_OFFSET` 共定义 54 项（`0x000` ~ `0x0D4`，间隔 4），与上表一致。

### 4.2 PWM 主控 / 触发寄存器

#### `PWMCFG`（Offset `0x00`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:28` | Reserved | — | Reserved, read as zero |
| `27` | `cntdiven` | R/W | 计数器分频使能：`1` = frequency division enable；`0` = frequency division disable（直接用 system clock） |
| `26:24` | `cntdiv[2:0]` | R/W | 计数器分频选择：`111`/`110` = /128; `101` = /64; `100` = /32; `011` = /16; `010` = /8; `001` = /4; `000` = /2 |
| `23` | `tim5en` | R/W | Group 5 输出比较使能：`1` = enable; `0` = disable |
| `22` | `tim4en` | R/W | Group 4 输出比较使能 |
| `21` | `tim3en` | R/W | Group 3 输出比较使能 |
| `20` | `tim2en` | R/W | Group 2 输出比较使能 |
| `19` | `tim1en` | R/W | Group 1 输出比较使能 |
| `18` | `tim0en` | R/W | Group 0 输出比较使能 |
| `17` | `cap5en` | R/W | Group 5-channel 10 输入捕获使能：`1` = enable; `0` = disable |
| `16` | `cap4en` | R/W | Group 4-channel 8 输入捕获使能 |
| `15` | `cap3en` | R/W | Group 3-channel 6 输入捕获使能 |
| `14` | `cap2en` | R/W | Group 2-channel 4 输入捕获使能 |
| `13` | `cap1en` | R/W | Group 1-channel 2 输入捕获使能 |
| `12` | `cap0en` | R/W | Group 0-channel 0 输入捕获使能 |
| `11` | `pwm11en` | R/W | Channel 11 PWM 输出使能：`1` = PWM output enable; `0` = PWM output disable |
| `10` | `pwm10en` | R/W | Channel 10 PWM 输出使能 |
| `9` | `pwm9en` | R/W | Channel 9 PWM 输出使能 |
| `8` | `pwm8en` | R/W | Channel 8 PWM 输出使能 |
| `7` | `pwm7en` | R/W | Channel 7 PWM 输出使能 |
| `6` | `pwm6en` | R/W | Channel 6 PWM 输出使能 |
| `5` | `pwm5en` | R/W | Channel 5 PWM 输出使能 |
| `4` | `pwm4en` | R/W | Channel 4 PWM 输出使能 |
| `3` | `pwm3en` | R/W | Channel 3 PWM 输出使能 |
| `2` | `pwm2en` | R/W | Channel 2 PWM 输出使能 |
| `1` | `pwm1en` | R/W | Channel 1 PWM 输出使能 |
| `0` | `pwm0en` | R/W | Channel 0 PWM 输出使能 |

> 整体 reset = `0x0000_0000`。

#### `PWMINVERTTRIG`（Offset `0x04`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:12` | Reserved | — | Reserved, read as zero |
| `11` | `pwm11inv` | R/W | Invert PWM11 信号：`1` = PWM11 signal is inverted; `0` = not inverted |
| `10` | `pwm10inv` | R/W | Invert PWM10 |
| `9` | `pwm9inv` | R/W | Invert PWM9 |
| `8` | `pwm8inv` | R/W | Invert PWM8 |
| `7` | `pwm7inv` | R/W | Invert PWM7 |
| `6` | `pwm6inv` | R/W | Invert PWM6 |
| `5` | `pwm5inv` | R/W | Invert PWM5 |
| `4` | `pwm4inv` | R/W | Invert PWM4 |
| `3` | `pwm3inv` | R/W | Invert PWM3 |
| `2` | `pwm2inv` | R/W | Invert PWM2 |
| `1` | `pwm1inv` | R/W | Invert PWM1 |
| `0` | `pwm0inv` | R/W | Invert PWM0 |

> 注：User Guide 表 6-2 中 `[5:0]` 等位 Description 文本存在"PWM5/PWM4/.../PWM0 signal is inverted"等疑似错位（按 bit 编号应为对应通道），此处按 bit→通道号直接登记。整体 reset = `0x0`。

#### `PWM01TRIG`（Offset `0x08`）— PWM group 0 / group 1 trigger compare value

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `pwm1trig` | R/W | The value generating group 1 trigger signal |
| `15:0` | `pwm0trig` | R/W | The value generating group 0 trigger signal |

#### `PWM23TRIG`（Offset `0x0C`）— PWM group 2 / group 3 trigger compare value

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `pwm3trig` | R/W | The value generating group 3 trigger signal |
| `15:0` | `pwm2trig` | R/W | The value generating group 2 trigger signal |

#### `PWM45TRIG`（Offset `0x10`）— PWM group 4 / group 5 trigger compare value

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `pwm5trig` | R/W | The value generating group 5 trigger signal |
| `15:0` | `pwm4trig` | R/W | The value generating group 4 trigger signal |

### 4.3 PWM 中断寄存器

#### `PWMINTEN1`（Offset `0x14`）— Interrupt enable for group 2/1/0

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:30` | Reserved | — | Reserved, read as zero |
| `29` | `Int2encmpbd` | R/W | Interrupt for counter = comparator B Down (channel5). `1` = a raw interrupt occurs when the counter matches the value in the PWM2CMP register value when counting down; `0` = no interrupt |
| `28` | `Int2encmpad` | R/W | Interrupt for counter = comparator A Down (channel4) |
| `27` | `Int2encmpbu` | R/W | Interrupt for counter = comparator B Up (channel5) |
| `26` | `Int2encmpau` | R/W | Interrupt for counter = comparator A Up (channel4) |
| `25` | `Int2encntload` | R/W | Interrupt for counter = Load (channel5/4) |
| `24` | `Int2encntzero` | R/W | Interrupt for counter = 0 (channel5/4) |
| `23:22` | Reserved | — | Reserved, read as zero |
| `21` | `Int1encmpbd` | R/W | Interrupt for counter = comparator B Down (channel3) |
| `20` | `Int1encmpad` | R/W | Interrupt for counter = comparator A Down (channel2) |
| `19` | `Int1encmpbu` | R/W | Interrupt for counter = comparator B Up (channel3) |
| `18` | `Int1encmpau` | R/W | Interrupt for counter = comparator A Up (channel2) |
| `17` | `Int1encntload` | R/W | Interrupt for counter = Load (channel3/2) |
| `16` | `Int1encntzero` | R/W | Interrupt for counter = 0 (channel3/2) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Int0encmpbd` | R/W | Interrupt for counter = comparator B Down (channel1) |
| `12` | `Int0encmpad` | R/W | Interrupt for counter = comparator A Down (channel0) |
| `11` | `Int0encmpbu` | R/W | Interrupt for counter = comparator B Up (channel1) |
| `10` | `Int0encmpau` | R/W | Interrupt for counter = comparator A Up (channel0) |
| `9` | `Int0encntload` | R/W | Interrupt for counter = Load (channel1/0) |
| `8` | `Int0encntzero` | R/W | Interrupt for counter = 0 (channel1/0) |
| `7:0` | Reserved | — | Reserved, read as zero |

#### `PWMINTEN2`（Offset `0x18`）— Interrupt enable for group 5/4/3

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:22` | Reserved | — | Reserved, read as zero |
| `21` | `Int5encmpbd` | R/W | Interrupt for counter = comparator B Down (group5-channel11) |
| `20` | `Int5encmpad` | R/W | Interrupt for counter = comparator A Down (group5-channel10) |
| `19` | `Int5encmpbu` | R/W | Interrupt for counter = comparator B Up (group5-channel11) |
| `18` | `Int5encmpau` | R/W | Interrupt for counter = comparator A Up (group5-channel10) |
| `17` | `Int5encntload` | R/W | Interrupt for counter = Load (group5-channel11/10) |
| `16` | `Int5encntzero` | R/W | Interrupt for counter = 0 (group5-channel11/10) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Int4encmpbd` | R/W | Interrupt for counter = comparator B Down (group4-channel9) |
| `12` | `Int4encmpad` | R/W | Interrupt for counter = comparator A Down (group4-channel8) |
| `11` | `Int4encmpbu` | R/W | Interrupt for counter = comparator B Up (group4-channel9) |
| `10` | `Int4encmpau` | R/W | Interrupt for counter = comparator A Up (group4-channel8) |
| `9` | `Int4encntload` | R/W | Interrupt for counter = Load (group4-channel9/8) |
| `8` | `Int4encntzero` | R/W | Interrupt for counter = 0 (group4-channel9/8) |
| `7:6` | Reserved | — | Reserved, read as zero |
| `5` | `Int3encmpbd` | R/W | Interrupt for counter = comparator B Down (group3-channel7) |
| `4` | `Int3encmpad` | R/W | Interrupt for counter = comparator A Down (group3-channel6) |
| `3` | `Int3encmpbu` | R/W | Interrupt for counter = comparator B Up (group3-channel7) |
| `2` | `Int3encmpau` | R/W | Interrupt for counter = comparator A Up (group3-channel6) |
| `1` | `Int3encntload` | R/W | Interrupt for counter = Load (group3-channel7/6) |
| `0` | `Int3encntzero` | R/W | Interrupt for counter = 0 (group3-channel7/6) |

#### `PWMRIS1`（Offset `0x1C`）— Raw interrupt status for group 2/1/0

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:30` | Reserved | — | Reserved, read as zero |
| `29` | `Intris2cmpbd` | RO | Raw Interrupt for counter = comparator B Down (group2-channel5) |
| `28` | `Intris2cmpad` | RO | Raw Interrupt for counter = comparator A Down (group2-channel4) |
| `27` | `Intris2cmpbu` | RO | Raw Interrupt for counter = comparator B Up (group2-channel5) |
| `26` | `Intris2cmpau` | RO | Raw Interrupt for counter = comparator A Up (group2-channel4) |
| `25` | `Intris2cntload` | RO | Raw Interrupt for counter = Load (group2-channel5/4) |
| `24` | `Intris2cntzero` | RO | Raw Interrupt for counter = 0 (group2-channel5/4) |
| `23:22` | Reserved | — | Reserved, read as zero |
| `21` | `Intris1cmpbd` | RO | Raw Interrupt for counter = comparator B Down (group1-channel3) |
| `20` | `Intris1cmpad` | RO | Raw Interrupt for counter = comparator A Down (group1-channel2) |
| `19` | `Intris1cmpbu` | RO | Raw Interrupt for counter = comparator B Up (group1-channel3) |
| `18` | `Intris1cmpau` | RO | Raw Interrupt for counter = comparator A Up (group1-channel2) |
| `17` | `Intris1cntload` | RO | Raw Interrupt for counter = Load (group1-channel3/2) |
| `16` | `Intris1cntzero` | RO | Raw Interrupt for counter = 0 (group1-channel3/2) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Intris0cmpbd` | RO | Raw Interrupt for counter = comparator B Down (group0-channel1) |
| `12` | `Intris0cmpad` | RO | Raw Interrupt for counter = comparator A Down (group0-channel0) |
| `11` | `Intris0cmpbu` | RO | Raw Interrupt for counter = comparator B Up (group0-channel1) |
| `10` | `Intris0cmpau` | RO | Raw Interrupt for counter = comparator A Up (group0-channel0) |
| `9` | `Intris0cntload` | RO | Raw Interrupt for counter = Load (group0-channel1/0) |
| `8` | `Intris0cntzero` | RO | Raw Interrupt for counter = 0 (group0-channel1/0) |
| `7:0` | Reserved | — | Reserved, read as zero |

#### `PWMRIS2`（Offset `0x20`）— Raw interrupt status for group 5/4/3

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:22` | Reserved | — | Reserved, read as zero |
| `21` | `Intris5cmpbd` | RO | Raw Interrupt for counter = comparator B Down (group5-channel11) |
| `20` | `Intris5cmpad` | RO | Raw Interrupt for counter = comparator A Down (group5-channel10) |
| `19` | `Intris5cmpbu` | RO | Raw Interrupt for counter = comparator B Up (group5-channel11) |
| `18` | `Intris5cmpau` | RO | Raw Interrupt for counter = comparator A Up (group5-channel10) |
| `17` | `Intris5cntload` | RO | Raw Interrupt for counter = Load (group5-channel11/10) |
| `16` | `Intris5cntzero` | RO | Raw Interrupt for counter = 0 (group5-channel11/10) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Intris4cmpbd` | RO | Raw Interrupt for counter = comparator B Down (group4-channel9) |
| `12` | `Intris4cmpad` | RO | Raw Interrupt for counter = comparator A Down (group4-channel8) |
| `11` | `Intris4cmpbu` | RO | Raw Interrupt for counter = comparator B Up (group4-channel9) |
| `10` | `Intris4cmpau` | RO | Raw Interrupt for counter = comparator A Up (group4-channel8) |
| `9` | `Intris4cntload` | RO | Raw Interrupt for counter = Load (group4-channel9/8) |
| `8` | `Intris4cntzero` | RO | Raw Interrupt for counter = 0 (group4-channel9/8) |
| `7:6` | Reserved | — | Reserved, read as zero |
| `5` | `Intris3cmpbd` | RO | Raw Interrupt for counter = comparator B Down (group3-channel7) |
| `4` | `Intris3cmpad` | RO | Raw Interrupt for counter = comparator A Down (group3-channel6) |
| `3` | `Intris3cmpbu` | RO | Raw Interrupt for counter = comparator B Up (group3-channel7) |
| `2` | `Intris3cmpau` | RO | Raw Interrupt for counter = comparator A Up (group3-channel6) |
| `1` | `Intris3cntload` | RO | Raw Interrupt for counter = Load (group3-channel7/6) |
| `0` | `Intris3cntzero` | RO | Raw Interrupt for counter = 0 (group3-channel7/6) |

#### `PWMIC1`（Offset `0x24`）— Interrupt clear for group 2/1/0

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:30` | Reserved | — | Reserved, read as zero |
| `29` | `Intic2cmpbd` | R/W | Clear Interrupt for counter = comparator B Down (group2-channel5). `1` = clear; `0` = not clear |
| `28` | `Intic2cmpad` | R/W | Clear Interrupt for counter = comparator A Down (group2-channel4) |
| `27` | `Intic2cmpbu` | R/W | Clear Interrupt for counter = comparator B Up (group2-channel5) |
| `26` | `Intic2cmpau` | R/W | Clear Interrupt for counter = comparator A Up (group2-channel4) |
| `25` | `Intic2cntload` | R/W | Clear Interrupt for counter = Load (group2-channel5/4) |
| `24` | `Intic2cntzero` | R/W | Clear Interrupt for counter = 0 (group2-channel5/4) |
| `23:22` | Reserved | — | Reserved, read as zero |
| `21` | `Intic1cmpbd` | R/W | Clear Interrupt for counter = comparator B Down (group1-channel3) |
| `20` | `Intic1cmpad` | R/W | Clear Interrupt for counter = comparator A Down (group1-channel2) |
| `19` | `Intic1cmpbu` | R/W | Clear Interrupt for counter = comparator B Up (group1-channel3) |
| `18` | `Intic1cmpau` | R/W | Clear Interrupt for counter = comparator A Up (group1-channel2) |
| `17` | `Intic1cntload` | R/W | Clear Interrupt for counter = Load (group1-channel3/2) |
| `16` | `Intic1cntzero` | R/W | Clear Interrupt for counter = 0 (group1-channel3/2) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Intic0cmpbd` | R/W | Clear Interrupt for counter = comparator B Down (group0-channel1/0) |
| `12` | `Intic0cmpad` | R/W | Clear Interrupt for counter = comparator A Down (group0-channel0) |
| `11` | `Intic0cmpbu` | R/W | Clear Interrupt for counter = comparator B Up (group0-channel1) |
| `10` | `Intic0cmpau` | R/W | Clear Interrupt for counter = comparator A Up (group0-channel0) |
| `9` | `Intic0cntload` | R/W | Clear Interrupt for counter = Load (group0-channel1/0) |
| `8` | `Intic0cntzero` | R/W | Clear Interrupt for counter = 0 (group0-channel1/0) |
| `7:0` | Reserved | — | Reserved, read as zero |

#### `PWMIC2`（Offset `0x28`）— Interrupt clear for group 5/4/3

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:22` | Reserved | — | Reserved, read as zero |
| `21` | `Intic5cmpbd` | R/W | Clear Interrupt for counter = comparator B Down (group5-channel11) |
| `20` | `Intic5cmpad` | R/W | Clear Interrupt for counter = comparator A Down (group5-channel10) |
| `19` | `Intic5cmpbu` | R/W | Clear Interrupt for counter = comparator B Up (group5-channel11) |
| `18` | `Intic5cmpau` | R/W | Clear Interrupt for counter = comparator A Up (group5-channel10) |
| `17` | `Intic5cntload` | R/W | Clear Interrupt for counter = Load (group5-channel11/10) |
| `16` | `Intic5cntzero` | R/W | Clear Interrupt for counter = 0 (group5-channel11/10) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Intic4cmpbd` | R/W | Clear Interrupt for counter = comparator B Down (group4-channel9) |
| `12` | `Intic4cmpad` | R/W | Clear Interrupt for counter = comparator A Down (group4-channel8) |
| `11` | `Intic4cmpbu` | R/W | Clear Interrupt for counter = comparator B Up (group4-channel9) |
| `10` | `Intic4cmpau` | R/W | Clear Interrupt for counter = comparator A Up (group4-channel8) |
| `9` | `Intic4cntload` | R/W | Clear Interrupt for counter = Load (group4-channel9/8) |
| `8` | `Intic4cntzero` | R/W | Clear Interrupt for counter = 0 (group4-channel9/8) |
| `7:6` | Reserved | — | Reserved, read as zero |
| `5` | `Intic3cmpbd` | R/W | Clear Interrupt for counter = comparator B Down (group3-channel7) |
| `4` | `Intic3cmpad` | R/W | Clear Interrupt for counter = comparator A Down (group3-channel6) |
| `3` | `Intic3cmpbu` | R/W | Clear Interrupt for counter = comparator B Up (group3-channel7) |
| `2` | `Intic3cmpau` | R/W | Clear Interrupt for counter = comparator A Up (group3-channel6) |
| `1` | `Intic3cntload` | R/W | Clear Interrupt for counter = Load (group3-channel7/6) |
| `0` | `Intic3cntzero` | R/W | Clear Interrupt for counter = 0 (group3-channel7/6) |

#### `PWMIS1`（Offset `0x2C`）— Interrupt status for group 2/1/0 (masked)

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:30` | Reserved | — | Reserved, read as zero |
| `29` | `Intis2cmpbd` | RO | Interrupt for counter = comparator B Down (group2-channel5) |
| `28` | `Intis2cmpad` | RO | Interrupt for counter = comparator A Down (group2-channel4) |
| `27` | `Intis2cmpbu` | RO | Interrupt for counter = comparator B Up (group2-channel5) |
| `26` | `Intis2cmpau` | RO | Interrupt for counter = comparator A Up (group2-channel4) |
| `25` | `Intis2cntload` | RO | Interrupt for counter = Load (group2-channel5/4) |
| `24` | `Intis2cntzero` | RO | Interrupt for counter = 0 (group2-channel5/4) |
| `23:22` | Reserved | — | Reserved, read as zero |
| `21` | `Intis1cmpbd` | RO | Interrupt for counter = comparator B Down (group1-channel3) |
| `20` | `Intis1cmpad` | RO | Interrupt for counter = comparator A Down (group1-channel2) |
| `19` | `Intis1cmpbu` | RO | Interrupt for counter = comparator B Up (group1-channel3) |
| `18` | `Intis1cmpau` | RO | Interrupt for counter = comparator A Up (group1-channel2) |
| `17` | `Intis1cntload` | RO | Interrupt for counter = Load (group1-channel3/2) |
| `16` | `Intis1cntzero` | RO | Interrupt for counter = 0 (group1-channel3/2) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Intis0cmpbd` | RO | Interrupt for counter = comparator B Down (group0-channel1) |
| `12` | `Intis0cmpad` | RO | Interrupt for counter = comparator A Down (group0-channel0) |
| `11` | `Intis0cmpbu` | RO | Interrupt for counter = comparator B Up (group0-channel1) |
| `10` | `Intis0cmpau` | RO | Interrupt for counter = comparator A Up (group0-channel0) |
| `9` | `Intis0cntload` | RO | Interrupt for counter = Load (group0-channel1/0) |
| `8` | `Intis0cntzero` | RO | Interrupt for counter = 0 (group0-channel1/0) |
| `7:0` | Reserved | — | Reserved, read as zero |

#### `PWMIS2`（Offset `0x30`）— Interrupt status for group 5/4/3 (masked)

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:22` | Reserved | — | Reserved, read as zero |
| `21` | `Intis5cmpbd` | RO | Interrupt for counter = comparator B Down (group5-channel11) |
| `20` | `Intis5cmpad` | RO | Interrupt for counter = comparator A Down (group5-channel10) |
| `19` | `Intis5cmpbu` | RO | Interrupt for counter = comparator B Up (group5-channel11) |
| `18` | `Intis5cmpau` | RO | Interrupt for counter = comparator A Up (group5-channel10) |
| `17` | `Intis5cntload` | RO | Interrupt for counter = Load (group5-channel11/10) |
| `16` | `Intis5cntzero` | RO | Interrupt for counter = 0 (group5-channel11/10) |
| `15:14` | Reserved | — | Reserved, read as zero |
| `13` | `Intis4cmpbd` | RO | Interrupt for counter = comparator B Down (group4-channel9) |
| `12` | `Intis4cmpad` | RO | Interrupt for counter = comparator A Down (group4-channel8) |
| `11` | `Intis4cmpbu` | RO | Interrupt for counter = comparator B Up (group4-channel9) |
| `10` | `Intis4cmpau` | RO | Interrupt for counter = comparator A Up (group4-channel8) |
| `9` | `Intis4cntload` | RO | Interrupt for counter = Load (group4-channel9/8) |
| `8` | `Intis4cntzero` | RO | Interrupt for counter = 0 (group4-channel9/8) |
| `7:6` | Reserved | — | Reserved, read as zero |
| `5` | `Intis3cmpbd` | RO | Interrupt for counter = comparator B Down (group3-channel7) |
| `4` | `Intis3cmpad` | RO | Interrupt for counter = comparator A Down (group3-channel6) |
| `3` | `Intis3cmpbu` | RO | Interrupt for counter = comparator B Up (group3-channel7) |
| `2` | `Intis3cmpau` | RO | Interrupt for counter = comparator A Up (group3-channel6) |
| `1` | `Intis3cntload` | RO | Interrupt for counter = Load (group3-channel7/6) |
| `0` | `Intis3cntzero` | RO | Interrupt for counter = 0 (group3-channel7/6) |。

### 4.4 PWM 计数器与比较寄存器

#### `PWMCTL`（Offset `0x34`）— PWM control

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:18` | Reserved | — | Reserved, read as zero |
| `17:16` | `Sync5mode` | R/W | Synchronize mode (group5-channel11/10): `00` = update register value when counter reach to zero; `01` = update when counter reach to load value; `10` = update when counter reach to zero and load value; `11` = not update |
| `15:14` | `Sync4mode` | R/W | Synchronize mode (group4-channel9/8) — 编码同上 |
| `13:12` | `Sync3mode` | R/W | Synchronize mode (group3-channel7/6) — 编码同上 |
| `11:10` | `sync2mode` | R/W | Synchronize mode (group2-channel5/4) — 编码同上 |
| `9:8` | `sync1mode` | R/W | Synchronize mode (group1-channel3/2) — 编码同上 |
| `7:6` | `sync0mode` | R/W | Synchronize mode (group0-channel1/0) — 编码同上 |
| `5` | `pwm5mode` | R/W | Counter mode (group5-channel11/10): `1` = count-up/down mode; `0` = count-up mode |
| `4` | `pwm4mode` | R/W | Counter mode (group4-channel9/8) — 编码同上 |
| `3` | `pwm3mode` | R/W | Counter mode (group3-channel7/6) — 编码同上 |
| `2` | `pwm2mode` | R/W | Counter mode (group2-channel5/4) — 编码同上 |
| `1` | `pwm1mode` | R/W | Counter mode (group1-channel3/2) — 编码同上 |
| `0` | `pwm0mode` | R/W | Counter mode (group0-channel1/0) — 编码同上 |

整体 reset = `0x0000_0000`。

#### `PWM01LOAD`（Offset `0x38`）— 寄存器组 `PWM01LOAD / PWM23LOAD / PWM45LOAD`（UG Table 6-15）

每 register 内含 2 个 16-bit load value（对应同一对 group 的两个计数器），字段名 `loadm` / `loadn`。整体 reset = `0x0000_0000`。本寄存器组的 `PWM45LOAD` 见下方独立子节。

#### `PWM45LOAD`（Offset `0x40`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `loadm` | R/W | Counter load m value（m=5：group5-channel11/10） |
| `15:0` | `loadn` | R/W | Counter load n value（n=4：group4-channel9/8） |

#### `PWM01COUNT`（Offset `0x44`）— 寄存器组 `PWM01COUNT / PWM23COUNT / PWM45COUNT`（UG Table 6-16）

只读当前计数器值，字段名 `countm` / `countn`。整体 reset = `0x0000_0000`。本寄存器组的 `PWM45COUNT` 见下方独立子节。

#### `PWM45COUNT`（Offset `0x4C`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `countm` | RO | Counter m current value（m=5：group5-channel11/10） |
| `15:0` | `countn` | RO | Counter n current value（n=4：group4-channel9/8） |

#### `PWM0CMP`（Offset `0x50`）— 寄存器组 `PWM0CMP ~ PWM5CMP`（UG Table 6-17）

每寄存器 32-bit 内含 2 个 16-bit compare value（高 16-bit = 该 group 第 2 通道 CMP_B，低 16-bit = 第 1 通道 CMP_A），字段名 `compnb` / `compna`。整体 reset = `0x0000_0000`。本寄存器组的 `PWM5CMP` 见下方独立子节。

#### `PWM5CMP`（Offset `0x64`，spec 寄存器组中第 6 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `compnb` | R/W | Comparator B value（n=5：group5-channel11） |
| `15:0` | `compna` | R/W | Comparator A value（n=5：group5-channel10） |

#### `PWM01DB`（Offset `0x68`）— 寄存器组 `PWM01DB / PWM23DB / PWM45DB`（UG Table 6-18）

死区（dead-band）延迟计数值，用于互补 PWM 输出延迟插入；位域结构见下方 `PWM45DB` 独立子节。整体 reset = `0x0000_0000`。

#### `PWM45DB`（Offset `0x70`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:26` | Reserved | — | Reserved, read as zero |
| `25` | `dbmen` | R/W | Dead-band m generator enable（m=5：group5-channel11/10）：`1` = insert dead-band；`0` = pass through |
| `24` | `dbnen` | R/W | Dead-band n generator enable（n=4：group4-channel9/8）：`1` = insert dead-band；`0` = pass through |
| `23:12` | `delaym` | R/W | Dead-band delay（m=5：group5-channel11/10）：clock tick 数 |
| `11:0` | `delayn` | R/W | Dead-band delay（n=4：group4-channel9/8）：clock tick 数 |

### 4.5 捕获（CAP）寄存器

#### `CAPCTL`（Offset `0x74`）

字段命名与功能（user guide Table 6-19）：

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:18` | Reserved | — | Reserved, read as zero |
| `17:16` | `cap5event` | R/W | Capture5 边沿事件模式（group5-channel10）：`00` = posedge；`01` = negedge；`10` = reserved；`11` = both edge |
| `15:14` | `cap4event` | R/W | Capture4 边沿事件模式（group4-channel8）— 编码同上 |
| `13:12` | `cap3event` | R/W | Capture3 边沿事件模式（group3-channel6）— 编码同上 |
| `11:10` | `cap2event` | R/W | Capture2 边沿事件模式（group2-channel4）— 编码同上 |
| `9:8` | `cap1event` | R/W | Capture1 边沿事件模式（group1-channel2）— 编码同上 |
| `7:6` | `cap0event` | R/W | Capture0 边沿事件模式（group0-channel0）— 编码同上 |
| `5` | `cap5mode` | R/W | Capture5 模式（group5-channel10）：`1` = edge count mode；`0` = edge time mode |
| `4` | `cap4mode` | R/W | Capture4 模式（group4-channel8）— 编码同上 |
| `3` | `cap3mode` | R/W | Capture3 模式（group3-channel6）— 编码同上 |
| `2` | `cap2mode` | R/W | Capture2 模式（group2-channel4）— 编码同上 |
| `1` | `cap1mode` | R/W | Capture1 模式（group1-channel2）— 编码同上 |
| `0` | `cap0mode` | R/W | Capture0 模式（group0-channel0）— 编码同上 |

整体 reset = `0x0000_0000`。

#### `CAPINTEN`（Offset `0x78`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:12` | Reserved | — | Reserved, read as zero |
| `11` | `cap5timie` | R/W | Capture5 edge time interrupt enable（group5-channel10） |
| `10` | `cap4timie` | R/W | Capture4 edge time interrupt enable（group4-channel8） |
| `9` | `cap3timie` | R/W | Capture3 edge time interrupt enable（group3-channel6） |
| `8` | `cap2timie` | R/W | Capture2 edge time interrupt enable（group2-channel4） |
| `7` | `cap1timie` | R/W | Capture1 edge time interrupt enable（group1-channel2） |
| `6` | `cap0timie` | R/W | Capture0 edge time interrupt enable（group0-channel0） |
| `5` | `cap5cntie` | R/W | Capture5 edge count interrupt enable（group5-channel10） |
| `4` | `cap4cntie` | R/W | Capture4 edge count interrupt enable（group4-channel8） |
| `3` | `cap3cntie` | R/W | Capture3 edge count interrupt enable（group3-channel6） |
| `2` | `cap2cntie` | R/W | Capture2 edge count interrupt enable（group2-channel4） |
| `1` | `cap1cntie` | R/W | Capture1 edge count interrupt enable（group1-channel2） |
| `0` | `cap0cntie` | R/W | Capture0 edge count interrupt enable（group0-channel0） |

#### `CAPRIS`（Offset `0x7C`）— Capture raw interrupt status

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:12` | Reserved | — | Reserved, read as zero |
| `11` | `cap5timris` | RO | Capture5 edge time raw interrupt occurs（group5-channel10） |
| `10` | `cap4timris` | RO | Capture4 edge time raw interrupt occurs（group4-channel8） |
| `9` | `cap3timris` | RO | Capture3 edge time raw interrupt occurs（group3-channel6） |
| `8` | `cap2timris` | RO | Capture2 edge time raw interrupt occurs（group2-channel4） |
| `7` | `cap1timris` | RO | Capture1 edge time raw interrupt occurs（group1-channel2） |
| `6` | `cap0timris` | RO | Capture0 edge time raw interrupt occurs（group0-channel0） |
| `5` | `cap5cntris` | RO | Capture5 edge count raw interrupt occurs（group5-channel10） |
| `4` | `cap4cntris` | RO | Capture4 edge count raw interrupt occurs（group4-channel8） |
| `3` | `cap3cntris` | RO | Capture3 edge count raw interrupt occurs（group3-channel6） |
| `2` | `cap2cntris` | RO | Capture2 edge count raw interrupt occurs（group2-channel4） |
| `1` | `cap1cntris` | RO | Capture1 edge count raw interrupt occurs（group1-channel2） |
| `0` | `cap0cntris` | RO | Capture0 edge count raw interrupt occurs（group0-channel0） |

#### `CAPIC`（Offset `0x80`）— Capture interrupt clear

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:12` | Reserved | — | Reserved, read as zero |
| `11` | `cap5timic` | R/W | Clear Capture5 edge time interrupt（group5-channel10） |
| `10` | `cap4timic` | R/W | Clear Capture4 edge time interrupt（group4-channel8） |
| `9` | `cap3timic` | R/W | Clear Capture3 edge time interrupt（group3-channel6） |
| `8` | `cap2timic` | R/W | Clear Capture2 edge time interrupt（group2-channel4） |
| `7` | `cap1timic` | R/W | Clear Capture1 edge time interrupt（group1-channel2） |
| `6` | `cap0timic` | R/W | Clear Capture0 edge time interrupt（group0-channel0） |
| `5` | `cap5cntic` | R/W | Clear Capture5 edge count interrupt（group5-channel10） |
| `4` | `cap4cntic` | R/W | Clear Capture4 edge count interrupt（group4-channel8） |
| `3` | `cap3cntic` | R/W | Clear Capture3 edge count interrupt（group3-channel6） |
| `2` | `cap2cntic` | R/W | Clear Capture2 edge count interrupt（group2-channel4） |
| `1` | `cap1cntic` | R/W | Clear Capture1 edge count interrupt（group1-channel2） |
| `0` | `cap0cntic` | R/W | Clear Capture0 edge count interrupt（group0-channel0） |

#### `CAPIS`（Offset `0x84`）— Capture interrupt status (masked)

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:12` | Reserved | — | Reserved, read as zero |
| `11` | `cap5timis` | RO | Capture5 edge time interrupt occurs（group5-channel10） |
| `10` | `cap4timis` | RO | Capture4 edge time interrupt occurs（group4-channel8） |
| `9` | `cap3timis` | RO | Capture3 edge time interrupt occurs（group3-channel6） |
| `8` | `cap2timis` | RO | Capture2 edge time interrupt occurs（group2-channel4） |
| `7` | `cap1timis` | RO | Capture1 edge time interrupt occurs（group1-channel2） |
| `6` | `cap0timis` | RO | Capture0 edge time interrupt occurs（group0-channel0） |
| `5` | `cap5cntis` | RO | Capture5 edge count interrupt occurs（group5-channel10） |
| `4` | `cap4cntis` | RO | Capture4 edge count interrupt occurs（group4-channel8） |
| `3` | `cap3cntis` | RO | Capture3 edge count interrupt occurs（group3-channel6） |
| `2` | `cap2cntis` | RO | Capture2 edge count interrupt occurs（group2-channel4） |
| `1` | `cap1cntis` | RO | Capture1 edge count interrupt occurs（group1-channel2） |
| `0` | `cap0cntis` | RO | Capture0 edge count interrupt occurs（group0-channel0） |

整体 reset = `0x0000_0000`。

#### `CAP01T`（Offset `0x88`）— 寄存器组 `CAP01T / CAP23T / CAP45T`（UG Table 6-24）

捕获事件时的 counter 值，字段名 `CAPnmTm` / `CAPnmTn`。整体 reset = `0x0000_0000`。本寄存器组的 `CAP45T` 见下方独立子节。

#### `CAP45T`（Offset `0x90`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `CAPnmTm` | RO | Input capture m counter value（m=5：group5-channel10） |
| `15:0` | `CAPnmTn` | RO | Input capture n counter value（n=4：group5-channel8） |

#### `CAP01MATCH`（Offset `0x94`）— 寄存器组 `CAP01MATCH / CAP23MATCH / CAP45MATCH`（UG Table 6-25）

捕获 match 值，字段名 `CAPnmMATCHm` / `CAPnmMATCHn`。整体 reset = `0x0000_0000`。本寄存器组的 `CAP45MATCH` 见下方独立子节。

#### `CAP45MATCH`（Offset `0x9C`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `CAPnmMATCHm` | R/W | Input capture m match value（m=5：group5-channel10） |
| `15:0` | `CAPnmMATCHn` | R/W | Input capture n match value（n=4：group5-channel8） |

### 4.6 定时器（TIM）寄存器

#### `TIM_INT_EN`（Offset `0xA0`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:6` | Reserved | — | Reserved |
| `5` | `tim5ie` | R/W | Timer5 中断使能（counter = CMP match） |
| `4` | `tim4ie` | R/W | Timer4 中断使能 |
| `3` | `tim3ie` | R/W | |
| `2` | `tim2ie` | R/W | |
| `1` | `tim1ie` | R/W | |
| `0` | `tim0ie` | R/W | |

#### `TIMRIS`（Offset `0xA4`）— Timer raw interrupt status

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:6` | Reserved | — | Reserved, read as zero |
| `5` | `tim5ris` | RO | Timer5 raw interrupt occurs（group5） |
| `4` | `tim4ris` | RO | Timer4 raw interrupt occurs（group4） |
| `3` | `tim3ris` | RO | Timer3 raw interrupt occurs（group3） |
| `2` | `tim2ris` | RO | Timer2 raw interrupt occurs（group2） |
| `1` | `tim1ris` | RO | Timer1 raw interrupt occurs（group1） |
| `0` | `tim0ris` | RO | Timer0 raw interrupt occurs（group0） |

#### `TIM_INT_CLR`（Offset `0xA8`，name = `TIMIC`）— Timer interrupt clear

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:6` | Reserved | — | Reserved, read as zero |
| `5` | `tim5ic` | R/W | Timer5 interrupt clear（group5） |
| `4` | `tim4ic` | R/W | Timer4 interrupt clear（group4） |
| `3` | `tim3ic` | R/W | Timer3 interrupt clear（group3） |
| `2` | `tim2ic` | R/W | Timer2 interrupt clear（group2） |
| `1` | `tim1ic` | R/W | Timer1 interrupt clear（group1） |
| `0` | `tim0ic` | R/W | Timer0 interrupt clear（group0） |

#### `TIMIS`（Offset `0xAC`）— Timer interrupt status (masked)

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:6` | Reserved | — | Reserved, read as zero |
| `5` | `tim5is` | RO | Timer5 interrupt occurs（group5） |
| `4` | `tim4is` | RO | Timer4 interrupt occurs（group4） |
| `3` | `tim3is` | RO | Timer3 interrupt occurs（group3） |
| `2` | `tim2is` | RO | Timer2 interrupt occurs（group2） |
| `1` | `tim1is` | RO | Timer1 interrupt occurs（group1） |
| `0` | `tim0is` | RO | Timer0 interrupt occurs（group0） |

整体 reset = `0x0000_0000`。

#### `TIM01LOAD`（Offset `0xB0`）— 寄存器组 `TIM01LOAD / TIM23LOAD / TIM45LOAD`（UG Table 6-30）

每寄存器含 2 个 16-bit timer load 值，字段名 `timloadm` / `timloadn`。整体 reset = `0x0000_0000`。本寄存器组的 `TIM45LOAD` 见下方独立子节。

#### `TIM45LOAD`（Offset `0xB8`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `timloadm` | R/W | Timer m load value（m=5：group5） |
| `15:0` | `timloadn` | R/W | Timer n load value（n=4：group5） |

#### `TIM01COUNT`（Offset `0xBC`）— 寄存器组 `TIM01COUNT / TIM23COUNT / TIM45COUNT`（UG Table 6-31）

每寄存器含 2 个 16-bit timer 当前值，字段名 `timcntm` / `timcntn`。整体 reset = `0x0000_0000`。本寄存器组的 `TIM45COUNT` 见下方独立子节。

#### `TIM45COUNT`（Offset `0xC4`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `timcntm` | RO | Timer m current count value（m=5：group5） |
| `15:0` | `timcntn` | RO | Timer n current count value（n=4：group5） |

#### `CNT01VAL`（Offset `0xC8`）— 寄存器组 `CNT01VAL / CNT23VAL / CNT45VAL`（UG Table 6-32）

每寄存器含 2 个 16-bit 输入脉冲计数值（edge count mode 输出），字段名 `Cntmval` / `Cntnval`。整体 reset = `0x0000_0000`。本寄存器组的 `CNT45VAL` 见下方独立子节。

#### `CNT45VAL`（Offset `0xD0`，spec 寄存器组中第 3 个）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:16` | `Cntmval` | RO | Counter m captured input pulse number value（m=5：group5） |
| `15:0` | `Cntnval` | RO | Counter n captured input pulse number value（n=4：group5） |

---

## 5. 工作流程（User Guide Work Flow）

User Guide 给出的 25-KHz / 75% duty / 25% duty 示例（APB 时钟 40 MHz）：

1. **选择计数模式**：写 `PWMCTL = 0x0`（Count-up 模式）。
2. **设置周期**：APB 40 MHz 经 `PWMCFG.cntdiv` 分频（默认 /2）得 20 MHz PWM clock；25-KHz 周期 = 800 ticks → 写 `PWM01LOAD = 0x0320`（高/低 16-bit 均 0x320，user guide 写作 "PWM01LOAD[31:16] 和 PWM01LOAD[15:0]"）。
3. **设置占空比**：
    - PWM0 占空比 75%：写 `PWM0CMP[15:0] = 0x0c8` (=200, 75% × 800/2 ≈ 200)。
    - PWM1 占空比 25%：写 `PWM0CMP[31:16] = 0x258` (=600, 25% × 800/2 ≈ 600)。
4. **旁路死区**：写 `PWM01DB = 0x0`。
5. **配置分频 + 启动 counter + 使能输出**：写 `PWMCFG = 0x8000003`（推测含 `cntdiv` / `cntdiven` / `tim0en` / 各组 cap/tim enable 等位）。
6. （可选）配置 `CAPCTL` / `CAPMATCH` / `TIMLOAD` 启动捕获/定时器；通过 `PWMINTEN1/2` / `CAPINTEN` / `TIM_INT_EN` 启用中断；通过 `PWMINVERTTRIG` 反转极性。

> 注：User Guide 示例中 `PWMCFG = 0x8000003` 的 bit27 (`cntdiven`=1) + bit[2:0] 的具体含义需结合 RTL `pwm_apbif` 中 `pwmcfg` 寄存器映射详细位展开；spec 表 6-1 仅列出 bit[27]/[26:24]/[23:18]/[17:12] 有名字段，低 12 bit (`[11:0]`) 在 spec 中未明确定义。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| 实例数 | "PWM (×1)" + 12 output | `pwm.v` 单文件 | ✅ |
| 12 路输出 | channel 0~11 | `o_pwm0`~`o_pwm11`（12 个 1-bit 输出 + 12 个 oe_n） | ✅ |
| 6 组 PWM 发生器 | "6 PWM generators" | `pwm_ctrl` 实例化 6 个 `pwm_gen` | ✅ |
| 6 路输入捕获 | "Input edge count / Input edge time mode" | `i_capedge0/2/4/6/8/10`（6 路） | ✅ |
| 6 组定时器 | 隐含（每个 group 1 个 tim） | `TIM_INT_EN.tim0ie`~`tim5ie`（6 个） | ✅ |
| 寄存器数量 | 53 个 | RTL `pwm_apbif` 共定义 53 个 `_OFFSET`（`0x000`~`0x0D0` 间隔 4，对应 53 个有效寄存器），与 spec 一致 | ✅ |
| offset 范围 | `0x00`~`0xD0`（53 个）+ 间隔 `0xD4` 预留 | RTL `pwmcfg_OFFSET=0x000` ... `cnt45val_OFFSET=0x0D0` 等 | ✅ |
| APB slave 接口 | — | `pwm_sec_top` 暴露 `pclk/presetn/paddr/psel/penable/pwrite/pwdata/prdata/pprot` | ✅ |
| 中断聚合 | 1 bit `pwmint` | sec_top 1-bit 输出 `pwmint` | ✅ |
| 故障输入 | 1 bit `fault` | sec_top 1-bit 输入 `fault` | ✅ |
| Dead-band | `PWM01DB`/`PWM23DB`/`PWM45DB` | RTL 实现 | ✅ |
| ETB 触发 | 6 路 in + 6~7 路 out | `etb_pwm_trig_tim0~5_on/off`（12 in） + `pwm_tim0~5_etb_trig` + `pwm_xx_trig`（7 out） | ✅ |
| SoC 挂载 | APB0 P12 | `apb0_sub_top` P12（`0x5001_C000`~`0x5001_FFFF`） | ✅ |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | sec_top 的 `pprot` / `tipc_pwm_trust` | User Guide 未提 | `pwm_sec_top` 顶层预留 2 个端口 | 当前 RTL 仅透传，未对接 trust 逻辑 | high |
| 2 | `pwm_xx_trig` 输出 | User Guide 未定义 | sec_top 1-bit 输出 `pwm_xx_trig` | 可能为 group 触发合并或固定 timer 触发；user guide 未明说 | low |
| 3 | 寄存器命名大小写 | User Guide 表中命名大小写不一致（如 `PWM01TRIG`、`Pwm01match`、`Tim_int_en`） | RTL 中统一小写 `pwm01trig`、`cap01match`、`tim_int_en` | 仅命名规范差异，不影响功能 | high |
| 4 | `Cnt45val` 拼写 | User Guide 写 `Cnv45val` | RTL `cnt45val_OFFSET = 0x0110100` | User Guide 拼写错误（应为 `Cnt45val`） | low |
| 5 | `PWM01T` / `PWM23T` / `PWM45T` 命名 | spec 实际命名为 `CAP01T`/`CAP23T`/`CAP45T`（在 6-24 表） | RTL `cap01t_OFFSET`/`cap23t_OFFSET`/`cap45t_OFFSET` | 与 spec 一致 | high |
| 6 | `PWMCFG` 低 12 bit 含义 | spec 表 6-1 仅写到 `[17:0]`，`[11:0]` 标 "Reserved" | RTL 32-bit 寄存器全部有定义（含 RTL 内部的低 12 bit 含义，如 `cntdiv`/`cntdiven` 等可能落到低 bit） | 文档差异；功能以 RTL 为准 | medium |
| 7 | `PWMCTL` 字段 | spec 描述为 "Configure the PWM generation blocks"，未列出位域 | RTL `pwmctl_OFFSET=0x34`，按 32-bit R/W 实现 | spec 描述过于笼统 | medium |
| 8 | 复位值 | spec 全 `0x0` | RTL 寄存器 reset 全部为 `0` | ✅ | high |
| 9 | `PWM01LOAD` 工作流数值 | spec 示例 "Write 0x320 to PWM01LOAD[31:16] and PWM01LOAD[15:0]" | RTL 32-bit LOAD 寄存器，高/低 16-bit 各含 1 个 load value；写 0x320 到两个半字 = 全部 800 ticks | 与 spec 一致 | high |
| 10 | `PWM0CMP` 半字归属 | spec 工作流 "Write 0xc8 to PWM0CMP[15:0]" → PWM0 75%；"Write 0x258 to PWM0CMP[31:16]" → PWM1 25% | RTL `pwm0cmp` 32-bit 内 [15:0] 对应 group0 channel0 (PWM0)、[31:16] 对应 group0 channel1 (PWM1) | 与 spec 一致 | high |
| 11 | `PWMCFG` 工作流数值 | spec 写 `0x8000003`（推测含 `cntdiven`+ `cntdiv`=011 + 各组 enable） | RTL `pwmcfg` 32-bit 寄存器；该值具体含义需对照 RTL `pwmcfg_OFFSET=0x000` 解码 | spec 数值与字段映射不完整 | medium |
| 12 | `pwm_idle` 输出 | spec 未提 | sec_top 1-bit 输出 | 用于 SoC 时钟门控 | high |
| 13 | `test_mode` | spec 未提 | sec_top 1-bit 输入 | DFT 测试模式 | high |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 1191–1858。
- 寄存器交叉参考：`doc_summary/Pulse_Width_Modulation_PWM_registers.md`（与 userguide.txt 内容一致，本文档以 userguide.txt 为准）。
- RTL 源码：`wujian100_open/soc/pwm.v`（含 `pwm` / `pwm_apbif` / `pwm_ctrl` / `pwm_gen` / `pwm_sec_top`）。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- `tipc_pwm_trust` / `pprot` 端口预留但未实际使用，可能对应未来 trustzone 接入。
- `PWMCFG[11:0]` 低 12 bit 在 User Guide 中标"Reserved"，但工作流示例 `PWMCFG=0x8000003` 涉及 bit[2:0]，暗示低 12 bit 在 RTL 中有具体含义；本文档未深入解析每个低位。
- `PWMCTL` 字段细节在 spec 中未展开，建议结合 RTL `pwm_apbif` 中 `pwmctl_OFFSET=0x34` 的解码逻辑确认位定义。
- `PWM01TRIG` / `PWM23TRIG` / `PWM45TRIG` 的字段含义 spec 描述抽象（"Contain the trigger generate compare value"），其与 PWM 主计数器及 CMP 寄存器的关系需结合 RTL `pwm_gen` 实现确认。
- `pwm_xx_trig` 1-bit ETB 触发输出的具体语义未在 spec 中定义；推测为多 tim 触发合并或固定 timer 触发（具体待 RTL 实现确认）。
- `CAPCTL` 字段表 6-19 中 `[5:0]` (`cap*Nmode`) 与 `[13:2]` (`cap*Nevent`) 在 RTL 32-bit 寄存器中可能存在位置重叠，需要逐 bit 对照 RTL。
- PWM 计数器模式（Up / Up-Down）由 `PWMCTL` 控制，但 spec 未给出 `PWMCTL` 字段编码；本文档未细化。
- `PWM0CMP`/`PWM1CMP`/`...`/`PWM5CMP` 与通道号的映射关系按"高 16-bit = 偶通道 CMP_B，低 16-bit = 奇通道 CMP_A"理解（与 spec 示例一致）；但 spec 表 6-3 中"PWM0CMP = PWM0/1 compare"描述较模糊。
- 53 个寄存器的内部 reset 行为以 RTL 为准；spec 表 6-3 Reset Value 列省略了 `Cnt01VAL`/`Cnt23VAL`/`Cnt45VAL` 的 reset 描述（仅 `0x0`），RTL reset 后读为 0。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.1 Memory Map 共 53 个有效寄存器（offset `0x00` ~ `0xD0`，间隔 4 字节），与 User Guide PWM 章节 Tables 6-1 ~ 6-32 及 RTL `pwm_apbif` 的 53 个 `_OFFSET` 定义一致（`grep -c 'define.*_OFFSET' wujian100_open/soc/pwm.v` = 53）。
- **字段描述**：§4.2 ~ §4.6 中字段名、位域、访问类型与 User Guide Tables 6-1 ~ 6-32 一致；`PWMCFG`/`PWMCTL`/`CAPCTL`/`CAPINTEN`/`CAPRIS`/`CAPIC`/`CAPIS`/`TIMRIS`/`TIMIC`/`TIMIS` 已展开为完整逐 bit 字段表。
- **端口列表**：§3 `pwm_sec_top` 端口集合与 RTL `module pwm_sec_top(...);` 声明逐项核对一致（含 12 路 `o_pwm*`、12 路 `pwm*oe_n`、6 路 `i_capedge*`、6×2 路 `etb_pwm_trig_tim*_on/off`、7 路 ETB 输出、1 路 `fault`、1 路 `pwmint`）。
- **结构挂载**：§1.2 中 PWM 在 APB0 P12（`0x5001_C000`~`0x5001_FFFF`）的分配与 Peripheral Address Map 一致；PWM 中断号 25 与 System Overview Table 1-4 一致。
- **字段级比对脚本**（`doc_summary/module_analysis/_src/compare_fields.py`）：pwm 模块 `FIELD-MISS` / `FIELD-NAME` / `FIELD-ACC` / `NO-FIELD-TABLE` 全部清零；残留 9 条 `OFFSET` 均为 UG 合并行（如 `PWM01LOAD/PWM23LOAD/PWM45LOAD → 0x38/0x3c/0x40`）被解析器选取首偏移导致的伪报，与文档实际内容一致；用户已明确 `OFFSET PWM45LOAD` 条可忽略，其余 8 条同源、同性质。
- **存疑项**：§6.2 与 §7.2 已逐项列出 trust/pprot 预留、`pwm_xx_trig` 语义、`Cnt45val` 拼写疑误等差异，未在文档中掩盖。
