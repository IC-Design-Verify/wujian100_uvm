# T-Head wujian100_open WDT (Watchdog, ×1) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Watchdog Timer (WDT)
  - 唯一 1 个实例，挂 APB0 P7，base `0x5000_8000`（外部地址空间 16 KB）
  - 6 个寄存器，offset `0x00` ~ `0x14`
  - 32-bit 计数器，喂狗 magic 值 `0x76`
  - 中断号 27（`WDT`，见 System Overview Table 1-4）

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/wdt.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `COUNTER_WIDTH` | `32` | WDT 内部计数器位宽 |
| `WDT_EN` 复位值 | `1'b0` | 复位后 WDT 禁用，需软件使能 |
| `RMOD` 复位值 | `1'b1` | 复位后默认 "先中断后复位" 模式 |
| `RPL[2:0]` 复位值 | `3'b000` | 默认复位脉宽 2 pclk |
| `TOP/TOP_INIT` | `4'b0000` | 复位后默认 timeout period（实际起算值 `0xFFFF`） |
| `Magic_value`（kick） | `8'h76` | 喂狗关键字；写入其他值无效 |
| `Magic_value_eoi`（in_clr） | 读清中断 | `WDT_int_clr` 读清中断但不喂狗 |
| `counter_init` | `32'h0000_FFFF` | `WDT_current_value` 复位值（counter 从 0xFFFF 起算） |

**关键配置含义**：
- WDT 一旦 `WDT_EN=1`，**仅可由系统复位清零**，软件不能 disable——这是 WDT 的核心安全特性。
- 喂狗 magic `0x76` 是防误触安全机制，软件必须严格按此值写；测试需覆盖"错值不生效"。
- WDT 复位会影响整芯片（`sys_rst_b` 接到 SoC reset 网络）；测试计划必须考虑复位后状态恢复（寄存器状态回到 §1.2 reset 值）。
- `RMOD=1`（复位默认）提供 2 次机会：第一次超时先中断，若不及时喂狗第二次超时再复位。

### 1.2 寄存器映射

| Offset | Name | Access | Reset | 说明 |
|--------|------|--------|-------|------|
| `0x00` | `WDT_CR` | R/W | `5'h02` | 控制寄存器：`RPL[4:2]`/`RMOD[1]`/`WDT_EN[0]` |
| `0x04` | `WDT_time_out` | R/W | `8'h00` | 超时范围：`TOP_INIT[7:4]`（首次 kick）/ `TOP[3:0]`（后续 kick） |
| `0x08` | `WDT_current_value` | RO | `32'h0000_FFFF` | 当前 32-bit 计数器值 |
| `0x0C` | `WDT_restart` | WO | `8'h00` | 喂狗：必须写 `0x76` 才生效；写其他值无效 |
| `0x10` | `WDT_int_status` | RO | `1'b0` | 中断状态：`bit[0]` 有效 |
| `0x14` | `WDT_int_clr` | RO | `1'b0` | 读清中断（不影响 counter） |

所有寄存器总线宽度 32-bit，仅有效位有意义；reserved 字段读返回 0。

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `apb0_sub_top.v`）

- **时钟**：`pclk`（APB 总线时钟）。
- **复位**：`prst_b`（高有效）；sec_top 内部反相为 `presetn` 给内部 `wdt` 模块。
- **总线挂载**：APB0 P7，base `0x5000_8000`；通过 `apb0_sub_top` → LS AHB → MAIN AHB 访问。
- **中断**：`intr` → SoC CLIC/VIC，中断号 27 = `WDT`。
- **系统复位**：`sys_rst_b`（高有效）→ SoC reset 网络（`retu_top` / `pdu_top` / `aou_top` 共同吸收）。
- **Trust**：`tipc_wdt_trust` / `pprot[2:0]` 端口预留但未对接 trust 逻辑。

### 1.4 关键 RTL 行为

