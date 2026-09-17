# T-Head wujian100_open SoC (E902 RISC-V, MAIN/LS/APB0/APB1 总线) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open SoC（E902 RISC-V 32-bit CPU + DMAC + TIM×8 + USI×3 + WDT + PWM + RTC + GPIO + SMS + PMU/AOU 域）
  - 总线拓扑：MAIN AHB (`ahb_matrix_top`) → LS AHB (`ls_sub_top`) → APB0/APB1 (`apb0_sub_top`/`apb1_sub_top`)
  - 电源域：AOU（Always-On，RTC/GPIO/PMU）+ 主域（CPU + DMAC + SMS + LS + APB + 外设）
  - 复位入口：`PAD_MCURST`（低有效）→ `aou_top`/`core_top`/`retu_top` 协同
  - 时钟源：`PIN_EHS`（高速振荡器）/ `PIN_ELS`（低速振荡器，32.768 kHz，RTC 用）
  - 基址：见 §1.2 存储映射 + §1.3 外设地址映射

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 SoC 顶层模块挂载关系（来自 `wujian100_open_top.v`）

```
wujian100_open_top (顶层，含 PAD 环、PMU 顶层互连)
├── x_aou_top      (Always-On: RTC、GPIO、PMU dummy)
├── x_cpu_top      (E902 + CLIC + CoreTim + WIC)
├── x_pdu_top      (Power-Down 单元：含主总线矩阵、低速总线桥、APB0/1 子桥)
│   ├── x_main_bus_top  (ahb_matrix_top, MAIN AHB 总线矩阵：7 master × 12 slave)
│   ├── x_sub_ls_top    (ls_sub_top,     LS AHB 总线矩阵)
│   ├── x_sub_apb0_top  (apb0_sub_top,   APB0 桥)
│   └── x_sub_apb1_top  (apb1_sub_top,   APB1 桥)
└── x_retu_top     (Reset Entry + 时钟/复位控制)
    └── x_smu_top       (smu_top, 系统管理单元)
        └── x_sms_top       (sms_top, SRAM 子系统——ISRAM 64KB + DSRAM 3×64KB)
```

### 1.2 存储地址映射（User Guide Table 1-1）

| Address Range | Size | Usage |
|---|---|---|
| `0x0000_0000` ~ `0x0000_7FFF` | 64 KB | Internal SRAM（inst，ISRAM） |
| `0x2000_0000` ~ `0x2002_FFFF` | 192 KB | Internal SRAM（data，DSRAM 3×64KB） |
| `0x4000_0000` ~ `0x401F_FFFF` | 2 MB | MAIN BUS Peripherals |
| `0x4020_0000` ~ `0x7FFF_FFFF` | 1024 MB − 2 MB | Low speed Peripherals |
| `0x8000_0000` ~ `0x9FFF_FFFF` | 768 MB | MAIN BUS Peripherals |
| `0xE000_E000` ~ `0xE000_EFFF` | 4 KB | TCIP |
| Other | — | Reserved |

### 1.3 外设地址映射（User Guide Table 1-2，节选）

#### 1.3.1 MAIN 总线外设

| Address Range | IP | Size | M/S |
|---|---|---|---|
| `0x0000_0000`~`0x0000_27FF` | ISRAM | 64 KB | S0 |
| `0x1000_0000`~`0x1007_FFFF` | MemDummy（inst） | 512 KB | S1 |
| `0x2000_0000`~`0x2002_FFFF` | DSRAM（3 块） | 192 KB | S2/S3/S4 |
| `0x3000_0000`~`0x3007_FFFF` | MemDummy（data） | 512 KB | S5 |
| `0x4000_0000`~`0x4000_3FFF` | DMA Controller | 16 KB | S6 |
| `0x4020_0000`~`0x7FFF_FFFF` | AHB LS BUS | — | S10 |

#### 1.3.2 AHB LS 子映射

| Address Range | IP | Size | S# |
|---|---|---|---|
| `0x5000_0000`~`0x5004_FFFF` | APB0 | 320 KB | S2 |
| `0x6000_0000`~`0x6004_FFFF` | APB1 | 320 KB | S3 |

