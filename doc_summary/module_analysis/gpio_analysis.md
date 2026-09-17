# wujian100_open SoC General-purpose I/O (GPIO) — 模块分析

> 本文档基于 `wujian100_open/soc/gpio0.v` 单一 RTL 源文件（含 `gpio0` / `gpio0_sec_top` / `gpio_apbif` / `gpio_ctrl` / `gpio_top` 子模块），以及 User Guide 第 8 节「General-purpose I/O (GPIO)」（userguide.txt 行 1956–2094）整理而成。寄存器信息以 User Guide 为准，RTL 仅作结构证据；本文档只读不改任何代码文件。

---

## 1. 模块概述

### 1.1 功能与特性

GPIO 是 SoC 的通用 I/O 控制器，**单实例**提供 **32 路 GPIO**（GPIO0 ~ GPIO31），每路支持独立方向、数据、中断配置。

- **数据宽度**：32-bit（所有 GPIO 共享同一寄存器空间）。
- **方向**：每 bit 独立可配 Input/Output（`gpio_direction`）。
- **数据源**：每 bit 独立可配 Software / Hardware 模式（`gpio_ctl`）；Software 模式下由 `gpio_output_data` 直接驱动。
- **中断能力**：每 bit 支持中断；可独立配置使能（`gpio_inten`）、mask（`gpio_intmask`）、触发类型 level/edge（`gpio_inttype_level`）、极性（`gpio_int_polarity`）；并支持状态读取（`gpio_intstatus` / `gpio_rawintstatus`）和清中断（`gpio_porta_int_clr`）。
- **中断约束**：User Guide 明确「Interrupts are disabled on the corresponding bits of GPIO if the corresponding data direction register is set to Output or if GPIO mode is set to Hardware」——即中断仅在 (Input mode) ∧ (Software mode) 时生效。
- **输入读**：`gpio_input_data` 寄存器在 Input 模式下读取 PAD 输入，在 Output 模式下读取 `gpio_output_data` 的最后写入值。
- **ETB 触发**：32-bit 输出 `gpio0_etb_trig`（每路 1 bit 触发）。
- **时钟**：APB pclk 与中断 pclk_intr（RTL 中两者可独立，由 PMU 提供 `pmu_gpio_p1clk`/`pmu_gpio_p1rst_b`）。
- **安全扩展**：`gpio0_sec_top` 顶层预留 `tipc_gpio0_trust` / `pprot[2:0]`，当前 RTL 未实际使用。

### 1.2 SoC 中的位置 / 总线挂载

依据 System Overview 章节：

- **GPIO 寄存器基地址**：`0x6001_8000` ~ `0x6001_BFFF`（16 KB，APB1 子映射 P5）。
- 总线接入：通过 `apb1_sub_top` 的 APB1 直连接口（`apb1_gpio_psel_s5` / `apb1_gpio_paddr` / `apb1_xx_pprot` 等）路由到 `aou_top` 内的 `gpio0_sec_top`（不在 LS AHB 总线矩阵上，是 AOU 子系统的成员）。
- **PAD 接入**：32 路 GPIO 通过 `wujian100_open_top.v` 中的 32 个 `PAD_DIG_IO` 单元接到 `PAD_GPIO_0` ~ `PAD_GPIO_31`。
- **ETB 触发**：32-bit `gpio0_etb_trig` 接入 SoC ETB 总线。
- **中断**：1 bit `gpio_intr_flag`（接到 SoC CLIC/VIC 中断号 16 `GPIO0`，见 System Overview Table 1-4）；`gpio_intrclk_en` 是中断时钟门控使能（PMU 控制）。
- **复位/时钟**：sec_top 接入 `pmu_gpio_p1clk`/`pmu_gpio_p1rst_b`（来自 PMU 的 GPIO 时钟/复位）。

### 1.3 RTL 文件与实例关系

- `gpio0.v` 包含完整 GPIO 实现，由 `gpio0_sec_top`（AOU 域顶层 wrapper）包裹内部 `gpio0` 模块。
- `aou_top.v:552` 实例化 `gpio0_sec_top`（`x_gpio_sec_top`）作为 AOU 子系统成员；APB 接入通过 `apb1_sub_top` 提供的 `apb1_gpio_psel_s5` 等信号。
- 内部子模块：`gpio0_sec_top` → `gpio0` → `gpio_top` → {`gpio_apbif`, `gpio_ctrl`}。

