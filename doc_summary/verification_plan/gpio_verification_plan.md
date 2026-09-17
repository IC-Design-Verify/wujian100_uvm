# T-Head wujian100_open GPIO (32-bit, ×1, AOU 域) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open General-purpose I/O (GPIO)
  - 唯一 1 个实例，挂 APB1 P5，base `0x6001_8000`（外部地址空间 16 KB）
  - 32 路 GPIO（GPIO0~GPIO31），每路独立方向 / 数据 / 中断配置
  - 11 个寄存器，offset `0x00` ~ `0x50`（含 `0x0C`~`0x2C`/`0x48` reserved gap）
  - AOU 域子系统（`aou_top.v:552` 实例化 `gpio0_sec_top`）
  - 中断号 16（`GPIO0`，见 System Overview Table 1-4）
  - 32 路 ETB 触发输出 `gpio0_etb_trig[31:0]`

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/gpio0.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `GPIO_WIDTH` | `32` | GPIO 数据宽度（bit 数） |
| `REG_NUM` | `11` | 有效寄存器数 |
| `INT_WIDTH` | `32` | 中断信号位宽 |
| `ETB_TRIG_WIDTH` | `32` | ETB 触发输出位宽 |
| `pclk_intr` | 独立时钟域 | 中断逻辑时钟（PMU 控制 `gpio_intrclk_en`） |
| `PAD_GPIO_NUM` | `32` | 对应 SoC 顶层 32 个 `PAD_DIG_IO` 单元 |
| `gpio_direction[bit]` | `0`=Input / `1`=Output | GPIO 方向 |
| `gpio_ctl[bit]` | `0`=Software / `1`=Hardware | 数据源选择 |

**关键配置含义**：
- 中断约束："Interrupts are disabled on the corresponding bits of GPIO if the corresponding data direction register is set to Output or if GPIO mode is set to Hardware"——中断仅在 `Input mode` ∧ `Software mode` 时生效。
- `gpio_input_data` 寄存器：在 Input 模式下读取 PAD 输入；在 Output 模式下读取 `gpio_output_data` 的最后写入值。
- GPIO 寄存器地址存在多个 reserved gap（`0x0C`~`0x2C` / `0x48`），这些地址读返回 0、写忽略。
- 中断逻辑时钟 (`pclk_intr`) 与 APB 时钟 (`pclk`) 可独立，由 PMU 提供 `pmu_gpio_p1clk` / `pmu_gpio_p1rst_b`。

### 1.2 寄存器映射

| Offset | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | `gpio_output_data` | RW | `0x0` | 32-bit 数据寄存器（Software 模式驱动 PAD 输出） |
| `0x04` | `gpio_direction` | RW | `0x0` | 32-bit 方向寄存器（每 bit：0=Input, 1=Output） |
| `0x08` | `gpio_ctl` | RW | `0x0` | 32-bit 数据源选择（每 bit：0=Software, 1=Hardware） |
| `0x0C-0x2C` | Reserved | — | — | gap，read = 0, write 忽略 |
| `0x30` | `gpio_inten` | RW | `0x0` | 32-bit 中断使能 |
| `0x34` | `gpio_intmask` | RW | `0x0` | 32-bit 中断屏蔽 |
| `0x38` | `gpio_inttype_level` | RW | `0x0` | 32-bit 触发类型（0=level, 1=edge） |
| `0x3C` | `gpio_int_polarity` | RW | `0x0` | 32-bit 中断极性 |
| `0x40` | `gpio_intstatus` | RO | `0x0` | 32-bit masked 后中断状态 |
| `0x44` | `gpio_rawintstatus` | RO | `0x0` | 32-bit 未 mask 原始中断状态 |
| `0x48` | Reserved | — | — | gap |
| `0x4C` | `gpio_porta_int_clr`（字段表名 `gpio_int_clr`） | WO | `0x0` | 写 1 清中断 |
| `0x50` | `gpio_input_data` | RO | `0x0` | 32-bit 外部输入数据（Input 模式读 PAD / Output 模式读 last write） |

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `aou_top.v` / `apb1_sub_top.v`）

