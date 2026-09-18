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
- **TB 端**：UVM `soc_top_for_c_case_test` 加载 C 端固件 + 4 个专用 UVM 测试（`soc_top_dma_vic_test` / `soc_top_dma_prot_test` / `soc_top_dma_dual_arb_test` / `soc_top_dma_etb_test`），通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记（`0x2002` = PASS / `0x1001` = FAIL；注入 bug 检测器主动 sim_fail）。
- **TB 监测**：已落地 4 个专用 UVM 测试 + AHB-Lite master monitor（采样 DMAC 主动事务的 hsize/haddr/hprot/htrans/hburst），用于并发仲裁、抢占、错误注入、VIC 路由、ETB force、电平观测等高级场景。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: 单通道 memory-to-memory 块传输（Block Trigger）
**目标**：配置 ch0 的 SAR/DAR/CTRLA(`BLOCK_TL+1`字节 + SINC/DINC incr + 8/16/32-bit TR_WIDTH)/CTRLB(TRGTMDC=block trigger + INT_EN=1)/EN=1，软触发后搬运完成，状态寄存器报告 tfr/htfr/trgetcmpfr，源/目的地址按 SINC/DINC 模式更新。
**已有 case**：`dma_test.c`（既有）— 配置 SAR=0x5000、DAR=0x20025000、CTRLA=`0x2300A`（block 36 byte + SINC/DINC incr + 32-bit width）、CTRLB=`0x5`（block trigger + INT_EN）、SOFT_REQ=1；轮询 INT_STATUS==0xE；校验 8 个 32-bit 数据搬运。
**检查**：C 端轮询 `INT_STATUS==0xE`（tfr+htfr+trgetcmpfr），读目的地址 8 个 32-bit 数据 == 源数据。

### F2: 源/目的地址递增模式（SINC/DINC）
**目标**：`SINC[1:0]/DINC[1:0]` = `00` increment / `01` decrease / `1x` no change 三种模式正确驱动 AHB 地址自增/自减/不变。
**已有 case**：`dma_test.c`（既有）覆盖 `00` increment 模式。
**新增 case（闭环）**：`dma_addr_mode.c` 覆盖 SINC=decr / DINC=no-change + SINC=no-change / DINC=incr 等组合；守护区 17 处越界作为**注入 RTL bug 检测器**（dmac.v:15717 `cntr_blk reload` 注入 `*` 而非 `-`），C 端主动 sim_fail；S2 固定地址末值=`0xBAD00018` 精确命中第 33 拍哨兵。
**检查**：C 端读 SAR/DAR 在传输前后确认地址变化方向；守护区脏字检查；TB 端 AHB monitor 采样 `m_haddr` 序列。
**结果**：⚠️ **功能验证通过，长度检查按设计 FAIL（注入 bug 检测器）** — 8/16/32-bit 实测 +1/+34/+100 字节越界（详验证报告 §4.2）。
**闭环**：✅ 3 种地址模式逻辑全部正确（addr 方向 + 数据本身）；长度越界为注入 bug，非 RTL 缺陷。

### F3: 传输宽度（SRC_TR_WIDTH / DST_TR_WIDTH）
**目标**：`SRC_TR_WIDTH[1:0] / DST_TR_WIDTH[1:0]` = `00` 8-bit / `01` 16-bit / `10` 32-bit 正确映射 AHB `hsize`；`11` reserved 行为待确认。
**已有 case**：`dma_test.c`（既有）覆盖 `10` 32-bit。
**新增 case（闭环）**：`dma_tr_width.c` 覆盖 8/16/32-bit + reserved (`2'b11`) 4 种配置；守护区脏字 1/9/24 严格对应实传 33/66/132B（配置 32B）；reserved 写后读回 0x11 PASS。
**检查**：C 端配置不同 TR_WIDTH 后读目的地址校验字节序正确性；守护区脏字检查；TB 端 AHB monitor 采样 `m_hsize` 信号。
**结果**：⚠️ **功能验证通过，长度检查按设计 FAIL（注入 bug 定量刻画）** — 8/16/32-bit 实测超传 1/34/100 字节（`(N+1)×t - N` 公式，详验证报告 §4.2）。
**闭环**：✅ 8/16/32-bit 数据通路本身全部正确（字节序 + hsize 映射）；超传为注入 `cntr_blk reload` bug 后果，非数据通路缺陷。

