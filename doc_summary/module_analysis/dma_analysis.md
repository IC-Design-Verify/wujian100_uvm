# wujian100_open SoC Direct Memory Access (DMA/DMAC) — 模块分析

> 本文档基于 `wujian100_open/soc/dmac.v` 单一 RTL 源文件（含 `dmac_top` 与 `arb_ctrl` / `bmux_ctrl` / `ch_ctrl` / `reg_ctrl` / `fsmc` / `gbregc` / `hpchn_decd` / `chntrg_latch` / `chregc0..15` 子模块），以及 User Guide 第 3 节「Direct Memory Access (DMA)」（userguide.txt 行 519–695）整理而成。寄存器信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

DMAC 是 wujian100_open SoC 内的 AHB-Lite 总线主控，负责在不占用 CPU 的情况下完成外设↔存储器、存储器↔存储器之间的数据搬运。

- **协议**：AMBA 2.0 AHB-Lite 规范（兼容）。
- **通道数**：最大 16 个独立通道（ch0 ~ ch15）。
- **触发模式**：每个通道均支持 **Block trigger** 模式（TRGTMDC = 2'b10 或 2'b11；其它值保留）；User Guide 概览段落表述 "Every channel only supports block trigger mode"，并提到 group trigger 描述（Function Description 中仅 block/half-block 等粒度）。
- **优先级**：16 级硬件优先级，**Channel 0 最高**；高优先级通道可抢占（interrupt）低优先级正在进行的传输。
- **地址控制**：源地址、目的地址均支持三种模式——increment / decrement / no change。
- **传输宽度**：源、目的独立可配 8 / 16 / 32-bit（AES-Lite 风格 `hsize` 映射，`11` 为 reserved）。
- **块大小**：单通道最大 4096 字节（`BLOCK_TL + 1` 字节；`BLOCK_TL[11:0]`）。
- **字节序**：源、目的分别可配 little-endian / big-endian（`SRCDTLGC` / `DSTDTLGC`）。
- **中断源**：每通道 4 种触发——block complete / half block complete / trigger event complete / error（HRESP error）。
- **保护**：每通道 `chn_en` 一旦置 1，硬件锁定 SAR/DAR/CTRLA/CTRBL 不允许软件改写；`DMACCFG.DMACEN` 是全局使能。
- **总线接入**：AHB-Lite master `m_*` 端口发起 DMA 读/写传输；AHB-Lite slave `s_*` 端口接收 CPU 对寄存器组的访问。

### 1.2 SoC 中的位置 / 总线挂载

依据 `wujian100_open_top.v` / `pdu_top.v` / `ahb_matrix_top.v` 中的端口命名：

- DMAC 在 `ahb_matrix_top`（主 AHB 总线矩阵）中作为：
    - **Master M3**：`dmac0_hmain0_m3_*`（含 `hburst/hbusreq/hlock/hsize/hprot/htrans/hwdata/hwrite/haddr`），用于主动发起总线事务。
    - **Slave S6**：`dmac0_hmain0_s6_*`（`hrdata/hready/hresp`），CPU 通过该 slave 端口访问 DMA 寄存器组。
- DMAC 在中断系统里产生 1 个 VIC 中断输出 `dmac0_wic_intr` → `core_top` → CLIC/VIC（SoC 中断源号 32：`DMAC0`，见 System Overview Table 1-4）。
- DMAC 与 ETB（Event Trigger Bus）紧密耦合：
    - 输入：`etb_dmacch0_trg` ~ `etb_dmacch15_trig`（来自其它外设的硬件触发）。
    - 输出：`ch0_etb_tfrdone/htfrdone/evtdone` ~ `ch15_etb_*`（每个通道 3 类事件送 ETB）；另外每通道还提供 1 bit `chN_prot_out`（与 AHB `hprot` 保护位相关）。
- DMA 寄存器基地址（外部）：`0x4000_0000` ~ `0x4000_3FFF`（System Overview Peripheral Address Map，S6 / DMA Controller，16 KB）。