1. **`WDT_EN` 锁定**（`wdt_regfile`）：`WDT_EN=1` 后只能由系统复位清零；写 0 无效（RTL 强制保留）。
2. **超时编码**（`WDT_time_out`）：`TOP[3:0]/TOP_INIT[3:0]` 各自 4-bit 编码 16 级 timeout range；user guide 给出 counter 从 `0xFFFF` 起算，实际 timeout period 长度由 TOP 决定。
3. **`RPL` 复位脉宽**（`WDT_CR[4:2]`）：8 档编码控制 sys_rst_b 拉低的 pclk 周期数（2/4/8/16/32/64/128/256）。
4. **`RMOD` 双模式**（`WDT_CR[1]`）：0 = 直接复位；1 = 先中断后复位（第二次超时再复位）。
5. **喂狗 magic**（`WDT_restart`）：写 `0x76` 重启 counter + 清中断；其它值无效。
6. **中断清零**（`WDT_int_clr`）：读该寄存器清 WDT 中断状态，**不喂狗**（与 `WDT_restart` 区别）。
7. **sys_rst 输出**（`wdt_sec_top`）：timeout 触发后按 RPL 配置的脉宽拉低 sys_rst_b。
8. **DFT**：`scan_mode` 输入控制扫描模式。

### 1.5 TB 检查架构

- **C 端检查**：`wdt_test.c`（既有）写 `WDT_time_out=0x10`、`WDT_CR=0x1d`（enable + RPL=256）、喂狗写 `0x78`（**注意：0x78 ≠ magic 0x76**，这是测试"错误值不生效"的反例）；随后 `while(1){}` 死循环等待 WDT 触发复位，由复位例程在 `0x20002000` 处放 magic `0x12345678` 作为"曾经被 WDT 复位过"的证据。
- **TB 端**：UVM `soc_top_for_c_case_test` 加载固件，WDT 复位会触发整个 SoC 复位——需在 TB 侧捕获复位事件并验证 `0x20002000` 被写入 magic。
- **关键时序**：WDT 复位需等待 `counter_init (0xFFFF)` 递减 + RMOD 决策 + RPL 脉宽，整个测试耗时长（与 TOP/TOP_INIT 编码相关）。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: WDT_EN 使能与锁定
**目标**：写 `WDT_CR[0]=1` 后，软件写 0 无效；只能由系统复位清零。
**已有 case**：`wdt_test.c`（既有）隐含覆盖（WDT_EN=1 后 while(1){} 等待复位）。
**检查**：C 端写 `WDT_CR=0x1`（enable），再写 `WDT_CR=0x0`（disable），读 `WDT_CR[0]` 应仍为 1；系统复位后读 `WDT_CR[0]==0`。
**缺口**：显式"使能后写 0 无效"测试**待新建 case（标记 TBD）**。

### F2: TOP / TOP_INIT 超时范围编码（16 级）
**目标**：`WDT_time_out[3:0] TOP` + `[7:4] TOP_INIT` 各自 4-bit 编码 16 级 timeout range；首次 kick 使用 TOP_INIT，后续 kick 使用 TOP。
**已有 case**：`wdt_test.c`（既有）配置 `WDT_time_out=0x10`（`TOP_INIT=1`、`TOP=0`）。
**检查**：C 端配置不同 TOP/TOP_INIT 编码（共 16×16 = 256 组合），校验实际 timeout period 与配置一致（TB 端测量 sys_rst_b 上升沿时间）。
**缺口**：TOP/TOP_INIT 全组合**待新建 case（标记 TBD）**，可能仅做抽样（如 0/8/15）。

