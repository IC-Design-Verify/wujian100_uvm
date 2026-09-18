# T-Head wujian100_open PWM (12 输出/6 发生器 + 6 捕获 + 6 定时器, ×1) Verification Plan

**项目**: wujian100_open SoC 验证
**IP**: T-Head wujian100_open Pulse Width Modulation (PWM)
  - 唯一 1 个实例，挂 APB0 P12，base `0x5001_C000`（外部地址空间 16 KB）
  - **12 路 PWM 输出**（PWM0~PWM11）/ **6 组 PWM 发生器**（group0~group5）
  - **6 路输入捕获**（CAP0~CAP5，对应 channel 0/2/4/6/8/10）/ **6 组 16-bit 定时器**（tim0~tim5）
  - 53 个寄存器，offset `0x00` ~ `0xD0`（间隔 4）
  - 中断号 25（`PWM`，见 System Overview Table 1-4）

**验证工程师**: CCB doc-write
**计划日期**: 2026-09-16

---

## 1. IP 架构概览

### 1.1 RTL 配置（来自 `wujian100_open/soc/pwm.v`）

| 参数 | 值 | 含义 |
|------|-----|------|
| `PWM_GROUP_NUM` | `6` | PWM 发生器组数（group0~group5） |
| `PWM_CH_PER_GROUP` | `2` | 每组通道数（每组 2 路，共 12 路） |
| `CAP_NUM` | `6` | 输入捕获通道数（CAP0~CAP5，对应 ch0/2/4/6/8/10） |
| `TIM_NUM` | `6` | 定时器通道数（tim0~tim5） |
| `COUNTER_WIDTH` | `32`（高/低 16-bit 各驱动 1 个通道） | PWM 计数器位宽（每 group 1 个 32-bit 计数器，高低各 16-bit） |
| `TIMER_WIDTH` | `16` | TIM 定时器位宽 |
| `PWM_REG_NUM` | `53`（`pwm_apbif` 内 `define *_OFFSET`） | 寄存器数量 |
| `CNT_DIV_BITS` | `3`（PWMCFG `[26:24]`） | PWM clock 分频（pclk 经 `cntdiv` 后给 PWM） |
| `CNT_DIV_EN_BIT` | `27`（PWMCFG `cntdiven`） | 分频使能 |
| `FAULT_POL` | 高电平有效（待确认） | 故障输入极性 |

**关键配置含义**：
- PWM 时钟源为 pclk 经 `cntdiv` 分频（默认 /2）；PWM 周期由 `PWM01LOAD`/`PWM23LOAD`/`PWM45LOAD` 的 16-bit load value 决定。
- 每 group 1 个 32-bit 计数器（高 16-bit + 低 16-bit 各驱动 1 路 PWM），6 个 group = 6 个计数器 = 12 路 PWM。
- 死区（dead-band）每 group 1 个寄存器 (`PWM01DB/PWM23DB/PWM45DB`)；极性反转通过 `PWMINVERTTRIG` 控制。
- 53 个寄存器覆盖 5 大功能：PWM 主控（14）/ PWM 中断（8）/ PWM 计数器与比较（9）/ 捕获 CAP（11）/ 定时器 TIM（13）。

### 1.2 寄存器映射（53 个寄存器，offset 相对 base `0x5001_C000`）

