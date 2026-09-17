# T-Head wujian100_open DMAC (16 通道 AHB-Lite) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Direct Memory Access Controller (DMAC)
  - 16 个独立通道 (ch0~ch15)，AMBA 2.0 AHB-Lite 协议
  - 每通道 base: `0x000` ~ `0x2D0`（stride = `0x30`）
  - 全局寄存器 base: `0x330`（CHSR @ `+0x08` = `0x338`，DMACCFG @ `+0x0C` = `0x33C`）
  - AHB 矩阵接入：Master M3 / Slave S6（`dmac0_hmain0_m3_*` / `dmac0_hmain0_s6_*`）
  - 外部 SoC 基址：`0x4000_0000` ~ `0x4000_3FFF`（16 KB）

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/dmac.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `CH_NUM` | `16` | 通道数（ch0~ch15） |
| `BLOCK_TL_WIDTH` | `12` | 块长度字段宽度；最大块 4096 字节（`BLOCK_TL + 1`） |
| `CH_PRI_W` | `4` | 通道优先级编码位宽（16 级，ch0 最高） |
| `AHB_ADDR_WIDTH` | `32` | AHB 地址位宽 |
| `AHB_DATA_WIDTH` | `32` | AHB 数据位宽 |
| `HTRANS_W` / `HSIZE_W` / `HBURST_W` / `HPROT_W` / `HRESP_W` | `2/3/3/4/2` | AHB-Lite 信号位宽 |
| `TR_WIDTH_VAL` | `2'b00/01/10` → 8/16/32-bit；`2'b11` reserved | `SRC_TR_WIDTH` / `DST_TR_WIDTH` 编码 |
| `SINC/DINC` | `00` incr / `01` decr / `1x` no change | 地址更新模式编码 |

**关键配置含义**：
- 16 通道意味着至少需要做 1 个通道的完整功能覆盖 + 至少 2 个通道做并发仲裁覆盖；可对 ch0 完成全功能覆盖，ch1/ch2/ch15 做镜像 / 仲裁抽样。
- `BLOCK_TL_WIDTH=12` 决定了最大单块传输 = 4096 字节；测试需覆盖普通值（如 36 byte，dma_test.c 既有）、边界值（0/1/4095/4096）。
- `PROTCTL[3:0]` 字段定义 AHB `HPROT` 驱动；测试需在 TB 端验证 hprot 信号与 PROTCTL 一致。

### 1.2 寄存器映射

#### 1.2.1 单通道寄存器（每通道相同，stride = `0x30`）

| Offset (相对通道 base) | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | `SARn` | RW | `0x0000_0000` | 通道 n 源地址 |
| `0x04` | `DARn` | RW | `0x0000_0000` | 通道 n 目的地址 |
| `0x08` | `CHn_CTRL_A` | RW | `0x0000_0000` | `BLOCK_TL[23:12]` + `SINC[7:6]` + `DINC[5:4]` + `SRC_TR_WIDTH[3:2]` + `DST_TR_WIDTH[1:0]` |
| `0x0C` | `CHn_CTRL_B` | RW | `0x0000_0000` | `PROTCTL[18:15]` + `DSTDTLGC[14]` + `SRCDTLGC[13]` + `TRGTMDC[2:1]` + `INT_EN[0]` |
| `0x10` | `CHn_INT_MASK` | RW | `0x0000_0000` | `masktrgetcmpfr[3]` / `maskhtfr[2]` / `masktfr[1]` / `maskErr[0]` |
| `0x14` | `CHn_INT_STATUS` | RO | `0x0000_0000` | `statustrgetcmpfr[3]` / `statushtfr[2]` / `statustfr[1]` / `statusErr[0]` |
| `0x18` | `CHn_INT_CLEAR` | WO | `0x0000_0000` | `cleartrgetcmpfr[3]` / `clearhtfr[2]` / `cleartfr[1]` / `clearErr[0]` |
| `0x1C` | `CHn_SOFT_REQ` | WO | `0x0000_0000` | `soft_req[0]` 软件触发 |
| `0x20` | `CHn_EN` | RW | `0x0000_0000` | `chn_en[0]`；置 1 后锁定 SAR/DAR/CTRLA/CTRLB；传输结束自动清 0 |

通道 base：ch0=`0x000`、ch1=`0x030`、... ch15=`0x2D0`。

#### 1.2.2 全局寄存器（基址 `0x330`）

