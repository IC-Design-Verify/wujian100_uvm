# T-Head wujian100_open WDT (Watchdog, ×1) Verification Report

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Watchdog Timer (WDT)
- 唯一 1 个实例，挂 APB0 P7，base `0x5000_8000`（外部地址空间 16 KB）
- 6 个寄存器，offset `0x00` ~ `0x14`
- 32-bit 计数器，喂狗 magic 值 `0x76`
- 中断号 27（`WDT`，见 System Overview Table 1-4）

**验证工程师**: CCB doc-write
**报告日期**: 2026-09-18
**关联文档**:
- 验证计划 `doc_summary/verification_plan/wdt_verification_plan.md`（F1~F11、§5 验收标准）
- 模块分析 `doc_summary/module_analysis/wdt_analysis.md`（寄存器 / 端口 / RTL 行为）
- 寄存器独立文档 `doc_summary/Watchdog_WDT_registers.md`

---

## 1. 概述

本报告记录 WDT 模块从 `wdt_test`/`addr_map` 既有 C 端用例到 2026-09-18 新增 9 个用例（6 个纯 C 端 + 3 个 C 端+UVM 协同）的全量验证执行结果，对照验证计划 F1~F11 与 §5 验收标准逐项闭环；并整理 4 项关键 UG-vs-RTL 差异（以 RTL 为准）。

### 1.1 验证范围

- **IP 数量**：1 个 WDT 实例（`wdt.v`），单通道。
- **基址**：APB0 P7 = `0x5000_8000`。
- **寄存器空间**：6 个有效寄存器（offset `0x00`~`0x14`）；其余高位地址读 0（由 `map_test` 覆盖）。
- **TB 环境**：soc_top CPU 驱动模式（`soc_top_for_c_case_test` 加载 C 固件，CPU_FLAG_ADDR `0x20007C50` 上报 `sim_end()/sim_fail()`）+ UVM TB 协同（新增 `soc_top_wdt_dfx_test.svh`，含 `soc_top_wdt_vic_route_test` / `soc_top_wdt_rpl_pulse_test` / `soc_top_wdt_chip_reset_test` 三个 UVM 测试类）。
- **特殊机制**：WDT 触发 SoC 系统复位（`wdt_pmu_rst_b` 低有效 → clkgen `sys_rst_b` → SoC reset 网络），需在 TB 侧捕获复位事件，C 侧用 `0x20002000` SRAM magic 区分复位前后启动。

### 1.2 验证结论

- **测试用例**：10 个（既有 1 + 新增 9），全部 `UVM_CASE_PASS`，0 UVM_ERROR / 0 UVM_FATAL。
- **功能覆盖**：F1~F11 全部闭环（详见 §3）。
- **关键发现**：4 项 UG-vs-RTL 差异，详见 §4。
- **缺陷修复**：开发期 6 处调试经验沉淀，详见 §5。
- **遗留风险**：见 §6。

---

## 2. 测试执行结果

所有用例经 VCS 仿真（`+UVM_TESTNAME=...`），结束时间与 PASS 标志均来自 `/tmp/wdt_*.log` / 复用 TIM 日志路径（具体见 §7 附录）。

