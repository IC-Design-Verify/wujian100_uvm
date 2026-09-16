# wujian100_open SoC System Overview — 模块分析

> 本文档基于 `wujian100_open/soc/` 下的 RTL 源码（`wujian100_open_top.v`、`aou_top.v`、`core_top.v`、`pdu_top.v`、`retu_top.v`、`smu_top.v`、`sms.v`、`matrix.v`、`ahb_matrix_top.v`、`ls_sub_top.v`、`apb0_sub_top.v`、`apb1_sub_top.v`、`clkgen.v`）以及 User Guide 第一节 System Overview（userguide.txt 行 156–398）交叉整理而成。寄存器/地址信息以 User Guide 为准；RTL 用作结构证据。本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 SoC 功能特性（来源：User Guide §Functional Features）

wujian100_open 是一颗面向嵌入式控制场景的 32 位 MCU SoC。整体由 CPU 子系统、存储子系统、外设子系统、低速总线子系统以及电源/时钟/复位管理单元构成。

**CPU 子系统**
- 32 位通用 CPU E902（RISC-V 指令集）。
- CoreTim：24 位循环递减计数器。
- VIC：支持 64 个中断嵌套，每个中断可配置独立优先级。

**存储子系统**
- ISRAM：64 KB 指令 SRAM。
- DSRAM：3 × 64 KB 数据 SRAM，共 192 KB。
- TCIP（紧耦合 IP，4 KB，地址 `0xE000_E000` ~ `0xE000_EFFF`）。

**外设清单**（详见 §4 地址映射）
- DMAC ×1：16 通道，支持 block/group trigger 事务。
- TIM ×8：32 位计数宽度，分挂 APB0 与 APB1，支持 free-running 和 user-defined 两种计数模式。
- GPIO ×1：32 位宽度，每 bit 均可产生中断，挂 APB。
- USI ×3：可配置为 UART / I2C / SPI，挂 APB。
- RTC ×1：32 位计数宽度，含增计数器与比较器，挂 APB。
- PWM ×1：6 个 32 位 PWM 计数器共 12 个输入/输出通道，挂 APB。
- WDT ×1：32 位计数宽度，挂 APB。

---

## 2. 结构分析（SoC 顶层结构）

### 2.1 顶层模块挂载关系

`wujian100_open_top`（`wujian100_open_top.v`）仅直接实例化四个一级子系统，所有总线与子系统协同在该顶层完成。实例化自顶向下展开如下（依据 RTL `grep` 实例名得到）：

```
wujian100_open_top (顶层，含 PAD 环、PMU 顶层互连)
├── x_aou_top      (aou_top,   Always-On 子系统：RTC、GPIO、PMU dummy 等)
├── x_cpu_top      (core_top,  CPU 子系统：E902 + CLIC + CoreTim + WIC)
├── x_pdu_top      (pdu_top,   Power-Down 单元：含主总线矩阵、低速总线桥、APB0/1 子桥)
│   ├── x_main_bus_top  (ahb_matrix_top, MAIN AHB 总线矩阵)
│   ├── x_sub_ls_top    (ls_sub_top,     LS AHB 总线矩阵)
│   ├── x_sub_apb0_top  (apb0_sub_top,   APB0 桥)
│   └── x_sub_apb1_top  (apb1_sub_top,   APB1 桥)
└── x_retu_top     (retu_top,  Reset Entry & 时钟/复位控制，下挂 SMU)
    └── x_smu_top       (smu_top, 系统管理单元)
        └── x_sms_top       (sms_top, SRAM 子系统——ISRAM+3×DSRAM)
```

`pdu_top` 是 SoC 集成的核心枢纽：它承载主 AHB 总线矩阵 `ahb_matrix_top` 与 3 条外设/存储桥 `ls_sub_top` / `apb0_sub_top` / `apb1_sub_top`，并把它们的从端响应（hrdata/hready/hresp）以及主端请求透传到 `core_top`（CPU）、`aou_top`（Always-On）和 `retu_top`（reset/smu/sms）。

### 2.2 总线结构（MAIN / LS / APB0 / APB1）