### F4: 多通道并发与仲裁
**目标**：同时使能 ch0/ch1（不同优先级）做不同传输，验证 ch0 优先级最高且高优先级可抢占；CHSR 准确反映 16 通道 busy 状态。
**已有 case**：无（既有 C 测试仅单通道）。
**新增 case（闭环）**：`dma_dual_ch_arb` UVM 测试（`soc_top_dma_dual_arb_test`）+ `dma_mirror_ch.c`（ch0/ch1/ch2/ch15 镜像）。`dma_dual_ch_arb` 区域 B→A→B 抢占序列：ch1 (区B, SRCB=0x5600/DSTB=0x2002A000) 启动 → ch0 (区A, SRCA=0x5500/DSTA=0x2002C000) 抢占 → ch1 恢复。
**检查**：TB 端 `classify()` 函数区域序列 B→A→B；`dma_mirror_ch` 验证 ch0/ch15 优先级 + 中断隔离。
**闭环**：✅ `dma_dual_ch_arb`（ch0/ch1 抢占恢复）+ `dma_mirror_ch`（4 通道镜像 + ch0 无串扰）。

### F5: 软件触发（soft_req）与 ETB 硬件触发
**目标**：CHn_SOFT_REQ.soft_req 写 1 触发对应通道传输；`etb_dmacchN_trig` 输入触发对应通道；CTRLB.INT_EN 决定触发完成后是否上报中断。
**已有 case**：`dma_test.c`（既有）覆盖软触发 + 中断使能。
**新增 case（闭环）**：`dma_etb_trigger` UVM 测试（`soc_top_dma_etb_test`）覆盖 ETB 硬件触发：UVM force `etb_dmacch0_trg=1` 保持 2us 后 release；C 侧无 soft_req 观察到 `INT_STATUS==0xE`。
**检查**：C 端写 SOFT_REQ=1 → 等待传输完成 → 中断置位；ETB force 触发通路 TB 端观测。
**闭环**：✅ `dma_test`（soft_req）+ `dma_global_cfg`（DMACEN=0 锁存）+ `dma_etb_trigger`（ETB 硬件触发 2us）。

### F6: 4 类中断（tfr/htfr/err/trgetcmpfr）的 mask/status/clear
**目标**：4 类中断源（`statustfr[1]` / `statushtfr[2]` / `statustrgetcmpfr[3]` / `statusErr[0]`）独立 mask（INT_MASK）、状态（INT_STATUS 只读）、清中断（INT_CLEAR 写 1 清）。
**已有 case**：`dma_test.c`（既有）隐含覆盖 tfr/htfr/trgetcmpfr 三类同时置位（mask 全开 → INT_STATUS==0xE）。
**新增 case（闭环）**：`dma_int_split.c` 4 子场景：
- S1 raw status 不被 mask 门控：全 mask=0 + 触发 → INT_STATUS=0xE
- S2 逐 bit clear：INT_CLEAR=0x2/0x4/0x8 + `dma_delay(50)` → status 逐位清 0
- S3 INT_EN 不门控 raw status：CTRLB.INT_EN=0 + 触发 → INT_STATUS 仍=0xE
- S4 INT_MASK 5bit 含未文档化 `maskpend bit4`：写 0x1F 读回 0x1F
**注**：3 拍 `INT_CLEAR` 写→读回 AHB 流水线竞态需 `dma_delay(50)` 缓解（详验证报告 §4.3）。
**闭环**：✅ `dma_int_split`（4 子场景）+ `dma_mirror_ch`（4 通道镜像验证 raw 隔离）。

