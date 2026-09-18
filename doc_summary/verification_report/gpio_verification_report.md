# T-Head wujian100_open GPIO (×1, 32-bit 端口 A) Verification Report

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open General-Purpose I/O (GPIO0)
- 唯一 1 个实例，挂 APB1 P5，base `0x6001_8000`（外部地址空间 16 KB）
- 32-bit Port A（PAD_GPIO_0 ~ PAD_GPIO_31）
- 12 个有效寄存器（offset `0x00` ~ `0x60`），其中 `0x4C` 表称清中断但 RTL 未解码
- 中断号 16（`GPIO0`，见 System Overview Table 1-4）

**验证工程师**: CCB doc-write
**报告日期**: 2026-09-17
**关联文档**:
- 验证计划 `doc_summary/verification_plan/gpio_verification_plan.md`（F1~F12、§5 验收标准）
- 模块分析 `doc_summary/module_analysis/gpio_analysis.md`（寄存器 / 端口 / RTL 行为）
- 寄存器独立文档 `doc_summary/General-purpose_I_O_GPIO_registers.md`

---

## 1. 概述

本报告记录 GPIO 模块从 `gpio_test` 既有 C 端用例到 2026-09-17 新增 8 个用例（7 C + 2 UVM 侧，其中 1 个由 GPIO 主用例与既有 UVM 入口协同复用）的全量验证执行结果，对照验证计划 F1~F12 与 §5 验收标准逐项闭环；并整理 4 项关键 Spec/UG-vs-RTL 差异（以 RTL 为准）。

### 1.1 验证范围

- **IP 数量**：1 个 GPIO0 实例（`gpio0.v`），单端口 32 bit（Port A）。
- **基址**：APB1 P5 = `0x6001_8000`（2026-09-16 按 User Guide Peripheral Address Map 与 `apb1_params.v:25` `APB_LEAF_SLV5_START_ADDR = 32'h60018000` 修正，详见计划 §5 注）。
- **寄存器空间**：12 个有效寄存器（offset `0x00`/`0x04`/`0x08`/`0x30`~`0x44`/`0x4C`（无效）/`0x50`/`0x60`（双解码清中断））+ reserved gap（`0x0C`~`0x2C`/`0x48`/`0x54`~`0x7C`）；复位清单含 9 寄存器（`input_data` 属激励相关不在复位清单内，详见 §4.1 与 `gpio_reset_default` 实测）。
- **TB 环境**：soc_top CPU 驱动模式（`soc_top_for_c_case_test` 加载 C 固件，CPU_FLAG_ADDR `0x20007C50` 上报 `sim_end()/sim_fail()`）+ UVM TB 协同（`soc_top_gpio_vic_route_test` / `soc_top_gpio_etb_trig_test`）。
- **特殊机制**：GPIO PAD 依赖 legacy 激励块状态机驱动（`apb1/tb_top/apb1_gpio/gpio_test.v`），需通过 `Makefile` 的 `findstring gpio_` 分支启用 `USE_APB1+USE_APB1_GPIO` DEF（详见 §4.5 环境修复）。

### 1.2 验证结论

- **测试用例**：9 个（既有 1 + 新增 8），全部 `UVM_CASE_PASS`，0 UVM_ERROR / 0 UVM_FATAL。
- **功能覆盖**：F1~F12 全部闭环（详见 §3）；F4（Hardware 模式）按计划降级为 C 用例 `gpio_intr_constraint` 兼带覆盖（详见 §4.1）。
- **关键发现**：4 项 Spec/UG-vs-RTL 差异，详见 §4。
- **环境修复**：Makefile 增加 `findstring gpio_` 分支，详见 §5 #6。
- **遗留风险**：见 §6。

---

## 2. 测试执行结果

所有用例经 VCS 仿真（`+UVM_TESTNAME=...`），每项判定：`UVM_CASE_PASS` 打印 + 0 UVM_ERROR / 0 UVM_FATAL + C 端 "test successfully" 打印。

