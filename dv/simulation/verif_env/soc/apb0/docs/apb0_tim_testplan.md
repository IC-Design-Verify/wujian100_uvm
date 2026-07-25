# tim0 (APB0 Timer) 验证计划

> 对应 `dv/simulation/verif_env/soc/apb0/sequence/apb0_tim/` 与 `dv/simulation/verif_env/soc/apb0/test/uvm_test/apb0_tim/`
> 版本：v1.0 — 2026-04-28

---

## 1. 设计概述

### 1.1 DUT

- 顶层模块：`tim0_sec_top`（位于 `wujian100_open/soc/tim.v`）
- 子模块：`tim_top` → `timers_top` → `timers_apbif` + 两个 `timers_frc`
- APB0 基地址：`0x5000_0000`（APB_LEAF_SLV0：`0x5000_0000 ~ 0x5000_03FF`）
- 总线：APB（32-bit data / 32-bit addr / 单 master）
- 时钟域：`pclk` 同时驱动 APB 接口与两路 timer；`presetn` 异步复位
- 两路独立 32-bit 定时器：timer1 / timer2，共享一个 APB slave

### 1.2 寄存器映射

| 偏移 | 寄存器 | 访问 | 描述 |
|------|--------|------|------|
| 0x00 | TIMER1LC | W | timer1 加载计数寄存器 |
| 0x04 | TIMER1CV | R | timer1 当前值（实时刻） |
| 0x08 | TIMER1CR | RW | timer1 控制寄存器 |
| 0x0C | TIMER1EOI | R-clear | 读清除 timer1 中断 |
| 0x10 | TIMER1INTST | R | timer1 中断状态（受 mask） |
| 0x14 | TIMER2LC | W | timer2 加载计数寄存器 |
| 0x18 | TIMER2CV | R | timer2 当前值 |
| 0x1C | TIMER2CR | RW | timer2 控制寄存器 |
| 0x20 | TIMER2EOI | R-clear | 读清除 timer2 中断 |
| 0x24 | TIMER2INTST | R | timer2 中断状态（受 mask） |
| 0xA0 | TIMERSINTST | R | 两路 timer 合并中断状态（受 mask） |
| 0xA4 | TIMERSEOI | R-clear | 读清除两路 timer 中断 |
| 0xA8 | TIMERSRAW | R | 两路 timer 原始中断状态（不受 mask） |

> 注：RTL 中 `paddr[TIMER_ADDR_LHS:2]` 即 `paddr[7:2]` 用于寄存器解码，单字对齐访问。

### 1.3 控制寄存器位定义（TIMER?CR，5 bit）

| Bit | 名称 | 描述 |
|-----|------|------|
| [0] | ena | 使能定时器倒计数 |
| [1] | mode | 0=free-run（到 0 重新装载 MAX=0xFFFF_FFFF）；1=user模式（到 0 重新装载 load_count） |
| [2] | intmask | 1=屏蔽中断（INTST 永为 0），0=不屏蔽 |
| [4] | hwen | 硬件触发使能（`timertrig` 输出） |

> [3] 保留

### 1.4 关键时序行为

- 使能后第一名周期完成 load_value 装载（`rising_edge==0` 的下一拍）
- 倒计数到 0 → `atzero` 拉高一拍 → `interrupt` / `timertrig` 生成
- `int_mask` 仅影响 `INTST` 与 `TIMERSINTST`，不影响 `TIMERSRAW` 与 EOIlatch
- `EOI` 既清除 `timer_int_tmp` latch，也清除 `INTST` 的 visible 状态
- `mode=0`（free-run）：到 0 后从 `MAX(0xFFFF_FFFF)` 重新倒计
- `mode=1`（user）：到 0 后从 `load_count` 重新倒计
- 已使能 timer 写 `ena=0` 也会立即清 latch（`timeren[i]==0` 路径）

---

## 2. 验证目标

| 维度 | 目标 |
|------|------|
| 功能正确性 | 所有寄存器读/写、定时器倒计、中断生成与清除符合本规范 |
| 模式覆盖 | free-run 与 user-mode 各方向均被测试 |
| 中断机制 | mask/unmask、单 timer、双 timer 并发，EOI/TIMERSEOI 清除 |
| 边界 | 0、MAX-1、MAX、load_count=1、load_count=MAX 等边界值 |
| 一致性 | 两条 timer 独立/并发工作；寄存器读出值与 RTL 镜像一致 |
| Self-checking | 通过 `soc_apb0_tim_checker` 实现观察+比对，自动报告 pass/fail |

---

## 3. 验证场景

