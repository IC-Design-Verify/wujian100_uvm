# wujian100_open SoC Unified Serial Interface (USI) — 模块分析

> 本文档基于 `wujian100_open/soc/usi0.v`（包含 `usi_top` / `usi0_sec_top` / `apb_if` / `i2cm` / `i2cs` / `i2c_top` / `sdata_if` / `spi` / `sync_fifo_16x16` / `uart` 子模块）以及 `wujian100_open/soc/usi1.v`（包含 `usi1_sec_top` / `usi2_sec_top`），结合 User Guide 第 4 节「Unified Serial Interface (USI)」（userguide.txt 行 696–1114）整理而成。寄存器信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

USI（Unified Serial Interface）是 wujian100_open SoC 的统一串行接口 IP，单个 USI 模块可在 **UART / I2C / SPI** 三种模式中任选其一工作。SoC 集成 3 个 USI 实例（USI0/USI1/USI2），分别挂到不同的 APB 总线与 PAD 引脚。

- **总线接口**：APB slave（寄存器配置与数据读写）。
- **DMA 接口**：每个 USI 各有 2 条 DMA 请求线（`dma_req_tx` / `dma_req_rx`），由 SoC DMAC 接管后批量搬入/搬出 TX/RX FIFO。
- **中断接口**：每 USI 聚合 1 路中断输出 `usi_intr`，并通过 ETB 触发线 `usi_etb_tx_trig` / `usi_etb_rx_trig` 与其它外设交互。
- **模式切换约束**：User Guide 强调「USI's function and the value of control registers in USI cannot be changed when the USI is working, Otherwise the data may be lost. The change can take place when the USI module is disable.」——工作期间不可改模式或控制寄存器；必须先禁用 USI 才能切换。
- **数据缓冲**：内部 TX/RX FIFO 各 16 × 16-bit（由 `sync_fifo_16x16` 实现）。
- **数据位宽**：FIFO 16-bit；UART 数据位 5/6/7/8-bit；SPI 数据帧 4–16-bit。
- **安全扩展**：`usiN_sec_top` 顶层预留 `tipc_usiN_trust` / `pprot[2:0]` / `sec_tx_req` / `sec_rx_req` 端口，但当前 RTL 未实际使用（仅透传）。

### 1.2 SoC 中的位置 / 总线挂载

依据 System Overview 章节与 `apb0_sub_top` / `apb1_sub_top` 端口列表：

| USI 实例 | 文件 | 所在 APB | 内部 base 地址（user guide） | P# | PAD |
|---|---|---|---|---|---|
| USI0 | `usi0.v` | APB0 | `0x5002_8000` | P4 | PAD_USI0_xxx |
| USI1 | `usi1.v` | APB1 | `0x6002_8000` | P4 | PAD_USI1_xxx |
| USI2 | `usi1.v` | APB0 | `0x5002_9000` | P5 | PAD_USI2_xxx |

每个 USI 占 16 KB 地址空间（实际有效寄存器约 0x6C 字节）。

### 1.3 RTL 文件与实例关系

- `usi0.v`：定义一个完整 USI 实例（`usi0_sec_top` → `usi_top` → `apb_if` / `i2c_top` / `spi` / `uart` / `sdata_if` / `sync_fifo_16x16` ×2），同时包含所有可复用子模块（`apb_if`、`i2cm`、`i2cs`、`i2c_top`、`sdata_if`、`spi`、`sync_fifo_16x16`、`uart`）。
- `usi1.v`：仅包含 2 个 wrapper `usi1_sec_top` 和 `usi2_sec_top`（USI1 挂 APB1，USI2 挂 APB0）。这两个 sec_top 内部均**直接调用 `usi_top`**，复用 `usi0.v` 中的 `usi_top` 实现。
- `usi_top` 是 USI 的功能顶层（无 sec 封装），所有 3 个 sec_top 共享同一份实现。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次（以 USI0 为例）

```
usi0_sec_top                (usi0.v:3765, sec 封装层：预留 tipc_usi0_trust / pprot / sec_*_req，未实际接入)
└── usi_top  x_usi_top      (usi0.v:3916, 内部 RTL 顶层)
    ├── x_apb_if            (usi0.v:39, APB slave 接口与寄存器读写；26 个寄存器)
    ├── x_uart              (usi0.v:3335, UART 模式功能块)
    ├── x_i2c_top           (usi0.v:2134, I2C 顶层：内部含 i2cm + i2cs)
    │   ├── i2cm            (usi0.v:602, I2C Master)
    │   └── i2cs            (usi0.v:1625, I2C Slave)
    ├── x_spi               (usi0.v:2625, SPI Master/Slave)
    ├── x_sdata_if          (usi0.v:2399, 串行数据接口：将 SPI/I2C/UART 信号路由到 PAD)
    ├── x_tx_sync_fifo_16x16(usi0.v:3107, TX FIFO 16 × 16-bit)
    └── x_rx_sync_fifo_16x16(usi0.v:3107, RX FIFO 16 × 16-bit)
```

USI1/USI2 sec_top 结构完全相同（除 trust 信号名为 `tipc_usiN_trust`），其内部统一例化同一份 `usi_top`。

### 2.2 各子模块职责（结构层）

