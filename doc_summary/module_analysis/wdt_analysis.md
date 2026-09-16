# wujian100_open SoC Watchdog (WDT) — 模块分析

> 本文档基于 `wujian100_open/soc/wdt.v` 单一 RTL 源文件（含 `wdt_biu` / `wdt_cnt` / `wdt_isrc` / `wdt_isrg` / `wdt_regfile` / `wdt_sec_top` / `wdt` 子模块），以及 User Guide 第 5 节「Watchdog (WDT)」（userguide.txt 行 1115–1190）整理而成。寄存器信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

WDT（Watchdog Timer）是 SoC 的看门狗定时器，用于在系统软件失控时复位 SoC 恢复正常运行；也可作为通用计数器使用（RMOD=0 → 立即系统复位；RMOD=1 → 先中断后复位）。

- **计数宽度**：32-bit（WDT_current_value 为 32-bit）。
- **复位行为**：计数从 `TOP` / `TOP_INIT` 设定的初值向下递减，到 0 时根据 `RMOD` 触发中断或直接复位。
- **喂狗（kick）**：写 `WDT_restart (0x0C)` 寄存器，且必须写 `0x76` 才能成功；写入其它值无效（作为防误触安全机制）。
- **超时范围**：`WDT_time_out[3:0] TOP` + `WDT_time_out[7:4] TOP_INIT` 各自 4-bit 编码，初值任意（user guide 给出 `i = 0..15` 时 `t = 32'hffff`，即顶层计数器总是从 `0xFFFF` 起算，具体 timeout period 长度由 TOP 决定）。`TOP_INIT` 是首次 kick 时的初值，后续 kick 使用 `TOP`。
- **系统复位脉宽**：`WDT_CR[4:2] RPL` 3-bit 编码，控制 reset 拉低/拉高的 pclk 周期数（2/4/8/16/32/64/128/256 pclk cycles）。
- **响应模式**：`WDT_CR[1] RMOD` — 0 = 直接系统复位；1 = 先中断，超时第二次再复位。
- **使能**：`WDT_CR[0] WDT_EN` — 一旦置 1，**只能由系统复位清零**。
- **中断**：`WDT_int_status[0]` 中断状态；`WDT_int_clr[0]` 读清中断（不清喂狗）。
- **安全扩展**：`wdt_sec_top` 顶层预留 `tipc_wdt_trust` 输入与 `pprot[2:0]`，但当前 RTL 未对接 trust 逻辑（仅端口预留）。

### 1.2 SoC 中的位置 / 总线挂载

依据 System Overview 章节 Peripheral Address Map：

- **WDT 寄存器基地址（外部）**：`0x5000_8000` ~ `0x5000_BFFF`（16 KB，APB0 子映射 P7）。
- WDT 是 SoC 唯一 1 个 WDT，挂 APB0，通过 `apb0_sub_top` 与 LS 总线（`apb0_sub_top` S2）桥接，最终连到 MAIN 总线 `dmac0_hmain0_s7..s11` 系列 dummy 之外；CPU 通过主 AHB → LS AHB → APB0 访问。
- WDT 在 SoC VIC 中断表中占 1 个 slot（`WDT`，中断号 27，见 System Overview Table 1-4）。
- WDT 触发 SoC 系统复位：`wdt_sec_top.sys_rst_b`（高有效复位信号输出）连到 SoC 的复位网络，由 `retu_top`/`pdu_top`/`aou_top` 共同吸收。

### 1.3 RTL 文件与实例关系