### F7: 大小端转换（DSTDTLGC / SRCDTLGC）
**目标**：`SRCDTLGC[13] / DSTDTLGC[14]` = 0 little-endian / 1 big-endian，独立控制源读与目的写的字节序变换。
**已有 case**：`dma_test.c`（既有）配置 SRCDTLGC=0、DSTDTLGC=0（little-endian）。
**新增 case（闭环）**：`dma_endian.c` 覆盖 4 种 (SRCDTLGC, DSTDTLGC) 组合：LE→LE / LE→BE / BE→LE / BE→BE bswap 校验。`0x12345678` 输入对应输出 `0x12345678` / `0x78563412` / `0x78563412` / `0x12345678`。
**检查**：C 端分别配置大小端组合，校验搬运后数据字节序。
**闭环**：✅ `dma_endian`（4 种 bswap 组合全对）。

### F8: CH_EN 锁定与硬件自清
**目标**：写 CHn_EN.chn_en=1 后，SAR/DAR/CTRLA/CTRLB 寄存器拒绝任何后续写访问；传输结束后 chn_en 由硬件自动清 0，寄存器恢复可写。
**已有 case**：`dma_test.c`（既有）隐含覆盖 enable 后传输（但未测试锁定写保护行为）。
**新增 case（闭环）**：`dma_en_lock.c` 4 子场景全程 53us：
- S1 EN=0 → 写 SAR=0xAA → 读 SAR==0xAA（可写）
- S2 EN=1 → 写 SAR=0xBB → 读 SAR==0xAA（拒绝写）
- S3 SOFT_REQ=1 → 等待 53us → INT_STATUS==0xE → 读 EN==0（硬件自清）
- S4 EN=0 → 写 SAR=0xCC → 读 SAR==0xCC（恢复可写）
**检查**：C 端写 EN=1 后尝试写 SAR 应被忽略；传输完成读 EN 应 == 0。
**闭环**：✅ `dma_en_lock`（4 子场景全程 53us）。

### F9: PROTCTL 保护位驱动 AHB hprot
**目标**：CTRLB.PROTCTL[3:0] 正确驱动 DMAC 主动发起 AHB 事务时的 `m_hprot[3:0]`，含 secure/normal 属性。
**已有 case**：无。
**新增 case（闭环）**：`dma_prot_hprot` UVM 测试（`soc_top_dma_prot_test`）：C 端 CTRLB=0x0000A000（PROTCTL=0xA=1010=secure/normal/non-cache/non-buf）触发传输；UVM `fork wait(===)` 采样 74 笔 AHB 事务 `m_hprot`，全部 == 0xA。
**检查**：TB 端 AHB monitor 采样 `m_hprot` 与 CTRLB.PROTCTL 一致。
**闭环**：✅ `dma_prot_hprot`（74 笔 `m_hprot==0xA` 全对）。

### F10: 全局 CHSR / DMACCFG
**目标**：`CHSR` 16 bit 准确反映通道 busy；`DMACCFG.DMACEN[0]` = 0 时所有通道传输被全局禁用。
**已有 case**：`dma_test.c`（既有）隐含覆盖 DMACEN=1 后单通道可工作。
**新增 case（闭环）**：`dma_global_cfg.c` 2 子场景：
- S1 DMACEN=0 全局禁用：DMACCFG=0 → SOFT_REQ=1 → 轮询 CHSR 始终=0；dst 不变
- S2 DMACEN=0 锁存触发（chntrg_latch dmac.v:3549）：DMACEN=0 → SOFT_REQ=1 → 锁存 → DMACEN=1 → 释放；但 SAR 空转（trace: SAR 0x5000→0xE262C, cntr_blk 不减, DAR 不动）→ statusErr + 数据全损 → 软件必须先开 DMACEN 再触发（详验证报告 §4.3）
**检查**：C 端写 DMACEN=0 后触发软触发，CHSR 应保持 0；写 DMACEN=1 后再触发，CHSR.bit0=1。
**闭环**：✅ `dma_global_cfg`（S1 + S2 latch corner）。