| Offset (相对 `0x330`) | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | Reserved | — | — | — |
| `0x04` | Reserved | — | — | — |
| `0x08` (= `0x338`) | `CHSR` | RO | `0x0000_0000` | 16 个通道 busy 状态（bit0~bit15） |
| `0x0C` (= `0x33C`) | `DMACCFG` | RW | `0x0000_0000` | `DMACEN[0]` 全局使能 |

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `ahb_matrix_top.v`）

- **时钟**：`hclk`（AHB 总线时钟）；`hrst_n`（低有效复位）。
- **总线挂载**：
  - **Master M3**：DMAC 通过 `m_hbusreq/m_hgrant/m_h*` 主动发起 AHB 传输。
  - **Slave S6**：CPU 通过 `s_hsel/s_haddr/s_*` 访问 DMA 寄存器组。
- **中断**：`dmac_vic_if`（1 bit）→ `core_top` → CLIC/VIC，System Overview Table 1-4 中断号 32 = `DMAC0`（待确认当前文档版本行号）。
- **ETB**：
  - 输入：`etb_dmacch0_trg` ~ `etb_dmacch15_trig`（16 个硬件触发）。
  - 输出：每通道 4 bit（`chN_etb_tfrdone` / `chN_etb_htfrdone` / `chN_etb_evtdone` / `chN_prot_out`），共 64 个 1-bit 输出。
- **PDU 电源**：`pdu_top.v` 控制 DMAC 时钟门控与复位（与 ahb_matrix_top 协调）。

### 1.4 关键 RTL 行为

1. **CH_EN 锁定**（`chregc0..15`）：`chn_en=1` 后 SAR/DAR/CTRLA/CTRLB 寄存器拒绝写；DMA 传输结束后由硬件自动清 0（dma_analysis.md §4.4.9）。
2. **块传输 FSM**（`fsmc`）：从 SAR 读、SINC/DINC 更新、DAR 写；支持 burst 拆包；总字节数 = `BLOCK_TL + 1`。
3. **16 通道仲裁**（`arb_ctrl`）：Channel 0 优先级最高；高优先级通道可抢占（interrupt）低优先级正在进行的传输。
4. **触发 latch**（`chntrg_latch`）：接收 `soft_req` 与 `etb_dmacchN_trig`，置位对应通道触发位。
5. **中断聚合**（`gbregc`）：每通道 4 类中断源（tfr/htfr/trgetcmpfr/err），各自由 INT_MASK 屏蔽，状态存 INT_STATUS，写 INT_CLEAR 清。
6. **大小端**（`DSTDTLGC` / `SRCDTLGC`）：源/目的字节序独立可配。
7. **保护控制**（`PROTCTL[3:0]`）：驱动 AHB `HPROT[3:0]`，含 secure/normal 属性。

### 1.5 TB 检查架构

- **C 端检查**：`dma_test.c`（既有）通过 `mem_write32_(0x4000_0000, SAR)` 配置 ch0、软触发后轮询 `INT_STATUS (0x4000_0014) == 0xE`（bit1 tfr + bit2 htfr + bit3 trgetcmpfr），再读目的地址空间校验数据搬运正确性。
- **TB 端**：UVM `soc_top_for_c_case_test` 加载 `dma_test.c` 固件，通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记（`0x2002` = PASS / `0x1001` = FAIL）。
- **TB 监测**：未来新增 UVM 序列时需 AHB-Lite master agent（发起 CPU 配置读写）+ AHB-Lite master monitor（采样 DMAC 主动发起的事务），用于并发仲裁、抢占、错误注入等高级场景。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: 单通道 memory-to-memory 块传输（Block Trigger）
**目标**：配置 ch0 的 SAR/DAR/CTRLA(`BLOCK_TL+1`字节 + SINC/DINC incr + 8/16/32-bit TR_WIDTH)/CTRLB(TRGTMDC=block trigger + INT_EN=1)/EN=1，软触发后搬运完成，状态寄存器报告 tfr/htfr/trgetcmpfr，源/目的地址按 SINC/DINC 模式更新。
**已有 case**：`dma_test.c`（既有）— 配置 SAR=0x5000、DAR=0x20025000、CTRLA=`0x2300A`（block 36 byte + SINC/DINC incr + 32-bit width）、CTRLB=`0x5`（block trigger + INT_EN）、SOFT_REQ=1；轮询 INT_STATUS==0xE；校验 8 个 32-bit 数据搬运。
**检查**：C 端轮询 `INT_STATUS==0xE`（tfr+htfr+trgetcmpfr），读目的地址 8 个 32-bit 数据 == 源数据。