### 1.3 RTL 文件与实例关系

DMAC 由单一文件 `dmac.v` 描述；`wujian100_open/soc/dmac.v` 内一次性包含 16 个通道寄存器控制器（`chregc0` ~ `chregc15`）以及全局控制器 `gbregc`、`fsmc`、`arb_ctrl`、`bmux_ctrl`、`ch_ctrl`、`reg_ctrl`、`hpchn_decd`、`chntrg_latch`，由顶层 `dmac_top` 统一整合。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次

```
dmac_top                        (dmac.v:13123, AHB-Lite master + slave + 16 通道 ETB + VIC 中断输出)
├── x_ahbbusmux_ctrl            (bmux_ctrl, 总线多路复用：把当前 active 通道的 AHB 命令切换到 m_* 主端口)
├── x_reg_ctrl                  (reg_ctrl, 寄存器控制层：AHB-Lite slave s_* 解码、寄存器读写)
│   ├── x_chregc0 ~ x_chregc15  (chregcN, 16 通道寄存器组：每通道 9 个寄存器 SAR/DAR/CTRLA/CTRLB/INT_MASK/INT_STATUS/INT_CLEAR/SOFT_REQ/EN)
│   └── x_gbregc                (gbregc, 全局寄存器：CHSR/DMACCFG，输出 dmac_vic_if)
├── x_arb_ctrl                  (arb_ctrl, 16 通道仲裁器；Channel 0 优先级最高，支持高优先级抢占)
└── x_chfsm_ctrl                (ch_ctrl, 通道 FSM 控制：解析 transfer mode/数据搬运时序、与 fsmc 交互)
    ├── fsmc                    (dmac.v:15225, Finite-State-Machine Core：实际执行读/写 burst 拆包、地址更新、busy 信号)
    ├── hpchn_decd              (dmac.v:16224, 高优先级通道解码)
    └── chntrg_latch            (dmac.v:3549, 通道触发 latch：从 ETB/soft_req 接收触发信号)
```

### 2.2 各子模块职责（结构层）

| 子模块 | 位置 | 职责（结构层） |
|---|---|---|
| `dmac_top` | dmac.v:13123 | DMAC 顶层；对外暴露 AHB-Lite master/slave 端口、16 通道 ETB 触发输入/输出、`dmac_vic_if` 中断输出，整合 `reg_ctrl` + `arb_ctrl` + `bmux_ctrl` + `ch_ctrl` 四大子系统。 |
| `reg_ctrl` | dmac.v:16237 | AHB-Lite slave 端寄存器读写控制器；包含 16 个 `chregc` 与 1 个 `gbregc`；把 CPU 写入的 SAR/DAR/CTRLA/CTRLB/EN 等转发到对应通道，把通道状态送回 `prdata`。 |
| `chregc0` ~ `chregc15` | dmac.v:3587/... | 16 份**单通道寄存器组**，每份实现 9 个寄存器（见 §4）。当 `chn_en=1` 时锁定配置。 |
| `gbregc` | dmac.v:15929 | 全局寄存器控制：实现 `CHSR`（busy status 0x338）与 `DMACCFG`（0x33C），聚合各通道中断到 `dmac_vic_if`。 |
| `arb_ctrl` | dmac.v:11 | 16 通道仲裁：Channel 0 优先级最高；产生"当前获总线权的通道号" + busy/grant 信号。 |
| `bmux_ctrl` | dmac.v:631 | 总线多路复用：把获准通道的 AHB 命令/地址/数据送至 DMAC master 端口 `m_*`。 |
| `ch_ctrl` | dmac.v:1077 | 通道 FSM 控制：依据 `TRGTMDC` 等配置驱动 `fsmc`；包含触发 latch 与高优先级通道解码。 |
| `fsmc` | dmac.v:15225 | FSM Core：执行实际 AHB-Lite 读/写拆包、地址自增/自减/不变、数据宽度调整、错误检测。 |
| `chntrg_latch` | dmac.v:3549 | 触发 latch：接受 `soft_req` 与 ETB 触发，置位对应通道触发位。 |
| `hpchn_decd` | dmac.v:16224 | 高优先级通道解码：向仲裁器反馈"是否有更高优先级通道在等待"。 |