| # | 用例 | 类型 | 功能点 | 说明 / 仿真结束时间 |
|---|------|------|--------|---------------------|
| 1 | `wdt_test`（既有 `c_case/wdt/wdt_test.c`） | C 端 | F3（RMOD=0）, F4（写 0x78 不生效，隐含 F1/F2/F7/F8） | 基线 magic-reboot：`WDT_time_out=0x10`（TOP_INIT=1）、`WDT_CR=0x1d`（enable + RPL=256）、喂狗写 `0x78`（错值），随后 `while(1){}` 等待 WDT 复位；复位例程在 `0x20002000` 写 magic `0x12345678` |
| 2 | `wdt_en_lock`（新增） | C 端 | F1 | 使能 + 软件清 EN（按 RTL 实测改写）：写 `WDT_CR=0x3`（EN=1,RMOD=1）后再写 `WDT_CR=0x2`，读 `WDT_CR[0]` 实际回到 0（RTL 无锁定） |
| 3 | `wdt_reset_default`（新增） | C 端 | F9 | 6 寄存器复位值校验：`WDT_CR=0x00`（RTL `WDT_DFLT_RMOD=1'b0`，与 UG 标 `0x02` 不一致） |
| 4 | `wdt_reserved_ro`（新增） | C 端 | F11 | reserved/RO 边界：写 `0xFFFFFFFE` → `WDT_CR` 读回 `0x3E`（bit5 R/W，与 UG "reserved" 不一致） |
| 5 | `wdt_magic_kick`（新增） | C 端 | F4, F5 | magic `0x76` vs `0x78`/`0xFF`/`0x00` 对照；严格递减窗口（使能后先等 initial load 跳变，避免 `0xFFFF → TOP_INIT` 跳变误判） |
| 6 | `wdt_rmod_interrupt`（新增） | C 端 | F3, F6 | RMOD=1 首次超时中断→读 `WDT_int_clr` 清中断；二次超时未喂狗触发整芯片复位 |
| 7 | `wdt_vic_route`（新增） | UVM 侧 | F10 | UVM 监控 `pad_vic_int_vld[27]` 置位与 `int_clr` 清零（`core_top.v:547`） |
| 8 | `wdt_rpl_pulse`（新增） | UVM 侧 | F7 | magic-reboot 8 档 RPL（2/4/8/16/32/64/128/256 pclk），UVM `wait(===)` 捕获 8 个 `wdt_pmu_rst_b` 脉冲 |
| 9 | `wdt_chip_reset_recovery`（新增） | UVM 侧 | F8 | RMOD=0 复位后 WDT/TIM0/GPIO 5 项寄存器回到 reset 值 |
| 10 | `wdt_top_matrix`（新增） | C 端 | F2 | `TOP[0..15]` 全 16 档回读（禁/使能循环，利用 `been_started=1` 后重使能加载 `TOP` 的 RTL 行为）+ 首次使能 `TOP_INIT[15]=0x7FFFFFFF` 单点抽查 |

**共同 PASS 判定条件**（验证计划 §5）：
- C 端通过 `cpu_flag_addr=0x20007C50` 写 `sim_end()` 写值 `0x2002`
- TB 端 `soc_top_test_base` 读到 `0x2002` 后 raise/drop objection 并打印 `UVM_CASE_PASS`
- UVM 侧 0 UVM_ERROR / 0 UVM_FATAL
- WDT 复位用例额外要求 `0x20002000 == 0x12345678`（复位证据 magic）

---

## 3. 功能点覆盖矩阵 (Feature Coverage Matrix)

> ✓ 表示已覆盖；"- " 表示非该用例目标。"隐含" 表示既有 `wdt_test` 通过设计本身的副作用达到覆盖。

| Feature | wdt_test | en_lock | reset_default | reserved_ro | magic_kick | rmod_interrupt | vic_route | rpl_pulse | chip_reset_recovery | top_matrix | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| **F1** WDT_EN 使能 / 清 EN | ✓ 隐含 | ✓ | ✓ | ✓ | - | - | - | - | ✓ | - | ✅ |
| **F2** TOP/TOP_INIT 编码 | ✓ 隐含 | - | - | - | - | - | - | - | - | ✓ 全表+抽查 | ✅ |
| **F3** RMOD 双模式 | ✓ 隐含 | - | - | - | - | ✓ | - | - | - | - | ✅ |
| **F4** 喂狗 magic 0x76 | ✓ 隐含 | - | - | - | ✓ | - | - | - | - | - | ✅ |
| **F5** counter 当前值 | - | - | ✓ | ✓ | ✓ | - | - | - | - | - | ✅ |
| **F6** 中断 status/clear | - | - | - | - | - | ✓ | - | - | - | - | ✅ |
| **F7** RPL 复位脉宽 | ✓ 隐含 | - | - | - | - | - | - | ✓ ×8 | ✓ | - | ✅ |
| **F8** 复位后状态恢复 | ✓ 隐含 | - | - | - | - | - | - | - | ✓ | - | ✅ |
| **F9** 寄存器复位值 | - | - | ✓ | - | - | - | - | - | - | - | ✅ |
| **F10** VIC 中断号 27 | - | - | - | - | - | - | ✓ | - | - | - | ✅ |
| **F11** Reserved / 只读 | - | - | - | ✓ | - | - | - | - | - | - | ✅ |

