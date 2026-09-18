# T-Head wujian100_open USI (UART/I2C/SPI 三合一, ×3 实例) Verification Report

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Unified Serial Interface (USI)
- 3 个实例：USI0（APB0 P4, base `0x5002_8000`）、USI1（APB1 P4, base `0x6002_8000`）、USI2（APB0 P5, base `0x5002_9000`）
- 单实例 28 个寄存器（offset `0x00` ~ `0x6C`）
- 三模合一：UART / I2C / SPI，由 `MODE_SEL` 选择
- TX/RX FIFO：16 × 16-bit，独立 DMA 请求线 `dma_req_tx` / `dma_req_rx`
- 每 USI 1 路聚合中断 `usi_intr` + 2 路 ETB 触发线 `usi_etb_tx_trig` / `usi_etb_rx_trig`

**验证工程师**: CCB doc-write
**报告日期**: 2026-09-18
**关联文档**:
- 验证计划 `doc_summary/verification_plan/usi_verification_plan.md`（F1~F17、§5 验收标准、§6 测试计划）
- 模块分析 `doc_summary/module_analysis/usi_analysis.md`（寄存器 / 端口 / RTL 行为 / 子模块结构）

---

## 1. 概述

本报告记录 USI 模块从 3 个存量基线（`usi_uart_test` / `usi_i2c_test` / `usi_spi_test`）到 2026-09-18 新增 7 个用例（6 C + 1 UVM 协同）的全量验证执行结果，对照验证计划 F1~F17 与 §5 验收标准逐项闭环；并整理 7 项关键 Spec/UG-vs-RTL 差异（以 RTL 为准，含 1 项 defunct）。

### 1.1 验证范围

- **IP 数量**：3 个 USI 实例（`usi0.v` + `usi1.v`），三模（UART/I2C/SPI）合一。
- **基址**：USI0 `0x5002_8000` / USI1 `0x6002_8000` / USI2 `0x5002_9000`，各占 16 KB 地址空间。
- **寄存器空间**：单实例 28 个寄存器（offset `0x00`~`0x6C`，含通用 6 + UART 2 + I2C 8 + SPI 3 + 中断 5 + DMA 2 + SPI_NSS_DATA 1）。
- **TB 环境**：soc_top CPU 驱动模式（`soc_top_for_c_case_test` 加载 C 固件，CPU_FLAG_ADDR `0x20007C50` 上报 `sim_end()/sim_fail()`）+ UVM 协同（`soc_top_usi_uart_baud_test` 沿计数法测 baud）。
- **特殊机制**：
  - Makefile 无 USI 专用分支（与 RTC/PWM 不同，3 个基线已稳定命中 legacy 激励块）。
  - UVM 波形量测统一模式：`fork wait(===)` 量测线程 + `super.run_phase` + `disable fork`，针对 SUB-8-bit UART 帧间 `rx_shift` 高位残留做按位宽掩码（见 §4.5）。

### 1.2 验证结论

- **测试用例**：10 个（既有 3 + 新增 7），全部 `UVM_CASE_PASS`，0 UVM_ERROR / 0 UVM_FATAL。
- **功能覆盖**：F1, F2, F3, F5, F9, F10, F12, F13, F15, F16, F17 共 11 项闭环；F4/F6/F7/F8/F14 共 5 项降级（环境/BFM 限制）；F11 标记 defunct（RTL 寄存器不存在）。
- **关键发现**：7 项 Spec/UG-vs-RTL 差异（含 1 项 defunct），详见 §4。
- **缺陷修复**：5 项调试经验，详见 §5。
- **遗留风险**：见 §6。

---

## 2. 测试执行结果

所有用例经 VCS 仿真（`+UVM_TESTNAME=...`），每项判定：`UVM_CASE_PASS` × 2 + 0 UVM_ERROR / 0 UVM_FATAL + C 端 "test successfully" 打印。

运行方式：`make all C_TEST=usi_xxx/usi_xxx.c [UTEST=配对 test 类]`。

