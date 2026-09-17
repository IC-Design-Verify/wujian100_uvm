# T-Head wujian100_open TIM (×8, T0~T7, 各 2 通道 Timer1/Timer2) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Timer (×8 实例：TIM0 ~ TIM7；每实例 2 通道 Timer1 / Timer2)
**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16
**基址**（按实例，APB0/APB1 各占 P0~P3）：

| 实例 | RTL 文件 | APB | 基址 | P# |
|---|---|---|---|---|
| TIM0 | `tim.v` | APB0 | `0x5000_0000` | P0 |
| TIM1 | `tim1.v` | APB1 | `0x6000_0000` | P0 |
| TIM2 | `tim2.v` | APB0 | `0x5000_0400` | P1 |
| TIM3 | `tim3.v` | APB1 | `0x6000_0400` | P1 |
| TIM4 | `tim4.v` | APB0 | `0x5000_0800` | P2 |
| TIM5 | `tim5.v` | APB1 | `0x6000_0800` | P2 |
| TIM6 | `tim6.v` | APB0 | `0x5000_0C00` | P3 |
| TIM7 | `tim7.v` | APB1 | `0x6000_0C00` | P3 |

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/tim.v` / `tim1.v` ~ `tim7.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `TIMER1_WIDTH` | `32` | Timer1 计数器位宽 |
| `TIMER2_WIDTH` | `32` | Timer2 计数器位宽 |
| `TIMER1_PULSE_EXTD` | 默认 `0`（待确认具体默认值） | Timer1 中断脉冲展宽（0=单拍脉冲） |
| `TIMER2_PULSE_EXTD` | 默认 `0`（待确认具体默认值） | Timer2 中断脉冲展宽 |
| `APB_ADDR_WIDTH` | `12`（每实例 1 KB 地址空间） | APB 地址宽度（取自 sec_top 端口 `paddr[11:0]`） |
| `PRDATA_WIDTH` | `32` | APB 读数据位宽 |

**关键配置含义**：
- 32-bit 计数器决定 overflow 周期 = `2^32 / pclk`；软件测试需选用远小于此的 load value（如 `0x400` 已在既有 C 测试中使用）。
- 每实例 1 KB 地址空间内有效寄存器仅占前 40 字节（10 × 4B），其余高位地址读取返回 0。
- 8 个实例完全同构（参数化 `TIMER1_WIDTH`/`TIMER2_WIDTH` 等），验证策略可对 TIM0 完成功能覆盖，其余 7 个做镜像扫一遍即可。

### 1.2 寄存器映射（每实例 10 个寄存器，offset 相对于实例基址）

| Offset | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | Timer1 Load Count | RW | `0x0000_0000` | Timer1 计数器加载值（user-defined 模式下生效） |
| `0x04` | Timer1 Current Value | RO | `0x0000_0000` | Timer1 当前计数值（RO） |
| `0x08` | Timer1 Control Reg | RW | `0x0000_0000` | bit0 enable / bit1 mode（user-defined=1, free-running=0）/ bit2 interrupt mask / bit4 hardware trigger enable |
| `0x0C` | Timer1_int_clr | RO（读清中断） | `0x0000_0000` | 读该寄存器清除 Timer1 中断状态 |
| `0x10` | Timer1Int Status | RO | `1'b0` | Timer1 中断状态（bit0 有效，其余 reserved） |
| `0x14` | Timer2 Load Count | RW | `0x0000_0000` | Timer2 加载值 |
| `0x18` | Timer2 Current Value | RO | `0x0000_0000` | Timer2 当前值 |
| `0x1C` | Timer2 Control Reg | RW | `0x0000_0000` | Timer2 控制（同 Timer1） |
| `0x20` | Timer2_int_clr | RO（读清中断） | `0x0000_0000` | 读清 Timer2 中断 |
| `0x24` | Timer2Int Status | RO | `1'b0` | Timer2 中断状态 |

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `apb0_sub_top.v` / `apb1_sub_top.v`）