### F3: RMOD 双模式（直接复位 vs 先中断后复位）
**目标**：`WDT_CR[1]=0` 直接复位；`WDT_CR[1]=1` 第一次超时中断、第二次超时复位（默认）。
**已有 case**：`wdt_test.c`（既有）配置 `WDT_CR=0x1d`（bit[1]=0 即 RMOD=0，直接复位模式）；隐含测试 RMOD=0。
**检查**：C 端 RMOD=1 时，第一次超时触发 `WDT_int_status=1` 但不复位；及时喂狗或读 `WDT_int_clr` 清中断；若再次超时则触发复位。
**缺口**：RMOD=1 中断先行模式**待新建 case（标记 TBD）**。

### F4: 喂狗 magic `0x76`（错误值不生效）
**目标**：写 `WDT_restart=0x76` 重启 counter 并清中断；写其他值（如 `0x78`、`0xFF`、`0x00`）无效（counter 继续递减）。
**已有 case**：`wdt_test.c`（既有）**故意写 `WDT_restart=0x78`**（错值），随后 `while(1){}` 等待 WDT 复位成功——隐含验证"错误值不生效"。
**检查**：C 端写 magic `0x76` 后读 `WDT_current_value` 应回到 `0xFFFF`；写 `0x78` / `0xFF` / `0x00` 后 counter 应继续递减（TB 端多次采样）。
**缺口**：多种错误值 + magic 对照**待新建 case（标记 TBD）**。

### F5: 计数器当前值读取
**目标**：`WDT_current_value` 反映 32-bit counter 当前值；WDT_EN=0 时该寄存器读 `0xFFFF`（reset 值），WDT_EN=1 后实时递减。
**已有 case**：`wdt_test.c`（既有）未显式读取 `WDT_current_value`。
**检查**：C 端使能 WDT 后连续读 `WDT_current_value`，验证值在递减；disable 后读应稳定在 `0xFFFF`。
**缺口**：**待新建 case（标记 TBD）**。

### F6: 中断产生 / 状态 / 清除
**目标**：超时（RMOD=1）触发 `WDT_int_status[0]=1`；写 `WDT_int_clr` 寄存器读清中断；不影响 counter。
**已有 case**：`wdt_test.c`（既有）未覆盖 RMOD=1 中断路径。
**检查**：C 端 RMOD=1 + WDT_EN=1，第一次超时后读 `WDT_int_status==1`；读 `WDT_int_clr` 后再读 `WDT_int_status==0`；counter 仍在递减（未喂狗）。
**缺口**：**待新建 case（标记 TBD）**。

### F7: RPL 系统复位脉宽
**目标**：`WDT_CR[4:2]` 8 档编码控制 sys_rst_b 拉低的 pclk 周期数（2/4/8/16/32/64/128/256）。
**已有 case**：`wdt_test.c`（既有）配置 `WDT_CR=0x1d`（RPL=111=256 pclk）但目的是确保被 oscclk 采样；未显式验证脉宽。
**检查**：TB 端 sys_rst_b 波形采样，验证拉低宽度与 RPL 配置一致（2/4/8/16/32/64/128/256 pclk cycles 8 档）。
**缺口**：**待新建 case（标记 TBD）**。

### F8: WDT 系统复位后整芯片状态恢复
**目标**：WDT 触发 sys_rst_b 后，SoC 寄存器状态回到 reset 值（具体哪些模块会被复位需要 SoC 复位架构支持）。
**已有 case**：`wdt_test.c`（既有）通过 `0x20002000` 处的 `0x12345678` magic 验证"曾经被复位过"——但要求 reset 流程/boot ROM 在复位后跳到该 magic 写入处。
**检查**：TB 端在 sys_rst_b 拉低/拉高后采样各模块 reset 值；CPU 端 boot ROM 跳到测试用例读取 magic。
**缺口**：**待新建 case（标记 TBD）**，依赖 boot ROM 复位流程配合。

### F9: 寄存器复位值
**目标**：复位后 6 个寄存器回到 §1.2 reset 值（`WDT_CR=5'h02`、`WDT_current_value=32'hFFFF` 等）。
**已有 case**：无（既有 `wdt_test.c` 未做复位后初始状态校验）。
**检查**：`prst_b` 释放后立即读 6 个寄存器，校验 reset 值。
**缺口**：**待新建 case（标记 TBD）**。