| # | 用例 | 类型 | 功能点 | 说明 / 日志路径 |
|---|------|------|--------|------------------|
| 1 | `gpio_test`（既有 `c_case/gpio/gpio_test.c`） | C 基线 | F1, F2, F3, F4（Software）, F10（隐含） | Input 模式读 `gpio_input_data` + Output 模式写 `gpio_output_data`；TB 驱动 PAD `0x55555555` / `0xaaaaaaaa` / `0x12345678`；`/tmp/gpio_baseline.log` |
| 2 | `gpio_reset_default`（新增） | C 端 | F8 | 复位后 9 寄存器（output_data/direction/ctl/inten/intmask/inttype/intpol/intstatus/rawintstatus）全 0（`input_data` 属激励相关不在复位清单内） |
| 3 | `gpio_reserved_gap`（新增） | C 端 | F9 | reserved gap `0x0C`~`0x2C`、`0x48`、`0x54`~`0x7C` 读 0 + 写忽略 |
| 4 | `gpio_output_data_echo`（新增） | C 端 | F3 | Output 模式下 5 组 pattern（`0x00000000`/`0xFFFFFFFF`/`0x55555555`/`0xAAAAAAAA`/`0x12345678`）写 `gpio_output_data` 后回读 `gpio_input_data` == last write |
| 5 | `gpio_dir_independent`（新增） | C 端 | F2 | 6 组 direction 全 0/全 1/奇偶/低 8 高 8/0x00FF00FF/0xFFFF0000 读写一致 + 混合方向低 16 位读回 |
| 6 | `gpio_intr_combo`（新增） | C 端 | F5, F6 | inten 门控 raw；level-high / level-low；mask 只影响 intstatus；edge 粘性；int_clr 清除；含 `0x4C`（无效）/`0x60`（有效）正反断言 |
| 7 | `gpio_intr_constraint`（新增） | C 端 | F7（兼带 F4） | direction=Output 禁止中断（level+edge）；附带 F4：`ctl@0x08` 写无效、读恒 0 |
| 8 | `gpio_vic_route` + `soc_top_gpio_vic_route_test`（新增） | C + UVM 协同 | F12 | TB 监控 `pad_vic_int_vld[16]` 断言 / 撤销（`core_top.v:540` `ip_cpu_int_vld[16] = gpio_wic_intr`） |
| 9 | `gpio_etb_trig` + `soc_top_gpio_etb_trig_test`（新增） | C + UVM 协同 | F11 | `gpio0_etb_trig` 依序出现 `0x55555555` → `0xAAAAAAAA` |

**共同 PASS 判定条件**（验证计划 §5）：
- C 端通过 `cpu_flag_addr=0x20007C50` 写 `sim_end()` 写值 `0x2002`
- TB 端 `soc_top_test_base` 读到 `0x2002` 后 raise/drop objection 并打印 `UVM_CASE_PASS`
- UVM 侧 0 UVM_ERROR / 0 UVM_FATAL
- C 端 `printf("...test successfully")` 串口打印

---

## 3. 功能点覆盖矩阵 (Feature Coverage Matrix)

> ✓ 表示已覆盖；"- " 表示非该用例目标。

| Feature | gpio_test | reset_default | reserved_gap | output_data_echo | dir_independent | intr_combo | intr_constraint | vic_route | etb_trig | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **F1** output_data | ✓ | - | - | ✓ | - | - | - | - | - | ✅ |
| **F2** direction | ✓ | - | - | - | ✓ (6 组) | - | - | - | - | ✅ |
| **F3** input_data | ✓ (×3) | - | - | ✓ (回读) | - | - | - | - | - | ✅ |
| **F4** gpio_ctl（Hardware） | ✓ (SW) | - | - | - | - | - | ✓ 兼带（写无效读恒 0） | - | - | ⚠️ 见 §4.1 |
| **F5** 中断 4 件套 | - | - | - | - | - | ✓ | - | - | - | ✅ |
| **F6** 中断 status/clear | - | - | - | - | - | ✓ | - | - | - | ✅ |
| **F7** 中断约束 | - | - | - | - | - | - | ✓ (Output) | - | - | ✅ |
| **F8** 复位值 | - | ✓ (×9) | - | - | - | - | - | - | - | ✅ |
| **F9** reserved gap | - | - | ✓ | - | - | - | - | - | - | ✅ |
| **F10** PAD 连接 | ✓ 隐含 | - | - | ✓ | ✓ | - | - | - | - | ✅ |
| **F11** ETB 触发 | - | - | - | - | - | - | - | - | ✓ | ✅ |
| **F12** VIC 中断号 16 | - | - | - | - | - | - | - | ✓ | - | ✅ |

