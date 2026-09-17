# T-Head wujian100_open USI (UART/I2C/SPI 三合一, ×3 实例) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Unified Serial Interface (USI)
  - 3 个实例：USI0（APB0 P4, base `0x5002_8000`）、USI1（APB1 P4, base `0x6002_8000`）、USI2（APB0 P5, base `0x5002_9000`）
  - 单实例 28 个寄存器（offset `0x00` ~ `0x6C`），其中 26 个核心寄存器 + 2 个扩展
  - 三模合一：UART / I2C / SPI，由 `MODE_SEL` 选择
  - TX/RX FIFO：16 × 16-bit，独立 DMA 请求线 `dma_req_tx` / `dma_req_rx`
  - 每 USI 1 路聚合中断 `usi_intr` + 2 路 ETB 触发线 `usi_etb_tx_trig` / `usi_etb_rx_trig`

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/usi0.v` / `usi1.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `FIFO_DEPTH` | `16` | TX/RX FIFO 深度 |
| `FIFO_WIDTH` | `16` | TX/RX FIFO 数据位宽（bit） |
| `MODE_SEL` | `2'b00`=UART / `2'b01`=I2C / `2'b10`=SPI | 模式选择（`2'b11` reserved） |
| `APB_ADDR_WIDTH` | `32`（paddr 实际有效 `12` bit） | APB 地址位宽 |
| `UART_DATA_BITS` | `5/6/7/8` | UART 数据位宽可配 |
| `SPI_DATA_SIZE` | `4 ~ 16` | SPI 单帧数据位宽可配 |
| `USI_CTRL[3:0]` | `RX_FIFO_EN/TX_FIFO_EN/FM_EN/USI_EN` | 通用使能位（FM_EN=1 才能工作） |
| `pclk` | SoC APB 总线时钟（待确认 SoC 默认频率） | USI 工作时钟 |

**关键配置含义**：
- USI 工作期间（FM_EN=1）不可修改 `MODE_SEL` 与任何控制寄存器（user guide 明确约束）；切换模式前必须先 `USI_EN=0` + `FM_EN=0`。
- TX/RX FIFO 共用 `0x08` 偏移（写 TX_FIFO / 读 RX_FIFO），写入读取操作分流；FIFO 状态由 `FIFO_STA[tx_fifo_cnt/rx_fifo_cnt]` 读取。
- 26 个寄存器按功能分组：通用（6 个）/ UART（2 个）/ I2C（8 个）/ SPI（3 个）/ 中断（5 个）/ DMA（2 个）。

### 1.2 寄存器映射（每实例相同，offset 相对实例 base）