- **时钟**：`pclk`（APB 时钟，来自 PMU `pmu_gpio_p1clk`）+ `pclk_intr`（中断逻辑时钟）。
- **复位**：`presetn`（低有效，来自 PMU `pmu_gpio_p1rst_b`）。
- **总线挂载**：APB1 P5，base `0x6001_8000`；通过 `apb1_sub_top` → `aou_top`（`aou_top.v:552` 实例化 `gpio0_sec_top`）。
- **PAD 接入**：32 路 `gpio_ext_porta[31:0]` 来自 SoC 顶层 32 个 `PAD_DIG_IO` 单元（`PAD_GPIO_0`~`PAD_GPIO_31`）；32 路 `gpio_porta_dr[31:0]` + `gpio_porta_ddr[31:0]` 输出到 PAD。
- **中断**：`gpio_intr_flag`（1 bit）→ SoC CLIC/VIC，中断号 16 = `GPIO0`。
- **ETB 触发**：32-bit `gpio0_etb_trig[31:0]` 接入 SoC ETB fabric。
- **PMU 控制**：`gpio_intrclk_en` 是中断时钟门控使能（PMU 控制）。
- **Trust**：`tipc_gpio0_trust` / `pprot[2:0]` 端口预留但未对接 trust 逻辑。

### 1.4 关键 RTL 行为

1. **方向控制**（`gpio_direction`）：每 bit 决定 Input/Output 模式。
2. **数据源**（`gpio_ctl`）：Software 模式 → `gpio_output_data` 直接驱动 PAD；Hardware 模式 → 由外设驱动。
3. **输入读**（`gpio_input_data`）：Input 模式读 PAD；Output 模式读 last write value。
4. **中断使能 4 件套**：
   - `gpio_inten`：每 bit 中断使能（总开关）
   - `gpio_intmask`：每 bit 中断屏蔽（mask=1 → `intstatus` 不显示但 `rawintstatus` 仍置位）
   - `gpio_inttype_level`：0=level, 1=edge
   - `gpio_int_polarity`：中断极性（high/rising vs low/falling）
5. **中断约束**：中断仅在 `(direction=Input) ∧ (ctl=Software)` 时生效。
6. **中断状态**（`gpio_intstatus` masked 后 / `gpio_rawintstatus` 未 mask）+ 清（`gpio_porta_int_clr` 写 1 清）。
7. **ETB 触发**：32-bit `gpio0_etb_trig` 每 bit 1 路触发。
8. **PMU 控制**：`gpio_intrclk_en` 由 PMU 控制中断逻辑时钟门控。

### 1.5 TB 检查架构

- **C 端检查**：`gpio_test.c`（既有）配置 `gpio_ctl=0x0`（Software 模式）+ `gpio_direction=0x0`（Input），TB 端驱动 PAD 输入 `0x55555555`，轮询 `gpio_input_data (0x60018050)`；然后切到 Output 模式写 `gpio_output_data=0x5a5a5a5a`，再切回 Input 模式验证读 `0xaaaaaaaa` / `0x12345678`。
- **TB 端**：UVM `soc_top_for_c_case_test` 加载固件，通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记（`0x2002` = PASS / `0x1001` = FAIL）。
- **TB 监测**：未来新增 UVM 序列时需 PAD monitor（驱动/采样 32 路 `gpio_ext_porta` + `gpio_porta_dr`）、ETB monitor（采样 32-bit `gpio0_etb_trig`）、VIC monitor（采样 `gpio_intr_flag` → `cpu_intr[16]`）。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: 输出数据寄存器（gpio_output_data）
**目标**：`gpio_output_data[31:0]` 在 Software + Output 模式下直接驱动 PAD 输出。
**已有 case**：`gpio_test.c`（既有）切到 Output 模式写 `gpio_output_data=0x5a5a5a5a` / `0xa5a5a5a5a`。
**检查**：TB 端 PAD monitor 采样 32 路 `gpio_porta_dr` 验证与 `gpio_output_data` 写入值一致。

### F2: 方向控制（gpio_direction）
**目标**：`gpio_direction[bit]=1` 时 PAD 输出；`=0` 时 PAD 输入。
**已有 case**：`gpio_test.c`（既有）切 `gpio_direction=0x0` (Input) / `0xffffffff` (Output)。
**检查**：TB 端 PAD monitor 验证 `gpio_porta_ddr` 与 `gpio_direction` 写入值一致。
**缺口**：单 bit 独立方向（32 bit 不同方向组合）**待新建 case（标记 TBD）**。