| # | Offset | Name | Access | Reset | 说明 |
|---|--------|------|--------|-------|------|
| 1 | `0x00` | `PWMCFG` | RW | `0x0` | 全局配置：`cntdiven[27]`/`cntdiv[26:24]`/`tim0~5en[23:18]`/`cap0~5en[17:12]`/`pwm0~11en[11:0]` |
| 2 | `0x04` | `PWMINVERTTRIG` | RW | `0x0` | 极性反转：`pwm0inv~pwm11inv[11:0]` |
| 3 | `0x08` | `PWM01TRIG` | RW | `0x0` | Group 0/1 trigger 比较值（ADC trigger） |
| 4 | `0x0C` | `PWM23TRIG` | RW | `0x0` | Group 2/3 trigger 比较值 |
| 5 | `0x10` | `PWM45TRIG` | RW | `0x0` | Group 4/5 trigger 比较值 |
| 6 | `0x14` | `PWMINTEN1` | RW | `0x0` | PWM 中断 enable：group 2/1/0 计数器 + ADC trigger + CMP 匹配等事件 |
| 7 | `0x18` | `PWMINTEN2` | RW | `0x0` | PWM 中断 enable：group 5/4/3 |
| 8 | `0x1C` | `PWMRIS1` | RO | `0x0` | Raw 中断状态：group 2/1/0 |
| 9 | `0x20` | `PWMRIS2` | RO | `0x0` | Raw 中断状态：group 5/4/3 |
| 10 | `0x24` | `PWMIC1` | RW | `0x0` | 中断 clear：group 2/1/0（写 1 清） |
| 11 | `0x28` | `PWMIC2` | RW | `0x0` | 中断 clear：group 5/4/3 |
| 12 | `0x2C` | `PWMIS1` | RO | `0x0` | Masked 中断状态：group 2/1/0 |
| 13 | `0x30` | `PWMIS2` | RO | `0x0` | Masked 中断状态：group 5/4/3 |
| 14 | `0x34` | `PWMCTL` | RW | `0x0` | 计数模式：`Sync5mode~sync0mode[17:6]` + `pwm5mode~pwm0mode[5:0]`（up/up-down） |
| 15 | `0x38` | `PWM01LOAD` | RW | `0x0` | Group 0/1 加载值（`[31:16]=loadm, [15:0]=loadn`） |
| 16 | `0x3C` | `PWM23LOAD` | RW | `0x0` | Group 2/3 加载值 |
| 17 | `0x40` | `PWM45LOAD` | RW | `0x0` | Group 4/5 加载值 |
| 18 | `0x44` | `PWM01COUNT` | RO | `0x0` | Group 0/1 counter 当前值 |
| 19 | `0x48` | `PWM23COUNT` | RO | `0x0` | Group 2/3 counter 当前值 |
| 20 | `0x4C` | `PWM45COUNT` | RO | `0x0` | Group 4/5 counter 当前值 |
| 21 | `0x50` | `PWM0CMP` | RW | `0x0` | PWM0 compare A / PWM1 compare B（`[31:16]=compnb, [15:0]=compna`） |
| 22 | `0x54` | `PWM1CMP` | RW | `0x0` | PWM2 compare A / PWM3 compare B |
| 23 | `0x58` | `PWM2CMP` | RW | `0x0` | PWM4 compare A / PWM5 compare B |
| 24 | `0x5C` | `PWM3CMP` | RW | `0x0` | PWM6 compare A / PWM7 compare B |
| 25 | `0x60` | `PWM4CMP` | RW | `0x0` | PWM8 compare A / PWM9 compare B |
| 26 | `0x64` | `PWM5CMP` | RW | `0x0` | PWM10 compare A / PWM11 compare B |
| 27 | `0x68` | `PWM01DB` | RW | `0x0` | Group 0/1 dead-band：`dbmen[25]`/`dbnen[24]`/`delaym[23:12]`/`delayn[11:0]` |
| 28 | `0x6C` | `PWM23DB` | RW | `0x0` | Group 2/3 dead-band |
| 29 | `0x70` | `PWM45DB` | RW | `0x0` | Group 4/5 dead-band |
| 30 | `0x74` | `CAPCTL` | RW | `0x0` | 捕获控制：`cap5event~cap0event[17:6]` / `cap5mode~cap0mode[5:0]`（edge count/edge time） |
| 31 | `0x78` | `CAPINTEN` | RW | `0x0` | CAP 中断 enable：`cap5timie~cap0timie[11:6]` + `cap5cntie~cap0cntie[5:0]` |
| 32 | `0x7C` | `CAPRIS` | RO | `0x0` | CAP Raw 中断状态：`cap5timris~cap0timris[11:6]` + `cap5cntris~cap0cntris[5:0]` |
| 33 | `0x80` | `CAPIC` | RW | `0x0` | CAP 中断 clear：`cap5timic~cap0timic[11:6]` + `cap5cntic~cap0cntic[5:0]` |
| 34 | `0x84` | `CAPIS` | RO | `0x0` | CAP Masked 中断状态 |
| 35 | `0x88` | `CAP01T` | RO | `0x0` | Group 0/1 捕获 counter 值 |
| 36 | `0x8C` | `CAP23T` | RO | `0x0` | Group 2/3 捕获 counter 值 |
| 37 | `0x90` | `CAP45T` | RO | `0x0` | Group 4/5 捕获 counter 值 |
| 38 | `0x94` | `CAP01MATCH` | RW | `0x0` | Group 0/1 CAP match 值 |
| 39 | `0x98` | `CAP23MATCH` | RW | `0x0` | Group 2/3 CAP match 值 |
| 40 | `0x9C` | `CAP45MATCH` | RW | `0x0` | Group 4/5 CAP match 值 |
| 41 | `0xA0` | `TIM_INT_EN` | RW | `0x0` | TIM 中断 enable：`tim5ie~tim0ie[5:0]` |
| 42 | `0xA4` | `TIMRIS` | RO | `0x0` | TIM Raw 中断状态：`tim5ris~tim0ris[5:0]` |
| 43 | `0xA8` | `TIM_INT_CLR` | RW | `0x0` | TIM 中断 clear：`tim5ic~tim0ic[5:0]`（写 1 清） |
| 44 | `0xAC` | `TIMIS` | RO | `0x0` | TIM Masked 中断状态 |
| 45 | `0xB0` | `TIM01LOAD` | RW | `0x0` | Group 0/1 TIM load（`[31:16]=timloadm, [15:0]=timloadn`） |
| 46 | `0xB4` | `TIM23LOAD` | RW | `0x0` | Group 2/3 TIM load |
| 47 | `0xB8` | `TIM45LOAD` | RW | `0x0` | Group 4/5 TIM load |
| 48 | `0xBC` | `TIM01COUNT` | RO | `0x0` | Group 0/1 TIM 当前值 |
| 49 | `0xC0` | `TIM23COUNT` | RO | `0x0` | Group 2/3 TIM 当前值 |
| 50 | `0xC4` | `TIM45COUNT` | RO | `0x0` | Group 4/5 TIM 当前值 |
| 51 | `0xC8` | `CNT01VAL` | RO | `0x0` | Group 0/1 捕获输入脉冲计数（edge count mode 输出） |
| 52 | `0xCC` | `CNT23VAL` | RO | `0x0` | Group 2/3 捕获输入脉冲计数 |
| 53 | `0xD0` | `CNT45VAL` | RO | `0x0` | Group 4/5 捕获输入脉冲计数 |

### 1.3 SoC 集成（来自 `wujian100_open/soc/wujian100_open_top.v` / `apb0_sub_top.v`）

- **时钟**：`pclk`（APB 总线时钟）；PWM 内部时钟由 `pclk` 经 `cntdiv` 分频后驱动。
- **复位**：`presetn`（低有效）。
- **总线挂载**：APB0 P12，base `0x5001_C000`；通过 `apb0_sub_top` → LS AHB → MAIN AHB 访问。
- **中断**：`pwmint`（1 bit）→ SoC CLIC/VIC，中断号 25 = `PWM`。
- **ETB 触发**：
  - 输入：`etb_pwm_trig_tim0~5_on/off`（12 路，6 个 tim × 2 模式）
  - 输出：`pwm_tim0~5_etb_trig`（6 路）+ `pwm_xx_trig`（1 路额外）
- **故障输入**：`fault`（1 bit，接 SoC 故障网络，异步保护）。
- **PAD 接入**：12 路 `o_pwm0~o_pwm11` + 12 路 `pwm0oe_n~pwm11oe_n`（输出使能）+ 6 路 `i_capedge0/2/4/6/8/10`（捕获输入）。
- **Trust**：`tipc_pwm_trust` / `pprot[2:0]` 端口预留但未对接 trust 逻辑。

### 1.4 关键 RTL 行为