| Offset | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | `USI_CTRL` | RW | `0x0000_0000` | `RX_FIFO_EN[3]`/`TX_FIFO_EN[2]`/`FM_EN[1]`/`USI_EN[0]` |
| `0x04` | `MODE_SEL` | RW | `0x0000_0000` | `2'b00`=UART / `2'b01`=I2C / `2'b10`=SPI / `2'b11` reserved |
| `0x08` | `TX_FIFO`/`RX_FIFO` | WO/RO | `0x0000_0000` | 写 TX / 读 RX，同地址 |
| `0x0C` | `FIFO_STA` | RO | `0x0000_0011` | TX/RX FIFO 状态与计数 |
| `0x10` | `CLK_DIV0` | RW | `0x0000_0000` | 时钟分频（UART baud / I2C SCL high / SPI SCK） |
| `0x14` | `CLK_DIV1` | RW | `0x0000_0000` | I2C SCL 低电平计数 |
| `0x18` | `UART_CTRL` | RW | `0x0000_0000` | UART 数据位 / 停止位 / 校验 |
| `0x1C` | `UART_STA` | RO | `0x0000_0000` | UART RXD/TXD 工作状态 |
| `0x20` | `I2C_MODE` | RW | `0x0000_0000` | I2C master/slave 选择 |
| `0x24` | `I2C_ADDR` | RW | `0x0000_0000` | I2C slave 地址（10-bit 模式可配） |
| `0x28` | `I2CM_CTRL` | RW | `0x0000_0000` | I2C master 控制 |
| `0x2C` | `I2CM_CODE` | RW | `0x0000_0000` | I2C master code（高速模式） |
| `0x30` | `I2CS_CTRL` | RW | `0x0000_0000` | I2C slave 控制（GCALL filter） |
| `0x34` | `I2C_FM_DIV` | RW | `0x0000_0000` | I2C fast-mode 分频 |
| `0x38` | `I2C_HOLD` | RW | `0x0000_0000` | SDA hold time |
| `0x3C` | `I2C_STA` | RO | `0x0000_0000` | I2C 工作状态 |
| `0x40` | `SPI_MODE` | RW | `0x0000_0000` | SPI master/slave 选择 |
| `0x44` | `SPI_CTRL` | RW | `0x0000_0000` | CPOL/CPHA/TMOD/DATA_SIZE/NSS |
| `0x48` | `SPI_STA` | RO | `0x0000_0000` | SPI 工作状态 |
| `0x4C` | `INTR_CTRL` | RW | `0x0000_0000` | RX/TX FIFO threshold |
| `0x50` | `INTR_EN` | RW | `0x0000_0000` | 19 类中断源使能 |
| `0x54` | `INTR_STA` | RO | `0x0000_0000` | 中断状态（mask 后） |
| `0x58` | `RAW_INTR_STA` | RO | `0x0000_0000` | 原始中断状态（未 mask） |
| `0x5C` | `INTR_UNMASK` | RW | `0x0000_0000` | 中断 unmask 控制 |
| `0x60` | `INTR_CLR` | WO | `0x0000_0000` | 中断清零（写 1 清对应 raw 位） |
| `0x64` | `DMA_CTRL` | RW | `0x0000_0000` | DMA 接口使能 |
| `0x68` | `DMA_THRESHOLD` | RW | `0x0000_0000` | DMA trigger threshold |
| `0x6C` | `SPI_NSS_DATA` | RW | `0x0000_0000` | SPI NSS 软件控制位 |

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `apb0_sub_top.v` / `apb1_sub_top.v`）

- **时钟**：3 个 USI 实例共享 SoC APB `pclk`；`usi_top` 内部由 `clk` 输入。
- **复位**：`rst_n`（低有效）。
- **总线挂载**：
  - USI0 → APB0 P4，base `0x5002_8000`
  - USI1 → APB1 P4，base `0x6002_8000`
  - USI2 → APB0 P5，base `0x5002_9000`
- **DMA**：每 USI 各有 `dma_req_tx` / `dma_req_rx` 接 DMAC，触发后批量搬入搬出 FIFO（与 DMAC `dma_req_*` / `dma_ack_*` 配对）。
- **中断**：每 USI 1 bit `usi_intr` → SoC CLIC/VIC（具体中断号见 System Overview Table 1-4，待确认行号）。
- **ETB**：每 USI 2 bit `usi_etb_tx_trig` / `usi_etb_rx_trig` 接入 SoC ETB fabric。
- **PAD**：每个实例对应 `PAD_USI{0,1,2}_SCLK/SD0/SD1/NSS`（具体 PAD 命名见 wujian100_open_top.v）。

### 1.4 关键 RTL 行为

1. **模式锁定**（`MODE_SEL`）：USI 工作期间（FM_EN=1）改写 MODE_SEL 可能丢数据；切换模式流程：写 `USI_CTRL=0`（全部 disable）→ 等待 FIFO 空、状态机 idle → 写 `MODE_SEL` → 重新配置相关寄存器 → 写 `USI_CTRL=enable`。
2. **FIFO 同地址双工**（TX_FIFO/RX_FIFO @ `0x08`）：APB 写入 push TX，APB 读取 pop RX；FIFO_STA 实时反映 tx_fifo_cnt/rx_fifo_cnt。
3. **UART 波特率**：`CLK_DIV0 = (pclk / (16 × baud)) - 1`；USI_UART 默认 20 MHz APB → 9600 baud → `CLK_DIV0 = 0x81`（129）。
4. **I2C HS 模式**：`I2CM_CODE` + `CLK_DIV0`/`CLK_DIV1` 共同决定 SCL 高/低电平周期，HS 模式下切换到 `I2CM_CODE` 编码。
5. **SPI master NSS**：`SPI_CTRL` 配置 NSS 硬件/软件模式；`SPI_NSS_DATA` 在软件模式下输出 NSS 电平。
6. **中断聚合**：`INTR_EN` 屏蔽 → `RAW_INTR_STA`（未屏蔽）→ `INTR_STA`（屏蔽后）→ `INTR_CLR` 写 1 清 raw 位；`INTR_UNMASK` 用于控制某些特殊中断绕过 mask。
7. **DMA 接口**：`DMA_CTRL` 使能 DMA 模式后，FIFO count 达到 `DMA_THRESHOLD` 触发 `dma_req_tx` / `dma_req_rx`。
8. **数据位宽**：UART 5/6/7/8-bit；SPI 4~16-bit 帧长；FIFO 始终 16-bit 宽，需做位宽对齐。