### F2: 源/目的地址递增模式（SINC/DINC）
**目标**：`SINC[1:0]/DINC[1:0]` = `00` increment / `01` decrease / `1x` no change 三种模式正确驱动 AHB 地址自增/自减/不变。
**已有 case**：`dma_test.c`（既有）覆盖 `00` increment 模式。
**检查**：C 端读 SAR/DAR 在传输前后确认地址变化方向；TB 端 AHB monitor 采样 `m_haddr` 序列验证地址更新轨迹。
**缺口**：`01` decrease 与 `1x` no change 模式**待新建 case（标记 TBD）**。

### F3: 传输宽度（SRC_TR_WIDTH / DST_TR_WIDTH）
**目标**：`SRC_TR_WIDTH[1:0] / DST_TR_WIDTH[1:0]` = `00` 8-bit / `01` 16-bit / `10` 32-bit 正确映射 AHB `hsize`；`11` reserved 行为待确认（RTL 应忽略写或回退到默认值）。
**已有 case**：`dma_test.c`（既有）覆盖 `10` 32-bit。
**检查**：C 端配置不同 TR_WIDTH 后读目的地址校验字节序正确性；TB 端 AHB monitor 采样 `m_hsize` 信号。
**缺口**：8-bit / 16-bit / reserved 模式**待新建 case（标记 TBD）**。

### F4: 多通道并发与仲裁
**目标**：同时使能 ch0/ch1（不同优先级）做不同传输，验证 ch0 优先级最高且高优先级可抢占；CHSR 准确反映 16 通道 busy 状态。
**已有 case**：无（既有 C 测试仅单通道）。
**检查**：TB 端 CHSR.bit0/bit1 在传输中置 1、完成后清 0；并发时低优先级通道被抢占后恢复。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 AHB monitor 与并发配置序列。

### F5: 软件触发（soft_req）与 ETB 硬件触发
**目标**：CHn_SOFT_REQ.soft_req 写 1 触发对应通道传输；`etb_dmacchN_trig` 输入触发对应通道；CTRLB.INT_EN 决定触发完成后是否上报中断。
**已有 case**：`dma_test.c`（既有）覆盖软触发 + 中断使能。
**检查**：C 端写 SOFT_REQ=1 → 等待传输完成 → 中断置位；ETB 触发需 TB 侧驱动 `etb_dmacchN_trig`。
**缺口**：ETB 硬件触发**待新建 case（标记 TBD）**，需 ETB agent。

### F6: 4 类中断（tfr/htfr/err/trgetcmpfr）的 mask/status/clear
**目标**：4 类中断源（`statustfr[1]` / `statushtfr[2]` / `statustrgetcmpfr[3]` / `statusErr[0]`）独立 mask（INT_MASK）、状态（INT_STATUS 只读）、清中断（INT_CLEAR 写 1 清）。
**已有 case**：`dma_test.c`（既有）隐含覆盖 tfr/htfr/trgetcmpfr 三类同时置位（mask 全开 → INT_STATUS==0xE）。
**检查**：C 端分别 mask 单类中断验证 status 是否被屏蔽；写 INT_CLEAR 验证 status 清 0；4 类中断分别触发（待确认 error 中断是否可在 SoC 中构造）。
**缺口**：4 类中断独立 mask + 清中断**待新建 case（标记 TBD）**；error 中断需 AHB 错误注入。

### F7: 大小端转换（DSTDTLGC / SRCDTLGC）
**目标**：`SRCDTLGC[13] / DSTDTLGC[14]` = 0 little-endian / 1 big-endian，独立控制源读与目的写的字节序变换。
**已有 case**：`dma_test.c`（既有）配置 SRCDTLGC=0、DSTDTLGC=0（little-endian）；未覆盖 big-endian。
**检查**：C 端分别配置大小端组合，校验搬运后数据字节序。
**缺口**：big-endian 模式**待新建 case（标记 TBD）**。

### F8: CH_EN 锁定与硬件自清
**目标**：写 CHn_EN.chn_en=1 后，SAR/DAR/CTRLA/CTRLB 寄存器拒绝任何后续写访问；传输结束后 chn_en 由硬件自动清 0，寄存器恢复可写。
**已有 case**：`dma_test.c`（既有）隐含覆盖 enable 后传输（但未测试锁定写保护行为）。
**检查**：C 端写 EN=1 后尝试写 SAR 应被忽略；传输完成读 EN 应 == 0。
**缺口**：**待新建 case（标记 TBD）**。