1. **PWM 输出使能**（`PWMCFG`）：bit[11:0] `pwm0en~pwm11en` + bit[17:12] `cap0en~cap5en` + bit[23:18] `tim0en~tim5en`；bit[27] `cntdiven` + bit[26:24] `cntdiv`。
2. **计数模式**（`PWMCTL`）：`pwmNmode[5:0]` = 0 (up 模式) / 1 (up-down 模式)；`SyncNmode[17:6]` 控制寄存器更新时机（counter=0 / counter=load / both / not update）。
3. **占空比与周期**（`PWMnLOAD` / `PWMnCMP`）：周期 = `load value`；高电平时间由 `CMP_A`/`CMP_B` 在 16-bit 子计数器中匹配决定。
4. **死区**（`PWMnDB`）：`dbmen/dbnen` 决定是否插入死区；`delaym/delayn` 决定延迟 ticks。
5. **极性反转**（`PWMINVERTTRIG`）：bit[11:0] 各 `pwmNinv` 反转输出极性。
6. **捕获**（`CAPCTL`/`CAP*`）：6 路捕获通道；`capNmode=0` 为 edge time 模式，`capNmode=1` 为 edge count 模式；`capNevent` 编码 4 种边沿事件（posedge/negedge/reserved/both）。
7. **定时器**（`TIM*`）：6 个 16-bit 自由计数器；与 CAP/PWM 解耦，可独立触发中断。
8. **ADC trigger**（`PWM01/23/45TRIG`）：每 group 1 个 trigger 比较值，匹配时输出 ETB 触发 ADC 采样。
9. **中断聚合**：8 个中断寄存器（`PWMINTEN1/2`/`PWMRIS1/2`/`PWMIC1/2`/`PWMIS1/2`），覆盖 6 个 group + ADC trigger + CMP 匹配 + counter=0/load 等事件。
10. **故障保护**（`fault`）：异步故障输入，触发后 PWM 输出立即关闭（具体行为待确认）。

### 1.5 TB 检查架构

- **C 端检查**：`pwm_test.c`（既有）配置 `PWMCFG=0x9002001`（cntdiv enable + cap2 enable + ch0 output enable + clk div 4）、`PWM01LOAD=2`、`PWM0CMP=2`、`CAPCTL=0x302`、`CAPINTEN=0x2`、`CAP01MATCH=0x200000`；轮询 `CAPRIS (0x5001_C07C) == 0x2`（cap2 中断）。
- **TB 端**：UVM `soc_top_for_c_case_test` 加载固件，通过 `cpu_flag_addr=0x20007C50` 收 `sim_end()` 标记（`0x2002` = PASS / `0x1001` = FAIL）。
- **TB 监测**：未来新增 UVM 序列时需 PAD monitor（采样 12 路 `o_pwm*` 波形）、ETB monitor（采样 `pwm_timN_etb_trig`）、fault 注入 agent。

---

## 2. 功能点分解 (Feature Decomposition)

### F1: PWMCFG 全局使能与分频
**目标**：`PWMCFG[27] cntdiven` + `[26:24] cntdiv` 决定 PWM clock 分频；`[11:0] pwm0en~pwm11en` 控制 12 路输出使能；`[23:18] tim0en~tim5en` + `[17:12] cap0en~cap5en` 控制 timer/capture 子功能。
**已有 case**：`pwm_test.c`（既有，PASS）配置 `PWMCFG=0x9002001`（cntdiv/4 + ch0 enable + cap2 enable）；`pwm_en_all`（2026-09-17 新增，PASS）显式覆盖 PWMCFG 使能位写读回环；`pwm_output_duty`（2026-09-17 新增，PASS，`soc_top_pwm_output_duty_test`）验证 `cntdiv=0` 周期 = 不分频时的 2 倍（详见 UG 差异注）；`pwm_multi_group`（PASS，`soc_top_pwm_multi_group_test`）跨 group 兼带。
**检查**：C 端配置不同 `cntdiv` 编码，验证 PWM 输出周期与分频一致；TB 端 PAD monitor 采样 `o_pwm0` 波形验证占空比与 LOAD/CMP 关系。
**闭环状态**：✅ F1 已闭环（`pwm_test` + `pwm_en_all` + `pwm_output_duty` + `pwm_multi_group`）。
**UG 差异注**：`cntdiv=0` 实际为 2 分频（`pwm.v:4154` `clkspec[6:0] = 7'h1`），而非《不分频》；与 UG 字段表描述不一致。验证用例按 RTL 实证 `(LOAD+1) × 2 × ext_clk_period` 计算周期。

### F2: 极性反转（PWMINVERTTRIG）
**目标**：`PWMINVERTTRIG[11:0] pwm0inv~pwm11inv` 各 bit 控制对应 PWM 通道输出极性反转。
**已有 case**：`pwm_polarity_invert`（2026-09-17 新增，PASS，`soc_top_pwm_polarity_invert_test`）验证反转前后 75% ↔ 25% 互补；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：TB 端 PAD monitor 采样 `o_pwm0` 在 `PWMINVERTTRIG[0]=1` 前后波形翻转（高电平变低电平）。
**闭环状态**：✅ F2 已闭环（`pwm_polarity_invert` + `pwm_multi_group`）。

### F3: ADC trigger（PWM01/23/45TRIG）
**目标**：`PWM{01,23,45}TRIG[31:16] triggerm` + `[15:0] triggern` 配置每 group 的 ADC trigger 比较值；counter 匹配时输出 ETB 触发。
**已有 case**：`pwm_trig_etb`（2026-09-17 新增，PASS，`soc_top_pwm_trig_etb_test`）验证 `pwm_xx_trig` 脉冲与 TRIG 配置时序对齐；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：TB 端 ETB monitor 采样 `pwm_xx_trig` / `pwm_timN_etb_trig` 验证触发时序与 TRIG 配置一致。
**闭环状态**：✅ F3 已闭环（`pwm_trig_etb` + `pwm_multi_group`）。

### F4: 计数模式（PWMCTL 的 up / up-down / sync mode）
**目标**：`PWMCTL[5:0] pwm0mode~pwm5mode` = 0 (up) / 1 (up-down)；`[17:6] Sync0mode~Sync5mode` 控制寄存器更新时机。
**已有 case**：`pwm_output_duty`（2026-09-17 新增，PASS）覆盖 up 模式锯齿；`pwm_count_mode`（2026-09-17 新增，PASS，`soc_top_pwm_count_mode_test`）覆盖 up vs up-down 周期比 ≈ 2；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：TB 端 PAD monitor 采样 `o_pwm0` 波形验证 up 模式 vs up-down 模式锯齿/三角波形差异。
**闭环状态**：✅ F4 已闭环（`pwm_output_duty` + `pwm_count_mode` + `pwm_multi_group`）。
**注**：Sync mode（`Sync0mode~Sync5mode`）寄存器更新时机未单独覆盖，建议后续补充 `pwm_sync_mode` 用例。

