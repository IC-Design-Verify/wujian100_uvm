# 五阶段 Skill 使用规范

> 基于 `logic_op` IP 全流程验证实践与 `5-step-agent` 参考实现对比总结
> v2.0 — 增加根因分析与防错规则

---

## 目录

1. [根因分析与防错规则（必读）](#root-cause-rules)
2. [Stage 1: spec-to-testplan](#stage-1-spec-to-testplan)
3. [Stage 2: light_gui (UVM 环境生成)](#stage-2-light_gui)
4. [Stage 3: testcase-build (测试用例生成)](#stage-3-testcase-build)
5. [Stage 4: ksim (编译与仿真)](#stage-4-ksim)
6. [Stage 5: fsdb-analysis (波形调试)](#stage-5-fsdb-analysis)
7. [跨阶段约束与约定](#cross-stage-constraints)
8. [5-step-agent vs 主项目架构差异总结](#architecture-diff)

---

<a id="root-cause-rules"></a>
## 根因分析与防错规则（必读）

> 以下 7 条规则来自实际工作中的失败总结。每条规则对应一个导致多次返工的根因。
> **在开始任何阶段工作之前，必须逐条确认遵守。**

### 规则一：参数链一致性 — 不要手动修改模板生成的下游文件

**根因**：修改了 `env_gen.config` 的时钟域参数后，手动修改了部分下游文件（env_config.svh、test_base.svh）但遗漏了其他文件（itf_inst.sv、itf_config.sv），导致全链路参数不匹配。

**完整参数传播链**（env_gen.config 一处修改 → 所有文件自动更新）：

```
env_gen.config
  ├─ clocks/resets 定义
  │   ├→ env_config.svh      (virtual clock_if#(freq,phase) xxx_clk / virtual reset_if#(duration) xxx_rst)
  │   ├→ itf_inst.sv         (clock_if #(freq,phase) ... / reset_if #(duration) ...)
  │   ├→ itf_config.sv       (uvm_resource_db#(virtual clock_if#(freq,phase))::set / ...reset_if...)
  │   └→ test_base.svh       (uvm_resource_db#(virtual clock_if#(freq,phase))::read_by_name)
  ├─ dut_inst_connect
  │   ├→ tb_top.sv           (DUT 端口连接：logic_opwork_clk.CLOCK / logic_opwork_rst.RESET)
  │   └→ interface_assignment.sv (assign 接口信号)
  ├─ agents_inst
  │   ├→ env_config.svh      (has_xxx_agt, xxx_cfg 类型)
  │   ├→ env.svh             (agent 实例化、TLM 连接)
  │   └→ itf_inst.sv         (op_in_if / op_out_if 实例化参数)
  └─ reference_model_inst
      ├→ env.svh             (ref_model / scoreboard 实例名、has_xxx 位)
      └→ env_config.svh      (has_xxx 位名)
```

**强制规则**：
- `env_gen.py` 生成的文件之间有**严格的参数耦合**，不允许只修改其中一个
- 如需修改时钟域、复位参数、agent 参数等，**必须**修改 `env_gen.config` 后重新 `env_gen.py`
- 重新生成后，只需补全**手写部分**：reg_model、ref_model、scoreboard、test/vseq
- **反模式**：手动修改 `env_config.svh` 中的 `clock_if#(100,0)` 为 `clock_if#(50,0)` 但忘记修改 `itf_inst.sv` 和 `itf_config.sv` 中对应的参数

**自检方法**：修改后用 `grep -r "sys_clk\|work_clk\|clock_if#\|reset_if#"` 确认所有文件中参数一致。

---

### 规则二：先读 Driver 协议再写 Sequence — 理解 start→data→ed 三阶段协议

**根因**：不了解 op_in_driver 的 `send()` 协议，导致 sequence 中 `get_response()` 挂死、`data_num` 未设置、stimulus 时序错误。

**op_in_driver::send() 完整协议**：

```
时间线:  ──[start=1]──[start=0]──[data_en=1,data1,data2]×N──[data_en=0,ed=1]──[ed=0]──
                       ↑                                    ↑                    ↑
                   开始标志                            数据传输(N个周期)        结束标志
```

关键代码路径：
```systemverilog
task op_in_driver::send(op_in_seq_item tr);
  wait(vif.rst_n===1);
  @(`_DRV_VIF);          // ① 等一个时钟沿
  `_DRV_VIF.start  <= 1; // ② 发送 start=1
  @(`_DRV_VIF);          // ③ 等一个时钟沿
  `_DRV_VIF.start  <= 0; // ④ 拉低 start=0
  for(int i=0; i<tr.data_num; i++) begin  // ⑤ ★ 按 data_num 循环（不是 data1_q.size()）
    `_DRV_VIF.data_en <= 'b1;
    `_DRV_VIF.data1   <= tr.data1_q[i];
    `_DRV_VIF.data2   <= tr.data2_q[i];
    @(`_DRV_VIF);                          // ⑥ 每个数据等一个时钟沿
  end
  `_DRV_VIF.data_en <= 'b0;  // ⑦ 拉低 data_en
  `_DRV_VIF.ed     <= 1;     // ⑧ 发送 ed=1（结束标志）
  @(`_DRV_VIF);
  `_DRV_VIF.ed     <= 0;     // ⑨ 拉低 ed=0
endtask
```

**强制规则**：
- 在 vseq 中调用 `op_in_sequence` 时，**必须**显式设置 `data_num = data1_q.size()`
- Driver 只调用 `seq_item_port.item_done()`，**不**调用 `put_response()` — 所以 sequence 的 `body()` 中**不能**有 `get_response(req)`
- 模板生成的 `op_in_sequence::body()` 已正确使用 `start_item/finish_item` + 直接赋值模式，**不要改回 `uvm_do_with`**

**自检方法**：grep 新写的 vseq 确认所有 `op_in_sequence` 使用处都设置了 `data_num`。

---

### 规则三：先读 Monitor 协议再写 Reference Model — 理解批量发送 vs 逐个发送

**根因**：假设 monitor 逐个数据发送 transaction，实际 monitor 是批量发送（一次 `write()` 包含完整传输周期的所有数据队列）。

**op_in_monitor 采样协议**：

```
时间线:  ──[start=1]──[data_en=1]×N──[ed=1]──
          ↑ 开始采集    ↑ 累积data1_q/data2_q  ↑ analysis_port.write(tr_clone) 一次性发送
```

关键行为：
1. 检测到 `start=1` → 设置 `mon_flg=1`，开始采集
2. `mon_flg==1` 且 `data_en==1` → `tr.data1_q.push_back(data1)` / `tr.data2_q.push_back(data2)`
3. 检测到 `ed=1` → `analysis_port.write(tr_clone)` — **只发一次，包含本次全部数据**
4. Monitor **不填充** `data_num` 或 `logic_en` 字段

**op_out_monitor 采样协议**：

```
时间线:  ──[data_en=1]×N──[data_en=0]──
          ↑ 累积data_q     ↑ analysis_port.write(tr_clone) 一次性发送
```

**强制规则**：
- Reference Model 的 `write()` 函数接收的是**一个完整传输周期的所有数据**（队列为空到满），不是单个数据
- Reference Model 用 `tr.data1_q.size()` 作为循环边界，**不是** `tr.data_num`（monitor 不填充）
- Scoreboard 的 expect_q/actual_q 是 **transaction 级别**的队列比较，不是数据级别
- 不要在 `write()` 函数中调用 `uvm_reg::read()`（是 task 不是 function），改用 `get_mirrored_value()` 或 queue+run_phase 模式

---

### 规则四：Package 嵌套依赖链 — include 顺序必须严格遵循

**根因**：不理解 Package 之间有严格的依赖层次，include 顺序错误导致类型未定义或重复定义。

**正确的 Package 依赖链**（从底层到顶层）：

```
Layer 0: logic_op_reg_model_pkg    (reg_field.svh + reg_model.svh)
              ↓ import
Layer 1: logic_op_env_pkg          (env_config + env + v_sequencer + ref_model + scoreboard + coverage)
              ↓ import
Layer 2: logic_op_reg_sequence_pkg (reg_sequence_base + reg_sequence)
              ↓ import
Layer 3: logic_op_vseq_base_pkg    (v_sequence_base — imports env_pkg + reg_sequence_pkg)
              ↓ import
Layer 4: logic_op_base_test_pkg    (test_base — imports vseq_base_pkg + env_pkg)
              ↓ include in all_testcases.svh
Layer 5: logic_op_v_sequence_pkg   (所有 scenario vseq 类)
         logic_op_testcase_pkg     (所有 test 类)
```

**`all_testcases.svh` 的 include 顺序**：
```systemverilog
import logic_op_vseq_base_pkg::*;
import logic_op_base_test_pkg::*;

`include "logic_op_v_sequence_pkg.svh"   // ★ vseq 必须先 include
`include "logic_op_testcase_pkg.svh"     // ★ test 后 include（test 引用 vseq 类）
```

**强制规则**：
- vseq package **必须**在 test package 之前 include（test 类引用 vseq 类名）
- 不要同时在 `logic_op_test_lib.svh`（模板生成的旧测试）和独立的 `logic_op_<name>_test.svh` 中定义同名类 — 先清除模板旧文件再添加新文件
- 新增 vseq/test 文件时，只修改 `logic_op_v_sequence_pkg.svh` 和 `logic_op_testcase_pkg.svh`，添加 `include` 行
- **不要**在 `all_testcases.svh` 或 `logic_op_testbase_pkg.svh` 中直接 include vseq/test 文件

**自检方法**：新增文件后，确认只在对应的 `_pkg.svh` 中有一处 `include`，无重复。

---

### 规则五：tb_top.sv 的 VCS 解析顺序 — class 定义在前，module items 在后

**根因**：不理解 VCS 对 module 内 program items（class 定义）和 module items（wire/initial/assign）的严格顺序要求，导致 "token is 'initial'" 编译错误。

**VCS 解析规则**：
- `module` 内的项分两类：
  - **Program items**：class 定义、function/task 声明、import 语句
  - **Module items**：wire/reg 声明、assign 语句、always/initial 块、实例化
- VCS 允许 program items 在 module items 之前出现
- **一旦出现 module item，后续不能再有 program item**
- `` `include `` 展开后的内容类型取决于被 include 文件的内容

**tb_top.sv 的唯一正确结构**：

```systemverilog
module tb_top();
  // ═══════ Program Items 区 ═══════ (顺序不可变)
  import uvm_pkg::*;
  import svt_uvm_pkg::*;
  import svt_apb_uvm_pkg::*;
  import apb_env_pkg::*;
  import op_in_agent_pkg::*;
  import op_out_agent_pkg::*;

  `include "logic_op_include/all_testcases.svh"  // class 定义 (program item)

  // ═══════ Module Items 区 ═══════ (从第一个 initial/wire/assign 开始，之后不能再有 class)
  initial begin
    $timeformat(-9, 3, " ns", 10);
    run_test();
  end

  `include "logic_op_include/logic_op_itf_inst.sv"   // 接口实例化 (module item)

  // ★ wire 声明 — 在 itf_inst 之后、DUT 之前
  wire [3:0] in1;
  wire [3:0] in2;
  wire [3:0] out;
  wire [31:0] paddr;
  wire [31:0] pwdata;
  wire [31:0] prdata;

  logic_op dut (
    .clk(logic_opwork_clk.CLOCK),
    // ...
  );

  `include "logic_op_include/logic_op_program_download.sv"  // module item
  `include "logic_op_include/logic_op_interface_assignment.sv"  // assign 语句 (module item)

  initial begin
    `include "logic_op_include/logic_op_itf_config.sv"  // uvm_config_db 设置 (module item)
  end
endmodule
```

**强制规则**：
- wire 声明**只能**放在 `tb_top.sv` 的 `itf_inst` 之后、DUT 之前
- **绝不能**把 wire 声明放在 `all_testcases.svh` 之前（会让 VCS 认为 program items 区结束，后续 class 定义报错）
- **绝不能**把 wire 声明放在 `interface_assignment.sv` 中（`interface_assignment.sv` 只有 assign 语句，不含 wire 声明）
- 多位 DUT 端口（宽度 > 1）**必须**显式声明 wire，否则 VCS 创建隐式 1-bit wire

**自检方法**：`grep -n "wire\[" tb_top.sv` 确认 wire 声明在 `itf_inst` 和 DUT 实例化之间。

---

### 规则六：APB VIP 需要完整双接口接线 — master_vif 和 slave_vif

**根因**：删除了 `apb_slave_vif` 的接线（因为不理解用途），导致 APB slave VIP 内部 monitor/sequence 无法正确工作。

**APB VIP 架构**：

```
svt_apb_if apb_master_vif ──→ apb_env.apb_master_env  (master VIP: 发起 APB 读写)
svt_apb_if apb_slave_vif  ──→ apb_env.apb_slave_env   (slave VIP: 响应 APB 读写)
```

**`itf_config.sv` 同时配置两个 vif**：
```systemverilog
uvm_config_db#(svt_apb_vif)::set(..., "apb_env.apb_master_env", "vif", apb_master_vif);
uvm_config_db#(svt_apb_vif)::set(..., "apb_env.apb_slave_env",  "vif", apb_slave_vif);
```

**`interface_assignment.sv` 必须包含**：

```systemverilog
// APB master interface 时钟/复位 + 信号连接
assign `MERGE_ITF_NAME(..., apb_master_vif).pclk    = apb_clk.CLOCK;
assign `MERGE_ITF_NAME(..., apb_master_vif).presetn = apb_rst.RESET;
assign psel    = `MERGE_ITF_NAME(..., apb_master_vif).psel[0];
assign penable = `MERGE_ITF_NAME(..., apb_master_vif).penable;
assign pwrite  = `MERGE_ITF_NAME(..., apb_master_vif).pwrite;
assign paddr   = `MERGE_ITF_NAME(..., apb_master_vif).paddr;
assign pwdata  = `MERGE_ITF_NAME(..., apb_master_vif).pwdata;
assign `MERGE_ITF_NAME(..., apb_master_vif).slave_if[0].prdata  = prdata;
assign `MERGE_ITF_NAME(..., apb_master_vif).slave_if[0].pready  = pready;
assign `MERGE_ITF_NAME(..., apb_master_vif).slave_if[0].pslverr = 1'b0;

// ★ APB slave interface — 不能遗漏！
assign `MERGE_ITF_NAME(..., apb_slave_vif).pclk    = apb_clk.CLOCK;
assign `MERGE_ITF_NAME(..., apb_slave_vif).presetn = apb_rst.RESET;
assign `MERGE_ITF_NAME(..., apb_slave_vif).slave_if[0].prdata  = prdata;
assign `MERGE_ITF_NAME(..., apb_slave_vif).slave_if[0].pready  = pready;
assign `MERGE_ITF_NAME(..., apb_slave_vif).slave_if[0].pslverr = 1'b0;
```

**强制规则**：
- `apb_master_vif` 和 `apb_slave_vif` 是两个**独立的** interface 实例，都必须有时钟/复位和 DUT 输出连接
- 不要删除模板生成的 `apb_slave_vif` 接线
- `itf_inst.sv` 中两个 `svt_apb_if` 实例都必须存在

---

### 规则七：vseq 使用直接赋值 + start() — 避免 uvm_do_on 嵌套 randomize

**根因**：vseq 中使用 `uvm_do_on(op_in_seq, sqr)` + `randomize() with {data_num == 10}` 模式，但 `op_in_sequence::body()` 内部也会创建 `req` 并 randomize — 嵌套 randomize 导致队列数据被覆盖。

**错误模式**（我之前的做法）：
```systemverilog
// ✗ 错误 — uvm_do_on 会触发 op_in_sequence::body()，body() 内又 randomize req
op_in_seq = op_in_sequence#(4)::type_id::create("op_in_seq");
assert(op_in_seq.randomize() with { data_num == 10; })
`uvm_do_on(op_in_seq, p_sequencer.op_in_data_sqr)
// 结果：op_in_seq.data1_q 被 randomize 填充了随机值（因为 body() 中 req 也被 randomize）
```

**正确模式**（5-step-agent 做法 — 直接赋值 + start()）：
```systemverilog
// ✓ 正确 — 先赋值，再 start()
task drive_op_in(bit[3:0] data1_q[$], bit[3:0] data2_q[$]);
  op_in_sequence#(4) op_in_seq;
  op_in_seq = op_in_sequence#(4)::type_id::create("op_in_seq");
  op_in_seq.data_num = data1_q.size();   // ★ 显式设置 data_num
  op_in_seq.data1_q = data1_q;           // ★ 直接赋值队列
  op_in_seq.data2_q = data2_q;           // ★ 直接赋值队列
  op_in_seq.start(p_sequencer.op_in_data_sqr);
endtask

task drive_single(bit[3:0] in1, bit[3:0] in2);
  bit[3:0] q1[$], q2[$];
  q1.push_back(in1);
  q2.push_back(in2);
  drive_op_in(q1, q2);
endtask
```

**强制规则**：
- 在 vseq 中驱动 `op_in_sequence` 时，**必须**使用直接赋值 + `start()` 模式
- **禁止**使用 `uvm_do_on` / `uvm_do_on_with` 来驱动 `op_in_sequence`（VCS 不支持 queue 约束 + 嵌套 randomize 问题）
- 推荐创建 `logic_op_base_v_sequence` 中间基类，封装 `write_ctrl()` 和 `drive_op_in()` / `drive_single()` helper
- `op_in_sequence::body()` 中的 `req` 只使用 `start_item/finish_item` + 直接赋值，**不调用** `get_response(req)`

**推荐的 vseq 中间基类**：
```systemverilog
class logic_op_base_v_sequence extends logic_op_v_sequence_base;
  `uvm_object_utils(logic_op_base_v_sequence)

  // Helper: 写 CTRL 寄存器
  task write_ctrl(bit[1:0] logic_sel_val);
    uvm_status_e status;
    env_cfg.rgm.CTRL.write(status, {30'h0, logic_sel_val}, UVM_FRONTDOOR, .parent(this));
    `uvm_info("VSEQ", $sformatf("Wrote CTRL.logic_sel = %0d", logic_sel_val), UVM_MEDIUM)
  endtask

  // Helper: 驱动多个数据对
  task drive_op_in(bit[3:0] d1[$], bit[3:0] d2[$]);
    op_in_sequence#(4) op_in_seq;
    op_in_seq = op_in_sequence#(4)::type_id::create("op_in_seq");
    op_in_seq.data_num = d1.size();
    op_in_seq.data1_q = d1;
    op_in_seq.data2_q = d2;
    op_in_seq.start(p_sequencer.op_in_data_sqr);
  endtask

  // Helper: 驱动单个数据对
  task drive_single(bit[3:0] in1, bit[3:0] in2);
    bit[3:0] q1[$], q2[$];
    q1.push_back(in1);
    q2.push_back(in2);
    drive_op_in(q1, q2);
  endtask
endclass
```

---

<a id="stage-1-spec-to-testplan"></a>
## Stage 1: spec-to-testplan (`/spec-to-testplan`)

### 职责
从设计 Spec + RTL（仅端口）提取验证结构信息，生成结构化 JSON 输出。不涉及任何 EDA 工具。

### 输入

| 项目 | 必需 | 来源 |
|------|------|------|
| 设计规格书 | 是 | PDF / Markdown / Word / 纯文本 |
| RTL 顶层文件 | 是 | `.v` / `.sv` |
| RTL defines 文件 | 否 | 宏定义文件 |

### 输出

| 文件 | 路径 | 内容 |
|------|------|------|
| `spec_analysis.json` | `design/<module>/docs/` | 接口、寄存器、时钟/复位、错误条件、Spec-RTL 不匹配 |
| `fsm_analysis.json` | `design/<module>/docs/` | FSM 列表、状态、转换、编码、不匹配 |
| `testplan.json` | `design/<module>/docs/` | 验证场景列表（group、priority、steps、expected_result） |

### 执行步骤

1. **Step 0**: 输入就绪检查 — 确认 Spec 和 RTL 文件存在
2. **Step 0b**: 运行 `parse_rtl_structure.py` 提取 RTL 结构数据
   ```bash
   python3 .claude/skills/spec-to-testplan/scripts/parse_rtl_structure.py --dir <rtl_dir>
   ```
3. **Step 1**: Spec 分析 — 先读 Spec，再读 RTL 确认结构，标记 Spec-RTL 不匹配
4. **Step 2**: FSM 提取 — 双源交叉对比（Spec vs RTL）
5. **Step 3**: 寄存器编码分析 — 比对 Spec 与 RTL 的寄存器定义
6. **Step 4**: 生成 testplan — 按功能分组、按 P0/P1/P2 分优先级

### 关键约束

- **绝不分析 DUT 内部逻辑** — 只读 RTL 端口
- **Spec 是黄金参考** — Spec 与 RTL 矛盾时，以 Spec 为准，标记 RTL 为 mismatch
- **不修改任何代码文件** — 本阶段是纯分析
- **不确定推断标记 `confidence: "low"`** — 不捏造信息

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `width=-1` 在端口提取中 | 端口宽度含宏表达式 | 用提取的 `defines` 列表手动解析 |
| Spec 提到错误条件但提取为空 | Spec 描述模糊 | 输出 Warning，建议人工审查 |
| testplan 没有覆盖某 FSM 状态 | 场景遗漏 | 自动添加 gap 场景（`status: "not_covered"`） |

---

<a id="stage-2-light_gui"></a>
## Stage 2: light_gui (UVM 环境生成)

---

<a id="stage-3-testcase-build"></a>
## Stage 3: testcase-build (`/testcase-build`)

### 执行步骤

1. 从 testplan 提取场景列表，按 P0 → P1 → P2 排序
2. 为每个场景生成 vseq（继承中间基类或 v_sequence_base）
3. 为每个场景生成 test（继承 test_base）
4. 更新 testplan.json、package include、testname.f
5. 按优先级批次生成

### vseq 必须遵守的模式

**1. 使用中间基类封装 helper**（规则七）：

```systemverilog
// 所有 scenario vseq 继承此基类，而非直接继承 logic_op_v_sequence_base
class logic_op_base_v_sequence extends logic_op_v_sequence_base;
  `uvm_object_utils(logic_op_base_v_sequence)
  task write_ctrl(bit[1:0] val); ... endtask
  task drive_op_in(bit[3:0] d1[$], bit[3:0] d2[$]); ... endtask
  task drive_single(bit[3:0] in1, bit[3:0] in2); ... endtask
endclass
```

**2. 使用直接赋值 + start() 驱动 op_in_sequence**（规则七）：

```systemverilog
// ✓ 正确
task body();
  write_ctrl(2'h0);    // AND mode
  #100ns;              // 等待寄存器写入完成
  drive_single(4'hA, 4'h5);  // 发送单个数据对
  #100ns;              // 等待 DUT 处理
endtask
```

**3. 寄存器写入后加延时等待**：

```systemverilog
write_ctrl(2'h0);  // 通过 APB 总线写寄存器（task，需要时间）
#100ns;            // ★ 等待写入完成 + DUT 内部寄存器更新
drive_single(...); // 然后再发 stimulus
```

**4. 禁止使用的模式**：

```systemverilog
// ✗ 禁止 — VCS 不支持 queue 约束
uvm_do_on_with(op_in_seq, sqr, { data1_q == local_q; })

// ✗ 禁止 — 嵌套 randomize 问题
assert(op_in_seq.randomize() with { data_num == 10; })
uvm_do_on(op_in_seq, sqr)

// ✗ 禁止 — virtual sequencer 没有 driver
start_item(req); finish_item(req);  // 直接在 vseq 的 body() 中
```

### 测试类必须遵守的模式

```systemverilog
class logic_op_<scenario>_test extends logic_op_test_base;
  `uvm_component_utils(logic_op_<scenario>_test)

  function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    uvm_config_db #(uvm_object_wrapper)::set(this,
      "*m_env.logic_op_vseqr.run_phase","default_sequence", null);
  endfunction

  task main_phase(uvm_phase phase);
    logic_op_<scenario>_v_sequence vseq;
    phase.raise_objection(this);
    vseq = new();
    vseq.start(m_env.logic_op_vsqr);
    phase.drop_objection(this);  // ★ vseq 完成后立即 drop
  endtask
endclass
```

**注意**：test 的 `main_phase` 中**不需要**加 `#500ns` 等额外延时。vseq 完成后 objection 被 drop，`post_main_phase` 触发 `->test_done`，`run_phase` 退出。

### Package 注册规范

新增 vseq/test 文件时，只修改两个 `_pkg.svh` 文件：

```systemverilog
// logic_op_v_sequence_pkg.svh — 添加 vseq include
`include "logic_op_<name>_v_sequence.svh"   // 在已有 include 之后

// logic_op_testcase_pkg.svh — 添加 test include
`include "logic_op_<name>_test.svh"         // 在已有 include 之后
```

**vseq 必须在 test 之前 include**（test 引用 vseq 类名）。

### 自检清单

| 检查项 | 条件 | 自动修复 |
|--------|------|----------|
| 工厂注册 | 每个测试有 `uvm_component_utils` | 添加宏 |
| 工厂注册 | 每个 vseq 有 `uvm_object_utils` | 添加宏 |
| Objection 配对 | 每个 `raise_objection` 有匹配的 `drop_objection` | 添加缺失的 drop |
| Super 调用 | 每个重写的 phase 方法调用 `super.<phase>(phase)` | 添加调用 |
| data_num 设置 | vseq 中 drive_op_in 调用都显式设置 `data_num` | 添加赋值 |
| testname.f 同步 | 每个测试类名出现在 testname.f | 添加缺失条目 |
| Package include | 每个新文件被 include 在对应的 `_pkg.svh` 中 | 添加 include 行 |
| 无重复定义 | 同名类不在多个文件中定义 | 删除旧定义 |

---

<a id="stage-4-ksim"></a>
## Stage 4: ksim (编译与仿真)

---

<a id="stage-5-fsdb-analysis"></a>
## Stage 5: fsdb-analysis (`/fsdb-analysis`)

### FSDB 文件位置

```
${KSIM_SIM_DIR}/<project>/test/<module>.<testname>/novas.fsdb
```

### NPI Tcl API 注意事项

| 要点 | 说明 |
|------|------|
| 信号名必须精确 | `npi_fsdb_sig_by_name` 不支持通配符 |
| hex 转 int | 用 `scan $hex_str "%x" var`，不用 `expr {0x$val}` |
| 时间戳单位 | FSDB 时间戳是**皮秒**（736ns = 736000ps） |
| `npi_fsdb_sig_bits` | **不存在**于当前 Verdi 版本 |
| Scoreboard 时间偏移 | Scoreboard 报错时间有 delta delay，在 FSDB 中回溯查找 |

---

<a id="cross-stage-constraints"></a>
## 跨阶段约束与约定

### 绝对红线

1. **绝不修改 RTL 源代码**
2. **Spec 是编写 Reference Model 的唯一参考** — 仿真失败时绝不修改 ref_model 对齐 DUT
3. **绝不修改模板生成的代码**（除非是已知 Bug 修复）— 需要改动则修改 `env_gen.config` 重新生成
4. **env_gen.config 参数链全链路一致** — 时钟域、复位参数、agent 参数从 config 到所有下游文件必须一致

### 编译前自检清单

在运行 `make comp` 之前，逐项确认：

```bash
# 1. 参数链一致性（规则一）
grep -r "clock_if#\|reset_if#" dv/simulation/verif_env/ip/logic_op/
# 所有文件中的参数必须一致

# 2. wire 声明位置（规则五）
grep -n "wire\[" dv/simulation/verif_env/ip/logic_op/tb_top/tb_top.sv
# wire 声明必须在 itf_inst 之后、DUT 之前

# 3. APB slave_vif 接线（规则六）
grep "apb_slave_vif" dv/simulation/verif_env/ip/logic_op/tb_top/logic_op_include/logic_op_interface_assignment.sv
# 必须有时钟/复位和 slave_if 连接

# 4. data_num 设置（规则二+七）
grep -r "op_in_seq\." dv/simulation/verif_env/ip/logic_op/sequences/
# 所有 op_in_seq 使用处必须有 data_num 赋值

# 5. 无 get_response（规则二）
grep -r "get_response" dv/simulation/verif_env/ip/logic_op/
# 不应存在 get_response 调用

# 6. Package include 无重复（规则四）
grep -r "logic_op_and_v_sequence" dv/simulation/verif_env/ip/logic_op/sequences/logic_op_v_sequence_pkg.svh
# 每个 vseq 只在一处 include

# 7. PH_TIMEOUT 修复
grep "test_done" dv/simulation/verif_env/ip/logic_op/tests/uvm_test/logic_op_test_base.svh
# 必须有 event test_done 和 post_main_phase

# 8. f-string bug
grep "ENV\[" dv/simulation/verif_env/ip/logic_op/build/vcs_logic_op.file
# 不应有 "ENV[" — 应为 "{ENV["
```

---

<a id="architecture-diff"></a>
## 5-step-agent vs 主项目架构差异总结

| 维度 | 5-step-agent | 主项目 | 推荐 |
|------|-------------|--------|------|
| 时钟域 | `sys` (100MHz) | `work` (100MHz) | 均可，保持全链路一致（规则一） |
| Reference Model | 非参数化 + `UVM_FRONTDOOR` | 参数化 + `get_mirrored_value()` | 非参数化+FRONTDOOR 更安全 |
| Scoreboard | 非参数化，不 clone | 参数化，clone-before-push | clone 更安全 |
| vseq/test 组织 | 单文件含所有类 | 每场景独立文件 | 独立文件更易维护 |
| 中间 vseq 基类 | `logic_op_base_v_sequence`（含 helper） | 无中间基类 | 有 helper 更优雅（规则七） |
| test_base.run_phase | 无 objection | raise/drop objection + event | event 方式防 PH_TIMEOUT |
| Wire 声明位置 | tb_top.sv（itf_inst 后、DUT 前） | interface_assignment.sv | **tb_top.sv 中更可靠**（规则五） |
| APB slave_vif 接线 | 完整（master + slave） | 仅 master | **需要完整接线**（规则六） |
| TLM 端口命名 | `op_in_data1_imp` / `op_out_data1_ap` | `op_in1_imp` / `op_out1_ap` | 均可，env 中连接一致 |
| env 中实例命名 | `logic_op_ref_inst_ref_model` / `_scb` | `ref_ref_model` / `ref_scb` | 均可 |

### 5-step-agent 的已知缺陷

1. **test_base 缺 PH_TIMEOUT 修复**：无 `event test_done`、无 `post_main_phase`、无 run_phase objection
2. **testplan 分析错误**：FUNC_002 (OR) 的 review_note 错误声称 RTL 有 OR bug，实际 RTL 只在 XNOR 有 bug
3. **final_phase report_server**：两个 `#ifdef` 分支都写 `uvm_report_server server;` 和 `uvm_report_server::get_server()`，无差异化（模板 bug，不影响功能）

---

## 附录：快速参考命令

```bash
# ===== Stage 1: Spec 分析 =====
/spec-to-testplan
python3 .claude/skills/spec-to-testplan/scripts/parse_rtl_structure.py --dir design/<module>/

# ===== Stage 2: UVM 环境生成 =====

# ===== Stage 3: 测试用例生成 =====
/testcase-build                  # 生成 vseq + test，注册到 _pkg.svh 和 testname.f

# ===== Stage 4: 编译与仿真 =====
cd dv && source dv.bashrc

# ===== Stage 5: 波形调试 =====
/fsdb-analysis
```