### F9: PROTCTL 保护位驱动 AHB hprot
**目标**：CTRLB.PROTCTL[3:0] 正确驱动 DMAC 主动发起 AHB 事务时的 `m_hprot[3:0]`，含 secure/normal 属性。
**已有 case**：无。
**检查**：TB 端 AHB monitor 采样 `m_hprot` 与 CTRLB.PROTCTL 一致。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 hprot monitor。

### F10: 全局 CHSR / DMACCFG
**目标**：`CHSR` 16 bit 准确反映通道 busy；`DMACCFG.DMACEN[0]` = 0 时所有通道传输被全局禁用。
**已有 case**：`dma_test.c`（既有）隐含覆盖 DMACEN=1 后单通道可工作。
**检查**：C 端写 DMACEN=0 后触发软触发，CHSR 应保持 0；写 DMACEN=1 后再触发，CHSR.bit0=1。
**缺口**：DMACEN=0 全局禁用**待新建 case（标记 TBD）**。

### F11: 寄存器复位值
**目标**：hrst_n 释放后所有 16 通道 9 个寄存器 + CHSR + DMACCFG 共 16×9 + 2 = 146 个寄存器（不含 reserved）回到 §1.2 reset 值。
**已有 case**：无（既有 C 测试未做 reset 后初始状态校验）。
**检查**：`hrst_n` 释放后立即读 18 个寄存器（ch0 9 个 + CHSR + DMACCFG），校验 reset 值。
**缺口**：**待新建 case（标记 TBD）**。

### F12: 中断聚合与 VIC 路由
**目标**：DMAC 4 类中断聚合为 1 bit `dmac_vic_if`，经 SoC VIC 路由到 cpu_intr[32]（待确认具体号）。
**已有 case**：`dma_test.c`（既有）通过 INT_STATUS 间接验证（不验证 VIC 路由）。
**检查**：UVM 侧打开 `dmac_vic_if` monitor，验证中断发生时 `cpu_intr[N]` 上升沿。
**缺口**：**待新建 case（标记 TBD）**，依赖 SoC VIC monitor。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `dma_test`（既有 `c_case/dma/dma_test.c`） | `soc_top_for_c_case_test` | F1, F2(00), F3(10), F5(soft_req), F6(tfr+htfr+trgetcmpfr) | C 端基础用例 |
| 2 | `dma_addr_mode`（TBD） | `soc_top_for_c_case_test` | F2 (incr/decr/no-change) | C 端 |
| 3 | `dma_tr_width`（TBD） | `soc_top_for_c_case_test` | F3 (8/16/32 + reserved) | C 端 |
| 4 | `dma_endian`（TBD） | `soc_top_for_c_case_test` | F7 (little/big) | C 端 |
| 5 | `dma_int_split`（TBD） | `soc_top_for_c_case_test` | F6 (4 类中断独立 mask/clear) | C 端 |
| 6 | `dma_en_lock`（TBD） | `soc_top_for_c_case_test` | F8 (CH_EN 锁定 + 硬件自清) | C 端 |
| 7 | `dma_reset_default`（TBD） | `soc_top_for_c_case_test` | F11 | C 端复位检查 |
| 8 | `dma_global_cfg`（TBD） | `soc_top_for_c_case_test` | F10 (DMACEN 全局禁用) | C 端 |
| 9 | `dma_dual_ch_arb`（TBD） | UVM 侧 `ahb_ls_test` / `ahb_hs_test` | F4 (ch0+ch1 并发 + 抢占) | UVM 序列 |
| 10 | `dma_etb_trigger`（TBD） | UVM 侧 | F5 (ETB 硬件触发) | UVM ETB 序列 |
| 11 | `dma_prot_hprot`（TBD） | UVM 侧 | F9 (hprot monitor) | UVM AHB monitor |
| 12 | `dma_vic_route`（TBD） | UVM 侧 `soc_top_vseq` | F12 (中断聚合 + VIC 路由) | UVM 中断监测 |
| 13 | `dma_mirror_ch`（TBD） | `soc_top_for_c_case_test` | F1, F6（ch1, ch2, ch15 镜像回归） | C 端回归 |

### 功能覆盖矩阵