> ID 命名：`TIM_FUNC_<NNN>`（功能）/ `TIM_REG_<NNN>`（寄存器）/ `TIM_INT_<NNN>`（中断）/ `TIM_BOUND_<NNN>`（边界）/ `TIM_CONC_<NNN>`（并发）/ `TIM_ERR_<NNN>`（错误注入）

### 3.1 P0（必测，已实现或下个迭代实现）

| ID | 名称 | 描述 | 状态 | 实现 |
|----|------|------|------|------|
| TIM_FUNC_001 | timer1 free-run unmasked | load=0x100, mode=0, ena=1, intmask=0；轮询 INTST 直到为 1；读 EOI 清除；读 CV 验证递减 | covered | `soc_apb0_tim_smoke_v_sequence` Scenario 1 |
| TIM_FUNC_002 | timer1 user-mode masked | load=0x40, mode=1, intmask=1, ena=1；轮询 INTST 期望超时（保持 0） | covered | `soc_apb0_tim_smoke_v_sequence` Scenario 2 |
| TIM_FUNC_003 | timer1 user-mode unmasked | 关闭→重设 load=0x20, mode=1, intmask=0, ena=1；轮询 INTST 期望为 1；读 EOI | covered | `soc_apb0_tim_smoke_v_sequence` Scenario 3 |
| TIM_FUNC_004 | timer2 free-run unmasked | load=0x80, mode=0, ena=1, intmask=0；轮询 INTST，读 EOI | covered | `soc_apb0_tim_smoke_v_sequence` Scenario 4 |
| TIM_INT_001 | combined registers readback | 读 TIMERSINTST(0xA0) / TIMERSEOI(0xA4) / TIMERSRAW(0xA8) 验证值正确 | covered | `soc_apb0_tim_smoke_v_sequence` Scenario 5 |

### 3.2 P1（应在下一轮迭代补全）

| ID | 名称 | 描述 | 状态 | 备注 |
|----|------|------|------|------|
| TIM_REG_001 | 默认值上电检查 | 复位后读所有可读寄存器，验证默认值（CR=0, INTST=0, LC=0, CV=0xFFFFFFFF?） | not_covered | 需扩展 vseq |
| TIM_REG_002 | LC 写后回读（timer1/timer2） | 写 LC=随机值 → 读 LC 验证一致 | not_covered | 注意 LC 是 W-only，回读路径来自 `ri_timer1loadcount` |
| TIM_REG_003 | CR 写后回读 | 写各控制位组合 → 读 CR 验证 | not_covered | 含 hwen 位 |
| TIM_REG_004 | EOI 自清验证 | EOI 一次读后 INTST/`timer_int_tmp` 翻为 0；再次读 EOI 应为 0 | not_covered | |
| TIM_REG_005 | TIMERSEOI 联动清除 | 同时清除两路 timer 的 latch | not_covered | |
| TIM_BOUND_001 | load_count=1 触发瞬时中断 | 最短周期；测试 atzero 时序边界 | not_covered | |
| TIM_BOUND_002 | load_count=0xFFFF_FFFF | free-run 与 user-mode 等价观察 | not_covered | |
| TIM_BOUND_003 | load_count=0 复位行为 | 装载 0 应立即触发 1 拍 atzero | not_covered | |
| TIM_BOUND_004 | mode 切换不导致悬挂 | ena=0 时切换 mode 后再 ena=1，不应残留旧值 | not_covered | |
| TIM_INT_002 | mask 切换运行时生效 | 中断已 pending 时改 mask=0 → INTST 立即翻 1 | not_covered | |
| TIM_INT_003 | ena=0 立即清 latch | 已 pending 时写 ena=0 → `timer_int_tmp` 立即清 0 | not_covered | |
| TIM_CONC_001 | 双 timer 并发 | 两 timer 不同周期同时启动，多次中断交错 | not_covered | 需新建并发 vseq |

### 3.3 P2（拓展/压力）

| ID | 名称 | 描述 | 状态 |
|----|------|------|------|
| TIM_CONC_002 | 双 timer 高频随机回读 | 定时器运行中持续随机读 CV，验证单调递减 | not_covered |
| TIM_ERR_001 | 未实现地址读返回 0 | paddr 命中 0x28/0x40 等未定义偏移，prdata 应为 0 | not_covered |
| TIM_ERR_002 | 非对齐访问行为 | 32-bit APB 总线单拍对齐，仅测试 word 对齐 | not_covered |
| TIM_ERR_003 | 同地址连续写覆盖 | 多次写 LC/CR 验证最后写入生效 | not_covered |

---

## 4. Self-Checking Checker 描述

### 4.1 实现位置