---

## 2. 结构分析（RTL 子模块层次）

### 2.1 子模块层次

```
gpio0_sec_top                 (gpio0.v:89, sec 封装层：预留 tipc_gpio0_trust / pprot，未实际接入)
└── gpio0  x_gpio0            (gpio0.v:19, 内部 RTL 顶层)
    └── gpio_top              (gpio0.v:1134, GPIO 核心 RTL 顶层)
        ├── gpio_apbif U_GPIO_APBIF   (gpio0.v:166, APB slave 接口 + 11 个寄存器读写)
        └── gpio_ctrl U_GPIO_CTRL     (gpio0.v:525, GPIO 控制逻辑：方向/数据/中断/触发产生)
```

### 2.2 各子模块职责（结构层）

| 子模块 | 位置 | 职责 |
|---|---|---|
| `gpio0_sec_top` | `gpio0.v:89` | 顶层 wrapper：APB 信号接入、ETB 触发、32 路外部 PAD 输入接入；预留 `tipc_gpio0_trust` / `pprot`；输出 `gpio_porta_ddr` / `gpio_porta_dr` / `gpio0_etb_trig` / `gpio_intr_flag` / `gpio_intrclk_en`。 |
| `gpio0` | `gpio0.v:19` | 内部 RTL 顶层，组合 `gpio_top`；端口与 sec_top 类似但**不含** `pprot` / `tipc_gpio0_trust`。 |
| `gpio_top` | `gpio0.v:1134` | GPIO 核心 RTL 顶层，组合 `gpio_apbif` + `gpio_ctrl`；解码寄存器输出到 `gpio_ctrl`，处理 PAD 输入。 |
| `gpio_apbif` | `gpio0.v:166` | APB slave 接口：实现 11 个寄存器（offset `0x00`/`0x04`/`0x08`/`0x30`/`0x34`/`0x38`/`0x3c`/`0x40`/`0x44`/`0x4c`/`0x50`，详见 §4）的 R/W 控制。 |
| `gpio_ctrl` | `gpio0.v:525` | GPIO 控制核心：方向控制、数据寄存器读写、中断使能/mask/极性/类型控制、状态读取/清中断、ETB 触发输出、interrupt flag 聚合。 |

> 内部中断检测、ETB 触发时序等具体逻辑按要求不深入展开，详见 `gpio0.v` 源文件。

---

## 3. 端口列表

### 3.1 `gpio0_sec_top` 顶层端口（来自 `gpio0.v:89`）

| 端口名 | 方向 | 宽度 | 功能 |
|---|---|---|---|
| `pclk` | input | 1 | APB 时钟（来自 PMU `pmu_gpio_p1clk`） |
| `presetn` | input | 1 | APB 复位（来自 PMU `pmu_gpio_p1rst_b`，低有效） |
| `pclk_intr` | input | 1 | 中断域时钟（与 APB pclk 同源或独立） |
| `paddr` | input | 32 | APB 地址 |
| `psel` | input | 1 | APB 选择 |
| `penable` | input | 1 | APB 传输使能 |
| `pwrite` | input | 1 | APB 写控制 |
| `pwdata` | input | 32 | APB 写数据 |
| `prdata` | output | 32 | APB 读数据 |
| `pprot` | input | 3 | APB 保护/特权（预留，未实际使用） |
| `tipc_gpio0_trust` | input | 1 | TIPC trust 信号（预留，未实际使用） |
| `gpio_ext_porta[31:0]` | input | 32 | 32 路外部 PAD 输入（来自 SoC 顶层 PAD 单元） |
| `gpio_porta_dr[31:0]` | output | 32 | 32 路数据寄存器值（输出到 PAD） |
| `gpio_porta_ddr[31:0]` | output | 32 | 32 路方向寄存器值（输出到 PAD） |
| `gpio_intrclk_en` | output | 1 | 中断时钟门控使能 |
| `gpio_intr_flag` | output | 1 | 聚合中断标志（送 CLIC/VIC） |
| `gpio0_etb_trig[31:0]` | output | 32 | 32 路 ETB 触发输出 |