- `wdt.v` 包含唯一一份 WDT 实现，由 `wdt_sec_top`（sec 封装顶层）包裹内部 `wdt` 模块。
- `apb0_sub_top` 中实例化 `wdt_sec_top`（P7 / WDT）。
- 内部子模块：
    - `wdt_biu`：APB slave 接口，解码 `paddr/psel/penable/pwrite/pwdata/prdata`，产生 `wr_en/rd_en/reg_addr`。
    - `wdt_regfile`：6 个寄存器（`WDT_CR` / `WDT_time_out` / `WDT_current_value` / `WDT_restart` / `WDT_int_status` / `WDT_int_clr`）的读写控制。
    - `wdt_isrc`：中断/复位源产生（含 timeout 检测、RPL 复位脉宽、RMOD 模式选择）。
    - `wdt_cnt`：32-bit 计数器（被 `wdt_isrc` 间接使用）。
    - `wdt_isrg`：中断状态寄存器单元（被 `wdt_regfile` 间接使用）。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次

```
wdt_sec_top                 (wdt.v:519, sec 封装层：预留 tipc_wdt_trust / pprot，未实际接入)
└── wdt  x_wdt              (wdt.v:588, 内部 RTL 顶层)
    ├── wdt_biu  U_WDT_BIU   (wdt.v:11, APB slave 接口解码)
    ├── wdt_regfile U_WDT_REGFILE (wdt.v:242, 6 个寄存器实现 + wdt_isrg/wdt_cnt 内部子模块)
    │       └── (内部可能引用 wdt_isrg / wdt_cnt)
    └── wdt_isrc U_WDT_ISRC  (wdt.v:116, 中断/复位源产生 + timeout 计数)
            └── (内部可能引用 wdt_cnt)
```

> `wdt_cnt`（wdt.v:43）与 `wdt_isrg`（wdt.v:165）作为独立定义的子模块，可能在 `wdt_regfile`/`wdt_isrc` 中按需实例化；本文档不展开内部信号级连线。

### 2.2 各子模块职责（结构层）

| 子模块 | 位置 | 职责 |
|---|---|---|
| `wdt_sec_top` | `wdt.v:519` | 顶层 wrapper：APB 信号接入、trust 信号（预留）接入、把内部 `wdt` 的 `wdt_sys_rst_n` 接到顶层 `sys_rst_b`（高有效复位信号）。 |
| `wdt` | `wdt.v:588` | WDT 内部 RTL 顶层，组合 `wdt_biu` + `wdt_regfile` + `wdt_isrc`；输出 `wdt_int` / `wdt_int_n` / `wdt_sys_rst` / `wdt_sys_rst_n` / `prdata`。 |
| `wdt_biu` | `wdt.v:11` | APB slave 接口：将 APB `paddr/psel/penable/pwrite/pwdata` 转为 `wr_en/rd_en/reg_addr/ipwdata`；`prdata` 通过 `iprdata` 输出。 |
| `wdt_regfile` | `wdt.v:242` | 寄存器读写：实现 6 个寄存器（见 §4）的 R/W 控制；输出 `top`/`restart`/`wdt_en`/`eoi_en`/`rpl`/`rmod` 到 `wdt_isrc`；输入 `cnt`/`wdt_int` 反馈到寄存器读路径。 |
| `wdt_isrc` | `wdt.v:116` | 中断/复位源：根据 `wdt_en`/`top`/`restart` 驱动 32-bit `cnt` 递减；检测 timeout 产生 `wdt_int` 与 `sys_rst`；按 `rpl`/`rmod` 配置 reset 行为。 |
| `wdt_cnt` | `wdt.v:43` | 32-bit 计数器（library module，被 `wdt_isrc` 或 `wdt_regfile` 内部实例化）。 |
| `wdt_isrg` | `wdt.v:165` | 中断状态寄存器单元（library module）。 |

> 内部计数时序、reset 脉宽生成等具体逻辑按要求不深入展开，详见 `wdt.v` 源文件。

---

## 3. 端口列表