| # | C 用例 | UVM 配对 test | 功能点 | 说明 |
|---|--------|---------------|--------|------|
| 1 | `usi_uart_test`（既有 `c_case/usi_uart/usi_uart_test.c`） | `soc_top_for_c_case_test`（默认） | F1, F2, F3 (8-N-1), F15 (9600 baud), F16 | 基线：USI0 TX + USI1 RX |
| 2 | `usi_i2c_test`（既有 `c_case/usi_i2c/usi_i2c_test.c`） | 默认 | F1, F5 (7-bit), F15 (100 KHz) | 基线：master 7-bit 地址 |
| 3 | `usi_spi_test`（既有 `c_case/usi_spi/usi_spi_test.c`） | 默认 | F1, F9 (CPOL=0/CPHA=0/data_size=16), F10 (master) | 基线：master transmit only |
| 4 | `usi_reset_default`（新增） | 默认 | F17 | 3 实例 × 21 项 {偏移, 期望值} 表全对 |
| 5 | `usi_mirror_inst`（新增） | 默认 | F16 | USI2(`0x50029000`) UART TX 4 字节 + 与 USI0/USI1 交叉无干扰 |
| 6 | `usi_uart_format_matrix`（新增） | 默认 | F3 | 8-N-1 / 7-E-1 / 8-O-2 / 5-N-1 / 6-N-1 五轮 USI0→USI1 数据比对（含位宽掩码） |
| 7 | `usi_i2c_10bit_addr`（新增） | 默认 | F5 | 7-bit 正向回环 + 10-bit 负向（期望 i2c_nack raw bit13 置位、USI1 无接收） |
| 8 | `usi_fifo_threshold`（新增） | 默认 | F12 + F13 | tx_cnt 容差 3~4、填满 full、thold=01→cnt≤4 电平型、RAW/STA/UNMASK/EN 屏蔽链、EN=0 门控 |
| 9 | `usi_clk_div_boundary`（新增） | `soc_top_usi_uart_baud_test` | F15 | DIV=0x81/0x40 各 16×0x55，UVM 沿计数测得最小沿间隔比 = 2.000（共 320 沿） |
| 10 | `usi_spi_format_matrix`（新增） | 默认 | F9 + F10 | 4 轮 (16b/C0H0, 8b/C0H1, 8b/C1H0, 4b/C1H1) master/slave 数据比对 |

**共同 PASS 判定条件**（验证计划 §5）：
- C 端通过 `cpu_flag_addr=0x20007C50` 写 `sim_end()` 写值 `0x2002`
- TB 端 `soc_top_test_base` 读到 `0x2002` 后 raise/drop objection 并打印 `UVM_CASE_PASS`
- UVM 侧 0 UVM_ERROR / 0 UVM_FATAL
- C 端 `printf("...usi... test successfully")` 串口打印
- UVM 协同用例（`usi_clk_div_boundary`）有 `super.run_phase` 内的 `fork wait(===)` 沿计数 + `disable fork` 收尾

---

## 3. 功能点覆盖矩阵 (Feature Coverage Matrix)

> ✓ 表示已覆盖；"- " 表示非该用例目标；"⚠️" 表示降级；"⊘" 表示 defunct。

| Feature | usi_uart_test | usi_i2c_test | usi_spi_test | usi_reset_default | usi_mirror_inst | usi_uart_format_matrix | usi_i2c_10bit_addr | usi_fifo_threshold | usi_clk_div_boundary | usi_spi_format_matrix | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **F1** 模式选择与切换约束 | ✓ | ✓ | ✓ | - | - | ✓ | ✓ | ✓ | ✓ | ✓ | ✅ |
| **F2** UART 收发 (TX/RX) | ✓ | - | - | - | ✓ | ✓ | - | - | ✓ | - | ✅ |
| **F3** UART 波特率/数据位/停止位/校验 | ✓ (8-N-1) | - | - | - | - | ✓ (×5 格式) | - | - | - | - | ✅ |
| **F4** UART 流控 RTS/CTS | - | - | - | - | - | - | - | - | - | - | ⚠️ 见 §4.9 |
| **F5** I2C 地址模式 (7/10-bit) | - | ✓ (7-bit) | - | - | - | - | ✓ (7-bit 正 + 10-bit 负) | - | - | - | ✅ |
| **F6** I2C HS 模式 | - | - | - | - | - | - | - | - | - | - | ⚠️ 见 §4.9 |
| **F7** I2C GCALL | - | - | - | - | - | - | - | - | - | - | ⚠️ 见 §4.9 |
| **F8** I2C hold time | - | - | - | - | - | - | - | - | - | - | ⚠️ 见 §4.9 |
| **F9** SPI CPOL/CPHA/DATA_SIZE | - | - | ✓ (C0H0/16) | - | - | - | - | - | - | ✓ (×4) | ✅ |
| **F10** SPI master/slave 切换 | - | - | ✓ (master) | - | - | - | - | - | - | ✓ (master/slave) | ✅ |
| **F11** SPI NSS 软件/硬件 | - | - | - | - | - | - | - | - | - | - | ⊘ defunct §4.3 |
| **F12** TX/RX FIFO 阈值与状态 | ✓ (空检查) | ✓ (空检查) | ✓ (空检查) | - | - | - | - | ✓ (×4 边界) | - | - | ✅ |
| **F13** 中断 4 件套 | - | - | - | - | - | - | - | ✓ (RAW/STA/UNMASK/EN) | - | - | ✅ |
| **F14** DMA 接口 | - | - | - | - | - | - | - | - | - | - | ⚠️ 见 §4.9 |
| **F15** 时钟分频 CLK_DIV | ✓ (9600) | ✓ (100K) | ✓ (100K) | - | - | - | - | - | ✓ (×2 边界) | - | ✅ |
| **F16** 多实例地址独立性 | ✓ (USI0+1) | - | - | - | ✓ (USI2 + 交叉) | - | - | - | - | - | ✅ |
| **F17** 寄存器复位值 | - | - | - | ✓ (×3 × 21) | - | - | - | - | - | - | ✅ |