### 1.5 TB 检查架构

- **C 端检查**：`usi_uart_test.c` / `usi_i2c_test.c` / `usi_spi_test.c` 通过 `mem_write32_(base+offset, value)` 配置 USI，写入数据 FIFO、轮询 `FIFO_STA` 与 `UART_STA` / `I2C_STA` / `SPI_STA` 等待传输完成，最后 `sim_end()`。
- **TB 端**：UVM `soc_top_for_c_case_test` 加载固件，通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记（`0x2002` = PASS / `0x1001` = FAIL）。
- **TB 监测**：未来新增 UVM 序列时需 PAD monitor（采样 `PAD_USI0_*` 波形）、ETB monitor（采样 `usi_etb_tx_trig` / `usi_etb_rx_trig`）、DMA monitor（采样 `dma_req_tx` / `dma_req_rx` 时序）。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: 模式选择与切换约束
**目标**：`MODE_SEL` 选择 UART/I2C/SPI；USI 工作期间（FM_EN=1）禁止改 MODE_SEL 或控制寄存器；切换模式需先 disable USI。
**已有 case**：3 个 c_case 测试均覆盖"disable → 写 MODE_SEL → 配置 → enable"流程（`usi_uart_test.c` 第 41-49 行、`usi_i2c_test.c` 第 41-47 行、`usi_spi_test.c` 第 41-49 行）。
**检查**：C 端执行正常切换流程成功；尝试"FM_EN=1 时改 MODE_SEL"应被 RTL 忽略或导致数据丢失（需 TB 端采样验证）。

### F2: UART 收发（TX/RX）
**目标**：UART 模式下通过 `TX_FIFO` 写入数据 → 经 `PAD_USI{0,1,2}_SD0`（TXD）串行输出；外部数据从 RXD 串行输入 → 写入 `RX_FIFO` → C 端读出。
**已有 case**：`usi_uart_test.c`（既有）— 配置 USI0 UART TX + USI1 UART RX，USI0 发数据、USI1 接收（具体收发达成依赖 PAD 短接或 TB 端联接）。
**检查**：C 端写 TX_FIFO → 轮询 `UART_STA` tx_busy 清 0 → 读 RX_FIFO 数据与发送一致；`sim_end()`。

### F3: UART 波特率 / 数据位 / 停止位 / 校验
**目标**：`CLK_DIV0` 决定波特率（`CLK_DIV0 = pclk/(16×baud) - 1`）；`UART_CTRL` 决定数据位（5/6/7/8）、停止位（1/2）、校验（none/odd/even）。
**已有 case**：`usi_uart_test.c`（既有）配置 9600 baud (`CLK_DIV0=0x81`) + 8-N-1（`UART_CTRL=0x3`）。
**检查**：C 端配置不同 baud/data_bits/stop_bits/parity 组合，校验 `UART_STA` 与传输字节正确性；TB 端 PAD monitor 采样实际 baud 时序。
**缺口**：非 8-N-1 配置（如 7-E-1、8-O-2、5/6/7 数据位）**待新建 case（标记 TBD）**。

### F4: UART 流控 RTS/CTS
**目标**：RTS/CTS 硬件流控（`NSS`/`SD1` 引脚复用）；自动暂停/恢复传输。
**已有 case**：无（既有 c_case 未覆盖流控）。
**检查**：TB 端 PAD monitor 模拟 CTS 拉低/恢复，验证 TX FIFO 传输在 CTS 低时暂停；RTS 在 RX FIFO 接近满时拉高。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 PAD 驱动。