### F10: 中断号路由（VIC 中断号 27）
**目标**：`intr` 输出经 SoC VIC 路由到 `cpu_intr[27]` = `WDT`。
**已有 case**：无（C 端无法直接验证中断号，需 UVM 端 VIC monitor）。
**检查**：UVM 侧打开 `intr` monitor，验证 `cpu_intr[27]` 上升沿匹配 WDT 中断。
**缺口**：**待新建 case（标记 TBD）**，依赖 SoC VIC monitor。

### F11: Reserved 字段与只读行为
**目标**：reserved 字段读返回 0；写 reserved 字段被忽略；`WDT_current_value` / `WDT_int_status` / `WDT_int_clr` 写被忽略。
**已有 case**：无。
**检查**：C 端写全 1 到各寄存器后读 reserved 位应 == 0；写只读寄存器值不变。
**备注**：与 F9 复位值测试部分重叠，可合并。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `wdt_test`（既有 `c_case/wdt/wdt_test.c`） | `soc_top_for_c_case_test` | F1 (隐含), F2 (隐含, TOP=0), F3 (RMOD=0), F4 (写 0x78 不生效) | C 端基础 |
| 2 | `wdt_en_lock`（TBD） | `soc_top_for_c_case_test` | F1 (使能后写 0 无效) | C 端 |
| 3 | `wdt_top_matrix`（TBD） | `soc_top_for_c_case_test` + TB 测量 | F2 (TOP/TOP_INIT 抽样) | C 端 + TB 时序 |
| 4 | `wdt_rmod_interrupt`（TBD） | `soc_top_for_c_case_test` + TB | F3 (RMOD=1 中断先行), F6 (中断 status/clear) | C 端 |
| 5 | `wdt_magic_kick`（TBD） | `soc_top_for_c_case_test` | F4 (magic 对比), F5 (counter 当前值) | C 端 |
| 6 | `wdt_rpl_pulse`（TBD） | UVM 侧 | F7 (RPL 8 档脉宽) | UVM sys_rst_b monitor |
| 7 | `wdt_chip_reset_recovery`（TBD） | UVM 侧 + boot ROM | F8 (复位后状态恢复) | UVM 复位事件捕获 |
| 8 | `wdt_reset_default`（TBD） | `soc_top_for_c_case_test` | F9 | C 端复位检查 |
| 9 | `wdt_vic_route`（TBD） | UVM 侧 | F10 (cpu_intr[27] 路由) | UVM 中断监测 |
| 10 | `wdt_reserved_ro`（TBD） | `soc_top_for_c_case_test` | F11 | C 端寄存器边界 |

### 功能覆盖矩阵

| Feature | wdt_test | wdt_en_lock | wdt_top_matrix | wdt_rmod_interrupt | wdt_magic_kick | wdt_rpl_pulse | wdt_chip_reset_recovery | wdt_reset_default | wdt_vic_route | wdt_reserved_ro |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: WDT_EN 锁定 | ✓ (隐含) | ✓ | - | - | - | - | ✓ (复位恢复) | ✓ | - | ✓ |
| F2: TOP/TOP_INIT 编码 | ✓ (TOP=0) | - | ✓ (抽样) | - | - | - | - | - | - | - |
| F3: RMOD 双模式 | ✓ (RMOD=0) | - | - | ✓ (RMOD=1) | - | - | - | - | - | - |
| F4: 喂狗 magic 0x76 | ✓ (写 0x78) | - | - | - | ✓ (对照) | - | - | - | - | - |
| F5: counter 当前值 | - | - | - | - | ✓ | - | - | ✓ | - | - |
| F6: 中断 status/clear | - | - | - | ✓ | - | - | - | - | - | - |
| F7: RPL 复位脉宽 | ✓ (RPL=256 隐含) | - | - | - | - | ✓ (×8) | ✓ | - | - | - |
| F8: 复位后状态恢复 | ✓ (magic) | - | - | - | - | - | ✓ | ✓ | - | - |
| F9: 寄存器复位值 | - | - | - | - | - | - | - | ✓ | - | ✓ |
| F10: VIC 中断号 27 | - | - | - | - | - | - | - | - | ✓ | - |
| F11: Reserved/只读 | - | - | - | - | - | - | - | ✓ (部分) | - | ✓ |