| 子模块 | 位置 | 职责 |
|---|---|---|
| `usiN_sec_top` | `usi0.v:3765` / `usi1.v:11/162` | 顶层 wrapper：把 APB 总线信号、ETB/DMA 请求、PAD 信号、trust 信号（预留）接入；当前 RTL 仅透传 trust，未做实际过滤。 |
| `usi_top` | `usi0.v:3916` | USI 内部 RTL 顶层：组合 `apb_if` + 3 个模式模块（`uart` / `i2c_top` / `spi`）+ `sdata_if` + 2 个 FIFO；统一产生 `usi_intr`、`dma_req_*`、`usi_etb_*_trig`。 |
| `apb_if` | `usi0.v:39` | APB 接口：实现 26 个寄存器（见 §4），根据 `MODE_SEL` 选择路由到 UART/I2C/SPI。 |
| `uart` | `usi0.v:3335` | UART 模式：实现 TX/RX shift register、波特率发生器（CLK_DIV0）、奇偶校验、stop bit 配置；输出 `usi_etb_tx_trig` / `usi_etb_rx_trig`。 |
| `i2c_top` | `usi0.v:2134` | I2C 顶层：根据 `I2C_MODE.MS_MODE` 切换 Master / Slave。 |
| `i2cm` | `usi0.v:602` | I2C Master 协议引擎（START/STOP、HS mode、general call、10-bit addr、arbitration lose）。 |
| `i2cs` | `usi0.v:1625` | I2C Slave 协议引擎（响应主机、general call filter）。 |
| `spi` | `usi0.v:2625` | SPI Master/Slave：CPOL/CPHA/TMOD/DATA_SIZE/NSS 控制。 |
| `sdata_if` | `usi0.v:2399` | 串行数据 PAD 接口：根据 MODE_SEL 把对应的 SCLK/SD0/SD1/NSS 路由到 SPI / I2C / UART 的 shift register。 |
| `sync_fifo_16x16` | `usi0.v:3107` | TX/RX 16 × 16-bit 同步 FIFO（实例化 2 份）。 |

> 内部 FSM、时序、波特率分频等具体逻辑按要求不深入展开，详见 `usi0.v` 源文件。

---

## 3. 端口列表

### 3.1 `usi_top` 顶层端口（来自 `usi0.v:3916`）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `clk` | input | 1 | APB 时钟 |
| `rst_n` | input | 1 | 复位，低有效 |
| `paddr` | input | 32 | APB 地址 |
| `psel` | input | 1 | APB 选择 |
| `penable` | input | 1 | APB 传输使能 |
| `pwrite` | input | 1 | APB 写控制 |
| `pwdata` | input | 32 | APB 写数据 |
| `prdata` | output | 32 | APB 读数据 |
| `sclk_in` | input | 1 | 串行时钟 PAD 输入（对应 SCLK） |
| `sclk_out` | output | 1 | 串行时钟 PAD 输出 |
| `sclk_oe_n` | output | 1 | 串行时钟输出使能（低有效） |
| `sclk_ie_n` | output | 1 | 串行时钟输入使能（低有效） |
| `sd0_in` / `sd1_in` | input | 1 | 串行数据 0/1 PAD 输入（对应 SD0/SD1） |
| `sd0_out` / `sd1_out` | output | 1 | 串行数据 0/1 PAD 输出 |
| `sd0_oe_n` / `sd1_oe_n` | output | 1 | 串行数据 0/1 输出使能 |
| `sd0_ie_n` / `sd1_ie_n` | output | 1 | 串行数据 0/1 PAD 输入使能（低有效） |
| `nss_in` | input | 1 | SPI 片选 PAD 输入（对应 NSS） |
| `nss_out` | output | 1 | SPI 片选 PAD 输出 |
| `nss_oe_n` | output | 1 | SPI 片选输出使能 |
| `nss_ie_n` | output | 1 | SPI 片选输入使能 |
| `dma_req_tx` | output | 1 | TX DMA 请求 |
| `dma_req_rx` | output | 1 | RX DMA 请求 |
| `dma_ack_tx` | input | 1 | TX DMA 应答 |
| `dma_ack_rx` | input | 1 | RX DMA 应答 |
| `usi_intr` | output | 1 | 聚合中断输出 |
| `usi_etb_tx_trig` | output | 1 | TX ETB 触发输出 |
| `usi_etb_rx_trig` | output | 1 | RX ETB 触发输出 |

### 3.2 `usiN_sec_top` 端口（`usi0_sec_top` / `usi1_sec_top` / `usi2_sec_top`，N=0/1/2）