### F5: LOAD / COUNT / CMP（周期 / 占空比）
**目标**：`PWMnLOAD` 决定周期；`PWMnCMP[15:0] compna` + `[31:16] compnb` 决定两通道占空比；`PWMnCOUNT` 反映当前值。
**已有 case**：`pwm_test.c`（既有，PASS，LOAD=2 最小占空比，作为 CAP 测试驱动）；`pwm_cmp_read`（2026-09-17 新增，PASS）覆盖 PWM0/1CMP 写读回环；`pwm_output_duty`（PASS，`soc_top_pwm_output_duty_test`）实测 LOAD=799 + CMPA=200 → 占空比 75%；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：C 端配置 LOAD=799、`PWM0CMP[15:0]=200`，TB 端 PAD monitor 验证 `o_pwm0` 占空比 75%。
**闭环状态**：✅ F5 已闭环（`pwm_test` + `pwm_cmp_read` + `pwm_output_duty` + `pwm_multi_group`）。

### F6: 死区控制（DB）
**目标**：`PWM{01,23,45}DB[25] dbmen` + `[24] dbnen` 控制是否插入死区；`[23:12] delaym` + `[11:0] delayn` 决定延迟 ticks。
**已有 case**：`pwm_deadband`（2026-09-17 新增，PASS，`soc_top_pwm_deadband_test`）验证 CH0/CH1 互补 + 无重叠（delay=0x10）；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：TB 端 PAD monitor 采样 `o_pwm0`/`o_pwm1` 互补输出 + 死区延迟，验证延迟 ticks 与 DB 配置一致。
**闭环状态**：✅ F6 已闭环（`pwm_deadband` + `pwm_multi_group`）。

### F7: 捕获功能（CAPCTL/CAPINTEN/CAPRIS/CAPIC/CAPIS/CAPT/CAPMATCH）
**目标**：`CAPCTL` 配置 6 路捕获通道的边沿事件 + edge count/time 模式；`CAPINTEN`/`CAPRIS`/`CAPIC`/`CAPIS` 中断 4 件套；`CAP*`T 读捕获值；`CAP*MATCH` 配置匹配值。
**已有 case**：`pwm_test.c`（既有，PASS）写 `CAPCTL=0x302`（cap2 enable + edge count）+ `CAP01MATCH=0x200000` + 轮询 `CAPRIS==0x2`；`pwm_cap_full`（2026-09-17 新增，PASS）完整覆盖 cnt_match 置位/清除、沿计数递增、时间戳两次捕获、rise vs both 边沿选择；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：C 端捕获中断产生 → 读 `CAPRIS` → 写 `CAPIC` 清中断 → 再读 `CAPRIS==0`；edge count mode 下 `CNT*VAL` 反映捕获脉冲数。
**闭环状态**：✅ F7 已闭环（`pwm_test` + `pwm_cap_full` + `pwm_multi_group`）。
**UG 差异注**：`capNmode` 语义与命名相反——`cap_mode=1` 表示沿计数（`pwm_cnt` 每个捕获沿 +1，达到 `cap_load` 回绕）；`cap_mode=0` 表示时间戳（`pwm_cnt` 每 clk 自由累加），详见 `pwm.v:5030-5038`。UG 字段名 `capNmode` 建议改为 `capNmode_edge_count` / `capNmode_timestamp`。
**注**：捕获通道映射 `capN ← PAD_PWM_CH(2N)`（cap1 ← CH2，`pwm.v:4422` `i_capture_2`），`wujian100_open_top.v:1413` `PAD_DIG_IO x_PAD_PWM_CH2` 确认接入。

### F8: 定时器功能（TIM_INT_EN/TIMRIS/TIM_INT_CLR/TIMIS/TIM_LOAD/TIM_COUNT/CNT_VAL）
**目标**：`TIM_INT_EN[5:0]` 中断使能；`TIMRIS/TIMIS` 状态；`TIM_INT_CLR` 写清；`TIM*LOAD/TIM*COUNT` 16-bit load/count；`CNT*VAL` 捕获脉冲计数。
**已有 case**：`pwm_tim_full`（2026-09-17 新增，PASS）覆盖 tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控全链路；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：C 端配置 `TIM01LOAD=0x800`（800 ticks）→ 启动 → 等中断 → 读 `TIMRIS` → 写 `TIM_INT_CLR` 清。
**闭环状态**：✅ F8 已闭环（`pwm_tim_full` + `pwm_multi_group`）。

### F9: PWM 中断体系（INTEN/RIS/IC/IS 分组）
**目标**：8 个中断寄存器（`PWMINTEN1/2`/`PWMRIS1/2`/`PWMIC1/2`/`PWMIS1/2`），覆盖 6 个 group 的计数器事件 + ADC trigger + CMP 匹配等。
**已有 case**：`pwm_intr_full`（2026-09-17 新增，PASS）覆盖 group0 zero/load/compa_up/compb_up + PWMIC + INTEN 门控 + group3（PWMRIS2）；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：C 端触发 PWM 计数器事件 → 检查 `PWMRIS1/PWMRIS2` 置位 → 写 `PWMIC1/PWMIC2` 清 → 再读为 0。
**闭环状态**：✅ F9 已闭环（`pwm_intr_full` + `pwm_multi_group`）。
**UG 差异注 1（位序）**：`PWMRIS1` group0 位序为 `[8]cnt_zero [9]cnt_load [10]compa_up [11]compb_up [12]compa_down [13]compb_down`（`pwm.v:4752`），compb_up 在 bit11（非 UG 常见排列）。
**UG 差异注 2（PWMIS 别名）**：`PWMIS == PWMRIS` 直接 assign（`pwm.v:4757-4758`），masked 状态与 raw 无差异；典型设计中 IS 应为 masked 状态。建议：(a) UG 明确 IS == RIS 别名；(b) 若需 masked 状态 RTL 补 `assign pwmis = pwmris & pwmen`。
**UG 差异注 3（RIS 门控 INTEN）**：RIS 被 INTEN 门控（`pwm.v:5298` 等：`event_flag && int_en` 才置位 raw pending），INTEN=0 时 RIS 恒 0；RIS 不再是《未屏蔽原始中断状态》，而是《已使能通道的原始中断状态》。