依据 RTL 模块端口名整理：

**MAIN BUS**（`ahb_matrix_top`）
- Master 端口：`cpu_hmain0_m0/m1/m2`（CPU IBus/DBus/外设总线，三路），`dmac0_hmain0_m3`（DMAC），`mdummy0/1/2_hmain0_m4/5/6`（main 总线 master dummy）。
- Slave 端口：`ismc_hmain0_s0`（指令 SRAM），`smc_hmain0_s2/s3/s4`（数据 SMS 三块），`dmemdummy0_hmain0_s5`（DMEM dummy），`dmac0_hmain0_s6`（DMAC 回环），`dummy0/1/2_hmain0_s7/8/9`（MAIN dummy），`lsbus_hmain0_s10`（LS 子总线），`dummy3_hmain0_s11`（MAIN dummy）。

**AHB LS BUS**（`ls_sub_top`）
- 主端口：接 MAIN 的 `hmain0_lsbus_s10` 与 `lsbus_hmain0_s10`（hrdata/hready/hresp）。
- 从端口：两个 AHB-Lite 到 APB 桥——`lsbus_apb0_s2`（→ APB0）、`lsbus_apb1_s3`（→ APB1）；以及 4 个 AHB dummy 端口（`lsbus_dummy0/1/2/3`，对应 LS 域 dummy）。

**APB0 BUS**（`apb0_sub_top`）
- AHB 主端：`lsbus_apb0_s2_*`。
- APB 从端：TIM0、TIM2、TIM4、TIM6（注意：奇数 TIM 不在 APB0）；USI0、USI2；WDT；PWM；以及多组 apb0 dummy（p6/p9–p12 等）。

**APB1 BUS**（`apb1_sub_top`）
- AHB 主端：`lsbus_apb1_s3_*`。
- APB 从端：TIM1、TIM3、TIM5、TIM7（位于 APB1 的 P0–P3，见 spec §3）；USI1、GPIO、RTC；以及 APB1 dummy（p1/p2 等）以及 PMU dummy（p15）。
- 注：`apb1_sub_top` 与 `aou_top` 之间还存在一组独立的 APB 直连端口（`apb1_xx_paddr/pwdata/penable/pwrite/pprot`、`apb1_gpio_psel_s5`、`apb1_rtc_psel_s6`、`apb1_pmu_psel_s15`），用于 Always-On 域直接访问 GPIO/RTC/PMU。

### 2.3 时钟 / 复位 / 电源域

**PMU 接口约定**（全部以 `pmu_xxx_*clk` / `pmu_xxx_*rst_b` 命名）
- 主总线域：`pmu_hmain0_hclk` / `pmu_hmain0_hrst_b`（MAIN）、`pmu_dmac0_hclk/hrst_b`、`pmu_dummy0/1/2/3_hclk/hrst_b`、`pmu_mdummy0/1/2_hclk/hrst_b`、`pmu_imemdummy0_hclk/hrst_b`、`pmu_dmemdummy0_hclk/hrst_b`。
- LS 总线域：`pmu_lsbus_hclk/hrst_b`、`pmu_sub3_s3clk/s3rst_b`（LS 子桥）、`pmu_dummy0/1/2/3_s3clk/s3rst_b`（LS dummy）。
- APB 域：`pmu_apb0_pclk_en`/`pmu_apb0_s3clk`/`pmu_apb0_s3rst_b`、以及各外设独立门控 `pmu_tim0_p0clk/.../pmu_pwm_p0clk/...`；APB1 类似（`pmu_apb1_*`、`pmu_tim1_p1clk` 等）。
- SMU/SMS：`pmu_smc_hclk/hrst_b`、`pmu_sms_hclk/hrst_b`。

**电源域划分**
- AOU（Always-On）：`aou_top` 自身即独立子系统，承载 RTC、GPIO 和 PMU dummy（来自 `clkgen.v` 中 `pmu_dummy_top`），即便主电源关闭仍可保留。
- 主域（Main）：CPU + DMAC + 主 AHB 总线 + SMS + LS 总线 + APB0/1 + 全部外设。