### F5: I2C Master 地址模式（7-bit / 10-bit）
**目标**：`I2C_ADDR` 配置从机地址（7-bit 或 10-bit）；`I2C_MODE.MS_MODE=1` 选 master。
**已有 case**：`usi_i2c_test.c`（既有）— 配置 7-bit 地址 `0x3c` + master 模式。
**检查**：C 端 TB 模拟从机响应，验证主机发送的地址字节与配置一致；10-bit 地址测试（`I2C_ADDR` 高 2 bit 编码）。
**缺口**：10-bit 地址模式**待新建 case（标记 TBD）**。

### F6: I2C HS（High-Speed）模式
**目标**：`I2CM_CODE` 编码 master code（0000_1XXX）启动 HS 模式；HS 模式下切换 SCL 频率到 `I2CM_CODE` 决定的高速。
**已有 case**：无（既有 c_case 仅 100 KHz 标准模式）。
**检查**：TB 端 PAD monitor 采样 SCL 时序验证 HS 切换点（master code 发送后切换高速）。
**缺口**：**待新建 case（标记 TBD）**。

### F7: I2C General Call（GCALL）filter
**目标**：`I2CS_CTRL` 配置 GCALL filter；slave 模式下响应地址 `0x00` 的广播。
**已有 case**：无（既有 c_case 仅 master 模式）。
**检查**：TB 端发 `address=0x00` 的 general call，验证 slave 是否响应（filter enable 时不应 ACK）。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 I2C slave 模拟。

### F8: I2C hold time（SDA hold）
**目标**：`I2C_HOLD` 决定 SCL 下降沿到 SDA 变化之间的保持时间（满足 I2C 协议 tHD:DAT）。
**已有 case**：无（既有 c_case 使用默认 hold time）。
**检查**：TB 端 PAD monitor 采样 SCL/SDA 时序，验证 hold time 与寄存器配置一致。
**缺口**：**待新建 case（标记 TBD）**。

### F9: SPI CPOL / CPHA / DATA_SIZE
**目标**：`SPI_CTRL` 配置 CPOL（clock polarity）、CPHA（clock phase）、TMOD（transmit only/receive only/全双工）、DATA_SIZE（4~16）、NSS 软件/硬件。
**已有 case**：`usi_spi_test.c`（既有）— 配置 `SPI_CTRL=0x1f`（data_size=16 + transmit only + CPHA=0 + CPOL=0）。
**检查**：TB 端 PAD monitor 采样 SCK/MOSI/MISO 时序，验证 CPOL/CPHA 与配置一致；DATA_SIZE=4/8/12 等边界值。
**缺口**：CPOL=1 / CPHA=1 / DATA_SIZE=4~16 各种组合**待新建 case（标记 TBD）**。

### F10: SPI Master / Slave 切换
**目标**：`SPI_MODE` 选 master/slave；master 模式下 NSS 由硬件/CS 产生；slave 模式下 NSS 作为输入。
**已有 case**：`usi_spi_test.c`（既有）仅 master 模式。
**检查**：TB 端 2 个 USI 实例对测（一个 master、一个 slave），验证 SPI 全双工通信。
**缺口**：slave 模式 + master/slave 互联**待新建 case（标记 TBD）**。

### F11: SPI NSS 软件/硬件模式
**目标**：`SPI_CTRL.NSS=0` 硬件模式（master 自动产生）；`SPI_CTRL.NSS=1` 软件模式，由 `SPI_NSS_DATA` 控制 NSS 电平。
**已有 case**：`usi_spi_test.c`（既有）未明确配置 NSS 模式（隐含使用默认）。
**检查**：软件模式下写 `SPI_NSS_DATA` 验证 PAD NSS 输出电平；硬件模式下 NSS 由 master 协议自动控制。
**缺口**：**待新建 case（标记 TBD）**。

### F12: TX/RX FIFO 阈值与状态
**目标**：`FIFO_STA` 报告 tx_fifo_cnt / rx_fifo_cnt / tx_empty / tx_full / rx_empty / rx_full；`INTR_CTRL` 配置 TX/RX FIFO 中断阈值。
**已有 case**：3 个 c_case 测试均轮询 `FIFO_STA.tx_empty` / `.rx_empty`（`usi_uart_test.c` 第 89 行附近等）。
**检查**：C 端写满/读空 FIFO 验证 `tx_full`/`rx_empty` 位正确翻转；阈值触发中断。
**缺口**：阈值中断触发**待新建 case（标记 TBD）**。