- **时钟**：所有 8 个 TIM IP 共享 SoC 主 `pclk`（APB 总线时钟）；sec_top 内 `timer_1_clk` / `timer_2_clk` 端口当前未显式接入（仅内部声明，详见 §7 风险）。
- **复位**：`presetn` 全局复位，sec_top 内部生成 `timer_1_resetn` / `timer_2_resetn`。
- **总线挂载**：APB0 P0~P3 挂 TIM0/2/4/6；APB1 P0~P3 挂 TIM1/3/5/7；通过 `apb0_sub_top` / `apb1_sub_top` 实例化 `tim.v` ~ `tim7.v`。
- **中断映射**：每个实例 `intr[1:0]` 对应 2 个中断 slot；System Overview Table 1-4 中 `TIM0[1:0]` ~ `TIM7[1:0]` 共 16 个中断向量号（具体中断号待确认 System Overview 表行号）。
- **ETB 连接**：每通道 1 bit `timN_etb_trig` 输出 + 2 bit 输入（`etb_timN_trig_en_on` / `_off`）。
- **时钟门控**：`apb0_sub_top` / `apb1_sub_top` 提供 `pmu_tim{0..7}_{p0clk|p1clk|p0rst_b|p1rst_b}` 用于 SoC 电源管理。

### 1.4 关键 RTL 行为

1. `timers_apbif`（tim.v）：将 APB `paddr` 解码到 10 个内部寄存器；写 Control Reg bit0 后启动对应 timer；读 `int_clr` 自动清 `Int Status`。
2. `timers_frc`（tim.v）：32-bit 计数器核心，根据 Control Reg bit1 选择 `user-defined`（load + 当前值匹配产生中断） / `free-running`（load + 计数到 0 回卷）模式；bit2 控制中断 mask 输出。
3. `timN_sec_top`（tim.v / timN.v）：sec 封装层，目前对 `tipc_timN_trust` / `pprot[2:0]` 仅作端口透传，未做 trust 过滤（tim_analysis.md §7.2 标注）。
4. 中断聚合：`intr[1:0]` 在 sec_top 顶层输出，bit0=Timer1 中断、bit1=Timer2 中断；各 bit 经过 mask 后才上报。
5. `scan_mode` 输入：DFT 扫描模式控制（tim_analysis.md §6.2 item 4）。

### 1.5 TB 检查架构

- **C 端检查**：`timer_test.c`（既有）通过 `mem_read32_(0x5000_0010, &int_tim_flag)` 轮询 Timer1Int Status，等待 `== 0x1` 后调 `mem_read32_(0x5000_000C, &int_tim_eoi)` 读清中断，最后 `sim_end()`。
- **TB 采样**：UVM 侧 `soc_top_for_c_case_test` 通过 `c_case` 固件运行 `timer_test.c`，在 `cpu_flag_addr`（0x20007C50）处读 end marker（`0x2002` = pass / `0x1001` = fail）。
- **TB 计数**：base test 统计 UVM_ERROR 数量，为 0 时打印 `UVM_CASE_PASS`。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: Timer1/2 自由计数模式（Free-Running）
**目标**：Control Reg bit1=0 时，计数器从 Load Count 递减到 0 后回卷到 `0xFFFFFFFF` 继续递减；不产生中断。
**已有 case**：`timer_test.c`（既有）— 写 `0x400` 到 Load Count、Control Reg `0x3`（enable + user-defined），但**未覆盖 free-running**。
**检查**：C 端轮询 Current Value 在 `(0x400, 0x000)` 区间内循环；TB 端 `cpu_flag_addr` 读到 0x2002。
**缺口**：自由计数回卷 + 无中断 行为**待新建 case（标记 TBD）**。

### F2: Timer1/2 用户定义计数模式（User-Defined）
**目标**：Control Reg bit1=1 时，计数器从 Load Count 递减到 0 后停止在 0；递减过程中 Current Value == Load Count 时产生中断。
**已有 case**：`timer_test.c`（既有）— 写入 Load Count=0x400、Control Reg=0x3（enable + user-defined），等到 `Timer1Int Status==0x1`。
**检查**：C 端读 Timer1Int Status == 1 → 读 `int_clr` 清中断 → 读 Int Status 应回 0。

### F3: Timer1/2 硬件触发（Hardware Trigger, ETB）
**目标**：Control Reg bit4=1 打开 hardware trigger；ETB 输入 `etb_timN_trig_en_on/off` 控制是否在计数到 0 时自动 reload load value。
**已有 case**：无（既有 C 测试未涉及 ETB 联动）。
**检查**：TB 侧（apb0/apb1 test）通过 ETB agent 触发 software event，监测 Timer Current Value 在 trigger 后重新从 Load Count 开始递减。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 ETB 驱动支持。