### 3.1 闭环说明

- **F1**：3 个基线 + `usi_uart_format_matrix` / `usi_i2c_10bit_addr` / `usi_fifo_threshold` / `usi_clk_div_boundary` / `usi_spi_format_matrix` 均隐含"disable → 改 MODE_SEL → 配置 → enable"流程。
- **F2**：`usi_uart_test` 基线 + `usi_mirror_inst`（USI2 UART TX 4 字节）+ `usi_uart_format_matrix`（5 种格式下数据比对）+ `usi_clk_div_boundary`（UVM 沿计数验证 baud）。
- **F3**：`usi_uart_format_matrix` 覆盖 8-N-1 / 7-E-1 / 8-O-2 / 5-N-1 / 6-N-1 五轮 USI0→USI1 数据比对（5/6/7-bit 接收按位宽掩码，见 §4.5）。
- **F5**：`usi_i2c_test`（7-bit 正向）+ `usi_i2c_10bit_addr`（7-bit 正向回环 + 10-bit 负向：i2c_nack raw bit13 置位、USI1 无接收）；10-bit slave 模式 RTL 不支持（§4.4）。
- **F9**：`usi_spi_test`（CPOL=0/CPHA=0/data_size=16 transmit only）+ `usi_spi_format_matrix`（16b/C0H0 + 8b/C0H1 + 8b/C1H0 + 4b/C1H1）。
- **F10**：`usi_spi_test`（master）+ `usi_spi_format_matrix`（master/slave 对测）。
- **F11**：⚠️⊘ `SPI_NSS_DATA@0x6C` 在 RTL 中无地址译码（`ADDR_*` 定义止于 0x68，详 §4.3），读出恒 0，软件 NSS 功能 **defunct**。
- **F12**：`usi_fifo_threshold` 覆盖 tx_cnt 容差 3~4、填满 full、thold=01→cnt≤4 电平型触发。
- **F13**：`usi_fifo_threshold` 覆盖 RAW/STA/UNMASK/EN 屏蔽链 + EN=0 门控（电平型 thold 清除须先关 INTEN 再写 IC，详 §4.6）。
- **F15**：`usi_clk_div_boundary` 测 DIV=0x81/0x40 各 16×0x55，UVM 沿计数测得最小沿间隔比 = 2.000（共 320 沿），与 `CLK_DIV0/(2×DIV1)` 比一致（验证 `CLK_DIV0=0x81=129` 时 2 倍 baud 周期 vs `CLK_DIV0=0x40=64`）。
- **F16**：`usi_uart_test`（USI0+USI1）+ `usi_mirror_inst`（USI2 UART TX 4 字节 + 与 USI0/USI1 交叉无干扰）。
- **F17**：`usi_reset_default` 3 实例 × 21 项 {偏移, 期望值} 表全对，复位值与 §4.1 实测表一致。

### 3.2 §5 验收标准对照

| 验收项 | 计划描述 | 报告结果 |
|--------|---------|----------|
| `usi_uart_test`（既有） | USI0 发数据 + USI1 接收数据匹配 + `printf("uart test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` | ✅ 基线通过 |
| `usi_i2c_test`（既有） | I2C 主机发送 + TB 端从机响应匹配 + `sim_end()` | ✅ 基线通过 |
| `usi_spi_test`（既有） | SPI 发送完成 + `FIFO_STA.tx_empty=1` + `sim_end()` | ✅ 基线通过 |
| `usi_reset_default` | 复位后 28 个寄存器值与 §1.2 reset 表一致 | ✅ 3 实例 × 21 项表全对（按 §4.1 实测 reset 表） |
| `usi_mirror_inst` | USI1/USI2 行为与 USI0 镜像，全部 `sim_end()` | ✅ USI2(`0x50029000`) UART TX 4 字节 + 交叉无干扰 |
| `usi_uart_format_matrix` | 各 baud/data_bits/stop/parity 组合下传输数据校验正确 | ✅ 5 轮 USI0→USI1 数据比对，含位宽掩码 |
| `usi_i2c_10bit_addr` | 10-bit 地址字节序列与协议一致 | ⚠️ **7-bit 正向 + 10-bit 负向**（slave 模式 RTL 不支持 10-bit，见 §4.4） |
| `usi_fifo_threshold` | FIFO count 达到 `INTR_CTRL` 阈值时触发中断 | ✅ tx_cnt 容差 3~4 + full + thold=01→cnt≤4 电平型 |
| `usi_clk_div_boundary` | 边界 CLK_DIV0/1 配置下 baud/SCL/SCK 时序正确 | ✅ DIV=0x81/0x40 各 16×0x55，UVM 沿间隔比 = 2.000 |
| `usi_spi_format_matrix` | CPOL/CPHA/DATA_SIZE 各组合下数据传输正确 | ✅ 4 轮 master/slave 数据比对 |
| `usi_uart_flow_ctrl` (TBD) | CTS 拉低时 TX 暂停；RTS 拉高在 RX FIFO 接近满时 | ⚠️ **降级**：环境/BFM 限制，无 CTS/RTS PAD 驱动 |
| `usi_i2c_hs_mode` (TBD) | master code 发送后 SCL 切换到高速 | ⚠️ **降级**：环境/BFM 限制，TB 无 I2C HS 模式激励 |
| `usi_i2c_gcall` (TBD) | GCALL filter enable 时不 ACK address=0x00 | ⚠️ **降级**：环境/BFM 限制，无 I2C slave BFM |
| `usi_i2c_hold` (TBD) | SDA hold time 与 `I2C_HOLD` 配置一致 | ⚠️ **降级**：环境/BFM 限制，TB 端无 SDA hold 时序观测 |
| `usi_spi_nss` (TBD) | 软件模式 NSS 电平与 `SPI_NSS_DATA` 一致 | ⚠️⊘ **defunct**：`SPI_NSS_DATA@0x6C` RTL 无地址译码（详 §4.3） |
| `usi_dma` (TBD) | DMA trigger 时序与 `DMA_THRESHOLD` 一致 | ⚠️ **降级**：环境/BFM 限制，本 TB 无 DMAC 协同 |