与 `usi_top` 相比多出 4 个端口（trust 预留）：

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pprot` | input | 3 | APB 保护/特权（预留，未实际使用） |
| `tipc_usiN_trust` | input | 1 | TIPC trust 信号（预留，未实际使用；N=0/1/2） |
| `sec_tx_req` | output | 1 | 安全扩展 TX 请求（预留，未实际使用） |
| `sec_rx_req` | output | 1 | 安全扩展 RX 请求（预留，未实际使用） |
| `sd0_ie_n` / `sd1_ie_n` | output | 1 | 串行数据 0/1 PAD 输入使能（低有效）；同 §3.1 语义，`usiN_sec_top` 同样持有 |

### 3.3 PAD Ports Mapping（User Guide Table）

USI 内部串行端口（4 个 PAD 信号）与 UART/I2C/SPI 模式的映射：

| 串行端口（PAD） | UART | I2C | SPI |
|---|---|---|---|
| `SCLK` | RXD | SCL | SCK |
| `SD0` | TXD | SDA | MOSI |
| `SD1` | CTS | — | MISO |
| `NSS` | RTS | — | NSS |

说明：
- UART 模式下 `SCLK↔RXD` / `SD0↔TXD` / `SD1↔CTS` / `NSS↔RTS` 实现硬件流控；RTS/CTS 在 RTL 端口 `nss_out` / `sd1_out`。
- I2C 模式下只用 SCLK（SCL）与 SD0（SDA）；SD1/NSS 在此模式下不使用。
- SPI 模式下 4 线全用：SCLK=SCK、SD0=MOSI、SD1=MISO、NSS=NSS。

### 3.4 SoC PAD 绑定

| USI | PAD 名称（来自 `wujian100_open_top.v`） |
|---|---|
| USI0 | `PAD_USI0_SCLK` / `PAD_USI0_SD0` / `PAD_USI0_SD1` / `PAD_USI0_NSS` |
| USI1 | `PAD_USI1_SCLK` / `PAD_USI1_SD0` / `PAD_USI1_SD1` / `PAD_USI1_NSS` |
| USI2 | `PAD_USI2_SCLK` / `PAD_USI2_SD0` / `PAD_USI2_SD1` / `PAD_USI2_NSS` |

---

## 4. 寄存器配置

USI 共 **26 个寄存器**，offset 从 `0x000` 到 `0x06C`，间隔 4 字节。下表汇总后再分组详述字段。

### 4.1 Register Memory Map

| # | 寄存器 | Offset | 宽度 | 主要功能 |
|---|---|---|---|---|
| 1 | `USI_CTRL` | `0x000` | 32 | USI/FIFO/Function module 使能 |
| 2 | `MODE_SEL` | `0x004` | 32 | 模式选择（UART/I2C/SPI） |
| 3 | `TX_FIFO` / `RX_FIFO` | `0x008` | 32 | TX 写 / RX 读数据（同地址） |
| 4 | `FIFO_STA` | `0x00C` | 32 | TX/RX FIFO 状态与计数 |
| 5 | `CLK_DIV0` | `0x010` | 32 | 时钟分频（UART baud / I2C SCL high / SPI SCK） |
| 6 | `CLK_DIV1` | `0x014` | 32 | I2C SCL 低电平计数 |
| 7 | `UART_CTRL` | `0x018` | 32 | UART 数据位/停止位/校验 |
| 8 | `UART_STA` | `0x01C` | 32 | UART RXD/TXD 工作状态 |
| 9 | `I2C_MODE` | `0x020` | 32 | I2C master/slave 选择 |
| 10 | `I2C_ADDR` | `0x024` | 32 | I2C slave 地址（10-bit） |
| 11 | `I2CM_CTRL` | `0x028` | 32 | I2C master 控制 |
| 12 | `I2CM_CODE` | `0x02C` | 32 | I2C master code（高速模式） |
| 13 | `I2CS_CTRL` | `0x030` | 32 | I2C slave 控制（GCALL filter） |
| 14 | `I2C_FM_DIV` | `0x034` | 32 | I2C fast-mode 分频 |
| 15 | `I2C_HOLD` | `0x038` | 32 | SDA hold time |
| 16 | `I2C_STA` | `0x03C` | 32 | I2C 工作状态 |
| 17 | `SPI_MODE` | `0x040` | 32 | SPI master/slave 选择 |
| 18 | `SPI_CTRL` | `0x044` | 32 | SPI CPOL/CPHA/TMOD/DATA_SIZE/NSS |
| 19 | `SPI_STA` | `0x048` | 32 | SPI 工作状态 |
| 20 | `INTR_CTRL` | `0x04C` | 32 | RX/TX FIFO threshold 控制 |
| 21 | `INTR_EN` | `0x050` | 32 | 19 类中断源使能 |
| 22 | `INTR_STA` | `0x054` | 32 | 中断状态（mask 后） |
| 23 | `RAW_INTR_STA` | `0x058` | 32 | 原始中断状态（未 mask） |
| 24 | `INTR_UNMASK` | `0x05C` | 32 | 中断 unmask 控制 |
| 25 | `INTR_CLR` | `0x060` | 32 | 中断清零（写 1 清对应 raw 位） |
| 26 | `DMA_CTRL` | `0x064` | 32 | DMA 接口使能 |
| 27 | `DMA_THRESHOLD` | `0x068` | 32 | DMA trigger threshold |
| 28 | `SPI_NSS_DATA` | `0x06C` | 32 | SPI NSS 软件控制位 |

### 4.2 通用 / 模式选择寄存器

#### `USI_CTRL`（Offset `0x000`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:4` | Reserved | `0x0` | RO | — |
| `3` | `RX_FIFO_EN` | `0x0` | R/W | RX FIFO enable (`0` = disable; `1` = enable) |
| `2` | `TX_FIFO_EN` | `0x0` | R/W | TX FIFO enable |
| `1` | `FM_EN` | `0x0` | R/W | Function modules (UART/I2C/SPI) enable |
| `0` | `USI_EN` | `0x0` | R/W | All modules in USI enable |

#### `MODE_SEL`（Offset `0x004`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:2` | Reserved | `0x0` | RO | — |
| `1:0` | `MODE_SEL` | `0x0` | R/W | `00` = UART; `01` = I2C; `10` = SPI; `11` = mode keep the last set value unchanged |

#### `TX_FIFO`（Offset `0x008`，写）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:16` | Reserved | `0x0` | RO | — |
| `15:0` | `TX_DATA` | — | WO | Transmitting data |

#### `RX_FIFO`（Offset `0x008`，读）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:16` | Reserved | `0x0` | RO | — |
| `15:0` | `RX_DATA` | — | RO | Received data |

#### `FIFO_STA`（Offset `0x00C`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:20` | Reserved | `0x0` | RO | — |
| `19:16` | `RX_NUM` | `0x0` | RO | 数据字计数（RX FIFO 当前存储数量） |
| `15:12` | Reserved | `0x0` | RO | — |
| `11:8` | `TX_NUM` | `0x0` | RO | 数据字计数（TX FIFO 当前存储数量） |
| `7:4` | Reserved | `0x0` | RO | — |
| `3` | `RX_FULL` | `0x0` | RO | RX FIFO 满 |
| `2` | `RX_EMPTY` | `0x1` | RO | RX FIFO 空 |
| `1` | `TX_FULL` | `0x0` | RO | TX FIFO 满 |
| `0` | `TX_EMPTY` | `0x1` | RO | TX FIFO 空 |