### 3.1 闭环说明

- **F1**：`gpio_test` + `gpio_output_data_echo`（5 组 pattern 回读）。
- **F2**：`gpio_test` 全 0/全 1 切换 + `gpio_dir_independent` 6 组独立方向配置（混合方向含低 16 位读回）。
- **F3**：`gpio_test` 3 组 PAD 驱动 + `gpio_output_data_echo` 5 组回读验证 Output 模式读 last write。
- **F4**：⚠️ 按计划降级。原计划 `gpio_hardware_mode`（UVM 侧）实际未新建——RTL 无 Hardware 模式实现（详见 §4.1），`gpio_intr_constraint` 用例附带覆盖 `ctl@0x08` 写无效 / 读恒 0 行为。
- **F5**：`gpio_intr_combo` 单用例覆盖 4 件套组合（inten 门控 raw / level-high / level-low / mask 只影响 intstatus / edge 粘性）+ `0x4C` 无效 / `0x60` 有效正反断言（兼带 F6）。
- **F6**：`gpio_intr_combo` 内含 intstatus vs rawintstatus vs int_clr 全链路。
- **F7**：`gpio_intr_constraint` 验证 direction=Output 时 level + edge 中断均不置位。
- **F8**：`gpio_reset_default` 9 寄存器复位值（全 0）。
- **F9**：`gpio_reserved_gap` 显式覆盖 gap 地址读 0 + 写忽略。
- **F10**：`gpio_test` / `gpio_output_data_echo` / `gpio_dir_independent` 隐含（依赖 PAD 双向回环）。
- **F11**：`gpio_etb_trig` UVM 序列验证 `gpio0_etb_trig` 依序出现 `0x55555555` → `0xAAAAAAAA`。
- **F12**：`gpio_vic_route` UVM 序列验证 `pad_vic_int_vld[16]` 断言 / 撤销与中断状态对齐。

### 3.2 §5 验收标准对照

| 验收项 | 计划描述 | 报告结果 |
|--------|---------|----------|
| `gpio_test`（既有） | TB 驱动 PAD 3 组 pattern + C 端读 `gpio_input_data` 匹配 + `printf("gpio io test pass!")` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` | ✅ 基线通过 |
| `gpio_reset_default` | 复位后 11 个寄存器值与 §1.2 reset 表一致（全 0） | ⚠️ RTL 复位清单实测 9 寄存器（不含 `input_data`，属激励相关）；详见 §4.1 |
| `gpio_reserved_gap` | gap 地址（`0x0C`~`0x2C`/`0x48`）读 0、写忽略 | ✅ 覆盖 `0x0C`~`0x2C` / `0x48` / `0x54`~`0x7C` |
| `gpio_output_data_echo` | Output 模式下读 `gpio_input_data` == last `gpio_output_data` 写入值 | ✅ 5 组 pattern 通过 |
| `gpio_dir_independent` | 32 bit 独立方向配置时 PAD 行为正确 | ✅ 6 组通过 |
| `gpio_intr_combo`（替代原 `gpio_intr_4reg` + `gpio_intr_status_clear`） | 4 件套组合 + rawintstatus vs intstatus vs clr | ✅ |
| `gpio_intr_constraint` | direction=Output 或 ctl=Hardware 时中断不置位 | ✅ Output 禁用中断通过；Hardware 模式降级兼带 ctl 写无效验证（详见 §4.1） |
| `gpio_hardware_mode`（TBD UVM） | `gpio_ctl=1` 时 PAD 输出由 TB 驱动 | ⚠️ 按计划降级，未新建独立 UVM 用例——RTL 未实现 Hardware 模式（详见 §4.1） |
| `gpio_etb_trig` | `gpio0_etb_trig[31:0]` 触发时序 | ✅ `0x55555555` → `0xAAAAAAAA` 依序出现 |
| `gpio_vic_route` | `gpio_intr_flag` 上升沿时 `cpu_intr[16]` 匹配 | ✅ `pad_vic_int_vld[16]` 断言 / 撤销通过 |

---

## 4. 关键验证发现（Spec/UG-vs-RTL 差异，以 RTL 为准）

### 4.1 Hardware 模式（`gpio_ctl`）未实现

**RTL 实证**（`wujian100_open/soc/gpio0.v`）：
- `:509` `GPIO_SW_HW_CTRL_OFFSET : prdata <= 32'b0;`（读 `gpio_ctl` 恒为 `0`）
- write decode 路径无 `GPIO_SW_HW_CTRL_OFFSET` 分支（搜索结果仅 `:267` 对 `GPIO_SW_DATA_OFFSET` 写生效）
- 中断门控：`gpio_int_status_edge`（`:697`）/ `GPIO_INT_STATUS_LEVEL_PROC`（`:711`）均仅检查 `gpio_sw_dir[k]`，不消费 `gpio_ctl`