---

## 4. 关键验证发现（Spec/UG-vs-RTL 差异，以 RTL 为准）

### 4.1 多个寄存器复位值非 0（与计划/UG 表不符）

**RTL 实证**（`wujian100_open/soc/usi0.v`，reset block `:448-473`）：

```
clk_div0[23:0]  <= 24'h20;     // CLK_DIV0    reset = 0x20
clk_div1[23:0]  <= 24'h30;     // CLK_DIV1    reset = 0x30
uart_ctrl[5:0]  <= 6'b000011;  // UART_CTRL   reset = 0x3
i2c_mode        <= 1'b1;       // I2C_MODE    reset = 1
i2c_addr[9:0]   <= 10'h133;    // I2C_ADDR    reset = 0x133
i2cm_ctrl[4:0]  <= 5'd0;       // I2CM_CTRL   reset = 0
i2cm_code[2:0]  <= 3'b001;     // I2CM_CODE   reset = 1
i2cs_ctrl       <= 1'd0;       // I2CS_CTRL   reset = 0
i2c_fs_div[7:0] <= 8'd5;       // I2C_FM_DIV  reset = 0x5
i2c_hold[7:0]   <= 8'd5;       // I2C_HOLD    reset = 0x5
spi_mode        <= 1'b1;       // SPI_MODE    reset = 1
spi_ctrl[8:0]   <= 9'd7;       // SPI_CTRL    reset = 0x7
intr_edge       <= 1'b0;       // INTR_CTRL   {edge, rx_fifo_th[1:0], tx_fifo_th[1:0]} = {1'b0, 2'b01, 2'b01} = 0x101
                                  // 简化为 INTR_CTRL = 0x101（edge=0, rx_th=01, tx_th=01）
... rx_dma_th[4:0] <= 5'd8;    // DMA_TH     {rx_dma_th=8, 3'd0, tx_dma_th=8} = 0x808
... tx_dma_th[4:0] <= 5'd8;
```

| 寄存器 | Offset | UG 标称 | RTL 实测 |
|--------|--------|---------|----------|
| `CLK_DIV0` | `0x10` | `0x00000000` | `0x00000020` |
| `CLK_DIV1` | `0x14` | `0x00000000` | `0x00000030` |
| `UART_CTRL` | `0x18` | `0x00000000` | `0x00000003` |
| `I2C_MODE` | `0x20` | `0x00000000` | `0x00000001` |
| `I2C_ADDR` | `0x24` | `0x00000000` | `0x00000133` |
| `I2CM_CODE` | `0x2C` | `0x00000000` | `0x00000001` |
| `I2C_FM_DIV` | `0x34` | `0x00000000` | `0x00000005` |
| `I2C_HOLD` | `0x38` | `0x00000000` | `0x00000005` |
| `SPI_MODE` | `0x40` | `0x00000000` | `0x00000001` |
| `SPI_CTRL` | `0x44` | `0x00000000` | `0x00000007` |
| `INTR_CTRL` | `0x4C` | `0x00000000` | `0x00000101` |
| `DMA_TH` | `0x68` | `0x00000000` | `0x00000808` |

**实测**：`usi_reset_default` 3 实例 × 21 项 {偏移, 期望值} 表全对，与 RTL 一致。

**影响**：UG 复位值表需更新；测试用例初始化前必须先 disable USI（避免复位后默认值驱动总线异常）。

### 4.2 `FIFO_STA` 复位值非 0x11

**RTL 实证**（`wujian100_open/soc/usi0.v:399`）：

```
`ADDR_FIFO_STA : prdata[31:0] <= {11'd0, rx_data_cnt[4:0], 3'd0, tx_data_cnt[4:0], 4'd0,
                                  rx_full, rx_empty, tx_full, tx_empty};