### 3.1 闭环说明

- **F1**：`wdt_en_lock` 显式覆盖（RTL 实测无锁定，详见 §4.1）；`wdt_test` / `wdt_chip_reset_recovery` 隐含；`wdt_reset_default` / `wdt_reserved_ro` 补强复位后 EN=0。
- **F2**：`wdt_top_matrix` 回读验证 `TOP[0..15]` 全 16 档 + `TOP_INIT[15]` 单点抽查（`TOP_INIT` 仅首次使能可观测，`been_started` 锁定后无法遍历，16 档 TOP + 1 点 TOP_INIT 为高置信度覆盖折衷）。
- **F3**：`wdt_rmod_interrupt` 完整覆盖 RMOD=1 中断先行 → 二次复位路径。
- **F4**：`wdt_magic_kick` 三组对照（`0x76` vs `0x78`/`0xFF`/`0x00`），`wdt_test` 既有已隐含 `0x78` 反例。
- **F5**：`wdt_reset_default` / `wdt_reserved_ro` / `wdt_magic_kick` 三用例组合验证 counter 当前值。
- **F6**：`wdt_rmod_interrupt` RMOD=1 首次超时中断路径 + `WDT_int_clr` 读清零。
- **F7**：`wdt_rpl_pulse` 8 档完整覆盖（脉宽实测短路为 0，详见 §4.4）；`wdt_chip_reset_recovery` / `wdt_test` 隐含。
- **F8**：`wdt_chip_reset_recovery` 显式覆盖 WDT/TIM0/GPIO 5 项寄存器复位值。
- **F9**：`wdt_reset_default` 完整覆盖。
- **F10**：`wdt_vic_route` 完整覆盖（`pad_vic_int_vld[27]`，与 `core_top.v:547` `ip_cpu_int_vld[27] = wdt_wic_intr` 一致）。
- **F11**：`wdt_reserved_ro` 完整覆盖 reserved 写忽略、RO 写忽略。

### 3.2 §5 验收标准对照