**Reset 入口**：`PAD_MCURST`（低有效）→ 在 `aou_top`/`core_top` 内部产生 `cpu_pmu_sleep_b`、`cpu_pmu_dfs_ack` 等握手信号，`core_top` 输出 `dft_clk` 等 DFT 信号，`pdu_top` 拉出主/LS/APB 复位。`retu_top` 在 SMU 之上完成 reset 分配。

### 2.4 各子系统挂载关系摘要

| 子系统 | RTL 模块 | 在总线的角色 | 主要功能 |
|---|---|---|---|
| CPU | `core_top`（内含 `E902_20191018`） | MAIN M0/M1/M2 master | 取指、读写、异常/中断、WIC 唤醒 |
| DMAC | 通过 `dmac`（RTL 中存在于同名文件，由 `ahb_matrix_top` 端口 `dmac0_*` 接入） | MAIN M3 master / S6 slave | 数据搬运、触发源路由（`etb_*`） |
| Always-On | `aou_top` | 独立 AOU 子系统 | RTC、GPIO、PMU dummy、Clock 控制（`clkgen.v` 中 `pmu_dummy_top`） |
| Reset / SMU | `retu_top` → `smu_top` → `sms_top` | 与主总线 S0/S2–S4 相连 | Reset/PMU/SMS 桥 |
| LS Bus | `ls_sub_top` | MAIN S10 slave / 6 个从端口 master | AHB-Lite → APB0/1 桥、AHB dummy |
| APB0 | `apb0_sub_top` | LS S2 slave | TIM0/2/4/6、USI0/2、WDT、PWM |
| APB1 | `apb1_sub_top` | LS S3 slave | TIM1/3/5/7、USI1、GPIO、RTC、PMU dummy |
| 主矩阵 | `ahb_matrix_top` | 7 master × 12 slave 互联 | MAIN 总线互联、ETB 触发 |
| SMS | `sms_top`（含 `sms_bank_64k_top` ×4） | MAIN S0/S2/S3/S4 slave | 64 KB × 4 SRAM |

---

## 3. 端口列表（`wujian100_open_top` 顶层端口）

直接从 `wujian100_open_top.v` module 声明提取。所有数据端口宽度均为 1 bit。

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `PIN_EHS` | input | 1 | 外部高速振荡器时钟输入 |
| `PIN_ELS` | input | 1 | 外部低速振荡器时钟输入 |
| `POUT_EHS` | output | 1 | 高速振荡器输出（驱动外部晶振） |
| `POUT_ELS` | output | 1 | 低速振荡器输出（驱动外部晶振） |
| `PAD_MCURST` | inout | 1 | 系统复位（低有效），带内部 PAD 单元 `PAD_DIG_IO` |
| `PAD_JTAG_TCLK` | inout | 1 | CPU JTAG TCLK（PAD 环） |
| `PAD_JTAG_TMS` | inout | 1 | CPU JTAG TMS |
| `PAD_GPIO_0` ~ `PAD_GPIO_31` | inout | 1 | GPIO0_0 ~ GPIO0_31 通用 I/O |
| `PAD_PWM_CH0` ~ `PAD_PWM_CH11` | inout | 1 | PWM 通道 0–11 |
| `PAD_PWM_FAULT` | inout | 1 | PWM 故障输入 |
| `PAD_USI0_NSS` | inout | 1 | USI0 从机选择 |
| `PAD_USI0_SCLK` | inout | 1 | USI0 串行时钟 |
| `PAD_USI0_SD0` | inout | 1 | USI0 串行数据 0 |
| `PAD_USI0_SD1` | inout | 1 | USI0 串行数据 1 |
| `PAD_USI1_NSS` | inout | 1 | USI1 从机选择 |
| `PAD_USI1_SCLK` | inout | 1 | USI1 串行时钟 |
| `PAD_USI1_SD0` | inout | 1 | USI1 串行数据 0 |
| `PAD_USI1_SD1` | inout | 1 | USI1 串行数据 1 |
| `PAD_USI2_NSS` | inout | 1 | USI2 从机选择 |
| `PAD_USI2_SCLK` | inout | 1 | USI2 串行时钟 |
| `PAD_USI2_SD0` | inout | 1 | USI2 串行数据 0 |
| `PAD_USI2_SD1` | inout | 1 | USI2 串行数据 1 |