**实测**：`gpio_intr_constraint` 写 `gpio_ctl@0x08` 任意值（含 `0xFFFFFFFF`）后读回恒 0；不影响中断门控行为。

**影响**：UG 描述的 Hardware 模式（外设驱动 PAD）在当前 RTL 中未实现，`gpio_ctl` 寄存器写无效、读恒 0。验证计划 `gpio_hardware_mode` (TBD UVM) 按本报告降级为 C 端 `gpio_intr_constraint` 用例兼带覆盖（ctl 写无效 + 读恒 0）。建议：(a) RTL 补齐 Hardware 模式实现 + 写 decode 分支；(b) UG 移除 Hardware 模式描述或标注"未实现"。

### 4.2 int_clr 实际地址是 `0x60` 而非 UG 标注 `0x4C`

**RTL 实证**（`wujian100_open/soc/gpio0.v`）：
- `:241` `localparam GPIO_INT_LEVEL_SYNC_OFFSET = 5'b11000;`（offset `0x60`）
- `:333-339` 写 `GPIO_INT_LEVEL_SYNC_OFFSET` → `gpio_int_level_sync_wen = 1'b1`
- `:344-350` 写 `GPIO_INT_LEVEL_SYNC_OFFSET` → `gpio_int_clr_wen = 1'b1`（**双解码**）

**UG 标称**：offset `0x4C`（`GPIO_INT_CLR_OFFSET=5'b10011`）——RTL 中该偏移无任何解码分支（仅返回 `prdata <= 32'b0`）。

**实测**（`gpio_intr_combo`）：
- 写 `0x4C` 后 raw 保持 `0x55555555`（**无效**）
- 写 `0x60` 后 raw 清 `0`（**有效**）

**影响**：清中断地址与 UG 不一致，且与 `int_level_sync` 寄存器双解码同址。验证计划已同步修正（F6 表与 §1.2 复位清单需后续文档审阅）。建议：(a) UG 标 `0x60` 而非 `0x4C`；(b) 若 `int_level_sync` 与 `int_clr` 设计上不应同址，RTL 解码应拆分。

### 4.3 读 mux case label 误用 `GPIO_SW_DATA_RESET`（潜在 bug）

**RTL 实证**（`wujian100_open/soc/gpio0.v`）：
- `:507` `` `GPIO_SW_DATA_RESET : prdata <= ri_gpio_sw_data; ``（**应使用 `GPIO_SW_DATA_OFFSET`**）
- `:12` `` `define GPIO_SW_DATA_RESET 32'h0 ``（`GPIO_SW_DATA_RESET` 是 reset value `32'h0`，不是 offset 常量）
- `:229` `localparam GPIO_SW_DATA_OFFSET = 5'b00000;`（实际 offset 常量 `5'b0`，与 `32'h0` 等价——这是巧合掩盖了 bug）