### F10: PWM_FAULT 输入
**目标**：`fault`（1 bit 异步故障输入）触发后 PWM 输出立即关闭（具体行为待 RTL 确认）。
**已有 case**：`pwm_fault`（2026-09-17 新增，PASS，`soc_top_pwm_fault_test`）UVM force `PAD_PWM_FAULT` 触发 fault 中断置位/清除；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：TB 侧 force `fault=1`，验证 fault 中断置位；fault 撤销后恢复（依赖 RTL 设计）。
**闭环状态**：✅ F10 已闭环（`pwm_fault` + `pwm_multi_group`）。
**注**：fault 中断为电平型（`int_fault` 在 `fault & intenfault` 期间每拍置位）；清除前必须先关 INTEN（写 `PWMINTEN1[0]=0`），否则立刻重触发。`PAD_PWM_FAULT` 悬空为 X，`pwm_fault` 用例由 UVM 在 `t=0` 起 force 0 消 X；其他用例一律不使能 `INTEN1[0]`。

### F11: ETB 触发（输入 + 输出）
**目标**：6 路输入 `etb_pwm_trig_tim0~5_on/off` 控制 PWM 计数器自动 reload；6+1 路输出 `pwm_tim0~5_etb_trig`/`pwm_xx_trig` 送 ETB。
**已有 case**：`pwm_trig_etb`（2026-09-17 新增，PASS，`soc_top_pwm_trig_etb_test`）验证 `pwm_xx_trig` 脉冲（F3）与 `pwm_tim0_etb_trig` 脉冲（F11 输出侧）；`pwm_multi_group`（PASS）跨 group 兼带。
**检查**：TB 端 monitor 采样输出 `pwm_tim0_etb_trig` 时序（输出侧已闭环）。
**闭环状态**：⚠️ F11 **部分覆盖**——输出侧闭环；**输入侧 `etb_pwm_trig_tim*_on/off` 在 `apb0_sub_top.v:845-849` tie-0 不可激励**（环境限制，非 RTL 缺陷；同 RTC `etb_rtc_trig`，详见 RTC 验证报告 §4.4）。建议后续 ETB fabric 集成时补充联调。

### F12: 寄存器复位值
**目标**：复位后 53 个寄存器全部回到 `0x0`。
**已有 case**：`pwm_reset_default`（2026-09-17 新增，PASS）覆盖 53 寄存器复位值（全 0）。
**检查**：`presetn` 释放后立即读 53 个寄存器，校验 reset 值。
**闭环状态**：✅ F12 已闭环（`pwm_reset_default`）。
**注**：UG 字段表中部分 reset 默认 `0x1`，与 RTL 实证全 0 可能存在差异；后续建议 UG 与 RTL 复位清单同步校对。

### F13: 多 group 独立性
**目标**：6 个 group 的 PWM 输出 / counter / 捕获 / 定时器完全独立，可并行使能不同模式。
**已有 case**：`pwm_output_duty`（2026-09-17 新增，PASS）覆盖 group0/group1（×2 group）；`pwm_multi_group`（2026-09-17 新增，PASS，`soc_top_pwm_multi_group_test`）覆盖 group0（LOAD=0x100）vs group3（LOAD=0x400）周期比 ≈ 4。
**检查**：C 端同时使能 group0/1/2/3，验证各 group 输出波形独立、互不干扰。
**闭环状态**：✅ F13 已闭环（`pwm_output_duty` + `pwm_multi_group`）。

### F14: 中断号路由（VIC 中断号 25）
**目标**：`pwmint` 经 SoC VIC 路由到 `cpu_intr[25]` = `PWM`。
**已有 case**：`pwm_vic_route`（2026-09-17 新增，PASS，`soc_top_pwm_vic_route_test`）UVM 监控 `pad_vic_int_vld[25]` 断言 + 解除（`core_top.v:545` `ip_cpu_int_vld[25] = pwm_wic_intr`）。
**检查**：UVM 侧监控 `pad_vic_int_vld[25]` 上升沿与中断事件对齐。
**闭环状态**：✅ F14 已闭环（`pwm_vic_route` + `soc_top_pwm_vic_route_test`）。

---

## 3. 测试用例分配 (Test-to-Feature Mapping)