### F3: 输入采样（gpio_input_data）
**目标**：Input 模式下 `gpio_input_data` 读取 PAD 输入；Output 模式下读 last write value。
**已有 case**：`gpio_test.c`（既有）TB 端驱动 PAD `0x55555555` / `0xaaaaaaaa` / `0x12345678`，C 端读 `gpio_input_data` 验证。
**检查**：TB 端驱动不同 PAD 模式（高/低/随机），C 端读 `gpio_input_data` 匹配；Output 模式下读回 last write 值。
**缺口**：Output 模式读回 last write value 显式测试**待新建 case（标记 TBD）**。

### F4: 数据源控制（gpio_ctl 配置锁存）
**目标**：`gpio_ctl[bit]=0` Software 模式由 `gpio_output_data` 驱动；`=1` Hardware 模式由外设驱动。
**已有 case**：`gpio_test.c`（既有）配置 `gpio_ctl=0x0`（全部 Software）。
**检查**：TB 端验证 `gpio_ctl=1` 时 PAD 输出不受 `gpio_output_data` 控制；与中断约束联动验证（Hardware 模式禁用中断）。
**缺口**：Hardware 模式**待新建 case（标记 TBD）**。

### F5: 中断使能 / 屏蔽 / 类型 / 极性（4 件套）
**目标**：`gpio_inten` 总开关 + `gpio_intmask` 屏蔽 + `gpio_inttype_level` level/edge + `gpio_int_polarity` 极性。
**已有 case**：无（既有 c_case 未覆盖中断路径）。
**检查**：TB 端驱动 PAD 边沿/电平，验证 `intstatus` 正确反映配置后的中断。
**缺口**：4 件套独立 + 组合**待新建 case（标记 TBD）**。

### F6: 中断状态 / 原始状态 / 清除
**目标**：`rawintstatus` 反映未 mask 中断；`intstatus` 反映 mask 后；`gpio_porta_int_clr` 写 1 清。
**已有 case**：无。
**检查**：TB 端产生中断 → 读 `rawintstatus=1` → mask=1 → 读 `intstatus=0` 但 `rawintstatus=1` → 清中断 → `rawintstatus=0`。
**缺口**：**待新建 case（标记 TBD）**。

### F7: 中断约束（direction=Output 或 ctl=Hardware 时禁用中断）
**目标**：中断仅在 `(direction=Input) ∧ (ctl=Software)` 时生效。
**已有 case**：无。
**检查**：TB 端在 `direction=Output` 或 `ctl=Hardware` 时驱动 PAD，验证 `rawintstatus` 不置位。
**缺口**：**待新建 case（标记 TBD）**，需 32 bit 边界验证。

### F8: 复位值（全 0）
**目标**：复位后 11 个寄存器全部回到 `0x0`。
**已有 case**：无。
**检查**：`presetn` 释放后立即读 11 个寄存器，校验 reset 值。
**缺口**：**待新建 case（标记 TBD）**。

### F9: Reserved gap 行为
**目标**：`0x0C`~`0x2C` 与 `0x48` 地址读返回 0、写忽略。
**已有 case**：`map_test.c`（addr_map）通用测试已隐含覆盖 read 0（待确认 map_test.c 是否触达 0x6001_8000~0x6001_BFFF）。
**检查**：C 端读 `0x6001800C` 等 gap 地址应 == 0。
**备注**：若 map_test 已覆盖可标"已覆盖"。

### F10: SoC PAD 连接（PAD_GPIO_0~31）
**目标**：32 路 GPIO 通过 `PAD_DIG_IO` 单元接到 `PAD_GPIO_0`~`PAD_GPIO_31`。
**已有 case**：`gpio_test.c`（既有）依赖 TB 端 PAD 驱动/采样（隐含验证 PAD 连接）。
**检查**：TB 端确认 PAD 单元正确连接；`gpio_ext_porta[31:0]` 与 `gpio_porta_dr/ddr[31:0]` 时序对应。
**备注**：既有 case 已隐含。