### F4: 中断产生与清除
**目标**：timer 计数到触发条件后 `Int Status[0]` 置 1；读 `int_clr` 后 `Int Status` 清 0；中断 pulse 维持一拍或由 `PULSE_EXTD` 展宽。
**已有 case**：`timer_test.c`（既有）已覆盖 Timer1 中断产生 + 读清流程。
**检查**：C 端读 `Timer1Int Status` 等待 `== 0x1`；调 `mem_read32_(0x5000_000C)` 后再读 status 应回 0。

### F5: 中断屏蔽（Interrupt Mask）
**目标**：Control Reg bit2=1 时，`intr` 输出被屏蔽。**RTL 实证（tim.v: `ri_timerNintstatus = ri_timer_int & ~timerintmask`）：`Int Status` 寄存器为屏蔽后状态**，mask=1 时读为 0；raw pending 位不受 mask 影响，解屏蔽后 `Int Status` 立即反映 pending 中断。
**已有 case**：`timer_int_mask.c`（2026-09-17 新建，PASS）。
**检查**：写 Control Reg=0x7（enable + user-defined + mask），计数过期后 C 端读 `Int Status == 0`（屏蔽生效）；写 Control Reg=0x3 解屏蔽后 `Int Status == 1`（raw pending 暴露）；读 `int_clr` 清 0。

### F6: Timer1/2 双通道独立与并行
**目标**：同一 TIM 实例内 Timer1 与 Timer2 的 Control Reg、Load Count、Current Value、Int Status 完全独立，可并行使能不同模式与不同 load value。
**已有 case**：无（既有 C 测试只验证 Timer1）。
**检查**：C 端分别写 Timer1 与 Timer2 寄存器，分别读 status 验证独立性；并行使能时两个 status 互不影响。
**缺口**：**待新建 case（标记 TBD）**。

### F7: 复位值与只读行为
**目标**：复位后所有寄存器回到 §1.2 reset 值；Current Value / Int Status / int_clr 在运行时读不影响 counter；写 Current Value 被忽略。
**已有 case**：无（既有 C 测试未做 reset 后初始状态校验）。
**检查**：`presetn` 释放后立即读 10 个寄存器，校验 reset 值；写 `Current Value` 后再读应保持原值。
**缺口**：**待新建 case（标记 TBD）**。

### F8: 寄存器 R/W 边界（地址未解码区域）
**目标**：10 个寄存器以外的 offset（如 `0x28` ~ `0x3FF`）读返回 `0x0`，写忽略。
**已有 case**：`map_test.c`（addr_map 通用测试）— 已对多地址空间做了 read 0 检查，但 TIM 区域未单独标注（待确认）。
**检查**：C 端读 `base + 0x28` 应 == 0。
**备注**：若 map_test 已覆盖整个 0x5000_0000~0x5000_03FF，则可标记"已覆盖 by map_test"。

### F9: 多实例地址独立性
**目标**：TIM0 ~ TIM7 各自独立地址空间，互不干扰；对 TIM0 写不影响 TIM1 状态。
**已有 case**：`map_test.c`（addr_map）— 通过对 `0x50000000`/`0x60000000` 等做 read 测试，已隐含覆盖。
**检查**：写 TIM0 Control Reg=enable，再读 TIM1 Control Reg 应保持 reset 0。
**备注**：若 map_test 已逐实例验证，可标"已覆盖 by map_test"。

### F10: 中断向量与 SoC VIC 路由
**目标**：TIM `intr[1:0]` 经 SoC VIC 路由到 CPU 中断号，与 System Overview Table 1-4 中断号一致。
**已有 case**：无（C 端无法直接验证中断号，需 UVM 端 VIC monitor）。
**检查**：UVM 侧打开 TIM 中断后，cpu_intr[N] 上升沿匹配中断号。
**缺口**：**待新建 case（标记 TBD）**，依赖 SoC VIC monitor。