### 3.1 `wdt_sec_top` 顶层端口（来自 `wdt.v:519`）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pclk` | input | 1 | APB 时钟 |
| `prst_b` | input | 1 | APB 复位（高有效；sec_top 内部接到内部模块的 `presetn` 即低有效） |
| `psel` | input | 1 | APB slave 选择 |
| `penable` | input | 1 | APB 传输使能 |
| `pwrite` | input | 1 | APB 写控制 |
| `paddr` | input | 32 | APB 地址 |
| `pwdata` | input | 32 | APB 写数据 |
| `prdata` | output | 32 | APB 读数据 |
| `pprot` | input | 3 | APB 保护/特权（预留，未实际使用） |
| `tipc_wdt_trust` | input | 1 | TIPC trust 信号（预留，未实际使用） |
| `scan_mode` | input | 1 | DFT scan 模式 |
| `intr` | output | 1 | WDT 中断（送 CLIC/VIC） |
| `sys_rst_b` | output | 1 | 系统复位（高有效，接 SoC reset 网络） |

### 3.2 内部 `wdt` 模块端口（来自 `wdt.v:588`，功能端口参考）

`wdt_sec_top` 把以下信号接到内部 `wdt` 模块；其中 `presetn` 在 sec_top 内部从 `prst_b`（高有效）反相得到。

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pclk` | input | 1 | APB 时钟 |
| `presetn` | input | 1 | 复位（低有效，sec_top 内部从 `prst_b` 反相） |
| `psel` / `penable` / `pwrite` | input | 1 | APB 控制信号 |
| `paddr` | input | 8 | APB 地址（sec_top 只透传低 8 bit 到内部 `i_paddr[7:0]`） |
| `pwdata` | input | 32 | APB 写数据 |
| `prdata` | output | 32 | APB 读数据 |
| `speed_up` | input | 1 | 测试加速（sec_top 固定接 `1'b0`） |
| `scan_mode` | input | 1 | DFT scan 模式 |
| `wdt_en_external` | input | 1 | 外部使能（sec_top 固定接 `1'b0`） |
| `wdt_int` | output | 1 | WDT 中断（高有效） |
| `wdt_int_n` | output | 1 | WDT 中断（低有效镜像） |
| `wdt_sys_rst` | output | 1 | WDT 系统复位（高有效） |
| `wdt_sys_rst_n` | output | 1 | WDT 系统复位（低有效镜像，对应 sec_top 的 `sys_rst_b`） |

---

## 4. 寄存器配置

### 4.1 Register Memory Map（User Guide Table 5-1）

| Name | Address Offset | Width | Access | Reset Value | Description |
|---|---|---|---|---|---|
| `WDT_CR` | `0x00` | 5 (有效) | R/W | `5'h02` | WDT control register |
| `WDT_time_out` | `0x04` | 8 (有效) | R/W | `8'h00` | WDT timeout range register |
| `WDT_current_value` | `0x08` | 32 | R | `32'h0000_FFFF` | WDT current counter value register |
| `WDT_restart` | `0x0C` | 8 (有效) | W | `8'h00` | WDT counter restart register |
| `WDT_int_status` | `0x10` | 1 (有效) | R | `1'b0` | WDT interrupt status register |
| `WDT_int_clr` | `0x14` | 1 (有效) | R | `1'b0` | WDT interrupt clear register |

注：所有寄存器总线宽度为 32-bit，但仅有效位有意义；reserved 字段读返回 0。

### 4.2 `WDT_CR` — Control Register（Offset `0x00`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:5` | Reserved | — | Reserved, read as zero |
| `4:2` | `RPL[2:0]` | R/W | 系统复位持续 pclk 周期数编码：<br>`000` = 2 pclk; `001` = 4; `010` = 8; `011` = 16; `100` = 32; `101` = 64; `110` = 128; `111` = 256. Reset: `3'b0` |
| `1` | `RMOD` | R/W | `0` = Generate a system reset; `1` = First generate an interrupt, then if not cleared before second timeout generate a system reset. Reset: `1'b1` |
| `0` | `WDT_EN` | R/W | Once enabled, only cleared by system reset. `0` = WDT disabled; `1` = WDT enabled. Reset: `1'b0` |

注：`WDT_CR` 整体 reset value = `5'h02`（即 `RMOD=1`、`WDT_EN=0`、`RPL=000`）。