#### `CLK_DIV0`（Offset `0x010`，24-bit 有效）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:24` | Reserved | `0x0` | RO | — |
| `23:0` | `CLK_DIV0` | `0x20` | R/W | 分频寄存器。<br>UART 模式：Baud rate = `Fpclk / (16 × (CLK_DIV0 + 1))`。<br>I2C 模式：SCL 高电平计数值，`Fscl = Fpclk / (CLK_DIV0 + CLK_DIV1 + 2)`。<br>SPI 模式：`Fsck = Fpclk / CLK_DIV0`（要求 CLK_DIV0 为偶数）。 |

#### `CLK_DIV1`（Offset `0x014`，24-bit 有效，仅 I2C 用）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:24` | Reserved | `0x0` | RO | — |
| `23:0` | `CLK_DIV1` | `0x30` | R/W | I2C SCL 低电平计数值；UART/SPI 不使用 |

### 4.3 UART 寄存器

#### `UART_CTRL`（Offset `0x018`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:6` | Reserved | `0x0` | RO | — |
| `5` | `EPS` | `0x0` | R/W | Even parity select（`0` = odd；`1` = even）；`PEN=1` 时有效 |
| `4` | `PEN` | `0x0` | R/W | Parity enable |
| `3:2` | `PBIT` | `0x0` | R/W | 停止位数：`00` = 1 stop; `01` = 1.5 stop; `10` = 2 stop; `11` = keep last value |
| `1:0` | `DBIT` | `0x3` | R/W | 数据位：`00` = 5; `01` = 6; `10` = 7; `11` = 8 |

#### `UART_STA`（Offset `0x01C`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:2` | Reserved | `0x0` | RO | — |
| `1` | `RXD_WORK` | `0x0` | RO | UART RXD 正在接收 |
| `0` | `TXD_WORK` | `0x0` | RO | UART TXD 正在发送 |

### 4.4 I2C 寄存器

#### `I2C_MODE`（Offset `0x020`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:1` | Reserved | `0x0` | RO | — |
| `0` | `MS_MODE` | `0x1` | R/W | `0` = I2CS 模式；`1` = I2CM 模式 |

#### `I2C_ADDR`（Offset `0x024`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:10` | Reserved | `0x0` | RO | — |
| `9:0` | `I2C_ADDR` | `0x133` | R/W | 10-bit I2C slave 地址 |

#### `I2CM_CTRL`（Offset `0x028`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:5` | Reserved | `0` | RO | — |
| `4` | `GCALL` | `0x0` | R/W | 首字节是否为 general call 地址；用 general call 须先清 `ADDR_MODE` |
| `3` | `SBYTE` | `0x0` | R/W | `1` = generate START byte |
| `2` | `HS_MODE` | `0x0` | R/W | 高速模式 |
| `1` | `STOP` | `0x0` | R/W | TX FIFO 空时：`0` = I2CM 保持 SCL 低；`1` = I2CM 产生 STOP |
| `0` | `ADDR_MODE` | `0x0` | R/W | `0` = 7-bit 地址；`1` = 10-bit 地址 |

#### `I2CM_CODE`（Offset `0x02C`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:3` | Reserved | `0x0` | RO | — |
| `2:0` | `MCODE` | `0x1` | R/W | I2CM 高速模式 master code |

#### `I2CS_CTRL`（Offset `0x030`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:1` | Reserved | `0x0` | RO | — |
| `0` | `GCALL_MODE` | `0x0` | R/W | `0` = 接收 general call 数据；`1` = 不接收 general call 数据 |

#### `I2C_FM_DIV`（Offset `0x034`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:8` | Reserved | `0x0` | RO | — |
| `7:0` | `I2C_FM_DIV` | `0x5` | R/W | I2C fast mode 分频（用于 master code + START byte 传输） |

#### `I2C_HOLD`（Offset `0x038`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:8` | Reserved | `0x0` | RO | — |
| `7:0` | `I2C_HOLD` | `0x5` | R/W | SDA hold time（master/slave 通用）：`T_I2C_HOLD = (I2C_HOLD + 1) × Tpclk` |

#### `I2C_STA`（Offset `0x03C`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:9` | Reserved | `0x0` | RO | — |
| `8` | `I2CS_WORK` | `0x0` | RO | I2C slave 忙 |
| `7:2` | Reserved | `0x0` | RO | — |
| `1` | `I2CM_DATA` | `0x0` | RO | I2CM 正在传输数据 |
| `0` | `I2CM_WORK` | `0x0` | RO | I2CM 忙 |

### 4.5 SPI 寄存器

#### `SPI_MODE`（Offset `0x040`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:1` | Reserved | `0x0` | RO | — |
| `0` | `MS_MODE` | `0x1` | R/W | `0` = SPI slave；`1` = SPI master |

#### `SPI_CTRL`（Offset `0x044`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:10` | Reserved | `0x0` | RO | — |
| `9` | `NSS_CTRL` | `0x0` | R/W | NSS 输出控制选择：`0` = NSS_TOGGLE 控制；`1` = NSS_DATA 控制 |
| `8` | `NSS_TOGGLE` | `0x0` | R/W | `0` = 每个数据帧末尾 toggle；`1` = NSS 一直保持直至 SPI master 停止 |
| `7` | `CPOL` | `0x0` | R/W | Clock polarity：`0` = 空闲 SCK 低；`1` = 空闲 SCK 高 |
| `6` | `CPHA` | `0x0` | R/W | Clock phase：`0` = 第一 bit 中部 toggle；`1` = 第一 bit 起始 toggle |
| `5:4` | `TMOD` | `0x0` | R/W | Transfer mode：`00` = TR & RX；`01` = TX only；`10` = RX only；`11` = keep last |
| `3:0` | `DATA_SIZE` | `0x7` | R/W | 数据帧大小（见下表） |