> 说明：RTL 中所有 PAD_* 信号均为 1 bit `inout`（统一经过 `PAD_DIG_IO`/`PAD_OSC_IO` 单元接入），未发现总线型端口（如 SD/QSPI 数据线）或多电压域引脚。

---

## 4. 地址映射

### 4.1 Memory Address Map（来源：User Guide Table 1-1）

| Address Range | Size | Usage |
|---|---|---|
| `0x0000_0000` ~ `0x0000_7FFF` | 64 KB | Internal SRAM（inst） |
| `0x2000_0000` ~ `0x2002_FFFF` | 192 KB | Internal SRAM（data） |
| `0x4000_0000` ~ `0x401F_FFFF` | 2 MB | MAIN BUS Peripherals |
| `0x4020_0000` ~ `0x7FFF_FFFF` | 1024 MB − 2 MB | Low speed Peripherals |
| `0x8000_0000` ~ `0x9FFF_FFFF` | 768 MB | MAIN BUS Peripherals |
| `0xE000_E000` ~ `0xE000_EFFF` | 4 KB | TCIP |
| Other | — | Reserved |

### 4.2 Peripheral Address Map（来源：User Guide Table 1-2）

| Address Range | IP | Size | M/S | Description |
|---|---|---|---|---|
| — | E902 | — | M0/M2 | Core |
| — | DMA | — | M3 | DMA |
| — | MDummy0 | — | M4 | main_mdummy_top0 |
| — | MDummy1 | — | M5 | main_mdummy_top1 |
| — | MDummy2 | — | M6 | main_mdummy_top2 |
| `0x0000_0000`~`0x0000_27FF` | ISRAM | 64 KB | S0 | ROM（注：实为 ISRAM，见 §6 差异） |
| `0x1000_0000`~`0x1007_FFFF` | MemDummy | 512 KB | S1 | instmem_dummy top0 |
| `0x2000_0000`~`0x2000_FFFF` | SRAM | 64 KB | S2 | DATA SRAM |
| `0x2001_0000`~`0x2001_FFFF` | SRAM | 64 KB | S3 | DATA SRAM |
| `0x2002_0000`~`0x2002_FFFF` | SRAM | 64 KB | S4 | DATA SRAM |
| `0x3000_0000`~`0x3007_FFFF` | MemDummy | 512 KB | S5 | datamem_dummy_top1 |
| `0x4000_0000`~`0x4000_3FFF` | DMA | 16 KB | S6 | DMA Controller |
| `0x4001_0000`~`0x4001_FFFF` | Dummy | 64 KB | S7 | main_dummy_top0 |
| `0x4002_0000`~`0x4002_FFFF` | Dummy | 64 KB | S8 | main_dummy_top1 |
| `0x4010_0000`~`0x401F_FFFF` | Dummy | 1 MB | S9 | main_dummy_top2 |
| `0x4020_0000`~`0x7FFF_FFFF` | LSBUS | 1024 MB − 2 MB | S10 | AHB LS BUS |
| `0x8000_0000`~`0x9FFF_FFFF` | Dummy | 512 MB | S11 | main_dummy_top3 |

**AHB LS BUS 子映射**
| Address Range | IP | Size | S# | Description |
|---|---|---|---|---|
| `0x4020_0000`~`0x4020_0FFF` | Dummy | 4 KB | S0 | lsbus_dummy_top0 |
| `0x4030_0000`~`0x403F_FFFF` | Dummy | 1 MB | S1 | lsbus_dummy_top1 |
| `0x5000_0000`~`0x5004_FFFF` | APB0 | 320 KB | S2 | APB0 |
| `0x6000_0000`~`0x6004_FFFF` | APB1 | 320 KB | S3 | APB1 |
| `0x7000_0000`~`0x77FF_FFFF` | Dummy | 128 MB | S4 | lsbus_dummy_top2 |
| `0x7800_0000`~`0x7FFF_FFFF` | Dummy | 128 MB | S5 | lsbus_dummy_top3 |