> 内部计数、宽度变换、burst 拆分等具体逻辑按要求不深入展开，详见 `dmac.v` 源文件。

---

## 3. 端口列表（`dmac_top`）

直接来自 RTL `module dmac_top(...)` 端口声明（含宽度）。

### 3.1 时钟 / 复位 / 中断

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `hclk` | input | 1 | AHB 时钟 |
| `hrst_n` | input | 1 | AHB 复位，低有效 |
| `dmac_vic_if` | output | 1 | DMAC 聚合中断（送 CLIC/VIC） |

### 3.2 AHB-Lite Slave 端口（CPU 访问寄存器组）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `s_hsel` | input | 1 | Slave 选择信号 |
| `s_haddr` | input | 32 | 地址总线 |
| `s_hwdata` | input | 32 | 写数据 |
| `s_hwrite` | input | 1 | 写控制（1=写） |
| `s_htrans` | input | 2 | 传输类型 |
| `s_hprot` | input | 4 | 保护控制 |
| `s_hrdata` | output | 32 | 读数据 |
| `s_hready` | output | 1 | 传输完成 |
| `s_hresp` | output | 2 | 响应状态 |

### 3.3 AHB-Lite Master 端口（DMAC 主动发起传输）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `m_hbusreq` | output | 1 | 总线请求 |
| `m_hgrant` | input | 1 | 总线授权 |
| `m_hlock` | output | 1 | 锁定信号 |
| `m_haddr` | output | 32 | 地址总线 |
| `m_hburst` | output | 3 | Burst 类型 |
| `m_hwdata` | output | 32 | 写数据 |
| `m_hwrite` | output | 1 | 写控制 |
| `m_htrans` | output | 2 | 传输类型 |
| `m_hsize` | output | 3 | 传输宽度 |
| `m_hprot` | output | 4 | 保护控制 |
| `m_hrdata` | input | 32 | 读数据 |
| `m_hready` | input | 1 | 传输完成 |
| `m_hresp` | input | 2 | 响应状态 |

### 3.4 16 通道 ETB 触发输入（外设 → DMAC）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `etb_dmacch0_trg` ~ `etb_dmacch15_trig` | input | 1 | 各通道硬件触发信号（共 16 个） |

### 3.5 16 通道 ETB 触发输出（DMAC → 外设）

每通道 4 个 1-bit 输出：

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `chN_etb_tfrdone` | output | 1 | Channel N block transfer 完成事件 |
| `chN_etb_htfrdone` | output | 1 | Channel N half-block transfer 完成事件 |
| `chN_etb_evtdone` | output | 1 | Channel N trigger event 完成事件 |
| `chN_prot_out` | output | 1 | Channel N AHB `hprot` 相关保护位 |

（N = 0..15，共 16 × 4 = 64 个 1-bit 输出）

---

## 4. 寄存器配置

### 4.1 DMA Channel Base Address（User Guide §3.3.1）

| 通道 | 内部 base 地址 | 通道 | 内部 base 地址 |
|---|---|---|---|
| Channel 0 | `0x000` | Channel 8 | `0x180` |
| Channel 1 | `0x030` | Channel 9 | `0x1B0` |
| Channel 2 | `0x060` | Channel 10 | `0x1E0` |
| Channel 3 | `0x090` | Channel 11 | `0x210` |
| Channel 4 | `0x0C0` | Channel 12 | `0x240` |
| Channel 5 | `0x0F0` | Channel 13 | `0x270` |
| Channel 6 | `0x120` | Channel 14 | `0x2A0` |
| Channel 7 | `0x150` | Channel 15 | `0x2D0` |
| **Global registers** | **`0x330`** | — | — |

每通道寄存器组内部布局相同（stride = 0x30）。