### F11: ETB 触发输出（gpio0_etb_trig[31:0]）
**目标**：32-bit ETB 触发输出，每 bit 1 路触发；触发条件与中断状态关联。
**已有 case**：无。
**检查**：TB 端 ETB monitor 采样 `gpio0_etb_trig[31:0]`，验证触发时序与中断事件对齐。
**缺口**：**待新建 case（标记 TBD）**，需 UVM 侧 ETB monitor。

### F12: 中断号路由（VIC 中断号 16）
**目标**：`gpio_intr_flag` 经 SoC VIC 路由到 `cpu_intr[16]` = `GPIO0`。
**已有 case**：无（C 端无法直接验证中断号，需 UVM 端 VIC monitor）。
**检查**：UVM 侧打开 `gpio_intr_flag` monitor，验证 `cpu_intr[16]` 上升沿匹配。
**缺口**：**待新建 case（标记 TBD）**，依赖 SoC VIC monitor。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `gpio_test`（既有 `c_case/gpio/gpio_test.c`） | `soc_top_for_c_case_test` | F1 (写 output_data), F2 (direction 切换), F3 (input_data 读), F4 (Software mode), F10 (PAD 连接 隐含) | C 端基础 |
| 2 | `gpio_dir_independent`（TBD） | `soc_top_for_c_case_test` | F2 (单 bit 独立方向) | C 端 |
| 3 | `gpio_output_data_echo`（TBD） | `soc_top_for_c_case_test` | F3 (Output 模式读回 last write) | C 端 |
| 4 | `gpio_hardware_mode`（TBD） | UVM 侧 | F4 (Hardware 模式) | UVM PAD 驱动 |
| 5 | `gpio_intr_4reg`（TBD） | `soc_top_for_c_case_test` | F5 (inten/mask/type/polarity 4 件套) | C 端 |
| 6 | `gpio_intr_status_clear`（TBD） | `soc_top_for_c_case_test` | F6 (rawintstatus / intstatus / clr) | C 端 |
| 7 | `gpio_intr_constraint`（TBD） | `soc_top_for_c_case_test` + TB | F7 (Output/Hardware 模式禁用中断) | C 端 |
| 8 | `gpio_reset_default`（TBD） | `soc_top_for_c_case_test` | F8 (×11 全 0) | C 端复位检查 |
| 9 | `gpio_reserved_gap`（TBD） | `soc_top_for_c_case_test` | F9 (gap 地址 read 0) | C 端 |
| 10 | `gpio_etb_trig`（TBD） | UVM 侧 | F11 (32-bit ETB 触发) | UVM ETB monitor |
| 11 | `gpio_vic_route`（TBD） | UVM 侧 | F12 (cpu_intr[16] 路由) | UVM VIC monitor |

### 功能覆盖矩阵

| Feature | gpio_test | gpio_dir_independent | gpio_output_data_echo | gpio_hardware_mode | gpio_intr_4reg | gpio_intr_status_clear | gpio_intr_constraint | gpio_reset_default | gpio_reserved_gap | gpio_etb_trig | gpio_vic_route |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: output_data | ✓ | ✓ | - | - | - | - | - | - | - | - | - |
| F2: direction | ✓ (全 0/全 1) | ✓ (×32) | - | - | - | - | - | - | - | - | - |
| F3: input_data | ✓ (×3) | - | ✓ (回读) | - | - | - | - | - | - | - | - |
| F4: gpio_ctl | ✓ (Software) | - | - | ✓ (Hardware) | - | - | - | - | - | - | - |
| F5: 中断 4 件套 | - | - | - | - | ✓ (×32) | ✓ | - | - | - | - | - |
| F6: 中断 status/clear | - | - | - | - | - | ✓ | - | - | - | - | - |
| F7: 中断约束 | - | - | - | - | - | - | ✓ | - | - | - | - |
| F8: 复位值 | - | - | - | - | - | - | - | ✓ (×11) | ✓ | - | - |
| F9: reserved gap | - | - | - | - | - | - | - | - | ✓ | - | - |
| F10: PAD 连接 | ✓ (隐含) | - | - | - | - | - | - | - | - | - | - |
| F11: ETB 触发 | - | - | - | - | - | - | - | - | - | ✓ | - |
| F12: VIC 中断号 16 | - | - | - | - | - | - | - | - | - | - | ✓ |