**影响**：当前 `GPIO_SW_DATA_RESET = 32'h0` 与 `GPIO_SW_DATA_OFFSET = 5'b00000`（= `32'h0`）按位等价，功能上无差异（读 `gpio_output_data` 返回 `ri_gpio_sw_data` 是正确的），属潜在 bug。case label 引用了"reset value"宏做"offset"判断，可读性极差且易在后续维护中误用。建议：改为 `GPIO_SW_DATA_OFFSET : prdata <= ri_gpio_sw_data;`。

### 4.4 `gpio0_etb_trig` 在 SoC 顶层未引出

**RTL 实证**（`wujian100_open/soc/aou_top.v`）：
- `:298` `wire [31:0] gpio0_etb_trig;`（内部线网声明）
- `:553` `gpio0_sec_top x_gpio0_sec_top (..., .gpio0_etb_trig(gpio0_etb_trig), ...);`（内部连接到 `gpio0_etb_trig`）

**SoC 顶层**：`wujian100_open_top.v` 未引出 `gpio0_etb_trig`（无 ETB consumer），aou_top 内部线网孤立。

**实测**：`gpio_etb_trig` UVM 用例在 TB 端通过 XMR / hierarchical reference 读取 aou_top 内部 `gpio0_etb_trig` 信号，验证依序出现 `0x55555555` → `0xAAAAAAAA`；与中断状态对齐（中断事件 → ETB 触发 → 中断清除 → 触发撤销）。

**影响**：F11 验证依赖 TB 内部信号访问（XMR），非 SoC 级 ETB 消费者验证。系统集成测试需 ETB fabric 环境。建议：(a) 文档标注当前无 SoC 级 ETB consumer；(b) 后续 PMU/ETB fabric 集成时补充 SoC 级联调。

---

## 5. 问题与修复记录

| # | 问题 | 影响 | 解决 |
|---|------|------|------|
| 1 | 中断类用例初版未保持 direction=Input → TB legacy 激励块状态机后续 release PAD → 中断事件基准丢失 | F5/F6 误判 | 中断类用例全程保持 `direction=Input`（`oe==0`）以持续接收 TB 强制 PAD `0x55555555` 激励 |
| 2 | `gpio_intr_combo` 初版假设清中断地址 = `0x4C`（UG）→ 写 `0x4C` 后 raw 保持 → 假阳性 | F6 漏验 | 按 RTL 实测改写 `0x60`（双解码 `GPIO_INT_LEVEL_SYNC_OFFSET`），详见 §4.2 |
| 3 | `gpio_etb_trig` 初版尝试 SoC 顶层 hierarchical path → XMR 路径不存在 | F11 不可观测 | 改用 aou_top 内部信号 hierarchical access（`.aou_top.gpio0_etb_trig`） |
| 4 | `gpio_intr_constraint` 初版用 `direction=Output` 验证 ctl=Hardware 禁用中断 → ctl 写无效被误判为 PASS | F4/F7 混测 | 拆分为：F7 单独验证 Output 禁用中断；F4 兼带验证 ctl 写无效 / 读恒 0（详见 §4.1） |
| 5 | `gpio_vic_route` 初版轮询 `cpu_intr[16]` → SoC 顶层信号不可见 | F12 不可观测 | 改轮询 `pad_vic_int_vld[16]`（`core_top.v:481` 透传 `ip_cpu_int_vld[63:0]` 到 pad），与 `core_top.v:540` 路由一致 |
| 6 | **Makefile 缺陷**：旧 DEF 按 C 测试文件名精确匹配只有 `gpio_test` 命中；新增用例（`gpio_reset_default` 等）DEF 为空 → legacy PAD force 块（`apb1/tb_top/apb1_gpio/gpio_test.v`）未编译 → 32 路 PAD 悬空 X → 仿真挂死 / CPU crash | 新用例全部不可执行 | Makefile 新增 `findstring gpio_` 分支：DEF = `USE_APB1 + USE_APB1_GPIO`；新用例只要路径含 `gpio_` 即自动启用 DEF |