### F11: 寄存器复位值
**目标**：hrst_n 释放后所有 16 通道 9 个寄存器 + CHSR + DMACCFG 共 16×9 + 2 = 146 个寄存器（不含 reserved）回到 §1.2 reset 值。
**已有 case**：无（既有 C 测试未做 reset 后初始状态校验）。
**新增 case（闭环）**：`dma_reset_default.c` hrst_n 释放后立即读 ch0 9 个寄存器（SAR/DAR/CTRLA/CTRLB/INT_MASK/INT_STATUS/INT_CLEAR/SOFT_REQ/CH_EN）+ CHSR + DMACCFG，全部 = 0x0000_0000 符合 §1.2 reset 表。
**检查**：`hrst_n` 释放后立即读 18 个寄存器（ch0 9 个 + CHSR + DMACCFG），校验 reset 值。
**闭环**：✅ `dma_reset_default`（ch0 11 个寄存器全 0）。

### F12: 中断聚合与 VIC 路由
**目标**：DMAC 4 类中断聚合为 1 bit `dmac_vic_if`，经 SoC VIC 路由到 cpu_intr[32]（System Overview Table 1-4 中断号 32 = DMAC0）。
**已有 case**：`dma_test.c`（既有）通过 INT_STATUS 间接验证（不验证 VIC 路由）。
**新增 case（闭环）**：`dma_vic_route` UVM 测试（`soc_top_dma_vic_test`）：UVM `fork wait(===)` 采样 `pad_vic_int_vld[32]` 完整电平窗口（上升沿 + 回落沿），C 端 SOFT_REQ=1 → 等待 INT_STATUS=0xE → 校验 `pad_vic_int_vld[32]==1` → INT_CLEAR=0xE → 校验回落。
**检查**：UVM 侧打开 VIC monitor，验证中断发生时 `pad_vic_int_vld[32]` 上升沿。
**闭环**：✅ `dma_vic_route`（完整电平窗口观测）。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `dma_test`（既有 `c_case/dma/dma_test.c`） | `soc_top_for_c_case_test` | F1, F2(00), F3(10), F5(soft_req), F6(tfr+htfr+trgetcmpfr) | C 端基础用例 |
| 2 | `dma_reset_default` | `soc_top_for_c_case_test` | F11 (ch0 9 reg + CHSR + DMACCFG 复位值) | C 端复位检查 |
| 3 | `dma_en_lock` | `soc_top_for_c_case_test` | F8 (CH_EN 锁定 + 硬件自清，53us 全程) | C 端 |
| 4 | `dma_int_split` | `soc_top_for_c_case_test` | F6 (4 子场景: raw 不门控 / 逐 bit clear / INT_EN 不门控 / INT_MASK 5bit) | C 端 |
| 5 | `dma_global_cfg` | `soc_top_for_c_case_test` | F10 (DMACEN=0 + latch corner + SAR 空转) | C 端 |
| 6 | `dma_addr_mode` | `soc_top_for_c_case_test` | F2 (SINC/DINC ×3 + 守护区 17 处越界 + S2 哨兵) | C 端（注入 bug 检测器，**按设计 FAIL**） |
| 7 | `dma_tr_width` | `soc_top_for_c_case_test` | F3 (8/16/32 + reserved，守护区 1/9/24) | C 端（注入 bug 定量刻画，**按设计 FAIL**） |
| 8 | `dma_endian` | `soc_top_for_c_case_test` | F7 (LE/BE 4 种 bswap 组合) | C 端 |
| 9 | `dma_mirror_ch` | `soc_top_for_c_case_test` | F1, F6（ch0/ch1/ch2/ch15 镜像 + ch0 无串扰） | C 端回归 |
| 10 | `dma_vic_route` | `soc_top_dma_vic_test` | F12 (`pad_vic_int_vld[32]` 上升/回落完整电平窗口) | UVM 中断监测 |
| 11 | `dma_prot_hprot` | `soc_top_dma_prot_test` | F9 (74 笔 `m_hprot==0xA`) | UVM AHB monitor |
| 12 | `dma_dual_ch_arb` | `soc_top_dma_dual_arb_test` | F4 (ch0/ch1 区域 B→A→B 抢占序列) | UVM 序列 |
| 13 | `dma_etb_trigger` | `soc_top_dma_etb_test` | F5 (UVM force `etb_dmacch0_trg=1` 2us) | UVM ETB 序列 |