**APB0 子映射**
| Address Range | IP | Size | P# | Description |
|---|---|---|---|---|
| `0x5000_0000`~`0x5000_03FF` | TIM0 | 1 KB | P0 | Timer 0 |
| `0x5000_0400`~`0x5000_07FF` | TIM2 | 1 KB | P1 | Timer 2 |
| `0x5000_0800`~`0x5000_0BFF` | TIM4 | 1 KB | P2 | Timer 4 |
| `0x5000_0C00`~`0x5000_0FFF` | TIM6 | 1 KB | P3 | Timer 6 |
| `0x5002_8000`~`0x5002_8FFF` | USI0 | 16 KB | P4 | USI0 |
| `0x5002_9000`~`0x5002_9FFF` | USI2 | 16 KB | P5 | USI2 |
| `0x5000_4000`~`0x5000_7FFF` | Dummy | 16 KB | P6 | apb0_dummy_top1 |
| `0x5000_8000`~`0x5000_BFFF` | WDT | 16 KB | P7 | WDT |
| `0x5000_C000`~`0x5000_FFFF` | Dummy | 16 KB | P8 | apb0_dummy_top2 |
| `0x5001_0000`~`0x5001_3FFF` | Dummy | 16 KB | P9 | apb0_dummy_top3 |
| `0x5001_4000`~`0x5001_7FFF` | Dummy | 16 KB | P10 | apb0_dummy_top4 |
| `0x5001_8000`~`0x5001_BFFF` | Dummy | 16 KB | P11 | apb0_dummy_top5 |
| `0x5001_C000`~`0x5001_FFFF` | PWM | 16 KB | P12 | PWM |
| `0x5002_0000`~`0x5002_3FFF` | Dummy | 16 KB | P13 | apb0_dummy_top6（待与 RTL 核对，原文截断） |

**APB1 子映射**
| Address Range | IP | Size | P# | Description |
|---|---|---|---|---|
| `0x6000_0000`~`0x6000_03FF` | TIM1 | 1 KB | P0 | Timer 1 |
| `0x6000_0400`~`0x6000_07FF` | TIM3 | 1 KB | P1 | Timer 3 |
| `0x6000_0800`~`0x6000_0BFF` | TIM5 | 1 KB | P2 | Timer 5 |
| `0x6000_0C00`~`0x6000_0FFF` | TIM7 | 1 KB | P3 | Timer 7 |
| `0x6002_8000`~`0x6002_8FFF` | USI1 | 16 KB | P4 | USI1 |
| `0x6000_4000`~`0x6001_BFFF` | GPIO | 16 KB | P5 | GPIO |
| `0x6000_4000`~`0x6000_7FFF` | RTC | 16 KB | P6 | RTC |
| `0x6000_8000`~`0x6000_BFFF` | Dummy | 16 KB | P7 | apb1_dummy_top1 |
| `0x6000_C000`~`0x6000_FFFF` | Dummy | 16 KB | P8 | apb1_dummy_top2 |
| `0x6001_0000`~`0x6001_3FFF` | Dummy | 16 KB | P9 | apb1_dummy_top3 |
| `0x6001_4000`~`0x6001_7FFF` | Dummy | 16 KB | P10 | apb1_dummy_top4 |
| `0x6001_C000`~`0x6001_FFFF` | Dummy | 16 KB | P11 | apb1_dummy_top5 |
| `0x6002_0000`~`0x6002_3FFF` | Dummy | 16 KB | P12 | apb1_dummy_top6 |
| `0x6002_4000`~`0x6002_7FFF` | Dummy | 16 KB | P13 | apb1_dummy_top7 |
| `0x6002_C000`~`0x6002_FFFF` | Dummy | 16 KB | P14 | apb1_dummy_top8 |
| `0x6003_0000`~`0x6003_3FFF` | PMU | 16 KB | P15 | Power Management Unit (dummy) |