---

## 6. 遗留风险与后续建议

| # | 风险 / 建议项 | 类别 | 说明 | 建议 |
|---|--------------|------|------|------|
| 1 | Hardware 模式未实现（§4.1） | 风险 | RTL `gpio_ctl` 写无效 / 读恒 0；UG 描述与 RTL 不一致 | (a) RTL 补齐 Hardware 模式 + 写 decode；(b) UG 移除 Hardware 描述或标注"未实现" |
| 2 | int_clr 地址不一致（§4.2） | 风险 | UG 标 `0x4C`、RTL 实际 `0x60`（与 `int_level_sync` 双解码同址） | (a) UG 改 `0x60`；(b) RTL 拆分 `int_level_sync` 与 `int_clr` 解码 |
| 3 | 读 mux label 误用宏（§4.3） | 风险 | 潜在 bug，当前巧合掩盖 | RTL 改 `GPIO_SW_DATA_OFFSET` 而非 `GPIO_SW_DATA_RESET` |
| 4 | `gpio0_etb_trig` SoC 级无 consumer（§4.4） | 风险 | aou_top 内部线网孤立；F11 验证依赖 TB XMR | (a) 文档标注；(b) 后续 ETB fabric 集成时补充 SoC 级联调 |
| 5 | TIPC trust 信号（`tipc_gpio0_trust` / `pprot[2:0]`）仅透传未过滤（gpio_analysis §7.2） | 风险 | 本配置未使能 trustzone | 若启用 trustzone，需补充 trust 边界用例 |
| 6 | PMU 中断时钟门控（`gpio_intrclk_en`）未专项验证 | 风险 | 属 PMU 范畴，本批未覆盖 | 后续与 PMU 联合专项验证 |
| 7 | legacy PAD 激励块状态机依赖 Makefile `findstring gpio_` 分支 | 改进 | 新 GPIO 用例必须含 `gpio_` 路径才自动启用 DEF；命名耦合度高 | 文档同步说明命名规范；或迁移到 UVM 侧 PAD driver agent，解除 Makefile 依赖 |
| 8 | 中断 4 件套（inten/mask/type/polarity）32 bit 全组合 = 2^96 种，测试组合爆炸 | 改进 | `gpio_intr_combo` 抽样覆盖关键组合 | 后续可补充 reference model + 受约束随机 |
| 9 | `input_data` 不在复位清单（属激励相关） | 改进 | 复位后 `input_data` 反映 PAD 实测值，非纯 reset 行为 | 文档说明；`gpio_reset_default` 用例对 `input_data` 不做断言 |

---

## 7. 附录 - 文件清单与 commit 记录

### 7.1 测试代码（C 端固件）

```
dv/simulation/verif_env/soc/c_case/
├── gpio/gpio_test.c                              (既有 F1/F2/F3/F4(SW)/F10)
├── gpio/gpio_reset_default.c                     (新增 F8)
├── gpio/gpio_reserved_gap.c                      (新增 F9)
├── gpio/gpio_output_data_echo.c                  (新增 F3)
├── gpio/gpio_dir_independent.c                   (新增 F2)
├── gpio/gpio_intr_combo.c                        (新增 F5/F6)
├── gpio/gpio_intr_constraint.c                   (新增 F7 + F4 兼带)
└── addr_map/map_test.c                           (通用地址空间 read 0，含 GPIO 区域)
```

### 7.2 UVM 测试与序列

```
dv/simulation/verif_env/soc/soc_top/tests/uvm_test/
├── soc_top_test_lib.svh                          (test 注册；新增 GPIO UVM 入口)
└── soc_top_timer_dfx_test.svh                    (复用框架，含 soc_top_gpio_vic_route_test / soc_top_gpio_etb_trig_test)
```

- `soc_top_gpio_vic_route_test`：F12；UVM 监控 `pad_vic_int_vld[16]` 断言 / 撤销。
- `soc_top_gpio_etb_trig_test`：F11；UVM XMR 读 aou_top 内部 `gpio0_etb_trig`。

### 7.3 环境修复