> **结果统计**：13 用例 = 1 既有基线 + 12 新增；11 PASS + 2 **按设计 FAIL**（`dma_addr_mode` / `dma_tr_width` 作为注入 RTL bug 检测器主动 sim_fail，详验证报告 §4.2）。

### 功能覆盖矩阵

| Feature | dma_test | dma_addr_mode | dma_tr_width | dma_endian | dma_int_split | dma_en_lock | dma_reset_default | dma_global_cfg | dma_dual_ch_arb | dma_etb_trigger | dma_prot_hprot | dma_vic_route | dma_mirror_ch | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: 单通道 block 传输 |✓ (36B) | ✓ | ✓ | ✓ | ✓ | ✓ | - | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ (×4)| ✅ |
| F2: SINC/DINC 模式 |✓ (00) | ✓ (×3) | - | - | - | - | - | - | ✓ | - | - | - | -| ⚠️ FAIL 按设计 (注入 bug 检测) |
| F3: 传输宽度 |✓ (10) | - | ✓ (×4 含 resv) | ✓ | - | - | - | - | - | - | - | - | -| ⚠️ FAIL 按设计 (bug 定量刻画) |
| F4: 多通道仲裁 |- | - | - | - | - | - | - | - | ✓ (ch0/ch1) | - | - | - | ✓ (ch0/ch15)| ✅ |
| F5: soft_req / ETB 触发 |✓ (soft) | - | - | - | - | - | - | ✓ (soft) | - | ✓ (ETB) | - | - | -| ✅ |
| F6: 4 类中断 mask/clear |✓ (3类) | - | - | - | ✓ (×4) | - | - | - | ✓ | - | - | - | ✓ (×4)| ✅ |
| F7: 大小端 |✓ (LE) | - | - | ✓ (LE+BE) | - | - | - | - | - | - | - | - | -| ✅ |
| F8: CH_EN 锁定 |✓ (隐含) | - | - | - | - | ✓ | - | - | - | - | - | - | -| ✅ |
| F9: PROTCTL→hprot |- | - | - | - | - | - | - | - | - | - | ✓ | - | -| ✅ |
| F10: CHSR / DMACCFG |✓ (CHSR 隐含) | - | - | - | - | - | - | ✓ | ✓ | - | - | - | -| ✅ |
| F11: 复位值 |- | - | - | - | - | - | ✓ | - | - | - | - | - | -| ✅ |
| F12: 中断聚合 + VIC |- | - | - | - | - | - | - | - | - | - | - | ✓ | -| ✅ |

> 矩阵用 ✓/- 标记；括号内为覆盖量。"闭环" 列：✅=闭环 / ⚠️ FAIL 按设计（注入 bug 检测器）。

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

本项目无独立 Python `def_test` 注册表，DMA 测试通过以下 SoC top test 入口触发：