#### 1.3.3 APB0 子映射（关键外设）

| Address Range | IP | Size | P# |
|---|---|---|---|
| `0x5000_0000`~`0x5000_03FF` | TIM0 | 1 KB | P0 |
| `0x5000_0400`~`0x5000_07FF` | TIM2 | 1 KB | P1 |
| `0x5000_0800`~`0x5000_0BFF` | TIM4 | 1 KB | P2 |
| `0x5000_0C00`~`0x5000_0FFF` | TIM6 | 1 KB | P3 |
| `0x5002_8000`~`0x5002_8FFF` | USI0 | 16 KB | P4 |
| `0x5002_9000`~`0x5002_9FFF` | USI2 | 16 KB | P5 |
| `0x5000_8000`~`0x5000_BFFF` | WDT | 16 KB | P7 |
| `0x5001_C000`~`0x5001_FFFF` | PWM | 16 KB | P12 |

#### 1.3.4 APB1 子映射（关键外设）

| Address Range | IP | Size | P# |
|---|---|---|---|
| `0x6000_0000`~`0x6000_03FF` | TIM1 | 1 KB | P0 |
| `0x6000_0400`~`0x6000_07FF` | TIM3 | 1 KB | P1 |
| `0x6000_0800`~`0x6000_0BFF` | TIM5 | 1 KB | P2 |
| `0x6000_0C00`~`0x6000_0FFF` | TIM7 | 1 KB | P3 |
| `0x6002_8000`~`0x6002_8FFF` | USI1 | 16 KB | P4 |
| `0x6001_8000`~`0x6001_BFFF` | GPIO | 16 KB | P5 |
| `0x6000_4000`~`0x6000_7FFF` | RTC | 16 KB | P6 |
| `0x6003_0000`~`0x6003_3FFF` | PMU dummy | 16 KB | P15 |

> 详细 gap 地址与 dummy slave 见 system_overview_analysis.md §4.2 完整表。

### 1.4 中断源映射（User Guide Table 1-4 节选）

| Number | Interrupt Source | Number | Interrupt Source |
|---|---|---|---|
| 1 | CoreTim | 25 | PWM |
| 16 | GPIO0 | 26 | RTC |
| 17 | TIM0[0] | 27 | WDT |
| 18 | TIM0[1] | 28 | USI0 |
| 19 | TIM1[0] | 29 | USI1 |
| 20 | TIM1[1] | 30 | USI2 |
| 21~24 | TIM2/TIM3（[0]/[1]） | 31 | PMU |
| 32 | DMAC0 | 33~40 | TIM4~TIM7（[0]/[1]） |

完整 64 个中断源（含各 dummy slave 占位）见 system_overview_analysis.md §5.2。

### 1.5 PAD I/O（User Guide Table 1-3 节选）

| Pin | I/O | Width | 描述 |
|---|---|---|---|
| `PIN_EHS` / `POUT_EHS` | I/O | 1 | 外部高速振荡器（系统主时钟） |
| `PIN_ELS` / `POUT_ELS` | I/O | 1 | 外部低速振荡器（RTC 用，典型 32.768 kHz） |
| `PAD_MCURST` | I/O | 1 | 系统复位（低有效） |
| `PAD_JTAG_TCLK` / `TMS` | I/O | 1 | CPU JTAG（HAD 调试链路） |
| `PAD_GPIO_0` ~ `PAD_GPIO_31` | I/O | 1 | GPIO0_0~GPIO0_31 |
| `PAD_USI{0,1,2}_SCLK/SD0/SD1/NSS` | I/O | 1 | USI 串行接口 |
| `PAD_PWM_CH0` ~ `CH11` + `PAD_PWM_FAULT` | I/O | 1 | PWM 输出 + 故障输入 |

完整 PAD 列表见 system_overview_analysis.md §5.1。

### 1.6 时钟 / 复位 / 电源域（来自 `pmu_xxx_*clk` / `pmu_xxx_*rst_b` 端口）