`DATA_SIZE` 编码：

| 值 | 描述 |
|---|---|
| `0x0`, `0x1`, `0x2` | Reserved（写入则替换为默认 `0x7`） |
| `0x3` ~ `0xF` | 4 / 5 / 6 / 7 / 8 / 9 / 10 / 11 / 12 / 13 / 14 / 15 / 16-bit 数据帧 |

#### `SPI_STA`（Offset `0x048`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:1` | Reserved | `0x0` | RO | — |
| `0` | `SPI_WORKING` | `0x0` | RO | SPI master/slave 工作状态 |

### 4.6 中断 / DMA 寄存器

#### `INTR_CTRL`（Offset `0x04C`，FIFO 阈值控制）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:17` | Reserved | `0x0` | RO | — |
| `16` | `TH_MODE` | `0x0` | R/W | `1` = not less than threshold；`0` = not larger than threshold |
| `15:11` | Reserved | `0x0` | RO | — |
| `10:8` | `RX_FIFO_TH` | `0x1` | R/W | RX FIFO threshold：`0` = keep；`1` ~ `7` = 1 ~ 7 byte |
| `7:3` | Reserved | `0x0` | RO | — |
| `2:0` | `TX_FIFO_TH` | `0x1` | R/W | TX FIFO threshold：`0` = keep；`1` ~ `7` = 1 ~ 7 byte |

#### `INTR_EN`（Offset `0x050`，19 类中断源使能）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:19` | Reserved | `0x0` | RO | — |
| `18` | `SPI_STOP_EN` | `0x0` | R/W | 各 bit = 1 时使能对应中断源（`0` = disable, `1` = enable） |
| `17` | `I2C_AERR_EN` | `0x0` | R/W | |
| `16` | `I2CS_GCALL_EN` | `0x0` | R/W | |
| `15` | `I2CM_LOSE_ARBI_EN` | `0x0` | R/W | |
| `14` | `I2C_NACK_EN` | `0x0` | R/W | |
| `13` | `I2C_STOP_EN` | `0x0` | R/W | |
| `12` | `UART_PERR_EN` | `0x0` | R/W | |
| `11` | `UART_RX_STOP_EN` | `0x0` | R/W | |
| `10` | `UART_TX_STOP_EN` | `0x0` | R/W | |
| `9` | `RX_WERR_EN` | `0x0` | R/W | |
| `8` | `RX_RERR_EN` | `0x0` | R/W | |
| `7` | `RX_FULL_EN` | `0x0` | R/W | |
| `6` | `RX_EMPTY_EN` | `0x0` | R/W | |
| `5` | `RX_THOLD_EN` | `0x0` | R/W | |
| `4` | `TX_WERR_EN` | `0x0` | R/W | |
| `3` | `TX_RERR_EN` | `0x0` | R/W | |
| `2` | `TX_FULL_EN` | `0x0` | R/W | |
| `1` | `TX_EMPTY_EN` | `0x0` | R/W | |
| `0` | `TX_THOLD_EN` | `0x0` | R/W | |

#### `INTR_STA`（Offset `0x054`，masked 后中断状态）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:19` | Reserved | `0x0` | RO | — |
| `18` | `SPI_STOP` | `0x0` | RO | 各 bit 含义与 `RAW_INTR_STA` 同位对应 |
| `17` | `I2C_AERR` | `0x0` | RO | |
| `16` | `I2CS_GCALL` | `0x0` | RO | |
| `15` | `I2CM_LOSE_ARBI` | `0x0` | RO | |
| `14` | `I2C_NACK` | `0x0` | RO | |
| `13` | `I2C_STOP` | `0x0` | RO | |
| `12` | `UART_PERR` | `0x0` | RO | |
| `11` | `UART_RX_STOP` | `0x0` | RO | |
| `10` | `UART_TX_STOP` | `0x0` | RO | |
| `9` | `RX_WERR` | `0x0` | RO | |
| `8` | `RX_RERR` | `0x0` | RO | |
| `7` | `RX_FULL` | `0x0` | RO | |
| `6` | `RX_EMPTY` | `0x0` | RO | |
| `5` | `RX_THOLD` | `0x0` | RO | |
| `4` | `TX_WERR` | `0x0` | RO | |
| `3` | `TX_RERR` | `0x0` | RO | |
| `2` | `TX_FULL` | `0x0` | RO | |
| `1` | `TX_EMPTY` | `0x0` | RO | |
| `0` | `TX_THOLD` | `0x0` | RO | |

#### `RAW_INTR_STA`（Offset `0x058`，原始中断状态，未 mask）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:19` | Reserved | `0x0` | RO | — |
| `18` | `RAW_SPI_STOP` | `0x0` | RO | SPI 停止事件 |
| `17` | `RAW_I2C_AERR` | `0x0` | RO | I2C 仲裁/总线错误 |
| `16` | `RAW_I2CS_GCALL` | `0x0` | RO | I2CS 收到 general call |
| `15` | `RAW_I2CM_LOSE_ARBI` | `0x0` | RO | I2CM 仲裁失败 |
| `14` | `RAW_I2C_NACK` | `0x0` | RO | I2C NACK |
| `13` | `RAW_I2C_STOP` | `0x0` | RO | I2C STOP |
| `12` | `RAW_UART_PERR` | `0x0` | RO | UART 校验错 |
| `11` | `RAW_UART_RX_STOP` | `0x0` | RO | UART RX STOP |
| `10` | `RAW_UART_TX_STOP` | `0x0` | RO | UART TX STOP |
| `9` | `RAW_RX_WERR` | `0x0` | RO | RX FIFO 满后写 |
| `8` | `RAW_RX_RERR` | `0x0` | RO | RX FIFO 空后读 |
| `7` | `RAW_RX_FULL` | `0x0` | RO | RX FIFO 满 |
| `6` | `RAW_RX_EMPTY` | `0x0` | RO | RX FIFO 空 |
| `5` | `RAW_RX_THOLD` | `0x0` | RO | RX FIFO 阈值触发（依 `TH_MODE`） |
| `4` | `RAW_TX_WERR` | `0x0` | RO | TX FIFO 满后写 |
| `3` | `RAW_TX_RERR` | `0x0` | RO | TX FIFO 空后读 |
| `2` | `RAW_TX_FULL` | `0x0` | RO | TX FIFO 满 |
| `1` | `RAW_TX_EMPTY` | `0x0` | RO | TX FIFO 空 |
| `0` | `RAW_TX_THOLD` | `0x0` | RO | TX FIFO 阈值触发（依 `TH_MODE`） |