```text
+UVM_TESTNAME=soc_top_for_c_case_test    # 10 C 端用例（含 dma_test + 9 新增）
+UVM_TESTNAME=soc_top_dma_vic_test       # F12 VIC 路由
+UVM_TESTNAME=soc_top_dma_prot_test      # F9 hprot
+UVM_TESTNAME=soc_top_dma_dual_arb_test  # F4 抢占
+UVM_TESTNAME=soc_top_dma_etb_test       # F5 ETB
```

C 端用例沿用 `soc_top_for_c_case_test` 入口，由固件 `c_case/dma/*.c` 选择；4 个 UVM 专用测试已落地于 `soc_top_dma_dfx_test.svh`。

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
- **DMA 专用 monitor（已落地）**：
  - **AHB-Lite master monitor**：采样 `dmac0_hmain0_m3_*` 端口（DMAC 主动事务），验证 `m_hsize`/`m_haddr`/`m_hprot`/`m_htrans`/`m_hburst` 与配置一致（74 笔 `m_hprot==0xA` 实测通过）
  - **ETB trigger monitor**：UVM force `etb_dmacch0_trg=1` 保持 2us 后 release（`dma_etb_trigger` 已验证）
  - **VIC 中断 monitor**：`pad_vic_int_vld[32]` 上升沿 + 回落完整电平窗口观测（`dma_vic_route` 已验证）
  - **抢占 classifier monitor**：2 通道区域序列 B→A→B 抢占恢复（`dma_dual_ch_arb` 已验证）

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `dma_test`（既有） | 8 个 32-bit 数据搬运正确 + `printf("dma test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `dma_addr_mode` | SINC/DINC ×3 模式逻辑正确；**长度检查按设计 FAIL**（守护区 17 处越界 = 注入 bug 检测器，dmac.v:15717 `cntr_blk reload` 注入 `*`） |
| `dma_tr_width` | 8/16/32-bit 数据通路正确；reserved 容忍；**长度检查按设计 FAIL**（bug 定量刻画：守护区 1/9/24） |
| `dma_endian` | LE/BE 4 种 bswap 组合字节序与配置一致 |
| `dma_int_split` | S1 raw status 不被 mask 门控；S2 逐 bit clear；S3 不被 int_en 门控；S4 INT_MASK 5bit 含未文档化 maskpend bit4 |
| `dma_en_lock` | EN 锁定写无效 + 完成后硬件自清 EN=0，全程 53us |
| `dma_reset_default` | 复位后 ch0 9 reg + CHSR + DMACCFG 与 §1.2 reset 表一致（全 0） |
| `dma_global_cfg` | S1 DMACEN=0 传输不发生、dst 不变、CHSR=0；S2 latch 语义成立但 SAR 空转 → statusErr |
| `dma_dual_ch_arb` | 区域序列 B→A→B：ch0 抢占 ch1 后 ch1 恢复 |
| `dma_etb_trigger` | UVM force `etb_dmacch0_trg=1` 保持 2us 后 release，C 侧无 soft_req 观察到传输完成 |
| `dma_prot_hprot` | 74 笔 `m_hprot==0xA=PROTCTL` 全对 |
| `dma_vic_route` | `pad_vic_int_vld[32]` 上升/回落完整电平窗口观测 |
| `dma_mirror_ch` | ch1/ch2/ch15 镜像 stride-`0x30` 译码 + ch0 无串扰 |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 dma_test                (~7 min)   ✅ PASS（既有 C 端基本功能，36B 8 字）
3. 仿真 dma_reset_default       (~8 min)   ✅ PASS（F11：ch0 9 reg + CHSR + DMACCFG）
4. 仿真 dma_en_lock             (~12 min)  ✅ PASS（F8：EN 锁定 + 自清，53us）
5. 仿真 dma_int_split           (~10 min)  ✅ PASS（F6：4 子场景 + dma_delay(50)）
6. 仿真 dma_global_cfg          (~12 min)  ✅ PASS（F10：DMACEN=0 + latch corner）
7. 仿真 dma_addr_mode           (~13 min)  ⚠️ FAIL 按设计（F2 注入 bug 检测器，守护区 17 处）
8. 仿真 dma_tr_width            (~15 min)  ⚠️ FAIL 按设计（F3 bug 定量刻画，守护区 1/9/24）
9. 仿真 dma_endian              (~11 min)  ✅ PASS（F7：4 种 bswap 组合）
10. 仿真 dma_mirror_ch          (~19 min)  ✅ PASS（F1+F6：4 通道镜像 + 无串扰）
11. 仿真 dma_vic_route          (~7 min)   ✅ PASS（F12：完整电平窗口观测）
12. 仿真 dma_prot_hprot         (~8 min)   ✅ PASS（F9：74 笔 m_hprot==0xA）
13. 仿真 dma_dual_ch_arb        (~10 min)  ✅ PASS（F4：区域 B→A→B 抢占序列）
14. 仿真 dma_etb_trigger        (~8 min)   ✅ PASS（F5：UVM force etb 2us）
```