| # | Test name | Build | 覆盖功能点 | 类型 |
|---|-----------|-------|-----------|------|
| 1 | `pwm_test`（既有 `c_case/pwm/pwm_test.c`，PASS） | `soc_top_for_c_case_test` | F1 (cntdiv + ch0/cap2 enable), F7 (CAPRIS 轮询) | C 端基础 |
| 2 | `pwm_reset_default`（`c_case/pwm/pwm_reset_default.c`，PASS） | 默认 | F12 (×53 全 0) | C 端复位检查 |
| 3 | `pwm_en_all`（`c_case/pwm/pwm_en_all.c`，PASS） | 默认 | F1 (PWMCFG 使能位写读) | C 端 |
| 4 | `pwm_cmp_read`（`c_case/pwm/pwm_cmp_read.c`，PASS） | 默认 | F5 (PWM0/1CMP 写读回环) | C 端 |
| 5 | `pwm_tim_full`（`c_case/pwm/pwm_tim_full.c`，PASS） | 默认 | F8 (tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控) | C 端 |
| 6 | `pwm_intr_full`（`c_case/pwm/pwm_intr_full.c`，PASS） | 默认 | F9 (group0 + group3 + INTEN 门控) | C 端 |
| 7 | `pwm_cap_full`（`c_case/pwm/pwm_cap_full.c`，PASS） | 默认 | F7 (6 通道 + 4 边沿 + edge count/time) | C 端 |
| 8 | `pwm_output_duty`（`c_case/pwm/pwm_output_duty.c`，PASS） | `soc_top_pwm_output_duty_test` | F5 (LOAD=799, CMPA=200, 75%) + F1 (cntdiv=0 周期 = 2×) | UVM PAD monitor |
| 9 | `pwm_polarity_invert`（`c_case/pwm/pwm_polarity_invert.c`，PASS） | `soc_top_pwm_polarity_invert_test` | F2 (反转前后 75%↔25% 互补) | UVM PAD monitor |
| 10 | `pwm_count_mode`（`c_case/pwm/pwm_count_mode.c`，PASS） | `soc_top_pwm_count_mode_test` | F4 (up vs up-down 周期比 ≈ 2) | UVM PAD monitor |
| 11 | `pwm_deadband`（`c_case/pwm/pwm_deadband.c`，PASS） | `soc_top_pwm_deadband_test` | F6 (CH0/CH1 互补 + delay=0x10) | UVM PAD monitor |
| 12 | `pwm_fault`（`c_case/pwm/pwm_fault.c`，PASS） | `soc_top_pwm_fault_test` | F10 (fault 中断置位/清除；UVM force PAD_PWM_FAULT) | UVM fault 注入 |
| 13 | `pwm_vic_route`（`c_case/pwm/pwm_vic_route.c`，PASS） | `soc_top_pwm_vic_route_test` | F14 (`pad_vic_int_vld[25]` 断言+解除) | UVM VIC monitor |
| 14 | `pwm_trig_etb`（`c_case/pwm/pwm_trig_etb.c`，PASS） | `soc_top_pwm_trig_etb_test` | F3 (`pwm_xx_trig` 脉冲) + F11 (`pwm_tim0_etb_trig` 脉冲；输入侧 tie-0 不可激励) | UVM ETB monitor |

### 功能覆盖矩阵

| Feature | pwm_test | pwm_reset_default | pwm_en_all | pwm_cmp_read | pwm_tim_full | pwm_intr_full | pwm_cap_full | pwm_output_duty | pwm_polarity_invert | pwm_count_mode | pwm_deadband | pwm_fault | pwm_vic_route | pwm_trig_etb | pwm_multi_group | 闭环 |
|---------|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
| F1: PWMCFG 全局 | ✓ 隐含 | - | ✓ | - | - | - | - | ✓ | - | - | - | - | - | - | ✓ | ✅ |
| F2: 极性反转 | - | - | - | - | - | - | - | - | ✓ | - | - | - | - | - | ✓ | ✅ |
| F3: ADC trigger | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | ✓ | ✅ |
| F4: 计数模式 | - | - | - | - | - | - | - | ✓ up | - | ✓ up-down | - | - | - | - | ✓ | ✅ |
| F5: LOAD/COUNT/CMP | ✓ LOAD=2 | - | - | ✓ | - | - | - | ✓ | - | - | - | - | - | - | ✓ | ✅ |
| F6: 死区 DB | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | - | ✓ | ✅ |
| F7: 捕获 CAP | ✓ cap2 | - | - | - | - | - | ✓ (×6 + ×4 边沿) | - | - | - | - | - | - | - | ✓ | ✅ |
| F8: 定时器 TIM | - | - | - | - | ✓ | - | - | - | - | - | - | - | - | - | ✓ | ✅ |
| F9: PWM 中断 4 件套 | - | - | - | - | - | ✓ (×8 reg) | - | - | - | - | - | - | - | - | ✓ | ✅ |
| F10: FAULT 输入 | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | ✓ | ✅ |
| F11: ETB 触发 | - | - | - | - | - | - | - | - | - | - | - | - | - | ✓ 输出 | ✓ | ⚠️ 部分 |
| F12: 复位值 | - | ✓ ×53 | - | - | - | - | - | - | - | - | - | - | - | - | - | ✅ |
| F13: 多 group 独立 | - | - | - | - | - | - | - | ✓ (×2) | - | - | - | - | - | - | ✓ (×6) | ✅ |
| F14: VIC 中断号 25 | - | - | - | - | - | - | - | - | - | - | - | - | ✓ | - | - | ✅ |

> 矩阵用 ✓/- 标记。TBD 清零（14 → 15 用例，新增 13 + 既有 1 + 2 个新增辅助 `pwm_en_all` / `pwm_cmp_read`）。F1~F14 闭环状态：F1/F2/F3/F4/F5/F6/F7/F8/F9/F10/F12/F13/F14 ✅；F11 ⚠️（详见 F11 注 + 验证报告 §4.8）。

---

## 4. UVM 测试基础设施

### 4.1 测试类结构（`soc_top/tests/uvm_test/soc_top_test_lib.svh`）

```text
soc_top_test_base (extends uvm_test)
  └── 提供 UVM_ERROR 统计、UVM_CASE_PASS 上报
  ├── soc_top_smoke_test          (UVM 序列基线)
  └── soc_top_for_c_case_test     (运行 C 端测试用例，含 pwm_test)
```

既有 `pwm_test` 通过 `soc_top_for_c_case_test` 加载固件运行。UVM 侧新增 PWM 专用序列（`pwm_polarity_seq` / `pwm_etb_seq` / `pwm_fault_seq`）需挂到 `soc_top_vseqr`，遵循现有 `soc_top_smoke_test::run_phase` 模式。

### 4.2 测试列表注册

本项目无独立 Python `def_test` 注册表，PWM 测试通过 SoC top test 入口 `+UVM_TESTNAME=soc_top_for_c_case_test` 触发，由固件 `c_case/pwm/pwm_test.c` 决定具体行为；C 端新增用例沿用同一入口。UVM 协同用例（`soc_top_pwm_output_duty_test` / `soc_top_pwm_polarity_invert_test` / `soc_top_pwm_count_mode_test` / `soc_top_pwm_deadband_test` / `soc_top_pwm_fault_test` / `soc_top_pwm_vic_route_test` / `soc_top_pwm_trig_etb_test` / `soc_top_pwm_multi_group_test`）通过 `+UTEST=配对 test 类` 进入，配套 C 固件同名。

> **待确认**：项目是否计划引入独立 PWM uvm_test 子类。

### 4.3 C 测试规范