### F13: 中断使能 / 状态 / 清除 / 屏蔽
**目标**：`INTR_EN` 屏蔽各中断源；`RAW_INTR_STA` 记录原始中断；`INTR_STA` 反映 mask 后状态；`INTR_CLR` 写 1 清；`INTR_UNMASK` 控制绕过 mask。
**已有 case**：无（既有 c_case 未直接测中断寄存器）。
**检查**：C 端配置中断源触发 → 读 RAW_INTR_STA / INTR_STA → 写 INTR_CLR → 读状态应清 0；mask/unmask 测试。
**缺口**：**待新建 case（标记 TBD）**。

### F14: DMA 接口与阈值
**目标**：`DMA_CTRL` 使能 DMA 模式；`DMA_THRESHOLD` 配置 DMA trigger 阈值；FIFO count 达到阈值后 `dma_req_tx` / `dma_req_rx` 拉高。
**已有 case**：无（既有 c_case 仅 CPU 轮询 FIFO）。
**检查**：UVM 侧 DMAC 接管 USI 的 DMA 请求做批量搬运；验证 dma_req_tx / dma_ack_tx 时序与 threshold 配置一致。
**缺口**：**待新建 case（标记 TBD）**，需 DMAC + USI 联合 UVM 序列。

### F15: 时钟分频（CLK_DIV0 / CLK_DIV1）
**目标**：`CLK_DIV0` 决定 UART baud（公式见 §1.4）、I2C SCL 高电平计数、SPI SCK 周期；`CLK_DIV1` 仅用于 I2C SCL 低电平计数。
**已有 case**：3 个 c_case 测试均配置 `CLK_DIV0`（UART=0x81 / I2C=0x63 / SPI=0xc8）。
**检查**：C 端配置不同 CLK_DIV0/1，TB 端 PAD monitor 采样实际 baud/SCL/SCK 时序验证。
**缺口**：边界值（CLK_DIV0=0/1/最大值）、I2C SCL 高低不对称的非对称分频**待新建 case（标记 TBD）**。

### F16: 多实例地址独立性
**目标**：USI0 / USI1 / USI2 各自独立 16 KB 地址空间，互不干扰。
**已有 case**：`usi_uart_test.c` 同时访问 USI0（`0x50028000`）与 USI1（`0x60028000`），隐含覆盖。
**检查**：写 USI0 寄存器不影响 USI1 状态；读 USI2（`0x50029000`）应返回 USI2 内容。
**备注**：若既有 case 已隐含覆盖可标"已覆盖"。