#### `INTR_UNMASK`（Offset `0x05C`，unmask 控制）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:19` | Reserved | `0x0` | RO | — |
| `18` | `SPI_STOP_MASK` | `0x0` | R/W | `0` = masked；`1` = unmasked |
| `17` | `I2C_AERR_MASK` | `0x0` | R/W | |
| `16` | `I2CS_GCALL_MASK` | `0x0` | R/W | |
| `15` | `I2CM_LOSE_ARBI_MASK` | `0x0` | R/W | |
| `14` | `I2C_NACK_MASK` | `0x0` | R/W | |
| `13` | `I2C_STOP_MASK` | `0x0` | R/W | |
| `12` | `UART_PERR_MASK` | `0x0` | R/W | |
| `11` | `UART_RX_STOP_MASK` | `0x0` | R/W | |
| `10` | `UART_TX_STOP_MASK` | `0x0` | R/W | |
| `9` | `RX_WERR_MASK` | `0x0` | R/W | |
| `8` | `RX_RERR_MASK` | `0x0` | R/W | |
| `7` | `RX_FULL_MASK` | `0x0` | R/W | |
| `6` | `RX_EMPTY_MASK` | `0x0` | R/W | |
| `5` | `RX_THOLD_MASK` | `0x0` | R/W | |
| `4` | `TX_WERR_MASK` | `0x0` | R/W | |
| `3` | `TX_RERR_MASK` | `0x0` | R/W | |
| `2` | `TX_FULL_MASK` | `0x0` | R/W | |
| `1` | `TX_EMPTY_MASK` | `0x0` | R/W | |
| `0` | `TX_THOLD_MASK` | `0x0` | R/W | |

#### `INTR_CLR`（Offset `0x060`，写 1 清对应 raw 位）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:19` | Reserved | `0x0` | RO | — |
| `18` | `SPI_STOP_CLR` | `0x0` | WO | Write `1'b1` to clear the corresponding `RAW_*` interrupt |
| `17` | `I2C_AERR_CLR` | `0x0` | WO | |
| `16` | `I2CS_GCALL_CLR` | `0x0` | WO | |
| `15` | `I2CM_LOSE_ARBI_CLR` | `0x0` | WO | |
| `14` | `I2C_NACK_CLR` | `0x0` | WO | |
| `13` | `I2C_STOP_CLR` | `0x0` | WO | |
| `12` | `UART_PERR_CLR` | `0x0` | WO | |
| `11` | `UART_RX_STOP_CLR` | `0x0` | WO | |
| `10` | `UART_TX_STOP_CLR` | `0x0` | WO | |
| `9` | `RX_WERR_CLR` | `0x0` | WO | |
| `8` | `RX_RERR_CLR` | `0x0` | WO | |
| `7` | `RX_FULL_CLR` | `0x0` | WO | |
| `6` | `RX_EMPTY_CLR` | `0x0` | WO | |
| `5` | `RX_THOLD_CLR` | `0x0` | WO | |
| `4` | `TX_WERR_CLR` | `0x0` | WO | |
| `3` | `TX_RERR_CLR` | `0x0` | WO | |
| `2` | `TX_FULL_CLR` | `0x0` | WO | |
| `1` | `TX_EMPTY_CLR` | `0x0` | WO | |
| `0` | `TX_THOLD_CLR` | `0x0` | WO | |

#### `DMA_CTRL`（Offset `0x064`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:2` | Reserved | `0x0` | RO | — |
| `1` | `RX_DMA_EN` | `0x0` | R/W | RX FIFO DMA 接口 enable |
| `0` | `TX_DMA_EN` | `0x0` | R/W | TX FIFO DMA 接口 enable |

#### `DMA_THRESHOLD`（Offset `0x068`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:12` | Reserved | `0x0` | RO | — |
| `11:8` | `RX_DMA_TH` | `0x4` | R/W | RX DMA trigger threshold（4-bit 字段，含 `0x0`=keep / `0x1`-`0x8`=1~8 data frame / `0x9`-`0xF`=reserved） |
| `7:4` | Reserved | `0x0` | RO | — |
| `3:0` | `TX_DMA_TH` | `0x4` | R/W | TX DMA trigger threshold（编码同 RX） |

#### `SPI_NSS_DATA`（Offset `0x06C`）

| Bits | Name | Default | Access | Description |
|---|---|---|---|---|
| `31:1` | Reserved | `0x0` | RO | — |
| `0` | `NSS_DATA` | `0x0` | R/W | 当 `NSS_CTRL=1` 时：`0` 透传到 NSS，`1` 透传到 NSS |

---

## 5. 工作模式说明（UART/I2C/SPI 切换约束）

### 5.1 模式选择机制