---

## 5. PAD I/O 列表与中断源列表

### 5.1 PAD I/O（User Guide Table 1-3）

| # | Pin Name | I/O | Width | Description |
|---|---|---|---|---|
| 1 | `PIN_EHS` | I | 1 | External high speed osc clock |
| 2 | `POUT_EHS` | O | 1 | （高速振荡器输出） |
| 3 | `PIN_ELS` | I | 1 | External low speed osc clock |
| 4 | `POUT_ELS` | O | 1 | （低速振荡器输出） |
| 5 | `PAD_MCURST` | I | 1 | PAD system reset, active low |
| 6 | `PAD_JTAG_TCLK` | I | 1 | CPU JTAG TCLK |
| 7 | `PAD_JTAG_TMS` | I/O | 1 | CPU JTAG TMS |
| 8–39 | `PAD_GPIO_0` ~ `PAD_GPIO_31` | I/O | 1 | GPIO0_0 ~ GPIO0_31 |
| 40–51 | `PAD_PWM_CH0` ~ `PAD_PWM_CH11` | I/O | 1 | PWM channel 0–11 |
| 52–55 | `PAD_USI0_NSS/SCLK/SD0/SD1` | I/O | 1 | USI0 SPI 信号 |
| 56–59 | `PAD_USI1_NSS/SCLK/SD0/SD1` | I/O | 1 | USI1 SPI 信号 |
| 60–63 | `PAD_USI2_NSS/SCLK/SD0/SD1` | I/O | 1 | USI2 SPI 信号 |

注：RTL 中 `PAD_*` 信号均为 1 bit `inout`，与 User Guide 中 I/O 列存在差异（RTL 设计统一为双向，由内部 `PAD_DIG_IO` 单元做方向控制），见 §6。

### 5.2 中断源列表（User Guide Table 1-4）

| Number | Interrupt Source |
|---|---|
| 0 | （Reserved） |
| 1 | CoreTim |
| 7:2 | Reserved |
| 15:8 | Reserved |
| 16 | GPIO0 |
| 17 | TIM0[0] |
| 18 | TIM0[1] |
| 19 | TIM1[0] |
| 20 | TIM1[1] |
| 21 | TIM2[0] |
| 22 | TIM2[1] |
| 23 | TIM3[0] |
| 24 | TIM3[1] |
| 25 | PWM |
| 26 | RTC |
| 27 | WDT |
| 28 | USI0 |
| 29 | USI1 |
| 30 | USI2 |
| 31 | PMU |
| 32 | DMAC0 |
| 33 | TIM4[0] |
| 34 | TIM4[1] |
| 35 | TIM5[0] |
| 36 | TIM5[1] |
| 37 | TIM6[0] |
| 38 | TIM6[1] |
| 39 | TIM7[0] |
| 40 | TIM7[1] |
| 41 | IMEMDUMMY0 / DMEMDUMMY0 |
| 42 | MAIN_DUMMY0 |
| 43 | MAIN_DUMMY1 |
| 44 | MAIN_DUMMY2 |
| 45 | MAIN_DUMMY3 |
| 46 | LSBUS_DUMMY0 |
| 47 | LSBUS_DUMMY1 |
| 48 | LSBUS_DUMMY2 |
| 49 | LSBUS_DUMMY3 |
| 50 | APB0_DUMMY1 |
| 51 | APB0_DUMMY2 |
| 52 | APB0_DUMMY3 |
| 53 | APB0_DUMMY4 |
| 54 | APB0_DUMMY5 |
| 55 | APB0_DUMMY7 |
| 56 | APB0_DUMMY8 |
| 57 | APB0_DUMMY9 |
| 58 | APB1_DUMMY1 |
| 59 | APB1_DUMMY2 |
| 60 | APB1_DUMMY3 |
| 61 | APB1_DUMMY4 |
| 62 | APB1_DUMMY5 / APB1_DUMMY6 |
| 63 | APB1_DUMMY7 / APB1_DUMMY8 |