### F17: 寄存器复位值
**目标**：复位后 28 个寄存器回到 §1.2 reset 值（多数 `0x0000_0000`，`FIFO_STA=0x11`）。
**已有 case**：无。
**检查**：rst_n 释放后立即读 28 个寄存器验证 reset 值。
**缺口**：**待新建 case（标记 TBD）**。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `usi_uart_test`（既有 `c_case/usi_uart/usi_uart_test.c`） | `soc_top_for_c_case_test` | F1, F2, F3 (8-N-1), F15 (9600 baud), F16 | C 端基础 |
| 2 | `usi_i2c_test`（既有 `c_case/usi_i2c/usi_i2c_test.c`） | `soc_top_for_c_case_test` | F1, F5 (7-bit), F15 (100 KHz) | C 端基础 |
| 3 | `usi_spi_test`（既有 `c_case/usi_spi/usi_spi_test.c`） | `soc_top_for_c_case_test` | F1, F9 (CPOL=0/CPHA=0/data_size=16), F10 (master) | C 端基础 |
| 4 | `usi_uart_format_matrix`（TBD） | `soc_top_for_c_case_test` | F3 (5/6/7/8 数据位 + 1/2 stop + odd/even 校验) | C 端 |
| 5 | `usi_uart_flow_ctrl`（TBD） | UVM 侧 | F4 (RTS/CTS) | UVM PAD 模拟 |
| 6 | `usi_i2c_10bit_addr`（TBD） | UVM 侧 | F5 (10-bit 地址) | UVM I2C 模拟 |
| 7 | `usi_i2c_hs_mode`（TBD） | UVM 侧 | F6 (HS 模式) | UVM I2C 模拟 |
| 8 | `usi_i2c_gcall`（TBD） | UVM 侧 | F7 (general call filter) | UVM I2C slave 模拟 |
| 9 | `usi_i2c_hold`（TBD） | UVM 侧 | F8 (SDA hold time) | UVM PAD monitor |
| 10 | `usi_spi_format_matrix`（TBD） | UVM 侧 | F9 (CPOL/CPHA/DATA_SIZE 组合) | UVM PAD monitor |
| 11 | `usi_spi_master_slave`（TBD） | UVM 侧 | F10 (master/slave 互联) | UVM 2 实例对测 |
| 12 | `usi_spi_nss`（TBD） | UVM 侧 | F11 (NSS 软件/硬件模式) | UVM PAD monitor |
| 13 | `usi_fifo_threshold`（TBD） | `soc_top_for_c_case_test` | F12 (TX/RX FIFO 阈值) | C 端 |
| 14 | `usi_intr_full`（TBD） | `soc_top_for_c_case_test` | F13 (19 类中断 mask/clear) | C 端 |
| 15 | `usi_dma`（TBD） | UVM 侧 `soc_top_vseq` | F14 (DMA 接口与 threshold) | UVM DMAC 联动 |
| 16 | `usi_clk_div_boundary`（TBD） | UVM 侧 | F15 (CLK_DIV 边界值 + I2C 不对称) | UVM PAD monitor |
| 17 | `usi_reset_default`（TBD） | `soc_top_for_c_case_test` | F17 | C 端复位检查 |
| 18 | `usi_mirror_inst`（TBD） | `soc_top_for_c_case_test` | F1, F2, F5, F9 (USI1/USI2 镜像回归) | C 端回归 |

### 功能覆盖矩阵

| Feature | usi_uart_test | usi_i2c_test | usi_spi_test | usi_uart_format_matrix | usi_uart_flow_ctrl | usi_i2c_10bit_addr | usi_i2c_hs_mode | usi_i2c_gcall | usi_i2c_hold | usi_spi_format_matrix | usi_spi_master_slave | usi_spi_nss | usi_fifo_threshold | usi_intr_full | usi_dma | usi_clk_div_boundary | usi_reset_default | usi_mirror_inst |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: 模式切换约束 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| F2: UART 收发 | ✓ | - | - | ✓ | ✓ | - | - | - | - | - | - | - | - | - | - | - | - | ✓ |
| F3: UART 格式 | ✓ (8-N-1) | - | - | ✓ (×N) | ✓ | - | - | - | - | - | - | - | - | - | - | - | - | - |
| F4: UART RTS/CTS | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | - | - | - | - |
| F5: I2C 地址模式 | - | ✓ (7-bit) | - | - | - | ✓ (10-bit) | - | - | - | - | - | - | - | - | - | - | - | ✓ |
| F6: I2C HS 模式 | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | - | - |
| F7: I2C GCALL | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | - |
| F8: I2C hold time | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - |
| F9: SPI CPOL/CPHA | - | - | ✓ (CPOL=0/CPHA=0) | - | - | - | - | - | - | ✓ (×N) | ✓ | ✓ | - | - | - | - | - | - |
| F10: SPI master/slave | - | - | ✓ (master) | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | - |
| F11: SPI NSS | - | - | ✓ (隐含) | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - |
| F12: FIFO 阈值状态 | ✓ (空检查) | ✓ (空检查) | ✓ (空检查) | - | - | - | - | - | - | - | - | - | ✓ | ✓ | - | - | - | - |
| F13: 中断 4 件套 | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | ✓ | - | - | - | - |
| F14: DMA 接口 | - | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | - |
| F15: CLK_DIV | ✓ (9600) | ✓ (100K) | ✓ (100K) | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - |
| F16: 多实例 | ✓ (USI0+1) | - | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | ✓ |
| F17: 复位值 | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - |

> 矩阵用 ✓/- 标记；括号内为覆盖量。"TBD" 表示待新建 case，不阻塞既有 3 个 c_case 通过但属于覆盖缺口。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test          (UVM 序列基线)
  └── soc_top_for_c_case_test     (运行 C 端测试用例，含 usi_uart/usi_i2c/usi_spi)