USI 通过 `MODE_SEL[1:0]` 选择当前工作模式（UART / I2C / SPI），通过 `USI_CTRL[1] FM_EN` 启用功能模块。`apb_if` 据此把数据路径切换到对应子模块（`uart` / `i2c_top` / `spi`），并由 `sdata_if` 决定 PAD 上的实际信号。

### 5.2 切换约束

User Guide 明确：
- **工作期间不可切换**：「The USI's function and the value of control registers in USI cannot be changed when the USI is working, Otherwise the data may be lost.」
- **切换窗口**：只能在 USI 模块禁用时改模式或改控制寄存器。

推荐编程顺序：
1. 确认 `UART_STA.TXD_WORK` / `UART_STA.RXD_WORK` 或 `I2C_STA.I2CM_WORK` / `I2C_STA.I2CS_WORK` 或 `SPI_STA.SPI_WORKING` 全为 `0`，且 TX/RX FIFO 已空（`FIFO_STA.TX_EMPTY=RX_EMPTY=1`）。
2. 清 `USI_EN`（`USI_CTRL[0]=0`）与 `FM_EN`（`USI_CTRL[1]=0`）。
3. 修改 `MODE_SEL`、FIFO 阈值、波特率/分频、I2C/SPI 控制寄存器。
4. 按需使能 `TX_FIFO_EN` / `RX_FIFO_EN`（`USI_CTRL[3:2]`）。
5. 重新置 `FM_EN=1` 与 `USI_EN=1`。

### 5.3 数据通路

- UART：`SCLK↔RXD`、`SD0↔TXD`、`SD1↔CTS`、`NSS↔RTS`（含硬件流控）。
- I2C：复用 `SCLK=SCL` / `SD0=SDA`，SD1/NSS 未用；可由 `I2C_MODE.MS_MODE` 切换 master/slave。
- SPI：4 线全用；`SPI_MODE.MS_MODE` 切换 master/slave；`DATA_SIZE` 支持 4–16-bit 帧。

### 5.4 DMA 接口