| Feature | dma_test | dma_addr_mode | dma_tr_width | dma_endian | dma_int_split | dma_en_lock | dma_reset_default | dma_global_cfg | dma_dual_ch_arb | dma_etb_trigger | dma_prot_hprot | dma_vic_route | dma_mirror_ch |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: 单通道 block 传输 | ✓ (36B) | ✓ | ✓ | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ (×4) |
| F2: SINC/DINC 模式 | ✓ (00) | ✓ (×3) | - | - | - | - | - | - | ✓ | - | - | - | - |
| F3: 传输宽度 | ✓ (10) | - | ✓ (×4 含 resv) | ✓ | - | - | - | - | - | - | - | - | - |
| F4: 多通道仲裁 | - | - | - | - | - | - | - | - | ✓ (ch0/ch1) | - | - | - | ✓ (ch0/ch15) |
| F5: soft_req / ETB 触发 | ✓ (soft) | - | - | - | - | - | - | ✓ (soft) | - | ✓ (ETB) | - | - | - |
| F6: 4 类中断 mask/clear | ✓ (3类) | - | - | - | ✓ (×4) | - | - | - | ✓ | - | - | - | - |
| F7: 大小端 | ✓ (LE) | - | - | ✓ (LE+BE) | - | - | - | - | - | - | - | - | - |
| F8: CH_EN 锁定 | ✓ (隐含) | - | - | - | - | ✓ | - | - | - | - | - | - | - |
| F9: PROTCTL→hprot | - | - | - | - | - | - | - | - | - | - | ✓ | - | - |
| F10: CHSR / DMACCFG | ✓ (CHSR 隐含) | - | - | - | - | - | - | ✓ | ✓ | - | - | - | - |
| F11: 复位值 | - | - | - | - | - | - | ✓ | - | - | - | - | - | - |
| F12: 中断聚合 + VIC | - | - | - | - | - | - | - | - | - | - | - | ✓ | - |

> 矩阵用 ✓/- 标记；括号内为覆盖量。"TBD" 表示待新建 case，不阻塞既有 dma_test 通过但属于覆盖缺口。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test           (UVM 序列基线)
  └── soc_top_for_c_case_test      (运行 C 端测试用例，含 dma_test)