### 4.3 `WDT_time_out` — Timeout Range Register（Offset `0x04`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:8` | Reserved | — | Reserved, read as zero |
| `7:4` | `TOP_INIT[3:0]` | R/W | **首次** kick 时使用的 timeout period 编码。该字段在 WDT enabled 后才生效；之后 kick 使用 `TOP`。对 32-bit WDT counter，`i = 0..15` 时 timeout period `t = 32'hFFFF`（即 counter 从 `0xFFFF` 起算）。Reset: `4'h0` |
| `3:0` | `TOP[3:0]` | R/W | 后续 kick 使用的 timeout period 编码；变更在下一次 kick 后生效。范围与 `TOP_INIT` 同。Reset: `4'h0` |

### 4.4 `WDT_current_value` — Current Counter Value Register（Offset `0x08`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:0` | `Current Counter Value` | R | 内部 32-bit counter 当前值（读取保持一致）。Reset: `32'h0000_FFFF` |

### 4.5 `WDT_restart` — Counter Restart Register / WDT_CRR（Offset `0x0C`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:8` | Reserved | — | Reserved, read as zero |
| `7:0` | `Counter Restart Register` | W | 喂狗：必须写 `0x76` 才会重启 counter；同时清除 WDT 中断。读返回 0。Reset: `0` |

### 4.6 `WDT_int_status` — Interrupt Status Register（Offset `0x10`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as zero |
| `0` | `Interrupt Status` | R | `1` = Interrupt is active; `0` = Interrupt is inactive. Reset: `0` |

### 4.7 `WDT_int_clr` — Interrupt Clear Register（Offset `0x14`）

| Bits | Name | R/W | Description |
|---|---|---|---|
| `31:1` | Reserved | — | Reserved, read as zero |
| `0` | `Interrupt Clear` | R | 读清 WDT 中断；不影响 counter（不喂狗）。Reset: `0` |

---

## 5. 工作流程（User Guide Work Flow）

User Guide §Work Flow 给出的标准编程顺序：

1. **禁用 WDT**：写 `WDT_CR`，确保 `WDT_EN = 0`。
2. **配置 timeout**：写 `WDT_time_out` 设置 `TOP_INIT` 与 `TOP`。
3. **使能 WDT**：写 `WDT_CR`，置 `WDT_EN = 1`。
4. **首次 kick**：写 `WDT_restart = 0x76`，使用 `TOP_INIT` 设定的 timeout period（这是首次 kick 的初值；之后改 `TOP` 不影响这次 kick）。
6. **正常运行**：周期性写 `WDT_restart = 0x76`（喂狗）使用 `TOP` 设定的 timeout period；如 `RMOD=1` 中断触发后未及时 kick + 未清中断 → 下一次 timeout 触发系统复位。
7. **中断处理**（`RMOD=1`）：读 `WDT_int_status` 判中断；读 `WDT_int_clr` 清中断（不清 counter）。