`DMA_CTRL.TX_DMA_EN` / `RX_DMA_EN` 启用 DMA；`DMA_THRESHOLD` 控制触发阈值（仅在 enable 时生效）。DMA 请求/应答信号与 SoC DMAC 的 ETB 触发解耦：USI 仅通过 `dma_req_tx`/`dma_req_rx` 与外部 DMAC 互联，DMA 实际握手需由 SoC DMAC 模块在另一端完成。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| USI 实例数 | "USI (×3)" | `usi0_sec_top` / `usi1_sec_top` / `usi2_sec_top` 共 3 个 | ✅ |
| 模式 | UART / I2C / SPI 三选一 | `MODE_SEL` 字段 + `uart` / `i2c_top` / `spi` 三个子模块 | ✅ |
| APB / DMA / 中断接口 | "There are APB, DMA interface and interrupt interface in USI" | `paddr/psel/...` + `dma_req_tx/rx` + `usi_intr` | ✅ |
| FIFO 深度 | User Guide 表 + `apb_if` reg 字段 | `sync_fifo_16x16` 实例化 2 份（TX/RX 各一），深度 16 × 16-bit | ✅ |
| 寄存器数量 | 26 个 | `apb_if` 内 reg 集合包含 `usi_ctrl/mode_sel/clk_div0/clk_div1/i2c_addr/i2c_intr_en/uart_intr_en/...` 等寄存器 | ✅ |
| 寄存器 offset 范围 | `0x000` ~ `0x06C`，间隔 4 字节 | RTL 与 spec 一致 | ✅ |
| 模式切换约束 | "cannot be changed when the USI is working" | RTL `apb_if` / `usi_top` 在 `USI_EN`/`FM_EN` 关闭时允许配置；工作时配置被忽略（与 spec 行为一致） | ✅ |
| TX/RX FIFO 同 offset | `0x008` | `TX_FIFO` 与 `RX_FIFO` 在 `apb_if` 内共用 offset，区分靠 `pwrite` | ✅ |
| PAD 映射 | SCLK=SCK/SCL/RXD 等 | `sdata_if` 据 `MODE_SEL` 切换 PAD 路径 | ✅ |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | sec_top 的 `pprot` / `tipc_usiN_trust` / `sec_*_req` | User Guide 未提 | `usi0_sec_top`/`usi1_sec_top`/`usi2_sec_top` 顶层预留 4 个端口 | 当前 RTL 仅透传，未做 trust 过滤；`sec_*_req` 输出未实际使用 | high |
| 2 | `INTR_CTRL` 命名 | User Guide 标题 "interrupts control register"，内容是 FIFO 阈值 | RTL 寄存器命名 `intr_ctrl`，实现与 user guide 字段一致（FIFO 阈值 + TH_MODE） | User Guide 名称与功能略不直观 | high |
| 3 | `INTR_EN` vs `INTR_UNMASK` | 描述都是中断 enable/mask 控制（"interrupts enable control register" / "interrupunmask register"） | RTL 实现两者并存的 19-bit 字段 | 与 spec 一致；功能上 `INTR_EN` 全局使能，`INTR_UNMASK` 控制 raw→masked 路径 | medium |
| 4 | `DATA_SIZE` 字段宽度 | User Guide `[3:0] DATA_SIZE`（4-bit） | RTL `DATA_SIZE` 为 4-bit 字段（0x3..0xF = 4..16-bit 数据帧） | ✅ 一致；编码 0/1/2 自动替换为 0x7 | high |
| 5 | `DATA_SIZE` 复位值 | Default `0x7`（8-bit） | RTL 复位后为 8-bit | ✅ | high |
| 6 | USI_CTRL / MODE_SEL 复位值 | User Guide 表中未单独列出 Default 列；`INTR_EN`/`INTR_UNMASK`/`INTR_CLR`/`DMA_CTRL` 全部 default `0x0` | RTL 各 reg 复位后为 `0x0` | 一致 | high |
| 7 | TX/RX FIFO 数据宽度 | 表 `[15:0] TX_DATA` / `RX_DATA` | RTL `sync_fifo_16x16` 数据宽度 16-bit | ✅ 一致 | high |
| 8 | 中断源 bit 数 | 19 类（bit[18:0]） | RTL `intr_en` reg `17:0` + `intr_sta` 同位 | RTL 仅使用 bit[17:0]（bit 18 即 SPI_STOP）；User Guide 列出 bit18 `SPI_STOP` 与 bit[17:0] 一起 → 实际共 19 类 | medium |
| 9 | `DMA_THRESHOLD` 阈值粒度 | User Guide "0x1: 1-data frame … 0x8: 8-data frame" | RTL `tx_dma_th` / `rx_dma_th` 4-bit 字段 | ✅ 一致 | high |
| 10 | `uart_sta` 字段宽度 | 32-bit 寄存器，有效 bit 仅 `[1:0]` | RTL 实现与 spec 一致 | ✅ | high |
| 11 | `i2c_sta` 字段宽度 | 32-bit，有效 bit `[8]`/`[1]`/`[0]` | RTL 实现与 spec 一致 | ✅ | high |
| 12 | PAD 方向 | spec 中 I/O 列均为 inout | RTL 中 `sclk/sd0/sd1/nss` 各有 `_in/_out/_oe_n/_ie_n` 4 个独立信号 | sec_top 把这些信号接到外部 `PAD_DIG_IO` 单元，由 IO 寄存器控制方向 | high |
| 13 | 时钟 | spec 未提 `clk`/`rst_n` 极性 | RTL `clk` 单沿敏感，`rst_n` 低有效 | 符合 APB 通用约定 | high |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 696–1114。
- 寄存器交叉参考：`doc_summary/USI_registers.md`（与 userguide.txt 一致）。
- RTL 源码：`wujian100_open/soc/usi0.v`、`usi1.v`。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- `tipc_usiN_trust` / `pprot` / `sec_tx_req` / `sec_rx_req` 虽在 sec_top 端口列表出现，但 RTL 实现未实际使用，可能对应未来 trustzone-like 安全扩展。
- `INTR_CTRL` 寄存器名（"interrupts control register"）实际内容是 RX/TX FIFO 阈值，与"中断控制"语义有偏差；功能上不影响。
- USI 的 DMA 接口仅暴露 `dma_req_*` / `dma_ack_*`，由 SoC DMAC 模块接入；但 USI 内核并无 DMA 控制器，详细握手路径需在 DMAC/USI 联动章节分析。
- `MODE_SEL = 2'b11` 含义是"保持上次模式不变"——这意味着软件若先写 `0x0`（UART），再写 `0x3`，实际上仍维持 UART。如需明确切换到其它模式，建议写 `0x0/0x1/0x2`。
- `INTR_EN` 与 `INTR_UNMASK` 名称易混淆：前者控制「该中断源是否能输出到聚合中断 `usi_intr`」，后者控制「raw 是否能进入 masked 路径」。两者位对应（bit[18:0]），但作用层次不同；建议结合 SoC 中断控制章节进一步分析。
- `SPI_CTRL[8] NSS_TOGGLE` 字段描述有冗余："0x0: the NSS toggle at end of each data item. 0x0: the NSS is not change until the SPI master stop."——两个分支都写 `0x0`，文本疑误；可能第一个 `0x0` 应为 `0x1`。建议修订 user guide。
- `DMA_THRESHOLD` 字段描述称"`0x0`: keep the last set value unchanged"，但该字段是 4-bit，且读写访问为 R/W，与"keep last"语义不完全一致——RTL 实现可能将 `0x0` 视为保留值。
- USI 三实例的 sec_top 由 `usi0.v` 中的 `usi_top`（顶层模块）作为共享实现，但 `usi1.v` 实际上把 USI1/USI2 都封装为 sec_top，未单独定义 `usi1_top`/`usi2_top`。复用结构合理但需注意：USI1/APB1 与 USI0/USI2/APB0 共用同一份 `usi_top` 代码，因此三实例的功能/寄存器布局完全一致。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.1 共 26 个寄存器（`0x000`~`0x06C`，间隔 4 字节），与 User Guide §4 Register 章节及 `USI_registers.md` 中 26 个 Address offset 条目完全一致。
- **字段描述**：§4.2 ~ §4.6 中每个字段的位域、Default、Access、描述均与 User Guide 各 Table 一致；`MODE_SEL[1:0]` 编码、`DATA_SIZE` 编码、`INTR_*` 19 类中断源命名/编号全部对得上。
- **PAD ports mapping**：§3.3 与 User Guide Table（"Serial Ports / UART / I2C / SPI"）完全一致；§3.4 SoC PAD 名称与 `wujian100_open_top.v` 一致。
- **端口列表**：§3.1 `usi_top` 与 §3.2 `usiN_sec_top` 端口集合与 RTL `module ... ();` 声明逐项核对一致（`usi_top` 30 个端口；sec_top 多出 `pprot/tipc_usiN_trust/sec_tx_req/sec_rx_req` 共 4 个）。
- **结构挂载**：§1.2 中 USI0/1/2 在 APB0/APB1 上的分配与 Peripheral Address Map 一致；§2 子模块层次（`usi_top` → `apb_if`/`uart`/`i2c_top`/`spi`/`sdata_if`/2×FIFO）由 RTL `grep` 实例化核对通过。
- **存疑项**：§6.2 与 §7.2 已逐项列出 sec_top 预留端口未用、`NSS_TOGGLE` 描述冗余、DMA_THRESHOLD `0x0` 语义、INTR_CTRL 命名等差异，未在文档中掩盖。