```

既有 `dma_test` 通过 `soc_top_for_c_case_test` 加载 `dma_test.c` 固件运行。UVM 侧新增 DMA 专用序列（`dma_arb_seq` / `dma_etb_seq` / `dma_prot_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，DMA 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/dma/dma_test.c` 决定具体行为。后续 TBD 用例沿用同一入口，通过修改 `c_case/dma/` 下不同 .c 文件选择。

> **待确认**：项目是否计划引入独立 DMA uvm_test 子类（参考 GPIO/PWM/TIM 模块的既有 uvm_test 子类）。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `dma_test.c` 的 `printf("dma test successfully\n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **DMA 专用 monitor（TBD）**：未来新增 UVM 侧 case 时，需在 `soc_top_env` 内增加：
  - AHB-Lite master monitor：采样 `m_*` 端口（DMAC 主动事务），验证 hsize/haddr/hprot/hburst 与配置一致。
  - ETB trigger monitor：采样 `etb_dmacchN_trig` 输入与 `chN_etb_*` 输出。
  - 中断 monitor：采样 `dmac_vic_if` 上升沿、对应 VIC 路由。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `dma_test`（既有） | 8 个 32-bit 数据搬运正确 + `printf("dma test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `dma_addr_mode` (TBD) | SINC/DINC 三种模式各自搬运数据校验；地址更新方向匹配 |
| `dma_tr_width` (TBD) | 8/16/32-bit 各自数据搬运正确；reserved (`11`) 写被忽略或回退 |
| `dma_endian` (TBD) | LE/BE 组合各 4 种，搬运后字节序与配置一致 |
| `dma_int_split` (TBD) | 4 类中断分别 mask 后 status 不置位；写 clear 后 status 清 0 |
| `dma_en_lock` (TBD) | EN=1 后 SAR 写被忽略；传输结束读 EN==0 |
| `dma_reset_default` (TBD) | 复位后 ch0 9 个寄存器 + CHSR + DMACCFG 与 §1.2 reset 表一致 |
| `dma_global_cfg` (TBD) | DMACEN=0 后软触发无效、CHSR==0；DMACEN=1 后恢复 |
| `dma_dual_ch_arb` (TBD) | ch0/ch1 并发时 ch0 优先完成；ch1 被抢占后恢复 |
| `dma_etb_trigger` (TBD) | TB 端 ETB trigger 后通道自动传输并完成 |
| `dma_prot_hprot` (TBD) | `m_hprot` 与 CTRLB.PROTCTL 一致 |
| `dma_vic_route` (TBD) | DMAC 中断发生时 `cpu_intr[32]` 上升沿匹配 |
| `dma_mirror_ch` (TBD) | ch1/ch2/ch15 行为与 ch0 镜像，全部 `sim_end()` |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 dma_test                (~5 min)   既有 C 端基本功能
3. 仿真 dma_addr_mode           (~5 min)   TBD case 1
4. 仿真 dma_tr_width            (~5 min)   TBD case 2
5. 仿真 dma_endian              (~5 min)   TBD case 3
6. 仿真 dma_int_split           (~5 min)   TBD case 4
7. 仿真 dma_en_lock             (~5 min)   TBD case 5
8. 仿真 dma_reset_default       (~5 min)   TBD case 6
9. 仿真 dma_global_cfg          (~5 min)   TBD case 7
10. 仿真 dma_mirror_ch          (~10 min)  TBD case 8（ch1/ch2/ch15 回归）
11. 仿真 dma_dual_ch_arb        (~10 min)  TBD UVM case 9
12. 仿真 dma_etb_trigger        (~10 min)  TBD UVM case 10
13. 仿真 dma_prot_hprot         (~10 min)  TBD UVM case 11
14. 仿真 dma_vic_route          (~10 min)  TBD UVM case 12
```

预估总时间：~90-110 min（既有 case ~5 min + 12 个 TBD case ~85-105 min）

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| TRGTMDC=`2'b00/01` 在 user guide 标 reserved；RTL 行为待确认（dma_analysis.md §4.4.4） | 测试仅覆盖 `2'b10/11` block trigger 模式；reserved 编码测试中跳过或仅做读取回退验证 |
| SRC_TR_WIDTH=`2'b11` reserved 行为未明 | C 端尝试写 `2'b11` 后读 TR_WIDTH，确认 RTL 容忍方式（保持 / 回退 0） |
| AHB-Lite HRESP 错误注入依赖 SoC 总线矩阵响应 | error 中断测试需 TB 侧 force `m_hresp=1` 或依赖现有 AHB monitor；若 SoC 不支持错误注入则跳过 F6 err 中断单独 case |
| 16 通道并发抢占测试依赖 AHB monitor 与多 sequence 并发调度 | 短期仅做 ch0+ch1 双通道；UVM 侧 `dma_dual_ch_arb` (TBD) 标注风险 |
| ETB 硬件触发测试依赖 ETB agent | 短期内仅做 soft_req 测试；ETB agent 落地后补 `dma_etb_trigger` (TBD) |
| 中断号 32 = DMAC0 来自 System Overview Table 1-4；具体行号 / 编号以文档最新版本为准 | TB 侧硬编码中断号 32；后续以 doc_review 修复后版本对齐 |
| `dma_test.c` 既有 case 未测试 SINC=01/1x、TR_WIDTH=8/16/resv、big-endian、error 中断、CH_EN 锁定 | 全部由 TBD case 补齐；不阻塞既有 dma_test 回归 |
| DMAC 全局寄存器 base 在 0x330 与 0x338/0x33C 之间存在 0x330-0x337 reserved 区间 | reserved 区间读返回 0；addr_map 通用测试已隐含覆盖（待确认 map_test.c 是否触达 0x4000_0330~0x4000_0337） |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── dma/
│   └── dma_test.c               (F1, F2(00), F3(10), F5(soft_req), F6(3 类中断)：既有)
├── dma/                         (TBD 新增)
│   ├── dma_addr_mode.c          (F2)
│   ├── dma_tr_width.c           (F3)
│   ├── dma_endian.c             (F7)
│   ├── dma_int_split.c          (F6)
│   ├── dma_en_lock.c            (F8)
│   ├── dma_reset_default.c      (F11)
│   ├── dma_global_cfg.c         (F10)
│   └── dma_mirror_ch.c          (F1, F6：ch1/ch2/ch15 镜像)
└── addr_map/
    └── map_test.c               (通用地址空间 read 0 检查，含 DMA 0x4000_0000 区域)
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
- `dv/simulation/verif_env/soc/ahb_hs/`、`ahb_ls/` — AHB 总线侧 UVM test（待新增 DMA arb/prot/ETB 序列时使用）

### 交叉参考文档
- `doc_summary/module_analysis/dma_analysis.md` — DMA 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/Direct_Memory_Access_DMA_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 519-695 行 — User Guide DMA 章节原文