预估总时间：~135-150 min（13 case）；11 PASS + 2 按设计 FAIL（注入 bug 检测器）。

---

## 7. 风险与限制

> 本节为**初始计划**阶段风险登记 + 验证后新增风险；实测结果已对照验证报告 §6，本表加 ✅/⚠️/❌ 列。

| 风险 | 缓解措施 | 实际结果 |
|------|---------|---------|
| TRGTMDC=`2'b00/01` 在 user guide 标 reserved；RTL 行为待确认（dma_analysis.md §4.4.4） | 测试仅覆盖 `2'b10/11` block trigger 模式；reserved 编码测试中跳过或仅做读取回退验证 | ✅ `dma_test` / `dma_addr_mode` / `dma_tr_width` 仅 block trigger 模式；reserved 未单独测 |
| SRC_TR_WIDTH=`2'b11` reserved 行为未明 | C 端尝试写 `2'b11` 后读 TR_WIDTH，确认 RTL 容忍方式 | ✅ `dma_tr_width` S4 reserved=11 寄存器容忍 PASS（写后读回 0x11） |
| AHB-Lite HRESP 错误注入依赖 SoC 总线矩阵响应 | error 中断测试需 TB 侧 force `m_hresp=1` | ⚠️ error 中断未单独测；通过 `dma_global_cfg` S2 latch corner 路径触发 statusErr=1 |
| 16 通道并发抢占测试依赖 AHB monitor 与多 sequence 并发调度 | 短期仅做 ch0+ch1 双通道；UVM 侧 `dma_dual_ch_arb` 标注风险 | ✅ `dma_dual_ch_arb` 区域 B→A→B + `dma_mirror_ch` 4 通道镜像 + ch0 无串扰 |
| ETB 硬件触发测试依赖 ETB agent | ETB agent 落地后补 `dma_etb_trigger` | ✅ `dma_etb_trigger` UVM force `etb_dmacch0_trg=1` 保持 2us 后 release |
| 中断号 32 = DMAC0 来自 System Overview Table 1-4；具体行号 / 编号以文档最新版本为准 | TB 侧硬编码中断号 32；后续以 doc_review 修复后版本对齐 | ✅ `dma_vic_route` 验证 `pad_vic_int_vld[32]` 完整电平窗口 |
| `dma_test.c` 既有 case 未测试 SINC=01/1x、TR_WIDTH=8/16/resv、big-endian、error 中断、CH_EN 锁定 | 全部由 12 个新增 case 补齐；不阻塞既有 dma_test 回归 | ✅ 12 新增用例全部补齐上述覆盖；仅 error 中断单独测缺（通过 corner 路径触发） |
| DMAC 全局寄存器 base 在 0x330 与 0x338/0x33C 之间存在 0x330-0x337 reserved 区间 | reserved 区间读返回 0；addr_map 通用测试已隐含覆盖 | ✅ addr_map 通用测试覆盖 |