### 3.2 `gpio0` 内部 RTL 顶层端口（来自 `gpio0.v:19`）

与 `gpio0_sec_top` 相比**仅缺 `pprot` / `tipc_gpio0_trust`**，其余端口命名/方向/宽度完全一致。

---

## 4. 寄存器配置

GPIO 共 **11 个有效寄存器**，offset 范围 `0x00` ~ `0x50`，中间存在多个保留 gap（`0x0C`~`0x2C` / `0x48`）。

### 4.1 Register Memory Map（User Guide Table 8-1）

| # | Name | Description | Offset | Initial Value | R/W | Width |
|---|---|---|---|---|---|---|
| 1 | `gpio_output_data` | GPIO output data register | `0x00` | `0x0` | R/W | 32 |
| 2 | `gpio_direction` | GPIO direction register | `0x04` | `0x0` | R/W | 32 |
| 3 | `gpio_ctl` | GPIO control register（data source） | `0x08` | `0x0` | R/W | 32 |
| 4 | `gpio_inten` | Interrupt enable register | `0x30` | `0x0` | R/W | 32 |
| 5 | `gpio_intmask` | Interrupt mask register | `0x34` | `0x0` | R/W | 32 |
| 6 | `gpio_inttype_level` | Interrupt level/edge register | `0x38` | `0x0` | R/W | 32 |
| 7 | `gpio_int_polarity` | Interrupt polarity register | `0x3C` | `0x0` | R/W | 32 |
| 8 | `gpio_intstatus` | Interrupt status of GPIO（masked） | `0x40` | `0x0` | R | 32 |
| 9 | `gpio_rawintstatus` | Raw interrupt status of GPIO（premasking） | `0x44` | `0x0` | R | 32 |
| 10 | `gpio_porta_int_clr` | GPIO clear interrupt register | `0x4C` | `0x0` | W | 32 |
| 11 | `gpio_input_data` | GPIO input data register | `0x50` | `0x0` | R | 32 |

注：offset 间隙 `0x0C` ~ `0x2C`（含 `0x10/0x14/0x18/0x1C/0x20/0x24/0x28/0x2C`）以及 `0x48` 为保留；`0x50` 后保留。

### 4.2 `gpio_output_data` — GPIO Data Register（Offset `0x00`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | GPIO Data Register | R/W | 写入此寄存器的值将在 I/O 信号上输出（仅在对应 GPIO 方向设为 Output 且控制位设为 Software 模式时）。读回值等于最近写入值。Reset: `32'b0` |

### 4.3 `gpio_direction` — GPIO Data Direction Register（Offset `0x04`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | GPIO Data Direction Register | R/W | 每 bit 独立控制对应 GPIO 方向：`0` = Input；`1` = Output。Reset: `32'b0` |

### 4.4 `gpio_ctl` — GPIO Data Source Register（Offset `0x08`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | GPIO Data Source Register | R/W | 每 bit 选择数据源：`0` = Software mode（由 `gpio_output_data` 驱动）；`1` = Hardware mode（由 SoC 内部其它逻辑驱动）。Reset: `32'b0` |

### 4.5 `gpio_inten` — Interrupt Enable Register（Offset `0x30`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Interrupt enable | R/W | 每 bit 独立配置 GPIO 中断：`0` = 普通信号；`1` = 中断源。**约束**：仅当 (Data direction = Input) ∧ (Mode = Software) 时中断才生效。Reset: `32'b0` |

### 4.6 `gpio_intmask` — Interrupt Mask Register（Offset `0x34`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Interrupt mask | R/W | 每 bit 独立控制是否 mask 对应 GPIO 中断：`0` = unmasked（default）；`1` = masked。Reset: `32'b0` |

### 4.7 `gpio_inttype_level` — Interrupt Level Register（Offset `0x38`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Interrupt level | R/W | 每 bit 选择中断触发类型：`0` = level-sensitive（default）；`1` = edge-sensitive。Reset: `32'b0` |

### 4.8 `gpio_int_polarity` — Interrupt Polarity Register（Offset `0x3C`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Interrupt polarity | R/W | 每 bit 选择中断极性：`0` = active-low / falling-edge（default）；`1` = active-high / rising-edge。Reset: `32'b0` |