### 4.2 单通道 Memory Map（User Guide Table 3-1）

| Register | Offset | Width | Description | Reset Value |
|---|---|---|---|---|
| `SARn` | `0x00` | 32 | Channel n Source Address Register | `0x0000_0000` |
| `DARn` | `0x04` | 32 | Channel n Destination Address Register | `0x0000_0000` |
| `CHn_CTRL_A` | `0x08` | 32 | Channel n Control Register A | `0x0000_0000` |
| `CHn_CTRL_B` | `0x0C` | 32 | Channel n Control Register B | `0x0000_0000` |
| `CHn_INT_MASK` | `0x10` | 32 | Channel n Mask Interrupt Register | `0x0000_0000` |
| `CHn_INT_STATUS` | `0x14` | 32 | Channel n Status Interrupt Register | `0x0000_0000` |
| `CHn_INT_CLEAR` | `0x18` | 32 | Channel n Clear Interrupt Register | `0x0000_0000` |
| `CHn_SOFT_REQ` | `0x1C` | 32 | Channel n Software Handshaking Request Register | `0x0000_0000` |
| `CHn_EN` | `0x20` | 32 | Channel n Enable Control Register | `0x0000_0000` |

`0x24` ~ `0x2F` 为通道保留区间；stride = 0x30，与 16 通道 base 列表一致。

### 4.3 Global Register Memory Map（User Guide §3.3.2）

基地址 `0x330`。

| Register | Offset (相对 0x330) | Width | Description | Reset Value |
|---|---|---|---|---|
| Reserved | `0x00` | — | — | — |
| Reserved | `0x04` | — | — | — |
| `CHSR` | `0x08` | 32 | Channel busy status register | `0x0000_0000` |
| `DMACCFG` | `0x0C` | 32 | DMAC configure register | `0x0000_0000` |

### 4.4 寄存器字段描述

#### 4.4.1 `SARn` — Channel n Source Address Register（Offset `0x00`）

| Bits | R/W | Description |
|---|---|---|
| `31:0` | R/W | `SARn[31:0]` — Channel n Source read Address. Reset Value: `0x0` |

#### 4.4.2 `DARn` — Channel n Destination Address Register（Offset `0x04`）

| Bits | R/W | Description |
|---|---|---|
| `31:0` | R/W | `DARn[31:0]` — Channel n Destination write Address. Reset Value: `0x0` |

#### 4.4.3 `CHn_CTRL_A` — Channel n Control Register A（Offset `0x08`）

| Bits | R/W | Description |
|---|---|---|
| `31:24` | R/W | Reserved |
| `23:12` | R/W | `BLOCK_TL[11:0]` — Block Transfer Length. Must be written before the channel is enabled. Bytes total number = `BLOCK_TL + 1`; maximum = 4096 bytes. Reset: `0x0` |
| `11:8` | R/W | Reserved |
| `7:6` | R/W | `SINC[1:0]` — Source Address Increment. `00` = increment; `01` = decrease; `1x` = no change. Reset: `0x0` |
| `5:4` | R/W | `DINC[1:0]` — Destination Address Increment. `00` = increment; `01` = decrease; `1x` = no change. Reset: `0x0` |
| `3:2` | R/W | `SRC_TR_WIDTH[1:0]` — Source Transfer Width (mapped to AHB `hsize`). `00` = 8-bit; `01` = 16-bit; `10` = 32-bit; `11` = reserved. Reset: `0x0` |
| `1:0` | R/W | `DST_TR_WIDTH[1:0]` — Destination Transfer Width (mapped to AHB `hsize`). Same encoding as `SRC_TR_WIDTH`. Reset: `0x0` |

#### 4.4.4 `CHn_CTRL_B` — Channel n Control Register B（Offset `0x0C`）