### 7.1 验证后新增风险（重大发现 + UG-vs-RTL 差异）

| # | 风险 | 来源 |
|---|------|------|
| 1 | **❌ 注入 RTL bug: `cntr_blk` reload 公式错误**（dmac.v:15717）— `(N+1)×t` 而非 `(N+1)-t`，导致 8/16/32-bit 实测超传 1/34/100 字节 | 详验证报告 §4.2 |
| 2 | **❌ 注入 RTL bug: `cntr_grup` reload 公式错误**（dmac.v:15692）— 同上机理，group trigger 模式越界 | 详验证报告 §4.2 |
| 3 | **⚠️ DMACEN=0 锁存触发 + SAR 空转 corner**（dmac.v:3549 `chntrg_latch`）— 软件必须先开 DMACEN 再触发，否则 statusErr + 数据全损 | 详验证报告 §4.3 |
| 4 | **⚠️ INT_CLEAR 写→读回 3 拍 AHB 流水线竞态**（dmac.v:4030-4060 区域）— 紧随 lw 采到旧值，需 `dma_delay(50)` 缓解 | 详验证报告 §4.4 |
| 5 | **⚠️ INT_MASK 含未文档化 `maskpend bit4`**（dmac.v:4016）— VIC 输出含第 5 源 | 详验证报告 §4.5 |
| 6 | **⚠️ RAW_INTR_STA 不被 INT_EN 门控**（dma_int_split S3）— 与 USI/PWM 不同，DMAC raw status 不门控 | 详验证报告 §4.1 |
| 7 | **⚠️ AHB 矩阵饥饿** — CPU 轮询/写 DSRAM 严重拖慢 DMAC M3（36B 传输最长 5.15ms） | 详验证报告 §6 |
| 8 | **⚠️ firmware `-O3 -funroll-all-loops` 删除空 for 延时循环** — 延时必须 `__asm__ volatile("nop")` | 详验证报告 §6 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── dma/
│   ├── dma_test.c               (F1, F2(00), F3(10), F5(soft_req), F6(3 类)：既有基线)
│   ├── dma_reset_default.c      (F11：ch0 9 reg + CHSR + DMACCFG 复位值)
│   ├── dma_en_lock.c            (F8：EN 锁定 + 硬件自清，53us)
│   ├── dma_int_split.c          (F6：4 子场景 + dma_delay(50))
│   ├── dma_global_cfg.c         (F10：DMACEN=0 + latch corner + SAR 空转)
│   ├── dma_addr_mode.c          (F2：SINC/DINC ×3 + 守护区 17 处越界 + S2 哨兵，注入 bug 检测)
│   ├── dma_tr_width.c           (F3：8/16/32 + reserved，守护区 1/9/24，bug 定量刻画)
│   ├── dma_endian.c             (F7：LE/BE 4 种 bswap 组合)
│   ├── dma_mirror_ch.c          (F1, F6：ch0/ch1/ch2/ch15 镜像 + ch0 无串扰)
│   ├── dma_vic_route.c          (F12：UVM 协同)
│   ├── dma_prot_hprot.c         (F9：UVM 协同)
│   ├── dma_dual_ch_arb.c        (F4：UVM 协同)
│   └── dma_etb_trigger.c        (F5：UVM 协同)
└── addr_map/
    └── map_test.c               (通用地址空间 read 0 检查，含 DMA 0x4000_0000 区域)
```

### UVM 测试类 (`soc_top/tests/uvm_test/soc_top_dma_dfx_test.svh`)
- `soc_top_dma_vic_test` — F12 VIC 路由电平窗口观测
- `soc_top_dma_prot_test` — F9 hprot 74 笔采样
- `soc_top_dma_dual_arb_test` — F4 区域 B→A→B 抢占序列
- `soc_top_dma_etb_test` — F5 etb_dmacch0_trg force 2us

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