### F11: ETB 触发输出（tim_etb_trig）
**目标**：每通道 1 bit `timN_etb_trig` 输出，连接到 SoC ETB fabric；时序与中断状态对齐。
**已有 case**：无。
**检查**：TB 端采样 ETB fabric 上 timN_etb_trig，验证脉冲宽度与时机。
**缺口**：**待新建 case（标记 TBD）**，依赖 ETB monitor。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `timer_test`（既有 `c_case/timer/timer_test.c`） | `soc_top_for_c_case_test` | F2, F4 | C 端基础用例 |
| 2 | `timer_freerun_smoke`（待新建，TBD） | `soc_top_for_c_case_test` | F1, F7 | C 端冒烟 |
| 3 | `timer_dual_ch_parallel`（待新建，TBD） | `soc_top_for_c_case_test` | F6, F8 | C 端并行用例 |
| 4 | `timer_int_mask`（待新建，TBD） | `soc_top_for_c_case_test` | F5, F7 | C 端 |
| 5 | `timer_reset_default`（待新建，TBD） | `soc_top_for_c_case_test` | F7, F9 | C 端复位检查 |
| 6 | `timer_etb_hw_trig`（待新建，TBD） | UVM 侧 `apb0_test`/`apb1_test` | F3, F11 | UVM 序列 |
| 7 | `timer_vic_route`（待新建，TBD） | UVM 侧 `soc_top_vseq` | F10 | UVM 中断监测 |
| 8 | `addr_map`（既有 `c_case/addr_map/map_test.c`） | `soc_top_for_c_case_test` | F8, F9（部分） | C 端冒烟 |
| 9 | `timer_mirror_T1_T7`（待新建，TBD） | `soc_top_for_c_case_test` | F2, F4（覆盖镜像 7 个实例） | C 端回归 |

### 功能覆盖矩阵

| Feature | timer_test | timer_freerun_smoke | timer_dual_ch_parallel | timer_int_mask | timer_reset_default | timer_etb_hw_trig | timer_vic_route | addr_map | timer_mirror_T1_T7 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: free-running | - | ✓ (1 cycle) | - | - | - | - | - | - | ✓ (镜像) |
| F2: user-defined | ✓ (load=0x400) | - | ✓ | ✓ | - | - | - | - | ✓ (×8) |
| F3: HW trigger (ETB) | - | - | - | - | - | ✓ (1 trigger) | - | - | - |
| F4: 中断产生与清除 | ✓ | - | ✓ | ✓ | - | ✓ | - | - | ✓ (×8) |
| F5: 中断屏蔽 | - | - | - | ✓ | - | - | - | - | - |
| F6: 双通道独立 | - | - | ✓ | - | - | - | - | - | - |
| F7: 复位值/只读 | - | ✓ | - | ✓ | ✓ | - | - | - | - |
| F8: 地址边界 | - | - | ✓ | - | ✓ | - | - | ✓ (整片) | - |
| F9: 多实例独立性 | - | - | - | - | ✓ | - | - | ✓ (部分) | ✓ |
| F10: 中断向量路由 | - | - | - | - | - | - | ✓ | - | - |
| F11: ETB 输出 | - | - | - | - | - | ✓ | - | - | - |

> 矩阵用 ✓/- 标记；✓ 后括号注明覆盖量。"TBD" 表示待新建 case，不阻塞既有 timer_test 通过但属于覆盖缺口。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test          (UVM 序列基线)
  └── soc_top_for_c_case_test     (运行 C 端测试用例，含 timer_test)