| Bits | R/W | Description |
|---|---|---|
| `31:19` | — | Reserved |
| `18:15` | R/W | `PROTCTL[3:0]` — Channel n Protection Control; drives AHB `HPROT`. `PROTCTL[2]`: `1` = secure access only; `0` = normal (secure/non-secure both allowed). Reset: `0x0` |
| `14` | R/W | `DSTDTLGC` — Destination Write data endian change. `0` = little-endian; `1` = big-endian. Reset: `0x0` |
| `13` | R/W | `SRCDTLGC` — Source read data endian change. `0` = little-endian; `1` = big-endian. Reset: `0x0` |
| `12:3` | — | Reserved |
| `2:1` | R/W | `TRGTMDC[1:0]` — Trigger transfer mode. `2'b00` = reserved; `2'b01` = reserved; `2'b1x` = **Block trigger mode** (must be set to `2'b10` or `2'b11`). Reset: `0x0` |
| `0` | R/W | `INT_EN` — Interrupt Enable. `0` = all interrupt sources disabled; `1` = all enabled. Reset: `0x0` |

#### 4.4.5 `CHn_INT_MASK` — Channel n Mask Interrupt Register（Offset `0x10`）

| Bits | R/W | Description |
|---|---|---|
| `3` | R/W | `masktrgetcmpfr` — Mask for trigger event complete interrupt. `0` = mask; `1` = not mask. Reset: `0x0` |
| `2` | R/W | `maskhtfr` — Mask for transfer half complete interrupt. `0` = mask; `1` = not mask. Reset: `0x0` |
| `1` | R/W | `masktfr` — Mask for transfer complete interrupt. `0` = mask; `1` = not mask. Reset: `0x0` |
| `0` | R/W | `maskErr` — Mask for transfer error interrupt. `0` = mask; `1` = not mask. Reset: `0x0` |

#### 4.4.6 `CHn_INT_STATUS` — Channel n Status Interrupt Register（Offset `0x14`）

| Bits | R/W | Description |
|---|---|---|
| `3` | R | `statustrgetcmpfr` — Status for trigger event complete. Reset: `0x0` |
| `2` | R | `statushtfr` — Status for half-of-block-length transfer complete. Reset: `0x0` |
| `1` | R | `statustfr` — Status for block-length transfer complete. Reset: `0x0` |
| `0` | R | `statusErr` — Status for transfer error (only detects AHB `HRESP` error). Reset: `0x0` |

#### 4.4.7 `CHn_INT_CLEAR` — Channel n Clear Interrupt Register（Offset `0x18`）

| Bits | R/W | Description |
|---|---|---|
| `3` | W | `cleartrgetcmpfr` — Clear `statustrgetcmpif`. `0` = no clear; `1` = clear. Reset: `0x0` |
| `2` | W | `clearhtfr` — Clear `statushtfr`. `0` = no clear; `1` = clear. Reset: `0x0` |
| `1` | W | `cleartfr` — Clear `statustfr`. `0` = no clear; `1` = clear. Reset: `0x0` |
| `0` | W | `clearErr` — Clear `statusErr`. `0` = no clear; `1` = clear. Reset: `0x0` |

#### 4.4.8 `CHn_SOFT_REQ` — Channel n Software Handshaking Request Register（Offset `0x1C`）

| Bits | R/W | Description |
|---|---|---|
| `0` | W | `soft_req` — Software Request. Reset: `0x0` |

#### 4.4.9 `CHn_EN` — Channel n Enable Control Register（Offset `0x20`）

| Bits | R/W | Description |
|---|---|---|
| `0` | R/W | `chn_en` — Channel n Enable. When set to `1`, `DARn`/`SARn`/`CHn_CTRL_A`/`CHn_CTRL_B` cannot be modified. Reset: `0x0`. After transfer is over, this bit is auto-cleared to `0` by hardware. |

#### 4.4.10 `CHSR` — Channel Busy Status Register（Offset `0x338`，基地址 `0x330` + `0x08`）