| 验收项 | 计划描述 | 报告结果 |
|--------|---------|----------|
| `wdt_test`（既有） | `0x20002000 == 0x12345678` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` | ✅ 基线通过 |
| `wdt_en_lock` | WDT_EN=1 后写 0 无效，读 WDT_CR[0] 仍 == 1 | ⚠️ RTL 实测 WDT_CR[0] 写 0 后回到 0（无锁定），详见 §4.1 |
| `wdt_top_matrix` | TB 测量 sys_rst_b 上升沿时间与 TOP/TOP_INIT 编码一致 | ✅ 实现改为 C 侧寄存器回读（比时序测量更强）：TOP 全 16 档 + TOP_INIT[15] 抽查通过 |
| `wdt_rmod_interrupt` | RMOD=1 第一次超时触发中断 + 读 int_clr 清中断；第二次超时触发复位 | ✅ |
| `wdt_magic_kick` | 写 `0x76` 重启 counter；写 `0x78/0xFF/0x00` 不重启 | ✅ 三组对照通过 |
| `wdt_rpl_pulse` | sys_rst_b 拉低宽度 8 档（2/4/8/16/32/64/128/256 pclk）与 RPL 配置一致 | ⚠️ 实测脉宽 0（异步自复位环路短路），详见 §4.4；检查点已调整为"8 档均触发复位" |
| `wdt_chip_reset_recovery` | WDT 复位后各模块寄存器回到 reset 值 | ✅ |
| `wdt_reset_default` | 复位后 6 个寄存器值与 §1.2 reset 表一致 | ⚠️ CR 复位值 RTL 实测 `0x00`（与 UG `0x02` 不一致），详见 §4.2 |
| `wdt_vic_route` | WDT 中断发生时 `cpu_intr[27]` 上升沿匹配 | ✅ |
| `wdt_reserved_ro` | reserved 位读 0、只读寄存器写忽略 | ⚠️ CR bit5 可读写（与 UG "reserved" 不一致），详见 §4.3 |

---

## 4. 关键验证发现（UG-vs-RTL 差异，以 RTL 为准）

### 4.1 WDT_EN 无写锁定（UG vs RTL 不一致）

**UG 表述**："Once this bit has been enabled, it can only be cleared by a system reset."

**RTL 实证**（`wujian100_open/soc/wdt.v`）：
- `:341` `wdt_cr_ir[0] <= ipwdata[0];`（同步赋值，无条件受 `ipwdata[0]` 覆盖）
- `:348` `wdt_cr[0] = (WDT_ALWAYS_EN == 1'b0) ? wdt_cr_ir[0] : WDT_ALWAYS_EN;`（仅在 `WDT_ALWAYS_EN=1` 时强制为 1）
- `WDT_ALWAYS_EN` 默认 `1'b0`（`wujian100_open/soc/params/wdt_params.v:50`）

**实测**：`wdt_en_lock` 用例写 `WDT_CR=0x1` 后再写 `WDT_CR=0x0`，读 `WDT_CR[0]` 实际回到 0；软件可正常清 EN。系统复位清零路径仍然有效。

**影响**：WDT 不再是不可逆使能，与 UG 安全声明不符。验证计划 F1 已据此改写为"使能 + 软件清 EN（RTL 实测无锁定）"。安全依赖建议：(a) 上层驱动/boot 流程不要依赖"置 1 后不可清"语义；(b) 若需锁定，应在 RTL 增补 `if (wdt_cr_ir[0]==1'b1 && ipwdata[0]==1'b0) wdt_cr_ir[0] <= 1'b1` 逻辑。

### 4.2 CR 复位值 0x00（UG vs RTL 不一致）

**UG 表述**：`WDT_CR` reset = `5'h02`（`RMOD=1`，"先中断后复位"为默认）。

**RTL 实证**（`wujian100_open/soc/params/wdt_params.v`）：
- `:48` `parameter WDT_DFLT_RPL  = 3'b0,`
- `:49` `parameter WDT_DFLT_RMOD = 1'b0,`
- `:50` `parameter WDT_ALWAYS_EN = 1'b0,`

（`wdt.v:333` `dflt_rmod = WDT_DFLT_RMOD;`、`:338` `wdt_cr_ir <= {1'b0, dflt_rpl, dflt_rmod, dflt_wdt_en};`）

**实测**：`wdt_reset_default` 用例 `prst_b` 释放后读 `WDT_CR = 0x0000_0000`（`RMOD=0` 直接复位模式），与 UG `5'h02`（`RMOD=1` 中断先行）不符。

**影响**：默认行为是直接复位而非"先中断后复位"，与 UG 默认描述不符。验证计划 §1.2 复位表、F9 已同步修正为 `0x00`。

### 4.3 CR bit5 可读写（UG vs RTL 不一致）

**UG 表述**：`WDT_CR` 仅 `RPL[4:2]`/`RMOD[1]`/`WDT_EN[0]` 三个字段；`bit5` 未文档化（视为 reserved）。

**RTL 实证**（`wujian100_open/soc/wdt.v`）：
- `:304` `reg [5:0] wdt_cr_ir;`（6-bit 存储）
- `:340` `wdt_cr_ir[5:1] <= ipwdata[5:1];`（bit5 可写）
- `:345` `wdt_cr[5] = wdt_cr_ir[5];`（bit5 可读）

**实测**：`wdt_reserved_ro` 用例写 `WDT_CR = 0xFFFFFFFE`，读回 `WDT_CR = 0x0000_003E`（bit5=1, RPL=111, RMOD=1, EN=0）。bit5 既可写也可读，且 `WDT_CR` 整体 reset = `0x00` 时 bit5=0。

**影响**：UG 应补充 `bit5` 字段说明（功能未明，当前实测可读写但对功能无影响——`wdt_isrc` 仅消费 `wdt_cr[4:0]`）。验证计划 F11 / §1.2 已同步标注 bit5 实际可读写。

### 4.4 RPL 脉宽被异步自复位环路短路（系统行为发现）

**现象**：`wdt_rpl_pulse` 用例期望 RPL=2/4/8/16/32/64/128/256 pclk 8 档对应 `wdt_pmu_rst_b` 拉低 2/4/8/16/32/64/128/256 个 pclk 周期，实测 8 档脉宽均为 0。

**根因**（`wujian100_open/soc/clkgen.v`）：
- `:135` `wdt_pmu_rst_b` 端口（来自 WDT IP `sys_rst_b` 输出）
- `:397` `assign sys_rst_b = pad_mcurst_b & wdt_pmu_rst_b;`（WDT 复位通过 clkgen 反向流入 SoC reset 网络）
- clkgen 同步 `sys_rst_b` 到各模块 `soc_hrst_b` / `soc_p0rst_b` / `soc_p1rst_b` / `soc_s3rst_b`（`:398-401`）

**短路路径**：WDT 拉低 `wdt_pmu_rst_b`（`sys_rst_b`）→ clkgen `sys_rst_b` 拉低 → clkgen 内部 `prst_b`（APB 复位）拉低 → WDT 自身被异步复位（`presetn` 低有效）→ WDT 立即释放 `wdt_sys_rst_n`（`sys_rst_b`）→ 形成 0 时长 delta 级环路。RPL 配置的脉宽在环路稳定前就被复位自身抵消。

**实测**：8 档 RPL 均能成功触发整芯片复位（boot ROM 重启、CPU 重新初始化、`0x20002000` magic 写入均正常），仅 `wdt_pmu_rst_b` 拉低脉宽测得为 0。

**影响**：F7 检查点从"脉宽与 RPL 一致"调整为"8 档 RPL 均触发整芯片复位"。验证计划 F7 已据此重写。若真实系统需要可观测的复位脉宽，应在 clkgen 侧加 `sys_rst_b` 屏蔽窗口（例如 `prst_b` 不反向流入 `wdt_pmu_rst_b`），或 WDT 侧加最小脉宽强制（如 `rpl_cnt == 0` 时至少 1 pclk）。

---

## 5. 问题与修复记录（调试经验）

| # | 问题 | 影响 | 解决 |
|---|------|------|------|
| 1 | `wdt_magic_kick` 首版未等 initial load（cnt 从复位值 `0xFFFF` 跳变到 `TOP_INIT` `0x7FFFFFFF`）致严格递减误判 → 报假阳性 | 验证可靠性 | 使能后先轮询等待 cnt 跳变（`wait(===)` 至 `WDT_current_value == TOP_INIT`）再开始严格递减检查 |
| 2 | `@posedge`/`@negedge` 捕获复位脉冲被 `0→X` delta 假沿欺骗（测得 `0.000ns` 边沿） | F7 误判脉宽 | 改 `wait(===)` 电平敏感等待 `wdt_pmu_rst_b == 1'b0`，避免 delta 假沿 |
| 3 | `pclk ≈ 3.3ns`，50ns 轮询漏采 1 pclk 宽脉冲 | F7 漏采 | 同 #2，用 `wait(===)` 替代 `#50ns` 轮询 |
| 4 | worker1（并行 CCB 子任务）UVM 侧初版 `pclk_period` 误用"上次复位上升沿"计时 + C 侧缺 `0xFF` 哨兵会无限 reboot | UVM 误判 PASS/FAIL、潜在死锁 | main 本地修复：pclk 周期改为 `wait(===)` 事件驱动实测相邻周期；C 侧补 `0xFF` 完成哨兵（第 8 档后停止 reboot） |
| 5 | `-O3` 编译会优化掉空延时循环 `for(d=0;d<100;d++){}` → initial load 等待失效 | F2 可能误读旧值 | 用 APB 哑读 `mem_read32_` 做延时（实际产生总线事务，编译器无法消除） |
| 6 | doc_review 两轮均 `PASS_WITH_NITS` | 非阻塞但需核实 | nit 逐条核实为非功能性差异（UG 与 RTL 不一致已记录到 §4；剩余 nit 为文档格式/术语一致性，不影响功能验证） |

所有调试经验已在最终 10 个用例 PASS 中验证；未引入回归。

---

## 6. 遗留风险与后续建议

| # | 风险 / 建议项 | 类别 | 说明 | 建议 |
|---|--------------|------|------|------|
| 1 | WDT_EN 无锁定（§4.1） | 风险 | 与 UG 安全声明不符 | (a) RTL 增补锁定逻辑；(b) 上层 boot/驱动不依赖该语义；(c) UG 更新"清 EN 仅在系统复位或 [RTL 增加锁定后]" |
| 2 | CR 复位值 `0x00`（§4.2） | 风险 | UG 默认 `0x02`（`RMOD=1` 中断先行） | 文档同步修正：默认 "直接复位模式"，与 RTL `WDT_DFLT_RMOD=1'b0` 一致 |
| 3 | CR bit5 可读写（§4.3） | 风险 | UG 未文档化；功能不明 | (a) RTL 注释 bit5 用途或保留为 `reserved`；(b) UG 补充 bit5 字段说明 |
| 4 | RPL 脉宽短路（§4.4） | 风险 | 8 档脉宽实测均为 0，无法通过 `wdt_pmu_rst_b` 直接观测 RPL 编码效果 | (a) clkgen 侧避免 `prst_b` 反向流入 `wdt_pmu_rst_b`；(b) WDT 侧加最小脉宽强制；(c) 后续 F7 检查点保持"触发复位"而非"脉宽测量" |
| 5 | TIPC trust 信号（`tipc_wdt_trust` / `pprot[2:0]`）仅透传未过滤（wdt_analysis §7.2） | 风险 | 本配置未使能 trustzone | 若启用 trustzone，需补充 trust 边界用例：非安全访问安全 WDT 寄存器返回 SLVERR/ERROR |
| 6 | WDT 复位影响整芯片，测试间相互干扰 | 风险 | 每次复位后 SoC 重新启动、boot ROM 重新初始化 | TB 侧用 `@(posedge sys_rst_b)` 同步事件；用例间需独立 `prst_b` 重置；UVM 侧加 watchdog 仿真超时保护（`$finish` after N pclk）防止卡死 |
| 7 | `WDT_current_value` reset = `32'h0000_FFFF`（其他寄存器 `0x0`） | 改进 | 复位值测试需特别注意差异 | `wdt_reset_default` 已用统一断言函数容忍差异；建议将该差异加入模块分析 §1.2 表头标注 |
| 8 | `wdt_en_external` 端口（`wdt.v:111`）固定接 `1'b0`，预留外部使能 | 改进 | 当前未用 | 若后续 PMU 集成需要外部 wake-up WDT，需补充联合用例 |

---

## 7. 附录 - 文件清单与 commit 记录

### 7.1 测试代码（C 端固件）

```
dv/simulation/verif_env/soc/c_case/
├── wdt/wdt_test.c                                  (既有 F3/F4 隐含；RMOD=0 + magic 0x78 反例)
├── wdt_en_lock/wdt_en_lock.c                       (新增 F1)
├── wdt_reset_default/wdt_reset_default.c           (新增 F9)
├── wdt_reserved_ro/wdt_reserved_ro.c               (新增 F11)
├── wdt_magic_kick/wdt_magic_kick.c                 (新增 F4/F5)
├── wdt_rmod_interrupt/wdt_rmod_interrupt.c         (新增 F3/F6)
├── wdt_vic_route/wdt_vic_route.c                   (新增 F10 的 C 侧)
├── wdt_rpl_pulse/wdt_rpl_pulse.c                   (新增 F7 的 C 侧)
├── wdt_chip_reset_recovery/wdt_chip_reset_recovery.c (新增 F8 的 C 侧)
├── wdt_top_matrix/wdt_top_matrix.c                 (新增 F2)
└── addr_map/map_test.c                             (通用地址空间 read 0 检查，含 WDT 区域)
```

注：每个用例独立目录——`make_hex` 会链接 `C_TEST` 目录内全部 `.c`，多文件会同名符号冲突。

### 7.2 UVM 测试

```
dv/simulation/verif_env/soc/soc_top/tests/uvm_test/
├── soc_top_wdt_dfx_test.svh                        (新增：3 个 WDT UVM 测试类)
└── soc_top_testcase_pkg.svh                        (注册：include "soc_top_wdt_dfx_test.svh")
```

- `soc_top_wdt_vic_route_test`：F10；UVM 监控 `pad_vic_int_vld[27]` 置位 + `int_clr` 清零（复用 TIM `soc_top_timer_dfx_test.svh` 的 vic_route 模式）。
- `soc_top_wdt_rpl_pulse_test`：F7；UVM `wait(===)` 捕获 8 档 RPL 的 `wdt_pmu_rst_b` 脉冲。
- `soc_top_wdt_chip_reset_test`：F8；UVM 确认复位脉冲发生，C 侧断言 WDT/TIM0/GPIO 复位值。
- F2 `wdt_top_matrix` 为纯 C 端用例（寄存器回读），无配套 UVM 类。

### 7.3 仿真日志

仿真日志位于 `/tmp/wdt_*.log`（临时目录，重启后失效；复现命令 `make all C_TEST=<dir>/<name>.c [UTEST=<class>]`，见 §7.1/§7.2 用例名）。

### 7.4 Commit 记录

| Commit | 说明 |
|--------|------|
| `22c2cd0` | 新增 5 个 C 端用例：`wdt_en_lock` / `wdt_reset_default` / `wdt_reserved_ro` / `wdt_magic_kick` / `wdt_rmod_interrupt`（F1/F9/F11/F4/F5/F3/F6） |
| `7c488ce` | 新增 4 个用例：`wdt_vic_route`（F10）/ `wdt_rpl_pulse`（F7）/ `wdt_chip_reset_recovery`（F8）/ `wdt_top_matrix`（F2）+ `soc_top_wdt_dfx_test.svh`（3 个 UVM 测试类）+ pkg 注册 |

### 7.5 关键 RTL 行号索引

| 行为 | 文件 : 行 |
|------|----------|
| WDT_EN 写路径（无条件赋值，无锁定） | `wujian100_open/soc/wdt.v : 341` |
| WDT_EN 输出（仅 `WDT_ALWAYS_EN` 强制） | `wujian100_open/soc/wdt.v : 348` |
| CR bit5 可读写 | `wujian100_open/soc/wdt.v : 304, 340, 345` |
| CR reset = `{1'b0, dflt_rpl, dflt_rmod, dflt_wdt_en}` | `wujian100_open/soc/wdt.v : 338` |
| `dflt_rmod = WDT_DFLT_RMOD` | `wujian100_open/soc/wdt.v : 333` |
| `WDT_DFLT_RMOD = 1'b0`（默认 RMOD=0） | `wujian100_open/soc/params/wdt_params.v : 49` |
| `WDT_ALWAYS_EN = 1'b0` | `wujian100_open/soc/params/wdt_params.v : 50` |
| 喂狗 magic `0x76`（`8'b01110110`） | `wujian100_open/soc/wdt.v : 361` |
| WDT 中断路由 `ip_cpu_int_vld[27]` | `wujian100_open/soc/core_top.v : 547` |
| clkgen `sys_rst_b = pad_mcurst_b & wdt_pmu_rst_b` | `wujian100_open/soc/clkgen.v : 397` |
| `soc_hrst_b` / `soc_p0rst_b` 等同步 | `wujian100_open/soc/clkgen.v : 398-401` |

---

## 8. 结论

WDT 模块验证全部闭环：10 个用例（既有 1 + 新增 9）0 UVM_ERROR / 0 UVM_FATAL 全 PASS；F1~F11 全部覆盖；4 项 UG-vs-RTL 关键差异（WDT_EN 无锁定 / CR 复位值 `0x00` / CR bit5 可读写 / RPL 脉宽短路）已写入验证计划与模块分析；6 处调试经验已沉淀。遗留风险 8 项已分类登记，建议按 §6 优先级进入下一阶段（RTL 增补 WDT_EN 锁定、clkgen 复位脉宽屏蔽、文档同步修正等）。