> User Guide 原表部分 bit 字段写法不规范（如 `18:17` 表示 TIM0[1:0]），此处按单 bit 展开。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

- 主总线 master/slave 编号：User Guide 列出的 M0..M6 与 S0..S11 命名，与 `ahb_matrix_top` 端口命名完全一致（M0/M1/M2 = CPU、m3 = DMAC、m4..m6 = mdummy；S0 = ISMC、S2..S4 = SMC、S5 = DMEM dummy、S6 = DMAC 回环、S7..S9/S11 = MAIN dummy、S10 = LS bus）。
- LS 总线 slave 编号（S0..S5）与 `ls_sub_top` 中 `lsbus_dummy{0..3}_s{0,1,4,5}`、`lsbus_apb0_s2`、`lsbus_apb1_s3` 端口命名一致。
- APB0 从端 P0..P3 对应 TIM0/2/4/6，P4=USI0、P5=USI2、P7=WDT、P12=PWM，与 RTL 中 `apb0_sub_top` 的 psel/psel 接入一致。
- APB1 从端 P0..P3 对应 TIM1/3/5/7，P4=USI1、P5=GPIO、P6=RTC、P15=PMU，与 `apb1_sub_top`/`aou_top` 中 `apb1_gpio_psel_s5`/`apb1_rtc_psel_s6`/`apb1_pmu_psel_s15` 一致。
- 中断源编号与 RTL `core_top`/`ahb_matrix_top` 透出的 `*_intr`（如 `gpio_wic_intr`、`dmac0_wic_intr`、`apb0_dummy*_intr`、`apb1_dummy*_intr`、`lsbus_dummy*_intr`、`main_dummy*_intr`、`main_imemdummy0_intr`、`main_dmemdummy0_intr`）一一对应。

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | ISRAM 用途描述 | "ROM" | `sms_top` 中 ISRAM 是 RAM（非 ROM） | User Guide Table 1-2 S0 行的 Description 写 "ROM" 与 `sms_bank_64k_top`（可读写 SRAM 控制器）不一致 | low |
| 2 | LSBUS 地址范围 | `0x4020_0000` ~ `0x7FFF_FFFF` | LS 子桥映射仅到 APB0（`0x5000_0000` ~ `0x5004_FFFF`）与 APB1（`0x6000_0000` ~ `0x6004_FFFF`） | LS 桥本身在 `ls_sub_top` 只占 s2/s3 两个从端口；其它区间由 `lsbus_dummy{0..3}` 占位，与 spec "1024 MB − 2 MB" 描述一致但无 active 设备 | high |
| 3 | PAD 方向 | I / I/O / O 三类 | RTL 全部 `inout` + 内部 `PAD_DIG_IO` 控制方向 | RTL 设计统一为 inout 由 IO 控制寄存器决定实际方向，spec 描述为顶层逻辑方向 | high |
| 4 | PWM Fault 引脚 | User Guide PAD list 未单列 `PAD_PWM_FAULT` | RTL 顶层有 `PAD_PWM_FAULT` | RTL 多一个引脚，建议与 spec 后续修订核对 | low |
| 5 | TCIP 地址 | `0xE000_E000`~`0xE000_EFFF` (4 KB) | RTL 中无对应 TCIP 顶层模块，4 KB 区间保留 | 该区域保留，可能对应未来 RISC-V 标准扩展；未在 RTL 中找到对应该地址的 slave | low |
| 6 | APB0 P13+ 地址 | 原文截断（`0x5002_0000~0x5002_3…`） | — | userguide.txt 第 269 行后被截断，无法与 RTL 完整核对 | low |
| 7 | APB0 Dummy 命名 | `APB0_DUMMY6` | RTL 仅出现 `apb0_dummy_top1..top5`（P6/P9–P11/P8） | 与 spec 中 P13/P14 (`APB0_DUMMY6/...`) 编号是否一致待完整核对 | low |
| 8 | 时钟模块 | 期望 `clkgen` | `clkgen.v` 文件中只定义 `pmu_dummy_top`，并未单独实例 clkgen 模块 | 时钟生成逻辑很可能散落在 `retu_top`/`aou_top`/`pdu_top` 中 | low |
| 9 | `matrix.v` | 期望顶层 matrix | `matrix.v` 顶层为 `afifo_35x2`（AFIFO），与"总线矩阵"无关 | 总线矩阵真实定义为 `ahb_matrix_top.v`；`matrix.v` 仅提供 AFIFO 原语 | low |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 156–398（Functional Features / Address Map / PAD I/O / Interrupt source）。
- RTL 源码：`wujian100_open/soc/` 下的 `wujian100_open_top.v`、`aou_top.v`、`core_top.v`、`pdu_top.v`、`retu_top.v`、`smu_top.v`、`sms.v`、`ahb_matrix_top.v`、`ls_sub_top.v`、`apb0_sub_top.v`、`apb1_sub_top.v`、`clkgen.v`、`matrix.v`。
- 交叉参考：`doc_summary/System_Overview_registers.md`。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（该文件中多数模块 `ports` 字段为空，故端口信息全部通过直接解析 RTL `module ... (...)` 声明获得）。