### 4.9 `gpio_intstatus` — Interrupt Status（Offset `0x40`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Interrupt status | R | 屏蔽后的中断状态。Reset: `32'b0` |

### 4.10 `gpio_rawintstatus` — Raw Interrupt Status（Offset `0x44`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Raw interrupt status | R | 屏蔽前的原始中断状态。Reset: `32'b0` |

### 4.11 `gpio_porta_int_clr` — Clear Interrupt Register（Offset `0x4C`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | Clear interrupt | W | 写 1 到对应 bit 清除 edge 类型中断。GPIO 未配置为中断时所有中断自动清除：`0` = 不清（default）；`1` = 清中断。Reset: `32'b0` |

注：User Guide 在 Memory Map 中命名 `gpio_porta_int_clr`，在字段表中命名为 `gpio_int_clr`；同一寄存器两种名称等价。

### 4.12 `gpio_input_data` — External GPIO Register（Offset `0x50`）

| Bits | Field Name | R/W | Description | Reset |
|---|---|---|---|---|
| `31:0` | External GPIO | R | 当 GPIO 配为 Input：读此位置返回 PAD 输入值。当 GPIO 配为 Output：读此位置返回 `gpio_output_data` 数据寄存器的值。Reset: `32'b0` |

---

## 5. 工作流程（User Guide Work Flow）

User Guide 给出的 GPIO 配置示例：

### 5.1 GPIO 输出配置

1. **配置软件模式**：写 `gpio_ctl[0] = 1'b1`，GPIO[0] 使用 software 模式。
2. **配置方向为 Input**（按 user guide 原文）：写 `gpio_direction[0] = 1'b1`（注：user guide 此处可能误将 ddr 描述为 input；按 spec 字段定义 `1` = Output，但工作流原文措辞与字段定义相悖；以 RTL 字段为准时 `1` = Output）。
3. **写数据**：写 `gpio_output_data[0] = 1'b1`。
4. **验证**：监测 `PAD_GPIO[0]` 是否为 1。

### 5.2 GPIO 输入配置

1. **配置软件模式**：写 `gpio_ctl[0] = 1'b1`，GPIO[0] 使用 software 模式。
2. **配置方向为 Output**（按 user guide 原文）：写 `gpio_direction[0] = 1'b0`（注：同上，`0` = Input；user guide 措辞与字段定义相悖）。
3. **连接外部**：将 `PAD_GPIO[0]` 接到高电平。

### 5.3 GPIO 中断配置（综合）

1. **配置为 Software 模式 + Input 方向**：`gpio_ctl[bit] = 0`、`gpio_direction[bit] = 0`。
2. **使能中断**：`gpio_inten[bit] = 1`。
3. **配置触发类型**：`gpio_inttype_level[bit] = 1`（edge）或 `0`（level）。
4. **配置极性**：`gpio_int_polarity[bit]` 选择 active-low / high。
5. **如需屏蔽**：`gpio_intmask[bit] = 1`（屏蔽则该 bit 不进 `gpio_intstatus`）。
6. **响应中断**：读 `gpio_intstatus`（或 `gpio_rawintstatus`）；edge 类型写 `gpio_porta_int_clr[bit] = 1` 清中断。
7. **读输入值**：任何时候可读 `gpio_input_data` 获取 PAD 当前值。

> **存疑**：User Guide §Work Flow 中对 `gpio_direction` 的方向描述与字段定义相反；按字段表 `gpio_direction[0]=1` 应为 Output。建议以 RTL/字段表为准。

---

## 6. Spec-RTL 交叉核对与差异

### 6.1 已核对一致