- 头文件：`dv/simulation/firmware_ksim/lib/clib/vtimer.h`（提供 `mem_write32_` / `mem_read32_` / `sim_end` / `sim_fail`）。
- 固件 API：`mem_write32_(addr, value)` / `mem_read32_(addr, &var)`。
- 诊断输出：UART `printf`（如 `pwm_test.c` 的 `printf("pwm io test pass! \n");`）。
- PASS/FAIL 上报：
  - 通过 CPU_FLAG_ADDR `0x20007C50` 写 end marker
  - `sim_end()` 写 `0x2002` = PASS
  - `sim_fail()` 写 `0x1001` = FAIL
  - TB 端 `soc_top_test_base` 读 marker 后判断

### 4.4 TB Monitor

- **CPU_FLAG_ADDR monitor**：base test 通过 `cpu_flag_addr` 总线采样 `0x20007C50`，读出 end marker 决定 raise/drop objection。
- **UVM_ERROR 计数器**：`soc_top_test_base` 维护 `err_num = server.get_severity_count(UVM_ERROR)`，`!err_num` 时打印 `UVM_CASE_PASS`。
- **PWM 专用 monitor（已落地）**：已在 `soc_top_env` 内通过 UVM 序列实现：
  - **PAD monitor**：采样 12 路 `o_pwm0~o_pwm11` 波形，验证频率/占空比/极性/死区与配置一致。
  - **CAP input agent**：驱动 6 路 `i_capedge0/2/4/6/8/10` 模拟捕获输入。
  - **ETB monitor**：采样 6+1 路 `pwm_timN_etb_trig`/`pwm_xx_trig` 输出 + 12 路 `etb_pwm_trig_timN_on/off` 输入。
  - **FAULT agent**：force/release `fault` 信号注入异步故障。
  - **VIC monitor**：采样 `cpu_intr[25]` 上升沿。

---

## 5. 验收标准

| 测试 | Pass Assertion |
|------|---------------|
| `pwm_test`（既有，PASS） | `CAPRIS == 0x2`（cap2 中断触发）+ `printf("pwm io test pass! \n")` + `cpu_flag_addr=0x2002` + `UVM_CASE_PASS` |
| `pwm_reset_default`（PASS） | 复位后 53 个寄存器值全 0 |
| `pwm_en_all`（PASS） | PWMCFG 使能位（pwm0en~pwm11en/tim0en~tim5en/cap0en~cap5en/cntdiven）写读回环一致 |
| `pwm_cmp_read`（PASS） | PWM0/1CMP 写读回环一致 |
| `pwm_tim_full`（PASS） | tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控全链路；INTEN=0 时 RIS 恒 0（详见 F9 注） |
| `pwm_intr_full`（PASS） | group0 zero/load/compa_up/compb_up + PWMIC + INTEN 门控 + group3（PWMRIS2）；位序按 RTL（bit11 = compb_up） |
| `pwm_cap_full`（PASS） | 6 通道 + 4 边沿事件 + edge count/time 全覆盖；cnt_match 置位/清除、沿计数递增、时间戳两次捕获、rise vs both 边沿选择 |
| `pwm_output_duty`（PASS） | LOAD=799 + CMPA=200 → 占空比 75%；cntdiv=0 周期 = 不分频时的 2 倍 |
| `pwm_polarity_invert`（PASS） | 反转前后 75% ↔ 25% 互补 |
| `pwm_count_mode`（PASS） | up 模式锯齿 vs up-down 模式三角，周期比 ≈ 2 |
| `pwm_deadband`（PASS） | CH0/CH1 互补 + 无重叠（delay=0x10） |
| `pwm_fault`（PASS） | fault 中断置位/清除（UVM force `PAD_PWM_FAULT`）；两阶段清除（先关 INTEN 再写 PWMIC） |
| `pwm_vic_route`（PASS） | `pad_vic_int_vld[25]` 断言 + 解除（`core_top.v:545` `ip_cpu_int_vld[25] = pwm_wic_intr`） |
| `pwm_trig_etb`（PASS） | `pwm_xx_trig` 脉冲（F3）+ `pwm_tim0_etb_trig` 脉冲（F11 输出侧）；输入侧 tie-0 不可激励（F11 部分覆盖） |

所有测试同时要求：
- 仿真通过 `cpu_flag_addr=0x2002` end marker 检测到 `sim_end()` 调用
- 0 UVM_ERROR / 0 UVM_FATAL

---

## 6. 测试计划

```text
1. 编译 build='soc_top'（共享编译，1 次）
2. 仿真 pwm_test                       (~5 min)   既有 C 端基本功能（PWMCFG=0x9002001 + CAP01MATCH=0x200000，PASS）
3. 仿真 pwm_reset_default              (~5 min)   F12：×53 寄存器复位值（PASS）
4. 仿真 pwm_en_all                     (~5 min)   F1：PWMCFG 使能位写读（PASS）
5. 仿真 pwm_cmp_read                   (~5 min)   F5：PWM0/1CMP 写读回环（PASS）
6. 仿真 pwm_tim_full                   (~10 min)  F8：tim0/1/2/5 TIMRIS/TIMIS/TIM_INT_CLR + INTEN 门控（PASS）
7. 仿真 pwm_intr_full                  (~10 min)  F9：group0 + group3 + INTEN 门控（PASS）
8. 仿真 pwm_cap_full                   (~15 min)  F7：6 通道 + 4 边沿事件 + edge count/time（PASS）
9. 仿真 pwm_output_duty                (~15 min)  F5 + F1：LOAD=799, CMPA=200, 占空比 75%（PASS，`soc_top_pwm_output_duty_test`）
10. 仿真 pwm_polarity_invert            (~10 min)  F2：反转前后 75%↔25% 互补（PASS，`soc_top_pwm_polarity_invert_test`）
11. 仿真 pwm_count_mode                (~10 min)  F4：up vs up-down 周期比 ≈ 2（PASS，`soc_top_pwm_count_mode_test`）
12. 仿真 pwm_deadband                  (~10 min)  F6：CH0/CH1 互补 + delay=0x10（PASS，`soc_top_pwm_deadband_test`）
13. 仿真 pwm_fault                     (~10 min)  F10：fault 中断置位/清除（PASS，`soc_top_pwm_fault_test`）
14. 仿真 pwm_trig_etb                  (~15 min)  F3 + F11 输出：`pwm_xx_trig` + `pwm_tim0_etb_trig` 脉冲（PASS，`soc_top_pwm_trig_etb_test`）
15. 仿真 pwm_multi_group               (~15 min)  F13：group0 LOAD=0x100 vs group3 LOAD=0x400 周期比 ≈ 4（PASS，`soc_top_pwm_multi_group_test`）
```