```

既有 3 个 c_case 测试通过 `soc_top_for_c_case_test` 加载固件运行。UVM 侧新增 USI 专用序列（`usi_uart_flow_ctrl_seq` / `usi_i2c_slave_model` / `usi_spi_master_slave_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，USI 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/usi_uart|usi_i2c|usi_spi/usi_*_test.c` 决定具体行为。后续 TBD 用例沿用同一入口，通过修改 `c_case/` 下不同 .c 文件选择。

> **待确认**：项目是否计划引入独立 USI uvm_test 子类。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `usi_uart_test.c` 的 `printf("\nuart test successfully\n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **USI 专用 monitor（TBD）**：未来新增 UVM 侧 case 时，需在 `soc_top_env` 内增加：
  - **PAD monitor**：采样 `PAD_USI{0,1,2}_{SCLK,SD0,SD1,NSS}` 波形，验证 UART baud、SPI CPOL/CPHA、I2C SCL 时序与配置一致。
  - **I2C slave 模拟**：在 TB 侧提供从机响应模型，支持 master 测试中模拟 ACK/NACK、slave 地址匹配、general call。
  - **SPI master/slave 对测**：两个 USI 实例对测时互为 master/slave，TB 侧联接 PAD。
  - **ETB monitor**：采样 `usi_etb_tx_trig` / `usi_etb_rx_trig`。
  - **DMA monitor**：采样 `dma_req_tx` / `dma_req_rx` 时序。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `usi_uart_test`（既有） | USI0 发数据 + USI1 接收数据匹配 + `printf("uart test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `usi_i2c_test`（既有） | I2C 主机发送 + TB 端从机响应匹配 + `sim_end()` |
| `usi_spi_test`（既有） | SPI 发送完成 + `FIFO_STA.tx_empty=1` + `sim_end()` |
| `usi_uart_format_matrix` (TBD) | 各 baud/data_bits/stop/parity 组合下传输数据校验正确 |
| `usi_uart_flow_ctrl` (TBD) | CTS 拉低时 TX 暂停；RTS 拉高在 RX FIFO 接近满时 |
| `usi_i2c_10bit_addr` (TBD) | 10-bit 地址字节序列与协议一致 |
| `usi_i2c_hs_mode` (TBD) | master code 发送后 SCL 切换到高速 |
| `usi_i2c_gcall` (TBD) | GCALL filter enable 时不 ACK address=0x00 |
| `usi_i2c_hold` (TBD) | SDA hold time 与 `I2C_HOLD` 配置一致 |
| `usi_spi_format_matrix` (TBD) | CPOL/CPHA/DATA_SIZE 各组合下数据传输正确 |
| `usi_spi_master_slave` (TBD) | 2 USI 实例 master/slave 全双工通信匹配 |
| `usi_spi_nss` (TBD) | 软件模式 NSS 电平与 `SPI_NSS_DATA` 一致 |
| `usi_fifo_threshold` (TBD) | FIFO count 达到 `INTR_CTRL` 阈值时触发中断 |
| `usi_intr_full` (TBD) | 19 类中断源 mask/clear 行为符合 spec |
| `usi_dma` (TBD) | DMA trigger 时序与 `DMA_THRESHOLD` 一致 |
| `usi_clk_div_boundary` (TBD) | 边界 CLK_DIV0/1 配置下 baud/SCL/SCK 时序正确 |
| `usi_reset_default` (TBD) | 复位后 28 个寄存器值与 §1.2 reset 表一致 |
| `usi_mirror_inst` (TBD) | USI1/USI2 行为与 USI0 镜像，全部 `sim_end()` |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 usi_uart_test                (~5 min)   既有 C 端基本功能
3. 仿真 usi_i2c_test                 (~5 min)   既有 C 端基本功能
4. 仿真 usi_spi_test                 (~5 min)   既有 C 端基本功能
5. 仿真 usi_uart_format_matrix       (~5 min)   TBD case 1
6. 仿真 usi_fifo_threshold           (~5 min)   TBD case 2
7. 仿真 usi_intr_full                (~5 min)   TBD case 3
8. 仿真 usi_reset_default            (~5 min)   TBD case 4
9. 仿真 usi_mirror_inst              (~10 min)  TBD case 5（USI1/USI2 回归）
10. 仿真 usi_uart_flow_ctrl          (~10 min)  TBD UVM case 6
11. 仿真 usi_i2c_10bit_addr          (~10 min)  TBD UVM case 7
12. 仿真 usi_i2c_hs_mode             (~10 min)  TBD UVM case 8
13. 仿真 usi_i2c_gcall               (~10 min)  TBD UVM case 9
14. 仿真 usi_i2c_hold                (~10 min)  TBD UVM case 10
15. 仿真 usi_spi_format_matrix       (~10 min)  TBD UVM case 11
16. 仿真 usi_spi_master_slave        (~10 min)  TBD UVM case 12
17. 仿真 usi_spi_nss                 (~10 min)  TBD UVM case 13
18. 仿真 usi_dma                     (~10 min)  TBD UVM case 14
19. 仿真 usi_clk_div_boundary        (~10 min)  TBD UVM case 15
```

预估总时间：~145-170 min（既有 3 case ~15 min + 15 个 TBD case ~130-155 min）

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| USI 工作期间改 MODE_SEL 可能丢数据（user guide 明文约束） | 测试仅在 disable 后切换；"工作中改配置"作为负面测试需 TB 端 PAD monitor 观察数据丢失 |
| UART 收发对测依赖 PAD 短接或 TB 联接 | 既有 `usi_uart_test.c` 通过 USI0 TX + USI1 RX 实现（待确认 PAD 短接方式）；UVM 侧需 PAD loopback monitor |
| I2C slave 模式测试需要 TB 端提供 I2C master 模拟 | slave 模式测试（`usi_i2c_gcall` 等）依赖 UVM I2C master BFM；目前 SoC 内 3 个 USI 实例可互为 master/slave |
| SPI 数据位宽 4~16 全组合测试组合爆炸 | 抽样测试：4/8/12/16 + CPOL/CPHA 2×2 = 16 组合，标记为"抽样覆盖" |
| DMA 接口测试依赖 DMAC 协同 | `usi_dma` (TBD) 需在 DMAC 验证链路恢复后启动；短期仅验证 DMA_CTRL/DMA_THRESHOLD 寄存器读写 |
| USI 中断号依赖 System Overview Table 1-4；具体行号以文档最新版本为准 | TB 侧硬编码中断号（待确认）；后续以 doc_review 修复后版本对齐 |
| `FIFO_STA` reset 值 = `0x11`（`FIFO_STA[4]=tx_empty=1, [0]=rx_empty=1`）与其他寄存器 `0x0000_0000` 不同 | 复位值测试需注意差异；user guide 仅说明 tx_fifo_cnt/rx_fifo_cnt 初始为 0（FIFO 空），`0x11` 对应 reset 时 FIFO 默认空 |
| 工作模式切换约束在既有 3 个 c_case 中**已隐含遵守**（disable → 改 MODE_SEL → enable 流程），但未做"工作中改"负面测试 | 负面测试待 UVM 侧 PAD monitor 落地后补 |
| SPI master 与 slave 同时使用需要 2 个 USI 实例，且 PAD 互联正确 | `usi_spi_master_slave` (TBD) 需 TB 侧联接 `PAD_USI0_*` 与 `PAD_USI1_*` |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── usi_uart/
│   └── usi_uart_test.c           (F1, F2, F3(8-N-1), F15, F16：既有)
├── usi_i2c/
│   └── usi_i2c_test.c            (F1, F5(7-bit), F15(100K)：既有)
├── usi_spi/
│   └── usi_spi_test.c            (F1, F9(CPOL=0/CPHA=0), F10(master)：既有)
├── usi_uart/                     (TBD 新增)
│   ├── usi_uart_format_matrix.c  (F3)
│   ├── usi_fifo_threshold.c      (F12, F13)
│   ├── usi_reset_default.c       (F17)
│   └── usi_mirror_inst.c         (F1, F2, F5, F9：USI1/USI2 镜像)
└── addr_map/
    └── map_test.c                (通用地址空间 read 0 检查，含 USI 区域 0x50028000~0x50028FFF)
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
- `dv/simulation/verif_env/soc/apb0/`、`apb1/` — APB 总线侧 UVM test（待新增 USI UVM 序列时使用）

### 交叉参考文档
- `doc_summary/module_analysis/usi_analysis.md` — USI 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/USI_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 696-1114 行 — User Guide USI 章节原文