> 矩阵用 ✓/- 标记。"TBD" 表示待新建 case，不阻塞既有 wdt_test 通过但属于覆盖缺口。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test          (UVM 序列基线)
  └── soc_top_for_c_case_test     (运行 C 端测试用例，含 wdt_test)
```

既有 `wdt_test` 通过 `soc_top_for_c_case_test` 加载固件运行。UVM 侧新增 WDT 专用序列（`wdt_rpl_pulse_seq` / `wdt_chip_reset_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，WDT 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/wdt/wdt_test.c` 决定具体行为。后续 TBD 用例沿用同一入口，通过修改 `c_case/wdt/` 下不同 .c 文件选择。

> **待确认**：项目是否计划引入独立 WDT uvm_test 子类。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `wdt_test.c` 的 `printf("\nwdt reset test successfully\n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断
- **特殊约定**：WDT 测试额外使用 `0x20002000` 作为"被 WDT 复位过"的 magic 地址（`wdt_test.c` 写 `0x12345678`），由复位例程/boot ROM 检测。

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **WDT 复位事件 monitor（TBD）**：TB 侧需采样 `sys_rst_b` 信号，捕获复位事件；与 `0x20002000` 处 magic 联合验证。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **VIC monitor（TBD）**：未来新增 UVM 侧 case 时，需在 `soc_top_env` 内增加 cpu_intr[27] 采样 monitor。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `wdt_test`（既有） | `0x20002000 == 0x12345678`（曾被 WDT 复位）+ `printf("\nwdt reset test successfully\n")` + `cpu_flag_addr=0x2002` + TB `UVM_CASE_PASS` |
| `wdt_en_lock` (TBD) | WDT_EN=1 后写 0 无效，读 WDT_CR[0] 仍 == 1 |
| `wdt_top_matrix` (TBD) | TB 测量 sys_rst_b 上升沿时间与 TOP/TOP_INIT 编码一致 |
| `wdt_rmod_interrupt` (TBD) | RMOD=1 第一次超时触发中断 + 读 int_clr 清中断；第二次超时触发复位 |
| `wdt_magic_kick` (TBD) | 写 `0x76` 重启 counter；写 `0x78/0xFF/0x00` 不重启 |
| `wdt_rpl_pulse` (TBD) | sys_rst_b 拉低宽度 8 档（2/4/8/16/32/64/128/256 pclk）与 RPL 配置一致 |
| `wdt_chip_reset_recovery` (TBD) | WDT 复位后各模块寄存器回到 reset 值 |
| `wdt_reset_default` (TBD) | 复位后 6 个寄存器值与 §1.2 reset 表一致 |
| `wdt_vic_route` (TBD) | WDT 中断发生时 `cpu_intr[27]` 上升沿匹配 |
| `wdt_reserved_ro` (TBD) | reserved 位读 0、只读寄存器写忽略 |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 wdt_test                     (~10 min)  既有 C 端基本功能（含 WDT 复位等待时间）
3. 仿真 wdt_en_lock                  (~10 min)  TBD case 1（验证使能后写 0 无效）
4. 仿真 wdt_top_matrix               (~30 min)  TBD case 2（TOP/TOP_INIT 抽样 + TB 时序测量）
5. 仿真 wdt_rmod_interrupt           (~15 min)  TBD case 3（RMOD=1 中断先行）
6. 仿真 wdt_magic_kick               (~10 min)  TBD case 4（喂狗 magic 对比）
7. 仿真 wdt_reset_default            (~5 min)   TBD case 5（复位值）
8. 仿真 wdt_reserved_ro              (~5 min)   TBD case 6（reserved/只读）
9. 仿真 wdt_rpl_pulse                (~30 min)  TBD UVM case 7（RPL 8 档脉宽）
10. 仿真 wdt_chip_reset_recovery     (~15 min)  TBD UVM case 8（复位后状态恢复）
11. 仿真 wdt_vic_route               (~10 min)  TBD UVM case 9（VIC 中断号 27）
```

预估总时间：~140-170 min（既有 case ~10 min + 9 个 TBD case ~130-160 min）

> **测试计划考量**：WDT 测试因涉及系统复位，每次复位后需要等待 SoC 重新启动（boot ROM）才能进入下一个测试，因此仿真时间长；TOP/TOP_INIT 编码组合（16 级）也需要足够采样时间。

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| WDT 触发系统复位影响整芯片，测试间可能互相干扰 | 每个测试需独立的 `prst_b` 复位重置；TB 侧用 `@(posedge sys_rst_b)` 同步事件 |
| TOP/TOP_INIT 16 级全组合 = 256 种 timeout，测试时间过长 | 抽样测试：选 0/8/15 三档 + 默认 0，覆盖边界和中间值 |
| RPL 8 档脉宽验证需要 TB 侧高精度时序采样 | UVM 侧 `wdt_rpl_pulse` (TBD) 需在 SoC 复位事件 monitor 中加 sys_rst_b 边沿检测 |
| WDT 复位后 CPU 重新启动依赖 boot ROM / 复位向量约定 | `wdt_chip_reset_recovery` (TBD) 需要 boot ROM 配合在复位后跳转到测试入口；当前 SoC boot ROM 行为待确认 |
| 中断号 27 = `WDT` 来自 System Overview Table 1-4；具体行号 / 编号以文档最新版本为准 | TB 侧硬编码中断号 27；后续以 doc_review 修复后版本对齐 |
| `wdt_test.c` 既有 case 故意写 `0x78`（错误 magic），依赖 WDT 复位作为 PASS——若 WDT 失效无法复位则测试会卡死 | TB 侧需加 watchdog 仿真超时保护（`$finish` after N pclk）；UVM_ERROR 监控 |
| RMOD=1 中断先行模式需要精确时序配合（第二次超时前喂狗或清中断） | 测试代码使用 cycle 计数器循环喂狗；TB 端超时监控 |
| WDT_EN 一旦置 1 软件不可 disable——测试顺序需注意 | 复位值/边界测试必须在 WDT_EN=0 阶段完成；使能后只能测到下次复位 |
| `WDT_current_value` reset = `32'h0000_FFFF`（其他寄存器 `0x0`） | reset 值测试需特别注意此差异；TB 端不要误判为异常 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── wdt/
│   └── wdt_test.c                 (F1, F2, F3, F4, F7, F8：既有，故意写错 magic 0x78 触发复位)
├── wdt/                           (TBD 新增)
│   ├── wdt_en_lock.c              (F1)
│   ├── wdt_top_matrix.c           (F2)
│   ├── wdt_rmod_interrupt.c       (F3, F6)
│   ├── wdt_magic_kick.c           (F4, F5)
│   ├── wdt_reset_default.c        (F9)
│   └── wdt_reserved_ro.c          (F11)
└── addr_map/
    └── map_test.c                 (通用地址空间 read 0 检查，含 WDT 区域 0x50008000~0x5000BFFF)
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
- `dv/simulation/verif_env/soc/apb0/` — APB0 总线侧 UVM test（含 WDT monitor）

### 交叉参考文档
- `doc_summary/module_analysis/wdt_analysis.md` — WDT 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/Watchdog_WDT_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 1115-1190 行 — User Guide WDT 章节原文