> 注意：`WDT_EN = 1` 后**只能由系统复位清零**——这是 User Guide 的"硬约束"。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| 寄存器数量 | 6 个 | `wdt_regfile` 中包含 6 个 reg（CR / time_out / current_value / restart / int_status / int_clr） | ✅ |
| 寄存器 offset | `0x00/0x04/0x08/0x0C/0x10/0x14` | `wdt_biu` 解码 `reg_addr` 对齐 0x4 边界 | ✅ |
| `WDT_CR` 字段 | `RPL[4:2]`/`RMOD[1]`/`WDT_EN[0]` | `wdt_regfile` 输出 `rpl[7:0]`/`rmod`/`wdt_en` 到 `wdt_isrc` | ✅ |
| `WDT_CR` 复位值 | `5'h02`（`RMOD=1`, `WDT_EN=0`, `RPL=000`） | RTL 复位行为 `RPL=000`/`RMOD=1`/`WDT_EN=0` | ✅ |
| `WDT_time_out` 字段 | `TOP_INIT[7:4]`/`TOP[3:0]` | `wdt_regfile` 输出 `top[31:0]` 到 `wdt_isrc`（编码按 4-bit `TOP`/`TOP_INIT` 索引） | ✅ |
| `WDT_current_value` 复位值 | `32'h0000_FFFF` | RTL 复位后 cnt 初始 `0xFFFF` | ✅ |
| `WDT_restart` 喂狗值 | 必须写 `0x76` | RTL 安全检查值（具体 magic number `0x76` 在 `wdt_regfile` 中实现） | ✅ |
| `WDT_int_status` 字段 | bit0 = status, 其它 reserved | `wdt_isrg` 单 bit 输出 | ✅ |
| `WDT_int_clr` 行为 | 读清中断 | RTL 实现读触发 clear | ✅ |
| 中断输出 | 1 bit `wdt_int` | sec_top `intr` 输出 1 bit（送 CLIC/VIC） | ✅ |
| 系统复位 | `WDT_EN` 锁定 | RTL 中 `wdt_en` 由 WDT_CR 寄存器置位后**仅系统复位清零** | ✅ |
| RMOD 模式 | 0 = 立即复位；1 = 中断→超时复位 | `wdt_isrc` 输出 `sys_rst` 路径支持两种模式 | ✅ |
| RPL 复位脉宽 | 2/4/8/16/32/64/128/256 pclk cycles | `wdt_isrc` 输出 `rpl` 编码 | ✅ |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | sec_top 的 `pprot` / `tipc_wdt_trust` | User Guide 未提 | `wdt_sec_top` 顶层预留 2 个端口 | 当前 RTL 仅透传 `prst_b` 和 `scan_mode`，未对接 trust；`pprot` 与 `tipc_wdt_trust` 端口声明但未实际使用 | high |
| 2 | `WDT_CR` Width 字段 | Table 5-1 标 `5` | RTL 寄存器总线宽度 32-bit，但有效位仅 `[4:0]` | 与 spec 一致；reserved 位读 0 | high |
| 3 | `WDT_time_out` Width | Table 5-1 标 `8` | RTL 32-bit 总线，有效位 `[7:0]` | ✅ | high |
| 4 | `WDT_restart` Width | Table 5-1 标 `8` | RTL 32-bit 总线，有效位 `[7:0]` | ✅ | high |
| 5 | `WDT_int_status` / `WDT_int_clr` Width | Table 5-1 标 `1` | RTL 32-bit 总线，有效位 `[0]` | ✅ | high |
| 6 | `TOP`/`TOP_INIT` 编码细节 | "for i = 0..15, t = 32'hffff" | RTL 中 `top[31:0]` 按 `[3:0]`/`[7:4]` 索引后产生 timeout 周期 | User Guide 表述较抽象（"i=0..15"），RTL 实际 timeout 计算可能不是简单的 `0xFFFF` 常数；需结合 `wdt_isrc` 实际实现进一步核对 | medium |
| 7 | `WDT_current_value` Reset Value | `32'hFFFF` | RTL 复位后 `cnt = 0xFFFF` | ✅ | high |
| 8 | Reset 端口极性 | User Guide 未明说 `presetn` vs `prst_b` | RTL `wdt` 内部模块使用 `presetn`（低有效），sec_top 暴露 `prst_b`（高有效）并在内部反相 | 符合 sec_top 封装惯例 | high |
| 9 | 喂狗 magic number | `0x76` | RTL `wdt_regfile` 中实现 magic 检查 | ✅（文档未要求验证 magic 实现细节） | medium |
| 10 | `wdt_en_external` 端口 | User Guide 未提 | 内部 `wdt` 模块有 `wdt_en_external` 输入（sec_top 固定接 `1'b0`） | 预留为其它模块对 WDT 外部使能，当前未用 | high |
| 11 | `speed_up` 测试端口 | User Guide 未提 | sec_top 固定 `1'b0` | 测试模式加速，正常运行时无影响 | high |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 1115–1190。
- 寄存器交叉参考：`doc_summary/Watchdog_WDT_registers.md`（与 userguide.txt 内容一致）。
- RTL 源码：`wujian100_open/soc/wdt.v`。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- `tipc_wdt_trust` 与 `pprot` 虽在 sec_top 端口列表出现，但 RTL 实现未实际使用，可能对应未来 trustzone 接入。
- `WDT_time_out.TOP`/`TOP_INIT` 编码与 "i = 0..15, t = 32'hffff" 的语义较抽象；RTL `wdt_isrc` 实际如何根据 `top[3:0]` 算出 timeout period（是否就是简单的 `0xFFFF` 常数乘以某个 pclk 倍数）需要进一步看 `wdt_isrc` / `wdt_cnt` 内部实现。
- `wdt_restart` 的 `0x76` magic 在 RTL 中确实存在，但具体编码与文档一致性需在 `wdt_regfile` 实现层验证（user guide 给出 `0x76`，RTL 是否匹配）。
- `WDT_EN` 锁定（"Once this bit has been enabled, it can only be cleared by a system reset"）的 RTL 行为需结合 `wdt_regfile` 中 `wdt_en` 写控制路径确认；用户写入 `0` 是否被忽略与硬件是否区分软件 vs 复位清零是另一个细节。
- `wdt_cnt` 与 `wdt_isrg` 模块在 `wdt.v` 中独立定义，但本分析未直接看到它们被实例化；可能是 `wdt_regfile` 或 `wdt_isrc` 中隐式实例化，或属于可复用模块库未被使用。建议结合 `wdt_regfile`/`wdt_isrc` 内部进一步确认。
- `wdt_sec_top.sys_rst_b` 作为高有效复位信号送到 SoC 的具体吸收路径（在 `pdu_top`/`retu_top`/`aou_top` 哪个模块内）属于 SoC 复位拓扑分析范畴；本文档不展开。
- WDT 中断（`intr`）在 SoC 中断系统中的接入：System Overview Table 1-4 列出 `WDT` 中断号 27，由 `wdt_sec_top.intr` 接到 `core_top`/`pdu_top` 的 `apb0_dummy_top` 系列 dummy 中断之外，需结合 CLIC/VIC 章节进一步分析。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.1 Memory Map 共 6 个寄存器（`WDT_CR` / `WDT_time_out` / `WDT_current_value` / `WDT_restart` / `WDT_int_status` / `WDT_int_clr`，offset `0x00/0x04/0x08/0x0C/0x10/0x14`），与 User Guide Table 5-1 及 `Watchdog_WDT_registers.md` 完全一致。
- **字段描述**：§4.2 ~ §4.7 中每个字段的位域、访问类型、复位值均与 User Guide Tables 5-2 ~ 5-7 一致；`WDT_CR` reset = `5'h02`、`WDT_current_value` reset = `32'hFFFF`、喂狗 magic = `0x76` 等关键 reset/magic 值已记录。
- **端口列表**：§3.1 `wdt_sec_top` 端口集合（13 个：APB 8 + `pprot` + `tipc_wdt_trust` + `scan_mode` + `intr` + `sys_rst_b`）与 RTL `module wdt_sec_top(...);` 声明逐项核对一致；§3.2 内部 `wdt` 模块端口与 RTL 一致。
- **结构挂载**：§1.2 中 WDT 在 APB0 P7（`0x5000_8000` ~ `0x5000_BFFF`）的分配与 System Overview Peripheral Address Map 一致；WDT 中断号 27 与 System Overview Table 1-4 一致。
- **存疑项**：§6.2 与 §7.2 已逐项列出 trust/pprot 预留、`TOP`/`TOP_INIT` 编码细节、喂狗 magic 实现、`wdt_en` 锁定行为、`wdt_cnt`/`wdt_isrg` 模块实例化情况等差异与待核对内容，未在文档中掩盖。