> 矩阵用 ✓/- 标记。"TBD" 表示待新建 case，不阻塞既有 gpio_test 通过但属于覆盖缺口。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test          (UVM 序列基线)
  └── soc_top_for_c_case_test     (运行 C 端测试用例，含 gpio_test)
```

既有 `gpio_test` 通过 `soc_top_for_c_case_test` 加载固件运行。UVM 侧新增 GPIO 专用序列（`gpio_intr_seq` / `gpio_hardware_mode_seq` / `gpio_etb_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，GPIO 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/gpio/gpio_test.c` 决定具体行为。后续 TBD 用例沿用同一入口，通过修改 `c_case/gpio/` 下不同 .c 文件选择。

> **待确认**：项目是否计划引入独立 GPIO uvm_test 子类。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `gpio_test.c` 的 `printf("gpio io test pass! \n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **GPIO 专用 monitor（TBD）**：未来新增 UVM 侧 case 时，需在 `soc_top_env` 内增加：
  - **PAD monitor**：驱动/采样 32 路 `gpio_ext_porta[31:0]`（外部输入）+ `gpio_porta_dr[31:0]` + `gpio_porta_ddr[31:0]`（寄存器值输出）。
  - **ETB monitor**：采样 32-bit `gpio0_etb_trig[31:0]`。
  - **VIC monitor**：采样 `gpio_intr_flag` 上升沿、对应 `cpu_intr[16]`。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `gpio_test`（既有） | TB 端驱动 PAD `0x55555555` / `0xaaaaaaaa` / `0x12345678` + C 端读 `gpio_input_data` 匹配 + `printf("gpio io test pass! \n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `gpio_dir_independent` (TBD) | 32 bit 独立方向配置时 PAD 行为正确（每 bit 独立 Input/Output） |
| `gpio_output_data_echo` (TBD) | Output 模式下读 `gpio_input_data` == last `gpio_output_data` 写入值 |
| `gpio_hardware_mode` (TBD) | `gpio_ctl=1` 时 PAD 输出由 TB 驱动、`gpio_output_data` 无效 |
| `gpio_intr_4reg` (TBD) | 4 件套（inten/mask/type/polarity）独立配置后中断行为正确 |
| `gpio_intr_status_clear` (TBD) | `rawintstatus` 与 `intstatus` 在 mask=1 时不一致；写 `gpio_porta_int_clr` 清中断 |
| `gpio_intr_constraint` (TBD) | direction=Output 或 ctl=Hardware 时中断不置位 |
| `gpio_reset_default` (TBD) | 复位后 11 个寄存器值与 §1.2 reset 表一致（全 0） |
| `gpio_reserved_gap` (TBD) | reserved gap 地址（`0x0C`~`0x2C`/`0x48`）读 0、写忽略 |
| `gpio_etb_trig` (TBD) | `gpio0_etb_trig[31:0]` 各 bit 触发时序与中断事件对齐 |
| `gpio_vic_route` (TBD) | `gpio_intr_flag` 上升沿时 `cpu_intr[16]` 匹配 |

- **GPIO 基地址修正**：元信息与 §1.3 总线挂载中 GPIO 基址曾误写为 `0x6000_4000`（与 RTC 基址冲突），已于 2026-09-16 按 User Guide Peripheral Address Map（_src/userguide.txt L270）与 RTL wujian100_open/soc/params/apb1_params.v L25 (`APB_LEAF_SLV5_START_ADDR = 32'h60018000`) 修正为 `0x6001_8000`，范围 `0x6001_8000` ~ `0x6001_BFFF`。

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 gpio_test                       (~5 min)   既有 C 端基本功能（Input/Output 切换 + PAD 读写）
3. 仿真 gpio_reset_default              (~5 min)   TBD case 1（11 个寄存器复位值）
4. 仿真 gpio_reserved_gap               (~5 min)   TBD case 2（gap 地址 read 0）
5. 仿真 gpio_dir_independent            (~10 min)  TBD case 3（32 bit 独立方向）
6. 仿真 gpio_output_data_echo           (~5 min)   TBD case 4（Output 模式读回）
7. 仿真 gpio_intr_4reg                  (~10 min)  TBD case 5（4 件套中断寄存器）
8. 仿真 gpio_intr_status_clear          (~10 min)  TBD case 6（rawintstatus / intstatus / clr）
9. 仿真 gpio_intr_constraint            (~10 min)  TBD case 7（Output/Hardware 禁用中断）
10. 仿真 gpio_hardware_mode             (~10 min)  TBD UVM case 8（Hardware 模式）
11. 仿真 gpio_etb_trig                  (~10 min)  TBD UVM case 9（32-bit ETB 触发）
12. 仿真 gpio_vic_route                 (~10 min)  TBD UVM case 10（VIC 中断号 16）
```

预估总时间：~90-110 min（既有 case ~5 min + 10 个 TBD case ~85-105 min）

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| GPIO 测试依赖 TB 端 PAD 驱动/采样（32 路） | UVM 侧需 PAD driver agent + PAD monitor；32 路并发需确保时序一致 |
| 中断 4 件套（inten/mask/type/polarity）32 bit 全组合 = 2^96 种，测试组合爆炸 | 抽样测试：每 bit 单独配置 + 关键组合（level+rising / level+falling / edge+rising / edge+falling） |
| 中断约束 `(direction=Input) ∧ (ctl=Software)` 验证需测试 4 种组合（Input×Software / Input×Hardware / Output×Software / Output×Hardware） | 4 组合全覆盖，每组合测单 bit 中断触发 |
| `gpio_porta_int_clr` Memory Map 标 "W"（write-only），但字段表描述与 RTL 行为需一致 | TB 端验证写 1 清、读返回 0（WO 行为） |
| Reserved gap 地址（`0x0C`~`0x2C`/`0x48`）读返回 0、写忽略 | 由 `gpio_reserved_gap` (TBD) 显式覆盖；map_test 已隐含部分覆盖 |
| GPIO 在 AOU 域（`aou_top.v:552`），TB 复位顺序与 PMU 时钟控制依赖 SoC 复位流程 | TB 端需按 `aou_top` → `apb1_sub_top` → GPIO 顺序复位 |
| ETB 32-bit 触发测试依赖 ETB monitor 与 reference model | 短期仅做 GPIO 端寄存器读写 + 验证 `gpio0_etb_trig` 输出时序 |
| 中断号 16 = `GPIO0` 来自 System Overview Table 1-4；具体行号以文档最新版本为准 | TB 侧硬编码中断号 16；后续以 doc_review 修复后版本对齐 |
| `gpio_test.c` 既有 case 仅测试 Software 模式 + Input/Output 切换，未覆盖中断路径与 Hardware 模式 | 全部由 TBD case 补齐；不阻塞既有 gpio_test 回归 |
| 32 路 GPIO 并发中断测试需 CPU 端 `cpu_intr[16]` 1 bit 聚合，TB 端需在中断处理后确保 `gpio_porta_int_clr` 清中断避免循环 | 测试代码需在 EOI 后给一定延迟再轮询 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── gpio/
│   └── gpio_test.c                  (F1, F2, F3, F4 (Software), F10 (PAD 连接 隐含)：既有)
├── gpio/                            (TBD 新增)
│   ├── gpio_reset_default.c         (F8：11 个寄存器复位值)
│   ├── gpio_reserved_gap.c          (F9：gap 地址)
│   ├── gpio_dir_independent.c       (F2：32 bit 独立方向)
│   ├── gpio_output_data_echo.c      (F3：Output 模式读回)
│   ├── gpio_intr_4reg.c             (F5：4 件套中断寄存器)
│   ├── gpio_intr_status_clear.c     (F6：rawintstatus / intstatus / clr)
│   └── gpio_intr_constraint.c       (F7：Output/Hardware 禁用中断)
└── addr_map/
    └── map_test.c                   (通用地址空间 read 0 检查，含 GPIO 区域 0x6001_8000~0x6001_BFFF)
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
- `dv/simulation/verif_env/soc/aou_top.v` — AOU 域顶层（含 `gpio0_sec_top` 实例化）
- `dv/simulation/verif_env/soc/apb1/` — APB1 总线侧 UVM test（含 GPIO monitor）

### 交叉参考文档
- `doc_summary/module_analysis/gpio_analysis.md` — GPIO 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/General-purpose_I_O_GPIO_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 1956-2094 行 — User Guide GPIO 章节原文