- **主总线域**：`pmu_hmain0_hclk` / `pmu_hmain0_hrst_b`
- **LS 总线域**：`pmu_lsbus_hclk` / `pmu_lsbus_hrst_b`
- **APB 域**：`pmu_apb0_s3clk`/`s3rst_b`、`pmu_apb1_s3clk`/`s3rst_b`、各外设独立门控 `pmu_tim0_p0clk` / `pmu_pwm_p0clk` 等
- **AOU 域**（Always-On）：`aou_top` 自身即独立子系统，承载 RTC、GPIO、PMU dummy，主电源关闭时仍可保留
- **复位入口**：`PAD_MCURST`（低有效）→ `aou_top`/`core_top` 内部产生 `cpu_pmu_sleep_b`、`cpu_pmu_dfs_ack` 等握手

### 1.7 SoC 关键集成行为（结构层）

1. **总线拓扑**：MAIN → LS → APB0/1；CPU 通过 MAIN M0/M1/M2 三路 master（IBus/DBus/外设）；DMAC 通过 M3。
2. **跨域访问**：AOU 域（RTC/GPIO/PMU）通过 `apb1_sub_top` 与 `aou_top` 之间的 APB 直连信号（`apb1_gpio_psel_s5` / `apb1_rtc_psel_s6` / `apb1_pmu_psel_s15`）实现。
3. **ETB 触发 fabric**：DMAC ↔ TIM/USI/PWM/WDT/GPIO/RTC/PMU 跨模块触发（如 TIM trigger → DMAC、USI TX → DMAC RX、PWM trigger → ADC）。
4. **中断聚合**：64 个中断源 → CLIC/VIC → E902 CPU；CoreTim (#1)、GPIO0 (#16)、TIM0[0] (#17) ~ TIM7[1] (#40)、DMAC0 (#32)、PWM (#25)、RTC (#26)、WDT (#27)、USI0/1/2 (#28/29/30)、PMU (#31)。
5. **PMU 电源管理**：门控时钟/复位，控制主域断电/恢复；WIC 唤醒路径。
6. **HAD 调试链路**：`PAD_JTAG_TCLK` / `TMS` 接 E902 调试接口。

### 1.8 TB 检查架构（三种仿真模式）

- **C 端驱动**：`soc_top_for_c_case_test` + `c_case/*/*.c` 固件，CPU 跑程序，通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记。
- **VIP 驱动**：`soc_top_smoke_test` + UVM 序列，绕过 CPU 直接用 AHB-Lite VIP 操作 MAIN 总线。
- **VIP + DPI 混合**：`soc_top_vip_run_with_c_test` + 部分 C 固件 + 部分 UVM 序列（VIP 替代 CPU）。
- **TB 计数**：`soc_top_test_base` 通过 UVM_ERROR 计数 + `cpu_flag_addr` marker 决定 PASS/FAIL。

---

## 2. SoC 级功能点分解 (Feature Decomposition)

> 本节聚焦 SoC 集成层（跨模块行为），各模块内部功能见对应 `*_analysis.md` / `*_verification_plan.md`。

### F1: 地址映射与总线译码（各外设可访问性）
**目标**：CPU 通过 MAIN → LS → APB 路径访问所有外设地址；地址译码正确，错位访问返回 dummy/SLVERR。
**已有 case**：`map_test.c`（`c_case/addr_map/`，既有）—— 通用地址空间 read 测试，覆盖 ISRAM / DSRAM / 各 APB 外设 / 各 dummy。
**检查**：C 端读/写各外设寄存器地址与 §1.3 表一致；读未定义地址返回 0 / SLVERR。
**缺口**：特定错位访问（地址 offset 1/2/3 字节、reserved gap）**待新建 case（标记 TBD）**。

### F2: 总线互联与 master 仲裁（MAIN/LS/APB）
**目标**：MAIN 7 master × 12 slave 互联；CPU M0/M1/M2 三路同时访问不同 slave 互不阻塞；DMAC M3 与 CPU 并发；LS 子桥把 APB0/APB1 透传到 MAIN S10。
**已有 case**：无（既有 case 仅单 master 访问）。
**检查**：UVM 侧多 master 并发访问不同 slave 时，hready/hresp 时序正确；APB 桥透传延迟 ≤ N pclk。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧多 master sequence。

### F3: 时钟与复位（各域复位顺序/时钟门控）
**目标**：`PAD_MCURST` → 各域复位顺序（MAIN → LS → APB0/1 → 各外设）；PMU 控制各外设独立门控 `pmu_*_pclk`。
**已有 case**：各 c_case 测试隐含使用全局复位。
**检查**：TB 端采样 `pmu_hmain0_hrst_b` / `pmu_lsbus_hrst_b` / `pmu_apb0_s3rst_b` 时序；单独门控某个外设时钟验证该外设冻结。
**缺口**：**待新建 case（标记 TBD）**，需 PMU 时钟门控 agent。

### F4: 中断通路（外设中断 → VIC → CPU）
**目标**：各外设中断（如 PWM #25）经 SoC CLIC/VIC 路由到 `cpu_intr[N]`，CPU 响应中断；64 个中断源独立优先级、嵌套支持。
**已有 case**：各外设 c_case 测试隐含中断路径（PWM `pwmint` → CPU `cpu_intr[25]`、USI0 中断 → `cpu_intr[28]` 等）。
**检查**：TB 端 VIC monitor 采样 `cpu_intr[N]` 上升沿；CPU 进入中断处理例程后写 EOI。
**缺口**：64 个中断源全覆盖验证 + 中断嵌套**待新建 case（标记 TBD）**。

### F5: 中断优先级与嵌套
**目标**：VIC 支持 64 个中断嵌套；每个中断独立优先级。
**已有 case**：无（既有 case 未测中断嵌套）。
**检查**：TB 端高优先级中断在低优先级 ISR 中触发时能正确嵌套。
**缺口**：**待新建 case（标记 TBD）**。

### F6: ETB 跨模块触发链
**目标**：跨模块触发路径（如 TIM → DMAC、PWM → ADC trigger、USI TX → DMAC RX、GPIO → ETB fabric）。
**已有 case**：无（外设 c_case 未测 ETB 联动）。
**检查**：TB 端 ETB monitor 采样 `etb_*` 信号链；驱动 TIM trigger 后 DMAC 自动搬运。
**缺口**：**待新建 case（标记 TBD）**，依赖 ETB monitor 与 reference model。

### F7: PAD 复用与方向
**目标**：32 路 GPIO 可独立 Input/Output；USI/PWM 等专用 PAD 复用配置；JTAG PAD 调试模式。
**已有 case**：`gpio_test.c`（既有）覆盖 GPIO PAD 读写；`usi_*_test.c` 覆盖 USI 配置（不测实际 PAD 波形）。
**检查**：TB 端 PAD monitor 采样 32 路 GPIO 方向/数据、USI 串行波形。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧完整 PAD monitor。

### F8: 低功耗（PMU 域断电与 WIC 唤醒）
**目标**：主域断电时 AOU 域（RTC/GPIO）保留运行；CPU 睡眠后由 WIC 唤醒；RTC 触发唤醒后 CPU 恢复执行。
**已有 case**：无（既有 case 未覆盖低功耗）。
**检查**：TB 端 force 主域 `pmu_*_pwr_good=0`，AOU 域 `i_rtc_ext_clk` 仍在运行；force RTC 中断后 CPU 恢复。
**缺口**：**待新建 case（标记 TBD）**，需 PMU 电源管理 agent。

### F9: 调试链路（HAD / JTAG）
**目标**：`PAD_JTAG_TCLK` / `TMS` 接 E902 调试接口；HAD 模块可访问 CPU 寄存器、断点。
**已有 case**：`e902_had_test.c`（`c_case/had_soc/`，既有）—— 仅 `while(1)` 占位 stub，未实际测试 HAD。
**检查**：TB 端 JTAG VIP 驱动 TCLK/TMS 序列，CPU 进入调试模式；读 CPU 寄存器与预期一致。
**缺口**：**待新建 case（标记 TBD）**，需 JTAG VIP。

### F10: CPU 启动与取指（ISRAM/DSRAM/XIP）
**目标**：上电后 CPU 从 `0x0000_0000`（ISRAM）取指；可执行 XIP（`0x1000_0000` inst dummy）。
**已有 case**：`map_test.c`（既有）覆盖 ISRAM 读写；其他 c_case 隐含验证 CPU 取指执行。
**检查**：C 端测试程序从 ISRAM 取指后能正常 `sim_end()`。
**备注**：既有 case 已隐含覆盖。

### F11: DMA 与存储/外设端到端传输
**目标**：DMAC 从 ISRAM/DSRAM → 外设或外设 → ISRAM/DSRAM 端到端搬运；16 通道并发；busy/grant 仲裁。
**已有 case**：`dma_test.c`（既有）覆盖单通道 ISRAM → DSRAM 搬运。
**检查**：UVM 侧 DMAC 接管外设 DMA 请求做批量搬运；TB 端 AHB monitor 验证地址序列。
**缺口**：**待新建 case（标记 TBD）**（USI DMA / PWM DMA / 多通道并发）。

### F12: 跨域同步（AOU ↔ PDU / PDU ↔ 主域）
**目标**：AOU 域 RTC/GPIO 寄存器由 PDU 域 CPU 访问时通过跨域同步握手；PMU 控制跨域时钟门控。
**已有 case**：各 c_case 隐含使用跨域访问。
**检查**：TB 端采样跨域握手信号（如 `aou_pdu_*` / `pdu_aou_*`）时序正确。
**缺口**：**待新建 case（标记 TBD）**，需跨域 monitor。

### F13: 错误响应（HRESP / SLVERR）
**目标**：访问未实现地址或 reserved gap 时返回 HRESP=ERROR / SLVERR；CPU 正确捕获异常。
**已有 case**：无。
**检查**：TB 端访问 `0x4001_0000`（MAIN dummy S7）等返回 SLVERR；CPU trap handler 响应。
**缺口**：**待新建 case（标记 TBD）**。

### F14: 总线 timeout / 死锁检测
**目标**：hready 超时或总线死锁时 SoC 进入 safe state（如复位）。
**已有 case**：无。
**检查**：TB 端 force slave hready=0 长时间，验证 watchdog 或 reset 机制。
**缺口**：**待新建 case（标记 TBD）**（依赖 watchdog 配合）。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `map_test`（既有 `c_case/addr_map/map_test.c`） | `soc_top_for_c_case_test` | F1, F10 (隐含) | C 端基础 |
| 2 | `e902_had_test`（既有 `c_case/had_soc/e902_had_test.c`，stub） | `soc_top_for_c_case_test` | F9 (占位, 待完善) | C 端 stub |
| 3 | `addr_misalign`（TBD） | `soc_top_for_c_case_test` | F1 (错位访问) | C 端 |
| 4 | `bus_multi_master`（TBD） | UVM `soc_top_smoke_test` | F2 (7 master 并发) | UVM |
| 5 | `clock_reset_order`（TBD） | UVM 侧 | F3 | UVM PMU agent |
| 6 | `intr_route_full`（TBD） | `soc_top_for_c_case_test` + TB | F4 (64 个中断源路由) | C 端 + TB VIC monitor |
| 7 | `intr_nesting`（TBD） | UVM 侧 | F5 | UVM |
| 8 | `etb_chain`（TBD） | UVM 侧 | F6 | UVM ETB monitor |
| 9 | `pad_full`（TBD） | UVM 侧 | F7 | UVM PAD monitor |
| 10 | `low_power_wakeup`（TBD） | UVM 侧 | F8 | UVM PMU 电源 |
| 11 | `had_jtag`（TBD） | UVM `soc_top_vip_run_with_c_test` | F9 | UVM JTAG VIP |
| 12 | `dma_endtoend`（TBD） | UVM `soc_top_vip_run_with_c_test` | F11 | UVM DMAC + AHB monitor |
| 13 | `cross_domain_sync`（TBD） | UVM 侧 | F12 | UVM 跨域 monitor |
| 14 | `hresp_slverr`（TBD） | `soc_top_for_c_case_test` | F13 | C 端 |
| 15 | `bus_watchdog`（TBD） | UVM 侧 | F14 | UVM |

### 功能覆盖矩阵

| Feature | map_test | e902_had_test | addr_misalign | bus_multi_master | clock_reset_order | intr_route_full | intr_nesting | etb_chain | pad_full | low_power_wakeup | had_jtag | dma_endtoend | cross_domain_sync | hresp_slverr | bus_watchdog |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: 地址映射 | ✓ | - | ✓ | - | - | - | - | - | - | - | - | - | - | - | - |
| F2: 总线仲裁 | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | - | - |
| F3: 时钟复位 | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | - |
| F4: 中断通路 | - | - | - | - | - | ✓ (×64) | - | - | - | - | - | - | - | - | - |
| F5: 中断优先级 | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | - | - |
| F6: ETB 链 | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - | - |
| F7: PAD 复用 | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | - |
| F8: 低功耗 | - | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - |
| F9: HAD/JTAG | - | ✓ (stub) | - | - | - | - | - | - | - | - | ✓ | - | - | - | - |
| F10: CPU 启动 | ✓ (隐含) | - | - | - | - | - | - | - | - | - | - | - | - | - | - |
| F11: DMA 端到端 | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | - |
| F12: 跨域同步 | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - |
| F13: SLVERR | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - |
| F14: bus watchdog | - | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ |

> 矩阵用 ✓/- 标记。"TBD" 表示待新建 case，不阻塞既有 map_test/e902_had_test 通过但属于覆盖缺口。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报 + `USE_AHB_VIP_TO_REPLACE` 宏切换 VIP
  ├── soc_top_smoke_test              (UVM VIP 驱动序列基线)
  ├── soc_top_for_c_case_test         (运行 C 端测试用例：map_test / dma_test / usi_*_test 等)
  └── soc_top_vip_run_with_c_test     (VIP + C 混合仿真：VIP 替代 CPU 操作 MAIN 总线 + 部分 C 固件)
```

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表；通过 SoC top test 入口 `+UVM_TESTNAME=<class_name>` 触发：
- `+UVM_TESTNAME=soc_top_for_c_case_test` → 跑 C 固件（如 `map_test.c` / `dma_test.c`）
- `+UVM_TESTNAME=soc_top_smoke_test` → UVM VIP 序列
- `+UVM_TESTNAME=soc_top_vip_run_with_c_test` → VIP + C 混合

各 c_case 测试通过修改 `c_case/<module>/<module>_test.c` 决定具体行为；同一入口支持所有 c_case。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`。
- PASS/FAIL 上报：通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker，`sim_end()` 写 `0x2002` = PASS / `sim_fail()` 写 `0x1001` = FAIL。

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **SoC 级专用 monitor（TBD）**：
  - **MAIN AHB monitor**：采样所有 master/slave 端口的 `haddr/hburst/hsize/hprot/hready/hresp` 时序。
  - **LS AHB monitor**：采样 LS 子桥透传。
  - **APB0/APB monitor**：采样 APB0/1 桥透传。
  - **VIC monitor**：采样 64 个 `cpu_intr[N]` 输入与 CLIC 输出。
  - **ETB monitor**：采样所有 `etb_*_trig` / `*_etb_trig` 跨模块触发链。
  - **PAD monitor**：采样 32 路 GPIO + USI + PWM + JTAG PAD 波形。
  - **PMU 电源 agent**：force/release 各域 `pmu_*_pwr_good`。
  - **JTAG VIP**：驱动 TCLK/TMS 调试序列。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `map_test`（既有） | 各地址空间 read value 与期望一致 + `printf("Dummy IP read test Pass!\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `e902_had_test`（既有 stub） | `while(1)` 占位（需后续完善为 HAD 实际测试） |
| `addr_misalign` (TBD) | 错位地址访问返回 SLVERR 或 dummy 响应；CPU 不进入死循环 |
| `bus_multi_master` (TBD) | 7 master 并发访问不同 slave 时 hready/hresp 时序正确，无死锁 |
| `clock_reset_order` (TBD) | 各域复位顺序符合 PMU 时序图 |
| `intr_route_full` (TBD) | 64 个中断源触发时 `cpu_intr[N]` 上升沿匹配 |
| `intr_nesting` (TBD) | 高优先级中断在低优先级 ISR 中能正确嵌套 |
| `etb_chain` (TBD) | 跨模块触发链（如 TIM → DMAC）时序正确 |
| `pad_full` (TBD) | GPIO/USI/PWM/JTAG PAD 波形与配置一致 |
| `low_power_wakeup` (TBD) | 主域断电时 AOU 域运行，RTC 唤醒后 CPU 恢复 |
| `had_jtag` (TBD) | JTAG VIP 可访问 CPU 寄存器、断点、单步 |
| `dma_endtoend` (TBD) | DMAC 16 通道端到端搬运与各外设 DMA 请求匹配 |
| `cross_domain_sync` (TBD) | AOU ↔ PDU 跨域握手时序正确 |
| `hresp_slverr` (TBD) | 未实现地址访问返回 SLVERR，CPU trap handler 响应 |
| `bus_watchdog` (TBD) | hready 长时间拉低触发 SoC 复位或 safe state |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 map_test                       (~10 min)  既有 C 端地址空间覆盖
3. 仿真 addr_misalign                  (~10 min)  TBD case 1（错位访问）
4. 仿真 hresp_slverr                   (~10 min)  TBD case 2（未实现地址）
5. 仿真 intr_route_full                (~30 min)  TBD case 3（64 个中断源路由）
6. 仿真 intr_nesting                   (~15 min)  TBD case 4（中断嵌套）
7. 仿真 clock_reset_order              (~15 min)  TBD UVM case 5（复位顺序）
8. 仿真 bus_multi_master               (~20 min)  TBD UVM case 6（多 master 并发）
9. 仿真 cross_domain_sync              (~15 min)  TBD UVM case 7（跨域同步）
10. 仿真 etb_chain                     (~20 min)  TBD UVM case 8（跨模块 ETB）
11. 仿真 pad_full                      (~20 min)  TBD UVM case 9（PAD 全覆盖）
12. 仿真 dma_endtoend                  (~20 min)  TBD UVM case 10（DMAC 端到端）
13. 仿真 had_jtag                      (~20 min)  TBD UVM case 11（JTAG VIP）
14. 仿真 low_power_wakeup              (~30 min)  TBD UVM case 12（PMU 电源管理）
15. 仿真 bus_watchdog                  (~15 min)  TBD UVM case 13（总线 watchdog）
```

预估总时间：~250-310 min（既有 2 case ~10 min + 13 个 TBD case ~240-300 min）

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| 64 个中断源全覆盖测试组合爆炸 | 抽样测试：每个外设至少 1 个代表中断源（如 TIM 测 TIM0[0]，PWM/RTC/WDT/USI 各自 1 个中断）；完整覆盖需要专门的 VIC stress test |
| ETB 跨模块触发链测试依赖 reference model 与各外设 ETB 行为一致性 | TB 侧 reference model 与 RTL 输出对比；先做单元链路（如 DMAC↔TIM），再做端到端 |
| PMU 电源管理测试需 force/release 各域 `pmu_*_pwr_good`，可能影响其他模块 | 每个 PMU 测试独立仿真，避免与正常运行测试混跑 |
| JTAG VIP 与 CPU 调试接口握手协议复杂 | 短期 `had_jtag` (TBD) 仅做基础寄存器读写访问，断点/单步功能待后续 |
| DMA 端到端测试需各外设（USI/PWM）DMA 请求线采样 | 短期 `dma_endtoend` (TBD) 优先做 USI TX → DMAC RX 通道；PWM DMA 待后续 |
| 总线 watchdog 测试需 SoC 复位架构支持（依赖 sys_rst_b 等机制） | 短期仅做 hready=0 拉低时 CPU 总线异常捕获测试 |
| `e902_had_test.c` 既有 stub 仅 `while(1)`，未实际测 HAD | `had_jtag` (TBD) 需完全重写，需 JTAG VIP 支持 |
| 中断优先级嵌套测试需 CPU 中断处理例程配合 | 短期仅做硬件 VIC 嵌套验证，软件 ISR 由 C 固件简化实现 |
| 各 c_case 测试都通过 `cpu_flag_addr=0x20007C50` 上报 PASS，存在共享地址冲突风险 | 各 c_case 独立仿真 run；TB 端在 run 间复位 cpu_flag_addr |
| 系统级性能/功耗测试不在本计划范围内 | 性能/功耗属后续专项验证 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── addr_map/
│   └── map_test.c                   (F1, F10：既有，整片地址空间覆盖)
├── had_soc/
│   └── e902_had_test.c              (F9：既有 stub，待完善)
├── timer/   timer_test.c           (F4：TIM 中断，TIM0[0] 中断号 17)
├── dma/     dma_test.c              (F4, F11：DMAC 中断 #32)
├── gpio/    gpio_test.c             (F4, F7：GPIO0 中断 #16)
├── usi_uart/  usi_uart_test.c       (F4：USI0/1 中断 #28/29)
├── usi_i2c/   usi_i2c_test.c        (F4)
├── usi_spi/   usi_spi_test.c        (F4)
├── pwm/     pwm_test.c              (F4, F11：PWM 中断 #25)
├── wdt/     wdt_test.c              (F4：WDT 中断 #27 + 系统复位)
├── rtc/     rtc_test.c              (F4, F12：RTC 中断 #26 + AOU/PDU 跨域)
```

### 测试注册
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_test_lib.svh` — `soc_top_smoke_test` / `soc_top_for_c_case_test` / `soc_top_vip_run_with_c_test`
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_reg_test.svh` — `soc_top_reg_smoke_test`
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_testcase_pkg.svh` — include test_base + test_lib
- `dv/simulation/verif_env/soc/soc_top/tests/uvm_test/soc_top_test_base.svh` — UVM_ERROR 计数 + `UVM_CASE_PASS` 上报 + `USE_AHB_VIP_TO_REPLACE` 宏

### Header 文件
- `dv/simulation/firmware_ksim/lib/clib/vtimer.h` — `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail` / `sim_save`
- `dv/simulation/firmware_ksim/lib/clib/datatype.h` — `uint32_t` 等类型定义

### TB 文件
- `dv/simulation/verif_env/soc/soc_top/` — SoC top test 入口（含 env / monitor）
- `dv/simulation/verif_env/soc/ahb_hs/` — MAIN AHB UVM test（含 7 master × 12 slave monitor）
- `dv/simulation/verif_env/soc/ahb_ls/` — LS AHB UVM test
- `dv/simulation/verif_env/soc/apb0/` — APB0 UVM test
- `dv/simulation/verif_env/soc/apb1/` — APB1 UVM test（含 AOU 域直连）
- `dv/simulation/verif_env/soc/top_sim/` — 顶层集成 test

### RTL 关键源文件
- `wujian100_open/soc/wujian100_open_top.v` — 顶层（PAD 环、PMU 顶层互连）
- `wujian100_open/soc/aou_top.v` — Always-On 子系统（RTC + GPIO + PMU dummy）
- `wujian100_open/soc/core_top.v` — CPU 子系统（E902 + CLIC + CoreTim + WIC）
- `wujian100_open/soc/pdu_top.v` — Power-Down 单元（含主总线矩阵与 APB 子桥）
- `wujian100_open/soc/retu_top.v` / `smu_top.v` / `sms.v` — Reset/SMU/SRAM 子系统
- `wujian100_open/soc/matrix.v` / `ahb_matrix_top.v` — MAIN 总线矩阵
- `wujian100_open/soc/ls_sub_top.v` / `apb0_sub_top.v` / `apb1_sub_top.v` — LS + APB 子桥
- `wujian100_open/soc/clkgen.v` — 时钟生成（PMU dummy_top）

### 交叉参考文档
- `doc_summary/module_analysis/system_overview_analysis.md` — SoC 顶层模块分析（地址映射 / PAD / 中断源 / 总线拓扑）
- `doc_summary/module_analysis/tim_analysis.md` 等 7 个模块分析文档
- `doc_summary/module_analysis/_src/userguide.txt` 第 156-398 行 — User Guide System Overview 章节原文
- 各模块验证计划：`doc_summary/verification_plan/{tim,dma,usi,wdt,pwm,rtc,gpio}_verification_plan.md`