| Bits | R/W | Description |
|---|---|---|
| `23:16` | — | Reserved |
| `15` | R | `Ch15bsy` — Channel 15 busy (owns data bus) |
| `14` | R | `Ch14bsy` |
| `13` | R | `ch12bsy`（User Guide 拼写为 ch12bsy 但对应 bit13 = Channel 13） |
| `12` | R | `Ch12bsy` |
| `11` | R | `Ch11bsy` |
| `10` | R | `ch10bsy` |
| `9` | R | `Ch9bsy` |
| `8` | R | `Ch8bsy` |
| `7` | R | `Ch7bsy` |
| `6` | R | `Ch6bsy` |
| `5` | R | `Ch5bsy` |
| `4` | R | `Ch4bsy` |
| `3` | R | `Ch3bsy` |
| `2` | R | `ch2bsy` |
| `1` | R | `ch1bsy` |
| `0` | R | `ch0bsy` |

每个 busy bit：`1` = data transfer busy valid；`0` = data transfer busy invalid。Reset Value 整体为 `0x0000_0000`。

#### 4.4.11 `DMACCFG` — DMAC Configure Register（Offset `0x33C`，基地址 `0x330` + `0x0C`）

| Bits | R/W | Description |
|---|---|---|
| `31:1` | — | Reserved |
| `0` | R/W | `DMACEN` — Global DMAC enable. After this bit is set, all channels can work. Reset: `0x0` |

---

## 5. 工作流程（User Guide Work Flow）

User Guide §4 Work Flow 给出的标准流程：

1. **配置寄存器**：先写 `SARn`、`DARn`、`CHn_CTRL_A`、`CHn_CTRL_B` 等通道配置寄存器；最后写 `DMACCFG.DMACEN` = `1'b1` 与 `CHn_EN.chnen` = `1'b1`（注意顺序：先使能全局，再使能通道）。
2. **触发传输**：配置 `soft_req`（`CHn_SOFT_REQ[0]` = 1）或等待外部 ETB 触发 `etb_dmacchN_trig`。
3. **等待完成标志**：`CHn_INT_STATUS.statustfr` 置位后，硬件自动清零 `CHn_EN.chn_en`（传输结束自动停使能）。
4. **清中断**：读或写 `CHn_INT_CLEAR.cleartfr`（=1）清除该中断标志；如需清除其它中断源，对应 bit 写 1。
5. **优先级抢占**：在传输过程中，若更高优先级通道触发，仲裁器会抢占当前通道；抢占后低优先级通道继续传输（依实现细节）。