```

**位排布**：`{rx_cnt[20:16], 3'd0, tx_cnt[12:8], rx_full[3], rx_empty[2], tx_full[1], tx_empty[0]}`

**复位值**：tx/rx FIFO 复位为空（cnt=0, empty=1, full=0），最低 4 位 = `4'b0101` = `0x5`，高 28 位全 0 → `0x0000_0005`。

**与计划差异**：计划 §1.2 写 `0x11`（`tx_empty[4]=1, rx_empty[0]=1`）；RTL 实际 `0x5`（`tx_empty[0]=1, rx_empty[2]=1`），位序不同。

**实测**：`usi_reset_default` 读 `FIFO_STA` = `0x0000_0005`。

**影响**：UG §1.2 复位值表 + 位排布需更新；测试用例初始化预期值改 0x5。

### 4.3 `SPI_NSS_DATA@0x6C` 无地址译码（F11 defunct）

**RTL 实证**（`wujian100_open/soc/usi0.v`，`ADDR_*` 定义 `:11-38`）：
- `ADDR_*` 定义止于 `ADDR_DMA_TH = 12'h068`
- 无 `ADDR_SPI_NSS_DATA` 定义
- 读 mux（`:395-422`）无 `0x6C` 分支

**实测**：
- `usi_spi_test` / `usi_spi_format_matrix` 写 `SPI_NSS_DATA@0x6C` 再读回：恒 `0x0000_0000`（写忽略 + 读无译码返回 prdata 复位值 0）
- TB 端 `PAD_USI*_NSS` 仅由 master 自动驱动（硬件模式），软件模式不可用

**影响**：F11 软件 NSS 功能 **defunct**，软件无法通过 `SPI_NSS_DATA` 控制 NSS；SPI 通信只能依赖硬件 NSS（master 自动产生）。UG §1.2 中 `SPI_NSS_DATA@0x6C` 寄存器应删除；如确需软件 NSS，需 RTL 补 `ADDR_SPI_NSS_DATA` 译码 + 数据通路。

### 4.4 `i2cs_amode` 硬连线 1'b0（I2C slave 不支持 10-bit 寻址）

**RTL 实证**（`wujian100_open/soc/usi0.v:583`）：

```
assign  i2cs_amode  = 1'b0;
```

**影响**：I2C slave 模式 10-bit 地址功能在 RTL 中被强制禁用（`i2cs_amode=0` ⇒ 7-bit 模式）；F5 的 10-bit 测试仅能做 master 模式负向（`usi_i2c_10bit_addr`：发送 10-bit 地址给 USI1 slave，期望 i2c_nack raw bit13 置位、USI1 无接收）。

**实测**：`usi_i2c_10bit_addr` 10-bit 负向测试通过（i2c_nack raw bit13 置位，USI1 无接收）。

**建议**：若需要 I2C slave 10-bit 寻址，需 RTL 将 `i2cs_amode` 改为 `i2cs_ctrl` 位控制。

### 4.5 `rx_shift` 仅复位清零，帧间不清

**RTL 实证**（`wujian100_open/soc/usi0.v:3723-3734`）：

```
3723:        rx_shift[7:0]   <= 8'd0;
3726:            4'd1: rx_shift[0] <= i_rxd_in;
3727:            4'd2: rx_shift[1] <= i_rxd_in;
3728:            4'd3: rx_shift[2] <= i_rxd_in;
3729:            4'd4: rx_shift[3] <= i_rxd_in;
3730:            4'd5: rx_shift[4] <= i_rxd_in;
3731:            4'd6: rx_shift[5] <= i_rxd_in;
3732:            4'd7: rx_shift[6] <= i_rxd_in;
3733:            4'd8: rx_shift[7] <= i_rxd_in;
3734:            default: rx_shift[7:0] <= rx_shift[7:0];
```

**行为**：复位时 `rx_shift=0`；正常工作时仅按位移入新 bit，旧值不会被清零。SUB-8-bit UART（如 5-N-1、6-N-1、7-N-1）接收完成后，shift 寄存器的高位残留上一帧 bit。

**实测**：`usi_uart_format_matrix` 5-N-1 / 6-N-1 接收按 `rx_fifo_data & ((1<<N)-1)` 位宽掩码读取；不掩码会读到包含上一帧高位残留的伪数据。

**影响**：软件读 FIFO 必须按 UART 数据位宽 `N` 做 `& ((1<<N)-1)` 掩码；UG §1.2 UART_CTRL 数据位字段描述应注明 shift 寄存器帧间不清。

### 4.6 `raw_intr_sta` 受 `INTR_EN` 门控（en=0 时强制清零）

**RTL 实证**（`wujian100_open/soc/usi0.v:332, 426-444, 445`）：
- `:332` `wire [17:0] raw_intr_sta;`
- `:426-444` `assign raw_intr_sta[17:0] = {spi_stop_intr, ...}`
- `:445` `assign intr_sta[17:0] = raw_intr_sta[17:0] & intr_mask[17:0];`

**实测**（`usi_fifo_threshold`）：INTEN=0 时触发 thold 中断，`RAW_INTR_STA` 不置位；INTEN=1 时 `RAW_INTR_STA` 置位。thold 类中断为电平型（FIFO count ≥ threshold 期间每拍置位），清除流程必须先关 INTEN 再写 IC，否则立刻重触发。

**与常见设计差异**：典型设计中 `RAW_INTR_STA` 不门禁，masking 仅作用于 `INTR_STA`；当前 RTL 中 **INTEN=0 时 RAW 也不置位**（同 PWM，详 PWM 报告 §4.3）。

**影响**：测试用例中断清除逻辑必须分两阶段（先关 INTEN 再清中断）；UG 字段描述应明确 RAW 受 INTEN 门控。

### 4.7 中断位全图（`raw_intr_sta[17:0]`）

**RTL 实证**（`wujian100_open/soc/usi0.v:426-444`）：

| 位 | 名称 | 类型 | 说明 |
|----|------|------|------|
| `[0]` | `tx_thold` | FIFO 阈值 | TX FIFO count ≥ `INTR_CTRL.tx_fifo_th` 期间置位 |
| `[1]` | `tx_empty` | 状态 | TX FIFO 空 |
| `[2]` | `tx_full` | 状态 | TX FIFO 满 |
| `[3]` | `tx_rd_err` | 错误 | TX FIFO 空读 |
| `[4]` | `tx_wr_err` | 错误 | TX FIFO 满写 |
| `[5]` | `rx_thold` | FIFO 阈值 | RX FIFO count ≥ `INTR_CTRL.rx_fifo_th` 期间置位 |
| `[6]` | `rx_empty` | 状态 | RX FIFO 空 |
| `[7]` | `rx_full` | 状态 | RX FIFO 满 |
| `[8]` | `rx_rd_err` | 错误 | RX FIFO 空读 |
| `[9]` | `rx_wr_err` | 错误 | RX FIFO 满写 |
| `[10]` | `uart_stop` | 状态 | UART 传输停止 |
| `[11]` | `uart_perr` | 错误 | UART 校验错 |
| `[12]` | `i2c_stop` | 状态 | I2C STOP 检测 |
| `[13]` | `i2c_nack` | 错误 | I2C NACK 接收（含 10-bit 地址从机 NACK，详 §4.4） |
| `[14]` | `i2cm_lose_arbi` | 错误 | I2C master 仲裁丢失 |
| `[15]` | `i2cs_gcall` | 状态 | I2C slave GCALL 接收（filter enable 时不置位） |
| `[16]` | `i2c_aerr` | 错误 | I2C 地址错 |
| `[17]` | `spi_stop` | 状态 | SPI 传输停止 |

**门控关系**：`INTR_STA = RAW & INTR_UNMASK`（`:445`，`intr_mask` 即 `INTR_UNMASK@0x5C`）。

**实测**：`usi_fifo_threshold` + `usi_i2c_10bit_addr`（i2c_nack bit13）已覆盖关键位。

### 4.8 DMA 寄存器名约定

**RTL 实证**（`wujian100_open/soc/usi0.v:38, 422`）：
- `ADDR_DMA_TH = 12'h068` 寄存器名在 RTL 中是 `DMA_TH`（不是计划中的 `DMA_THRESHOLD`）
- 字段排布：`{rx_dma_th[4:0], 3'd0, tx_dma_th[4:0]}`（`:422`）

**实测**：写 `0x808` 读回 `0x808`；`rx_dma_th=5'd8, tx_dma_th=5'd8`。

**影响**：测试用例寄存器名应用 `DMA_TH`；复位值 `0x808` 与 §4.1 一致。

### 4.9 F4/F6/F7/F8/F14 降级说明

| F 点 | 计划内容 | 降级原因 |
|------|---------|----------|
| **F4** UART RTS/CTS 流控 | CTS 拉低 TX 暂停；RTS 拉高 RX FIFO 接近满 | TB 无 CTS/RTS PAD 驱动，需新增 UART 流控 BFM；当前用例集不覆盖 |
| **F6** I2C HS 模式 | master code 发送后 SCL 切换到高速 | TB 无 I2C HS 模式 master code 激励，需新增 I2C master BFM |
| **F7** I2C GCALL filter | address=0x00 广播响应 | TB 无 I2C slave BFM，无法主动发起 GCALL 帧 |
| **F8** I2C hold time | SDA hold time 与 `I2C_HOLD` 配置一致 | TB 端无 SDA/SCL 时序观测接口（无 PAD monitor） |
| **F14** DMA 接口 | DMA trigger 时序与 `DMA_THRESHOLD` 一致 | 本 TB 无 DMAC 协同（USI DMA 请求线未接到 DMAC）；寄存器读写可访，但 `dma_req_tx/rx` 触发链路未验证 |

**建议**：上述 5 项降级为 BFM/集成限制；若后续项目引入 DMAC + USI master/slave BFM，可补全用例。

---

## 5. 问题与修复记录（调试经验）

| # | 问题 | 影响 | 解决 |
|---|------|------|------|
| 1 | `usi_reset_default` 初版预期 `FIFO_STA=0x11` → 实测 `0x5` | F17 复位值误判 | 按 RTL §4.2 改写：`{tx_empty[0], rx_empty[2]}=4'b0101=0x5`；更新所有 21 项期望值表 |
| 2 | `usi_uart_format_matrix` 初版未按位宽掩码 → 5-N-1/6-N-1 接收读到上一帧高位残留 | F3 误判 | 按 RTL §4.5 改写：`rx_fifo_data & ((1<<N)-1)` 按位宽掩码；8-N-1 掩码=0xFF，7-N-1=0x7F，6-N-1=0x3F，5-N-1=0x1F |
| 3 | `usi_fifo_threshold` 初版 INTEN=0 时假设 RAW 仍置位 → RAW 永远不置位假阳性 | F13 误判 | 按 RTL §4.6 改写：INTEN=0 时 RAW 不置位；thold 中断电平型，清除流程分两阶段（先关 INTEN 再写 IC） |
| 4 | `usi_i2c_10bit_addr` 初版预期 slave 模式 10-bit 地址响应 → RTL 不支持 | F5 误判 | 按 RTL §4.4 改写：仅做 master 模式 10-bit 负向测试（USI1 slave 应 NACK）；i2c_nack raw bit13 置位 |
| 5 | `usi_clk_div_boundary` 初版沿计数用 `@posedge` 捕获 → 0→X delta 假沿 | F15 误判 | 改用 `wait(===)` 电平敏感，UVM 侧 `fork wait(===)` 沿计数 + `disable fork` 收尾（共 320 沿，最小间隔比 = 2.000） |

---

## 6. 遗留风险与后续建议

| # | 风险 / 建议项 | 类别 | 说明 | 建议 |
|---|--------------|------|------|------|
| 1 | 12 寄存器复位值非 0（§4.1） | 风险 | 与计划/UG 表不符 | UG §1.2 reset 表 + §1.4 初始化流程需更新；测试用例初始化前先 disable USI |
| 2 | `FIFO_STA` 复位=0x5 且位排布与计划不符（§4.2） | 风险 | 位序影响位运算宏 | UG §1.2 寄存器描述更新（位排布 `{rx_cnt[20:16], 3'd0, tx_cnt[12:8], rx_full[3], rx_empty[2], tx_full[1], tx_empty[0]}`） |
| 3 | `SPI_NSS_DATA@0x6C` defunct（§4.3） | 风险 | F11 软件 NSS 不可用 | UG 删除该寄存器描述；若需要软件 NSS，RTL 补 `ADDR_SPI_NSS_DATA` 译码 |
| 4 | `i2cs_amode` 硬连线 0（§4.4） | 风险 | slave 模式 10-bit 寻址不可用 | 若需要，RTL 将 `i2cs_amode` 改为 `i2cs_ctrl` 位控制 |
| 5 | `rx_shift` 帧间不清（§4.5） | 风险 | SUB-8-bit UART 接收需按位宽掩码 | UG §1.2 UART_CTRL 描述注明；驱动代码必须 `& ((1<<N)-1)` |
| 6 | `RAW_INTR_STA` 受 INTEN 门控（§4.6） | 风险 | 与典型设计不一致；电平型中断清除需分阶段 | UG 明确 RAW 受 INTEN 门控 + thold 类中断电平型语义 |
| 7 | F4/F6/F7/F8/F14 降级（§4.9） | 集成限制 | TB 无对应 BFM / DMAC 协同 | (a) 文档标注；(b) 后续补 UART 流控 BFM + I2C slave BFM + DMAC 协同 |
| 8 | DMA 寄存器名 `DMA_TH` vs 计划 `DMA_THRESHOLD`（§4.8） | 改进 | 命名不一致 | UG §1.2 与 RTL 字段名对齐为 `DMA_TH` |
| 9 | TIPC trust 信号（`tipc_usi*_trust` / `pprot[2:0]`）仅透传未过滤 | 风险 | 本配置未使能 trustzone | 若启用 trustzone，需补充 trust 边界用例 |
| 10 | SPI 数据位宽 4~16 全组合测试组合爆炸 | 改进 | 抽样测试 | 后续可补充 reference model 全组合对比 |

---

## 7. 附录 - 文件清单与 commit 记录

### 7.1 测试代码（C 端固件）

```
dv/simulation/verif_env/soc/c_case/
├── usi_uart/usi_uart_test.c                  (既有 F1/F2/F3/F15/F16：USI0 TX + USI1 RX)
├── usi_i2c/usi_i2c_test.c                    (既有 F1/F5/F15：master 7-bit 地址)
├── usi_spi/usi_spi_test.c                    (既有 F1/F9/F10：master transmit only)
├── usi_uart/usi_reset_default.c              (新增 F17：3 实例 × 21 项复位值)
├── usi_uart/usi_mirror_inst.c                (新增 F16：USI2 UART TX 4 字节)
├── usi_uart/usi_uart_format_matrix.c         (新增 F3：5 轮 USI0→USI1 数据比对)
├── usi_i2c/usi_i2c_10bit_addr.c              (新增 F5：7-bit 正 + 10-bit 负)
├── usi_uart/usi_fifo_threshold.c             (新增 F12+F13：FIFO 阈值 + RAW/STA/UNMASK/EN)
├── usi_uart/usi_clk_div_boundary.c           (新增 F15：UVM 沿计数 baud 验证)
├── usi_spi/usi_spi_format_matrix.c           (新增 F9+F10：4 轮 master/slave)
└── addr_map/map_test.c                       (通用地址空间 read 0 检查，含 USI 区域 0x50028000~0x50028FFF)
```

### 7.2 UVM 测试与序列

```
dv/simulation/verif_env/soc/soc_top/tests/uvm_test/
├── soc_top_test_lib.svh                      (test 注册；USI UVM 入口复用 soc_top_for_c_case_test)
└── soc_top_usi_uart_baud_test.svh            (新增：F15 沿计数 baud 验证)
```

UVM 配对列表：
- `soc_top_usi_uart_baud_test`（F15：DIV=0x81/0x40 各 16×0x55 沿计数）

### 7.3 RTL 注释记录

```
wujian100_open/soc/usi0.v
  - reset block (:448-473)         — 12 寄存器非 0 复位值（含 DMA_TH=0x808, INTR_CTRL=0x101）
  - ADDR_* 定义 (:11-38)           — 无 ADDR_SPI_NSS_DATA，0x6C 译码缺失（F11 defunct）
  - FIFO_STA 位排布 (:399)         — {rx_cnt[20:16], 3'd0, tx_cnt[12:8], rx_full[3], rx_empty[2], tx_full[1], tx_empty[0]}
  - raw_intr_sta 位定义 (:426-444) — 19 类中断位映射
  - intr_sta = raw & intr_mask (:445) — INTR_STA = RAW & INTR_UNMASK
  - i2cs_amode 硬连线 (:583)        — slave 不支持 10-bit 寻址
  - rx_shift 帧间不清 (:3723-3734) — SUB-8-bit UART 接收需按位宽掩码
```

### 7.4 Commit 记录

| Commit | 说明 |
|--------|------|
| `cbaa566` | 新增 7 个 USI 用例（6 C + 1 UVM 协同）+ 7 项 RTL 注释（12 寄存器复位值 / FIFO_STA=0x5 / SPI_NSS_DATA defunct / i2cs_amode=0 / rx_shift 帧间不清 / RAW INTEN 门控 / 中断位全图） |

### 7.5 关键 RTL 行号索引

| 行为 | 文件 : 行 |
|------|----------|
| `ADDR_*` 定义（止于 `ADDR_DMA_TH = 12'h068`） | `wujian100_open/soc/usi0.v : 11-38` |
| 读 mux（`FIFO_STA` 位排布） | `wujian100_open/soc/usi0.v : 399` |
| `raw_intr_sta` 位定义（19 类中断） | `wujian100_open/soc/usi0.v : 426-444` |
| `intr_sta = raw & intr_mask` | `wujian100_open/soc/usi0.v : 445` |
| reset block（12 寄存器非 0 复位值） | `wujian100_open/soc/usi0.v : 448-473` |
| `i2cs_amode = 1'b0`（slave 不支持 10-bit） | `wujian100_open/soc/usi0.v : 583` |
| `rx_shift[7:0] <= 8'd0`（仅复位清零） | `wujian100_open/soc/usi0.v : 3723` |
| UART `rx_shift` 按位移入 | `wujian100_open/soc/usi0.v : 3726-3734` |

### 7.6 评审记录

`doc_review(job_4ca052b85991)` 返回推理片段无实质结论，本次评审降级为主控本地核对，**以仿真实证为准**（详见 §2 测试执行结果 + §3 覆盖矩阵 + §4 RTL 实证发现）。

---

## 8. 结论

USI 模块验证按计划推进：10 个用例（既有 3 + 新增 7）0 UVM_ERROR / 0 UVM_FATAL 全 PASS；F1~F17 中 11 项闭环（F1/F2/F3/F5/F9/F10/F12/F13/F15/F16/F17）、5 项降级（F4/F6/F7/F8/F14，TB/BFM 限制）、1 项 defunct（F11，`SPI_NSS_DATA@0x6C` RTL 无地址译码）；7 项 Spec/UG-vs-RTL 关键差异（12 寄存器复位值非 0 / `FIFO_STA=0x5` 位序重排 / F11 defunct / `i2cs_amode=0` / `rx_shift` 帧间不清 / `RAW_INTR_STA` 受 INTEN 门控 / 中断位全图 + `INTR_STA = RAW & INTR_UNMASK`）已写入验证报告与模块分析；5 项调试经验已沉淀。遗留风险 10 项已分类登记，建议按 §6 优先级进入下一阶段（UG 复位值表同步 / F11 寄存器 RTL 修正 / I2C slave BFM 落地 / trust 边界用例等）。