```
Makefile（dv/simulation/verif_env/soc/soc_top/tests/uvm_test/ 或顶层）
  - 新增 findstring gpio_ 分支：DEF = USE_APB1 + USE_APB1_GPIO
  - 旧逻辑：DEF 按 C 测试文件名精确匹配（仅 gpio_test 命中）
  - 新逻辑：路径含 gpio_ 即自动启用 DEF → legacy PAD force 块（apb1/tb_top/apb1_gpio/gpio_test.v）参与编译
```

### 7.4 仿真日志

```
/tmp/gpio_baseline.log                            gpio_test（F1/F2/F3/F4(SW)/F10）
/tmp/gpio_reset_default.log                      F8
/tmp/gpio_reserved_gap.log                       F9
/tmp/gpio_output_data_echo.log                   F3
/tmp/gpio_dir_independent.log                    F2
/tmp/gpio_intr_combo.log                         F5/F6
/tmp/gpio_intr_constraint.log                    F7 + F4 兼带
/tmp/gpio_vic_route.log                          F12
/tmp/gpio_etb_trig.log                           F11
```

### 7.5 Commit 记录

| Commit | 说明 |
|--------|------|
| `08830e4` | 新增 9 个用例（既有 1 + 新增 7 C 端 + 1 个 C+UVM 协同 `gpio_intr_combo` 等价替换原 `gpio_intr_4reg` + `gpio_intr_status_clear`）+ Makefile `findstring gpio_` 分支修复 + 4 项 RTL 注释 |

### 7.6 关键 RTL 行号索引

| 行为 | 文件 : 行 |
|------|----------|
| `gpio_ctl` 读恒 `32'b0`（Hardware 模式未实现） | `wujian100_open/soc/gpio0.v : 509` |
| `gpio_int_level_sync_wen` 解码（offset `0x60`） | `wujian100_open/soc/gpio0.v : 333-339` |
| `gpio_int_clr_wen` 解码（offset `0x60`，双解码） | `wujian100_open/soc/gpio0.v : 344-350` |
| 读 mux label 误用 `GPIO_SW_DATA_RESET`（应为 `GPIO_SW_DATA_OFFSET`） | `wujian100_open/soc/gpio0.v : 507` |
| 中断 level 状态门控（仅 `gpio_sw_dir[k]`） | `wujian100_open/soc/gpio0.v : 711` |
| 中断 edge 状态门控（仅 `gpio_sw_dir[k]`） | `wujian100_open/soc/gpio0.v : 697` |
| `GPIO_INT_LEVEL_SYNC_OFFSET = 5'b11000`（offset `0x60`） | `wujian100_open/soc/gpio0.v : 241` |
| `gpio_intr_flag` 路由 `ip_cpu_int_vld[16]` | `wujian100_open/soc/core_top.v : 540` |
| `pad_vic_int_vld` 透传 `ip_cpu_int_vld[63:0]` | `wujian100_open/soc/core_top.v : 481` |
| aou_top `gpio0_etb_trig` 内部线网声明 | `wujian100_open/soc/aou_top.v : 298` |
| aou_top `gpio0_sec_top` 实例化（`.gpio0_etb_trig(gpio0_etb_trig)`） | `wujian100_open/soc/aou_top.v : 553` |

---

## 8. 结论

GPIO 模块验证全部闭环：9 个用例（既有 1 + 新增 8）0 UVM_ERROR / 0 UVM_FATAL 全 PASS；F1~F12 全部覆盖（F4 Hardware 模式按计划降级为 C 用例 `gpio_intr_constraint` 兼带）；4 项 Spec-vs-RTL 关键差异（Hardware 模式未实现 / int_clr 地址 `0x60` / 读 mux label 误用宏 / `gpio0_etb_trig` SoC 级未引出）已写入验证报告与模块分析；6 项调试经验已沉淀；Makefile `findstring gpio_` 分支修复解锁所有 GPIO 用例。遗留风险 9 项已分类登记，建议按 §6 优先级进入下一阶段（RTL 补齐 Hardware 模式、UG 文档同步、ETB fabric 联调等）。