> **注意**：User Guide 强调"SARn, DARn, CHn_CTRL_A, CHn_CTRL_B is enable protected (use channel internal enable protect, not DMACEN). Before set DMACCFG.DMACEN 1'b1, user should configure it's channel registers."——即通道使能锁定的是 SAR/DAR/CTRLA/CTRLB，而不是 DMACCFG；DMACCFG 全局使能可独立切换，但建议先配好通道寄存器再开 DMACEN。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| DMAC 通道数 | "16 channels" | `chregc0` ~ `chregc15` 共 16 份 | ✅ |
| 总线协议 | "AMBA2.0 AHB-lite" | `dmac_top` 暴露完整 AHB-Lite master (`m_*`) + slave (`s_*`) | ✅ |
| 寄存器组数量 | 16 + 1 global | `reg_ctrl` 实例化 16 × `chregc` + 1 × `gbregc` | ✅ |
| 每通道寄存器数 | 9 个（stride 0x30） | `chregc` 端口含 SAR/DAR/CTRLA/CTRLB/INT_MASK/INT_STATUS/INT_CLEAR/SOFT_REQ/EN 等对应字段 | ✅ |
| 全局寄存器基址 | `0x330`（CHSR @ `+0x08` = `0x338`，DMACCFG @ `+0x0C` = `0x33C`） | `gbregc` 内部解码与 user guide 一致 | ✅ |
| 优先级 | Channel 0 最高 | `arb_ctrl` 实现 16 级仲裁 | ✅ |
| Trigger mode | "Block trigger mode" | `CHn_CTRL_B[2:1] = 2'b1x` | ✅ |
| 中断源 | block / half-block / trigger-cmp / error | `CHn_INT_STATUS` / `CHn_INT_MASK` / `CHn_INT_CLEAR` 各 4 bit | ✅ |
| AHB 端 ETB 触发 | （隐含） | `etb_dmacch0_trg` ~ `etb_dmacch15_trig` 输入；`chN_etb_tfrdone/htfrdone/evtdone` 输出 | ✅（RTL 提供） |
| VIC 中断输出 | "DMAC0" 单中断 | `dmac_vic_if` 聚合输出 | ✅ |
| 复位值 | 所有寄存器 `0x0000_0000` | RTL 中所有寄存器 reset 行为 0 | ✅ |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | "Block trigger" 模式定义 | 概览段落写 "Every channel only supports block trigger mode"，但 Function Description 又给出 group/half-block 等描述 | RTL `CHn_CTRL_B[2:1] TRGTMDC` 仅 `2'b1x` 为 block，其它保留；未发现 group 模式实现 | User Guide 文本与寄存器描述一致，但功能描述与概览段略有差异；建议理解为"当前 RTL 仅实现 block trigger 模式，group trigger 预留扩展" | medium |
| 2 | `CHSR` bit13 名称 | User Guide 行 13 bit 名称写为 `ch12bsy`（数值上对应 Channel 13） | RTL 未单独标注，按 bit 位置理解 | User Guide 文字拼写错误（应为 `ch13bsy`）；不影响 RTL 实现 | low |
| 3 | DMA 寄存器宽度 | Table 3-1 列 width = 32 bits，但 `CHn_CTRL_A` 实际有意义位宽 ≤ 24 bit（其它 reserved） | `reg_ctrl` 按 32-bit 寄存器处理，读 reserved bit 返回 0 | 与 spec 一致；保留位读写行为遵循 SoC 总规约 | high |
| 4 | `CHn_SOFT_REQ` Width | Table 3-1 列 width = 32 | RTL 中只使用 bit0（`soft_req`） | 与 User Guide 字段描述一致（仅 bit0） | high |
| 5 | `dmac_vic_if` 形态 | User Guide 未描述具体位宽（仅"DMAC0"单中断） | RTL 输出为 1 bit | 实现与命名一致 | high |
| 6 | `chn_en` 锁定范围 | "SARn/DARn/CHn_CTRL_A/CHn_CTRL_B" 不能修改 | RTL `reg_ctrl`/`chregc` 中以 `chn_en` 作为 write-enable 控制 | 与 spec 字段描述一致；其它寄存器（INT_MASK/INT_CLEAR 等）按 RTL 可写 | high |
| 7 | Endian control bit 顺序 | `CHn_CTRL_B` bit13 = SRCDTLGC、bit14 = DSTDTLGC | RTL 与 spec 一致 | ✅ |
| 8 | `DMACCFG` 全局使能与通道保护 | User Guide 同时提及"DMACEN 全局使能"与"chn_en 通道保护" | RTL 区分 `gbc_chnc_dmacen`（全局）与 `chnc_gbc_chnen`（通道） | ✅ 一致 |
| 9 | Block transfer 自动清 `chn_en` | "After transfer is over, this bit will auto cleared to 0 by hardware" | RTL `fsmc` 完成态会清 `chn_en` | ✅ |
| 10 | 数据总线宽度 max 4096 | "max 4096bytes" | RTL `BLOCK_TL[11:0]` 最大 `0xFFF` = 4095，`+1` = 4096 | ✅ |
| 11 | `BLOCK_TL` 与 `BLOCK_TS` | User Guide "Programmable block transaction size" | RTL 仅 `BLOCK_TL`（block transfer length），未发现 BLOCK_TS（block transfer size）字段 | User Guide 表述与字段不对应；可能"transaction size"即 BLOCK_TL | low |
| 12 | ETB 触发源数量 | User Guide 未明确列举 | RTL 每个通道 1 个输入（`etb_dmacchN_trig`），DMAC 内部 `chntrg_latch` latch 触发 | 与 spec 一致 | medium |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 519–695。
- 寄存器交叉参考：`doc_summary/Direct_Memory_Access_DMA_registers.md`（与 userguide.txt 内容一致，本文档以 userguide.txt 为准）。
- RTL 源码：`wujian100_open/soc/dmac.v`（含 16 个 `chregc`、`gbregc`、`arb_ctrl`、`bmux_ctrl`、`ch_ctrl`、`fsmc`、`chntrg_latch`、`hpchn_decd`、`reg_ctrl`、`dmac_top`）。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- 16 通道 ETB 触发输入（`etb_dmacch0_trg` ~ `etb_dmacch15_trig`）的来源分布在 SoC 哪里（哪些外设/TIM/PWM 等）需要结合各外设的 `etb_dmacchN_trig` 输出综合判断；本分析文档不展开。
- DMAC 的中断路由：User Guide 仅说 `DMAC0` 单中断，但 RTL `dmac_vic_if` 是聚合输出。具体哪些通道中断源被 mask/合并需在 VIC/CLIC 子系统章节分析。
- `CHn_INT_STATUS` 与 `CHn_INT_CLEAR` 字段宽度 User Guide 列 32-bit，但实际只有 bit[3:0] 有效；其余 bit reserved，RTL 行为为 read as zero / write ignored。
- `DMACCFG` 仅 bit0 (`DMACEN`) 有定义；其它位 reserved。RTL 中保留位写无效。
- "Block trigger" vs "group trigger" 措辞：User Guide 同时提及两者，但 RTL `TRGTMDC` 仅实现 block 一种；可能后续版本补全 group trigger 实现。
- User Guide Table 3-1 中 width 字段对每个寄存器均列 32-bit（即使有效 bit 远小于 32），RTL 一律按 32-bit 总线处理；未发现 halfword/byte 访问限制。
- CHSR 中 bit13 命名为 `ch12bsy`（应为 `ch13bsy`）的文本拼写疑误，建议修订 spec。
- `BLOCK_TL` 字段描述中提到 "writes this field before the channel is enabled"——这与 §6 第 6 条 `chn_en` 锁定行为吻合；但与"enable protect"细节（如对 `INT_MASK`/`INT_CLEAR` 等是否锁定）User Guide 未明说，RTL 实现需进一步确认。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.2 Memory Map 共 9 个单通道寄存器（offset `0x00/0x04/0x08/0x0C/0x10/0x14/0x18/0x1C/0x20`，stride `0x30`），§4.3 全局寄存器 2 个有效 + 2 个 reserved，CHSR/DMACCFG offset 与 User Guide Table 3-1 / §3.3.2 完全一致。
- **通道 base address**：§4.1 列表（16 通道 base `0x000/0x030/.../0x2D0` + 全局 `0x330`）与 User Guide §3.3.1 表格完全一致。
- **字段描述**：§4.4 中每个字段的位域、访问类型、复位值均与 User Guide Table 3-2 ~ 3-12 / Global Register 描述一致；`CHn_CTRL_A`/`CHn_CTRL_B` 中 reserved 区间、`TRGTMDC`/`BLOCK_TL` 编码均按 spec 登记。
- **端口列表**：§3 中 `dmac_top` 端口集合与 RTL `module dmac_top(...)` 声明逐项核对（master 13 + slave 9 + 16 通道 ETB 输入 + 64 个 1-bit 输出 + 时钟/复位/中断 = 105 个端口）一致。
- **结构挂载**：DMAC 在 `ahb_matrix_top` 中的 Master M3 / Slave S6 接入与 RTL `dmac0_hmain0_m3_*` / `dmac0_hmain0_s6_*` 端口命名一致；`dmac_vic_if` 中断输出与 `dmac0_wic_intr` 端口命名一致。
- **存疑项**：§6.2 与 §7.2 已逐项列出 trigger mode 措辞、CHSR bit13 拼写、reserved 字段行为等差异，未在文档中掩盖。