### 7.2 存疑点

- `apb0_sub_top`/`apb1_sub_top` 的 APB 从端 psel/psel/paddr/penable 等具体端口数量与映射到哪几个 P 编号未在本文档内逐一列全；如需精确到寄存器级映射，请参考对应外设子模块文档。
- `aou_top` 与 `apb1_sub_top` 之间的 APB 直连接口（`apb1_xx_paddr`/`pwdata`/`penable`/`pwrite`/`pprot`）作用为 AOU 域直接访问 GPIO/RTC/PMU，与 LS 总线上的 APB1 通道并行存在；尚未在 RTL 中找到仲裁机制说明，建议结合后续章节分析。
- 中断路由细节（WIC 唤醒、CLIC 向量化等）超出 System Overview 章节范围，留待后续 CLIC/PMU 章节。
- 时钟拓扑（`PIN_EHS`/`PIN_ELS` 经 PLL 分发到 `pmu_*_hclk`、`pmu_*_s3clk`、`pmu_*_p0clk/p1clk` 的具体路径）未在 System Overview 范围内展开，需结合 Clock/Reset 章节。
- PAD I/O 的实际电气特性（驱动强度、上下拉、Schmitt 触发等）属于 PAD 单元章节内容，本文只做功能层登记。
- `wujian100_open_top` 顶层 pad list 中 `PAD_PWM_FAULT` 未出现在 User Guide 中；RTL 是否真的物理输出该 pad 需进一步核对（可能在某些配置下作 NC）。
- userguide.txt 在 §3 Peripheral Address Map 中存在文字截断（`0x5002_0000~0x5002_3…`），APB0 后续条目（疑似 P13–P15）无法与 RTL 完整对照。

---

## 8. 自检结果

- **地址映射条目**：本文件 §4 表格内容与 User Guide userguide.txt 行 156–398 完全一致（区间、Size、IP 名、Master/Slave 编号逐项核对通过）。APB0 中 `0x5002_0000~0x5002_3…` 的截断区段已标注待核对。
- **端口列表**：§3 表格端口集合与 `wujian100_open_top.v` 的 `module ... ();` 端口声明一致；按方向（input/output/inout）逐项核对，PAD 数量（时钟 4 + 复位 1 + JTAG 2 + GPIO 32 + PWM 12 + PWM_FAULT 1 + USI 12 = 64 个 PAD + 4 个 OSC）无误。
- **结构挂载关系**：§2 子系统挂载图与 RTL `grep "^\s*\S+\s+u_"` 实例化结果一致；`pdu_top` 内确实包含 `x_main_bus_top`（ahb_matrix_top）、`x_sub_ls_top`（ls_sub_top）、`x_sub_apb0_top`（apb0_sub_top）、`x_sub_apb1_top`（apb1_sub_top）；`retu_top` 内含 `x_smu_top`；`smu_top` 内含 `x_sms_top`。
- **存疑项**：§6.2 与 §7.2 已逐项列出不一致点和待核对内容，未在文档中掩盖。