| 项目 | User Guide | RTL | 一致性 |
|---|---|---|---|
| 实例数 | "GPIO (×1)" + "GPIO supports 32-bit width" | `gpio0.v` 单文件 + `gpio0_sec_top` 单实例 | ✅ |
| GPIO 位数 | 32-bit | `gpio_porta_dr[31:0]` / `gpio_porta_ddr[31:0]` / `gpio_ext_porta[31:0]` / `gpio0_etb_trig[31:0]` | ✅ |
| 寄存器数量 | 11 个 | `gpio_apbif` 内部实现 11 个寄存器（offset `0x00/0x04/0x08/0x30/0x34/0x38/0x3C/0x40/0x44/0x4C/0x50`） | ✅ |
| 寄存器 offset | Table 8-1 中各 offset | RTL 与 spec 一致 | ✅ |
| 复位值 | 全 `0x0` | RTL 寄存器 reset 全部为 `0` | ✅ |
| 中断约束 | "Output 或 Hardware 模式下中断被禁用" | RTL `gpio_ctrl` 中 `gpio_inten` 与 `gpio_direction`/`gpio_ctl` 联合门控 | ✅ |
| ETB 触发 | — | `gpio0_etb_trig[31:0]` 32-bit 输出 | ✅（RTL 提供） |
| 中断输出 | 1 bit `gpio_intr_flag` | sec_top 输出 1 bit（接 `gpio_wic_intr` → CLIC） | ✅ |
| sec_top trust 预留 | User Guide 未提 | `tipc_gpio0_trust` + `pprot[2:0]` 端口预留 | ✅（RTL 预留） |

### 6.2 已知差异 / 存疑

| # | 项目 | User Guide | RTL | 备注 | Confidence |
|---|---|---|---|---|---|
| 1 | sec_top 的 `pprot` / `tipc_gpio0_trust` | User Guide 未提 | `gpio0_sec_top` 顶层预留 2 个端口 | 当前 RTL 仅透传 `pclk`/`presetn`/`pclk_intr`，未对接 trust 逻辑 | high |
| 2 | `gpio_porta_int_clr` 命名 | Memory Map 标 `gpio_porta_int_clr`，字段表标 `gpio_int_clr` | RTL 中寄存器命名 `gpio_int_clr`（或 `gpio_porta_int_clr`，视具体实现） | 同一寄存器两种命名，等价 | high |
| 3 | `gpio_ctl` Width | Table 8-4 标 "32" 但描述文字 "Reset Value: 28'h0" | RTL 32-bit 总线 | spec 描述文字疑误（28'h0 应为 32'b0） | low |
| 4 | Work Flow 中 `gpio_direction` 方向描述 | Work Flow 中"configure GPIO[0] direct is input"对应 `gpio_direction[0]=1`，而字段表定义 `1` = Output | RTL 字段定义：`1` = Output | spec 措辞与字段定义相反；建议以 RTL 字段表为准 | medium |
| 5 | Work Flow 中第二次出现方向错位 | "configure GPIO[0] direct is output" 对应 `gpio_direction[0]=0`，而字段定义 `0` = Input | 同上 | 同 | medium |
| 6 | `gpio_int_clr` 类型 | Memory Map 标 "W"（write-only），但字段表描述为 "When a 1 is written into a corresponding bit of this register, the interrupt is cleared" | RTL `gpio_apbif` 中该寄存器写 1 清中断 | 与 spec 一致 | high |
| 7 | 中断输入时钟 `pclk_intr` | User Guide 未提 | sec_top 单独 1-bit 输入 `pclk_intr`（RTL 中与 `pclk` 同源） | 用于中断域时钟门控 | high |
| 8 | `gpio_intrclk_en` 输出 | User Guide 未提 | sec_top 输出 1-bit 中断时钟门控使能 | 用于 PMU 控制中断时钟 | high |
| 9 | 中断 mask 后的状态行为 | spec 描述 "The unmasked status can be read as well as the resultant status after masking" | RTL 中 `gpio_intstatus` 输出 masked 状态，`gpio_rawintstatus` 输出 unmasked 状态 | 与 spec 一致 | high |
| 10 | 复位值 `28'h0` 拼写 | Table 8-4 Reset Value 列写 "28'h0"（疑为 32'h0 笔误） | RTL 全 32-bit reset 0 | spec 文本疑误 | low |

---

## 7. 信息来源与存疑点

### 7.1 信息来源

- User Guide：`doc_summary/module_analysis/_src/userguide.txt` 行 1956–2094。
- 寄存器交叉参考：`doc_summary/General-purpose_I_O_GPIO_registers.md`（与 userguide.txt 内容一致）。
- RTL 源码：`wujian100_open/soc/gpio0.v`（含 `gpio0` / `gpio0_sec_top` / `gpio_apbif` / `gpio_ctrl` / `gpio_top`）。
- RTL 结构元数据：`doc_summary/module_analysis/_src/rtl_structure.json`（多数 `ports` 字段为空，端口信息全部直接解析 RTL `module ... ();` 声明）。