`apb0/sequence/apb0_tim/soc_apb0_tim_checker.svh`（已在上一会话创建）

### 4.2 工作模式

- 跟随式观察（observe-on-every-transaction）：vseq 中每次 `tim_seq.tim_reg_write/read` 后立即调用 `chk.observe_write(addr, data)` 或 `chk.observe_read(addr, data)`
- 维护内部参考模型：
  - `load_count[0..1]`
  - `control_reg[0..1]`（ena/mode/intmask/hwen）
  - `latched_int[0..1]`
  - `current_value` 软件递减（基于 pclk 周期估计）
- 每次 read 后比较读回值与期望读回值（按 mask），累加 `match_count` / `mismatch_count`
- `uvm_error` 报告不匹配地址与期望/实际值

### 4.3 限制与待扩展

- 当前 checker 不监控外部 `intr` 输出端口（可通过 monitor 接入 `apb_env.apb_master_env.monitor` 后扩展）
- `current_value` 软件模型以"读 CV 当下估算"近似，对非对齐读时机不精确（P2 项可放宽）
- ETB 触发输出（`tim1_etb_trig` / `tim2_etb_trig`）仅作功能观察，无独立 checker

---

## 5. 覆盖率规划

### 5.1 功能覆盖率（covergroup）

| Covergroup | 覆盖点 | 目标场景 |
|------------|--------|----------|
| cg_tim_ctrl | ena × mode × intmask × hwen 笛卡尔积 | 16 种关键组合 |
| cg_tim_load_value | load_count = {0, 1, 0xFF, 0xFFFF, 0xFFFF_FFFF, 随机} | 边界 + 一般 |
| cg_tim_int_status | timer1/timer2 pending ∧ mask 状态 | 8 种组合 |
| cg_tim_eoi_clear | 单 timer EOI / 联合 TIMERSEOI 清除 | 3 种 |
| cg_tim_readback | LC/CV/CR/INTST 回读命中各寄存器 | 8 个寄存器 |

### 5.2 代码覆盖率目标

| 类型 | 目标 |
|------|------|
| Line | ≥ 95% |
| Branch | ≥ 90% |
| Toggle | ≥ 60%（含 timer 计数 32-bit 各位翻转） |
| Condition | ≥ 90% |
| FSM | N/A（无显式 FSM） |

### 5.3 断言（可选）

| 断言 | 检查 |
|------|------|
| a_load_after_en | ena 上升后第一拍 `timer==load_value` |
| a_atzero_one_shot | `atzero` 高且仅高 1 拍 |
| a_intst_clear_after_eoi | EOI 读后 INTST 立即翻 0 |
| a_eoi_intr_correlation | `intr` 与 `INTST` 同步 |

---

## 6. 已实现测试用例清单

| Test 类名 | 继承 | 关联 vseq | 对应计划 ID |
|-----------|------|-----------|-------------|
| `apb0_tim_base_test` | `soc_top_test_base` | — | build_phase 验证 |
| `apb0_tim_smoke_test` | `apb0_tim_base_test` | `soc_apb0_tim_smoke_v_sequence` | TIM_FUNC_001~004, TIM_INT_001 |

运行命令（参考项目 Makefile）：

```bash
# VIP-driven 模式
make all_vip TESTNAME=apb0_tim_smoke_test

# 或编译后单独仿真
./simv +UVM_TESTNAME=apb0_tim_smoke_test
```

---

## 7. 缺口与后续工作

1. **P1 场景 vseq 补全**：至少补全 TIM_REG_001~005、TIM_BOUND_001~003、TIM_INT_002~003、TIM_CONC_001 共 12 个场景。
   - 建议新建 `soc_apb0_tim_reg_v_sequence`、`soc_apb0_tim_bound_v_sequence`、`soc_apb0_tim_concur_v_sequence`
   - 对应新建 `apb0_tim_reg_test` / `apb0_tim_bound_test` / `apb0_tim_conc_test`
2. **覆盖率组接入**：在 `apb0_tim_sequence` 中插入 covergroup sampling hook
3. **intr 输出监控**：扩展 `soc_top_env` 增加 timer `intr` 端口 monitor，或新增独立 monitor 接入 checker
4. **断言集成**：在 `apb0_tb_top` 中为 `tim0` 实例添加 SVA（项目已有 `assert svaext` 编译选项）
5. **Test plan JSON 版本**：见同目录 `apb0_tim_testplan.json`，可被 `testcase-build` skill 读取

---

## 8. 版本历史

| 版本 | 日期 | 内容 |
|------|------|------|
| v1.0 | 2026-04-28 | 初版：基于 RTL 与现有 smoke vseq 整理场景与计划 |