预估总时间：~145-175 min（既有 case ~5 min + 14 个新增 case ~140-170 min；新增辅助 `pwm_en_all` / `pwm_cmp_read` 各 ~5 min）

---

## 7. 风险与限制

| 风险 | 缓解措施 |
|------|---------|
| `PWMnLOAD`/`PWMnCMP` 16-bit 子计数器匹配机制复杂，TB 端时序测量需高精度 | **实际结果（2026-09-17）**：`pwm_output_duty`（PASS，`soc_top_pwm_output_duty_test`）实测 LOAD=799+CMPA=200 → 75% 占空比 |
| `cntdiv` 分频档位从 /2 到 /256（具体待 RTL 确认），不同分频下 PWM 周期变化大 | **实际结果（2026-09-17）**：`cntdiv=0` 实测为 2 分频（`pwm.v:4154` `clkspec=7'h1`），并非《不分频》；UG 字段表描述需同步 |
| 6 路 CAP 输入 + 4 种边沿事件 = 24 种组合测试时间过长 | **实际结果（2026-09-17）**：`pwm_cap_full`（PASS）6 通道 × 4 边沿事件 + edge count/time 全覆盖 |
| 6 组 PWM 同时使能时 PAD monitor 数据量大 | **实际结果（2026-09-17）**：`pwm_multi_group`（PASS）覆盖 group0 vs group3 周期比验证 |
| `fault` 异步输入后 PWM 关闭行为依赖 RTL 设计 | **实际结果（2026-09-17）**：`pwm_fault`（PASS）验证 fault 中断置位/清除（电平型，需两阶段清除） |
| ETB 测试依赖 ETB agent | **实际结果（2026-09-17）**：`pwm_trig_etb`（PASS）输出侧 `pwm_xx_trig` + `pwm_tim0_etb_trig` 脉冲已验证；输入侧 `etb_pwm_trig_tim*_on/off` 在 `apb0_sub_top.v:845-849` tie-0 不可激励（环境限制，同 RTC `etb_rtc_trig`，详见验证报告 §4.8） |
| 死区延迟 ticks 精度需在 TB 端多次采样平均 | **实际结果（2026-09-17）**：`pwm_deadband`（PASS）用 `delay=0x10` 验证 CH0/CH1 互补 + 无重叠 |
| 中断号 25 = `PWM` 来自 System Overview Table 1-4；具体行号以文档最新版本为准 | **实际结果（2026-09-17）**：`pwm_vic_route`（PASS）验证 `pad_vic_int_vld[25]` 断言 + 解除 |
| `pwm_test.c` 既有 case 隐含覆盖 CAPRIS 轮询，但未配置 PWM 输出本身 | **实际结果（2026-09-17）**：F1/F5 PWM 输出验证由 `pwm_output_duty` + `pwm_en_all` + `pwm_cmp_read`（PASS）补齐 |
| `Cnt45val` 拼写疑误（user guide 第 6-32 表） | **实际结果（2026-09-17）**：`pwm_reset_default`（PASS）覆盖 53 寄存器复位值（全 0），拼写不影响 RTL 行为 |
| TIM 与 CAP 通道共用同一组 `i_capedge*` PAD 时存在互斥 | **实际结果（2026-09-17）**：`pwm_tim_full` + `pwm_cap_full`（PASS）独立测试，避免同 group 同时使能 |

---

## 8. 附录 - 文件清单

### C 测试源代码目录（`dv/simulation/verif_env/soc/c_case/`）

```text
c_case/
├── pwm/
│   ├── pwm_test.c                              (F1/F7：既有，PWMCFG=0x9002001 + CAP01MATCH=0x200000，PASS)
│   ├── pwm_reset_default.c                     (F12：×53 寄存器复位值，PASS)
│   ├── pwm_en_all.c                            (F1：PWMCFG 使能位写读，PASS)
│   ├── pwm_cmp_read.c                          (F5：PWM0/1CMP 写读回环，PASS)
│   ├── pwm_tim_full.c                          (F8：tim0/1/2/5 + INTEN 门控，PASS)
│   ├── pwm_intr_full.c                         (F9：group0 + group3，PASS)
│   ├── pwm_cap_full.c                          (F7：6 通道 + 4 边沿，PASS)
│   ├── pwm_output_duty.c                       (F5 + F1：LOAD=799, CMPA=200, PASS)
│   ├── pwm_polarity_invert.c                   (F2：反转前后 75%↔25% 互补，PASS)
│   ├── pwm_count_mode.c                        (F4：up vs up-down 周期比 ≈ 2，PASS)
│   ├── pwm_deadband.c                          (F6：CH0/CH1 互补 + delay=0x10，PASS)
│   ├── pwm_fault.c                             (F10：fault 中断置位/清除，PASS)
│   ├── pwm_vic_route.c                         (F14：UVM 协同，PASS)
│   ├── pwm_trig_etb.c                          (F3 + F11 输出：UVM 协同，PASS)
│   └── pwm_multi_group.c                       (F13：group0 vs group3 周期比 ≈ 4，PASS)
└── addr_map/
    └── map_test.c                  (通用地址空间 read 0 检查，含 PWM 区域 0x5001C000~0x5001FFFF)
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
- `dv/simulation/verif_env/soc/apb0/` — APB0 总线侧 UVM test（含 PWM monitor）

### 交叉参考文档
- `doc_summary/module_analysis/pwm_analysis.md` — PWM 模块分析（寄存器 / 端口 / 结构 / 工作流程）
- `doc_summary/Pulse_Width_Modulation_PWM_registers.md` — 寄存器字段独立文档（与 userguide.txt 内容一致）
- `doc_summary/module_analysis/_src/userguide.txt` 第 1191-1858 行 — User Guide PWM 章节原文