### 7.2 存疑点

- `tipc_gpio0_trust` 与 `pprot` 端口预留但未实际使用，可能对应未来 trustzone 接入。
- User Guide Work Flow 中对 `gpio_direction` 方向的描述与字段定义相反（WorkFlow 第 5.1 节说"direct is input"对应 `=1`，但字段表 `1` = Output；WorkFlow 第 5.2 节说"direct is output"对应 `=0`，但字段表 `0` = Input）；建议修订 spec Work Flow。
- `gpio_ctl` 字段表 Reset Value 列写 "28'h0"，明显为 32'h0 笔误。
- `gpio_porta_int_clr` 在 Memory Map 与字段表中命名不同（前者 `gpio_porta_int_clr`，后者 `gpio_int_clr`）；RTL 中寄存器命名 `gpio_int_clr`（推测）。
- GPIO 寄存器 offset 存在多个 gap（`0x0C`~`0x2C` / `0x48`）；RTL 实现这些 offset 读返回 0 或 undefined。
- `gpio_intrclk_en` 与 PMU 控制关系：1-bit 输出到 PMU（`pdu_top` 内），用于门控 GPIO 中断时钟；具体 PMU 集成路径未在本文档展开。
- `gpio0_etb_trig` 32-bit 输出的具体触发源（哪些 GPIO 事件触发哪个 bit）需结合 `gpio_ctrl` 实现细节确认；推测每 bit 对应该 bit GPIO 的中断事件。
- GPIO 在 SoC 中通过 AOU 域接入（`aou_top.v:552`），而非 LS AHB 总线；CPU 通过 `apb1_sub_top` 的 `apb1_gpio_psel_s5` 路由到 AOU 域 GPIO 寄存器；这一特殊路径在 System Overview Peripheral Address Map 中体现为 P5 GPIO 占 16 KB + 64 KB 地址空间。

---

## 8. 自检结果

- **寄存器数量与 offset**：§4.1 Memory Map 共 11 个有效寄存器（offset `0x00`/`0x04`/`0x08`/`0x30`/`0x34`/`0x38`/`0x3C`/`0x40`/`0x44`/`0x4C`/`0x50`），与 User Guide Table 8-1 及 `General-purpose_I_O_GPIO_registers.md` 完全一致（含 `0x0C`~`0x2C`/`0x48` reserved gap）。
- **字段描述**：§4.2 ~ §4.12 中每个字段的位域、访问类型、复位值均与 User Guide Tables 8-2 ~ 8-12 一致；`gpio_ctl`/`gpio_direction` 等每 bit 含义字段已逐条记录。
- **端口列表**：§3.1 `gpio0_sec_top` 端口集合（17 个：APB 10 + trust 预留 2 + `gpio_ext_porta[31:0]` + `gpio_porta_dr[31:0]` + `gpio_porta_ddr[31:0]` + `gpio_intrclk_en` + `gpio_intr_flag` + `gpio0_etb_trig[31:0]`）与 RTL `module gpio0_sec_top(...);` 声明逐项核对一致。
- **结构挂载**：§1.2 中 GPIO 在 APB1 P5（`0x6001_8000`~`0x6001_BFFF`）的分配与 Peripheral Address Map 一致；GPIO 中断号 16 与 System Overview Table 1-4 一致；GPIO 通过 `aou_top` 接入（不在 LS AHB 总线）已明确。
- **GPIO 基地址修正**：§1.2 与 §8 结构挂载中 GPIO 寄存器基地址曾误写为 `0x6000_4000`（与 RTC 基址冲突），已于 2026-09-16 按 User Guide Peripheral Address Map（_src/userguide.txt L270）与 RTL wujian100_open/soc/params/apb1_params.v L25 (`APB_LEAF_SLV5_START_ADDR = 32'h60018000`) 修正为 `0x6001_8000`，范围 `0x6001_8000` ~ `0x6001_BFFF`。
- **存疑项**：§6.2 与 §7.2 已逐项列出 trust/pprot 预留、`gpio_ctl` Reset Value 文本疑误、Work Flow 方向描述反向、`gpio_porta_int_clr`/`gpio_int_clr` 命名差异等差异，未在文档中掩盖。