```

既有 `timer_test` 通过 `soc_top_for_c_case_test` 加载 `timer_test.c` 固件运行。UVM 侧新增 TIM 专用序列（如 `timer_intr_seq`、`timer_etb_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，TIM 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/timer/timer_test.c` 决定具体行为。后续 TBD 用例沿用同一入口，通过修改 `c_case/timer/` 下不同 .c 文件选择。

> **待确认**：项目是否计划引入独立 TIM uvm_test 子类（参考 GPIO/PWM 模块的既有 uvm_test 子类）。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `timer_test.c` 的 `printf("\ntimer test successfully\n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **TIM 专用 monitor（TBD）**：未来新增 UVM 侧 case 时，需在 `soc_top_env` 内增加 TIM intr / ETB trig / 当前计数 采样 monitor。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `timer_test`（既有） | `printf("\ntimer test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `timer_freerun_smoke` (TBD) | 计数器回卷后 Current Value 在 `[0x400, 0x000]` 循环 + `sim_end()` |
| `timer_dual_ch_parallel` (TBD) | Timer1/Timer2 状态独立，各自 status 与 counter 互不干扰 |
| `timer_int_mask` | mask=1 时计数过期后 Int Status 保持 0（IntStatus 为屏蔽后状态，tim.v: `raw & ~mask`）；写 ControlReg 解屏蔽后 pending 中断暴露为 Int Status == 1；int_clr 读清零 |
| `timer_reset_default` (TBD) | 复位后 10 个寄存器值与 §1.2 reset 表一致 |
| `timer_etb_hw_trig` (TBD) | TB 端 ETB trigger 事件 → counter 自动重载 Load Count |
| `timer_vic_route` (TBD) | TB 端 cpu_intr[N] 上升沿匹配 System Overview 中断号 |
| `addr_map`（既有） | 各地址空间 read value 与期望一致；TIM 区域（待确认 map_test 是否覆盖 0x5000_0000~0x5000_03FF） |
| `timer_mirror_T1_T7` (TBD) | TIM1 ~ TIM7 与 TIM0 行为镜像，全部 `sim_end()` |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 timer_test                (~5 min)   既有 C 端基本功能
3. 仿真 addr_map                  (~5 min)   既有地址空间覆盖
4. 仿真 timer_freerun_smoke       (~5 min)   新增 case 1
5. 仿真 timer_reset_default       (~5 min)   新增 case 2
6. 仿真 timer_dual_ch_parallel    (~5 min)   新增 case 3
7. 仿真 timer_int_mask            (~5 min)   新增 case 4
8. 仿真 timer_mirror_T1_T7        (~10 min)  新增 case 5（覆盖 7 个镜像实例）
9. 仿真 timer_etb_hw_trig         (~10 min)  新增 UVM 侧 case 6
10. 仿真 timer_vic_route          (~10 min)  新增 UVM 侧 case 7
```

预估总时间：~60-75 min（既有 case ~10 min + 7 个 TBD case ~50-65 min）

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| `tipc_timN_trust` / `pprot[2:0]` 端口预留但 RTL 未对接 trust 过滤（tim_analysis.md §7.2） | 当前测试视为不影响行为；后续若启用 trustzone，需补充 trust 边界 case |
| `timer_1_clk` / `timer_2_clk` 在 sec_top 未接入外部，2 通道共享 pclk | 当前测试假设通道共享时钟；若未来提供独立时钟门控，需在 TB 端扩展多时钟域 |
| `TimerN_int_clr` user guide 描述读清中断但访问类型未明（tim_analysis.md §7.2） | RTL 实现为 RO，C 端通过 `mem_read32_` 触发清中断；测试以 RTL 行为为准 |
| `PULSE_EXTD` 默认值在文档未明（tim_analysis.md §6.2 item 1） | 测试以 RTL 默认 0 为基线；若行为不符，需用 force/release 在 TB 侧覆盖 |
| 中断号具体值依赖 System Overview Table 1-4（tim_analysis.md §6.2 item 1） | 在 TB 侧从寄存器读 `intr_id_base` 或 hardcode 中断号；后续以 doc_review 修复后版本为准 |
| UVM 侧 ETB / VIC monitor 缺失（F3/F10/F11） | 短期内仅做 C 端覆盖；UVM 侧 case 6/7 标注 TBD，待 monitor 落地 |
| `timer_test.c` 既有 case 未覆盖 free-running（仅 user-defined） | F1 由新增 `timer_freerun_smoke` 补齐；不阻塞既有 case 回归 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── addr_map/
│   └── map_test.c                (F8, F9：整片地址空间 read 0 检查)
├── timer/
│   └── timer_test.c              (F2, F4：Timer1 user-defined + 中断产生与清除；既有)
├── timer_freerun_smoke/timer_freerun_smoke.c      (F1, F7；已建，PASS)
├── timer_dual_ch_parallel/timer_dual_ch_parallel.c (F6, F8；已建，PASS)
├── timer_int_mask/timer_int_mask.c                (F5, F7；已建，PASS)
├── timer_reset_default/timer_reset_default.c      (F7, F9；已建，PASS)
├── timer_mirror/timer_mirror_T1_T7.c              (F2, F4：7 个实例回归；已建，PASS)

（注：make_hex 会链接 C_TEST 同目录下所有 .c，故每个新用例独立目录）
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
- `dv/simulation/verif_env/soc/apb0/`、`apb1/` — APB 总线侧 UVM test（待新增 TIM ETB 序列时使用）

### 交叉参考文档
- `doc_summary/module_analysis/tim_analysis.md` — TIM 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/Timer_TIM_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 399-518 行 — User Guide Timer 章节原文
