# UVM 验证环境调试经验规范

> 基于多项目 IP 验证实践总结的通用规范
> v2.0 — 2026-05-28

---

## 目录

1. [Scoreboard 比较机制选型](#1-scoreboard-比较机制选型)
2. [op_in Driver 协议与 Sequence 规范](#2-op_in-driver-协议与-sequence-规范)
3. [Sequence 与 Sequencer 架构](#3-sequence-与-sequencer-架构)
4. [Package 结构与 Include 规范](#4-package-结构与-include-规范)
5. [Register Model 访问](#5-register-model-访问)
6. [已知模板 Bug 的修复](#6-已知模板-bug-的修复)
7. [RTL Bug 识别模式](#7-rtl-bug-识别模式)
8. [编译/仿真问题速查表](#8-编译仿真问题速查表)

---

## 1. Scoreboard 比较机制选型

### 1.1 两种方案对比 ★★★ CRITICAL

Scoreboard 有三种比较机制，选型错误会导致多操作场景下大量误报。

| 方案 | 实现方式 | 适用场景 | 风险 |
|------|----------|----------|------|
| A. write函数中立即比较 | `write_golden()` push + 立即pop比较 | 仅当expected和actual严格同步到达 | **多操作场景下时序偏移导致误报** |
| B. 队列 + run_phase顺序比较 | `write()` 只push，`run_phase` 中pop比较 | **通用场景，推荐** | 需要event唤醒机制 |
| C. transaction ID匹配 | 每个transaction带ID，scoreboard按ID匹配 | 异步/乱序场景 | 实现复杂度高 |

### 1.2 方案A的问题：时序偏移 ★★★ CRITICAL

**根因**：Reference Model在收到输入数据后立即计算期望值并发送，但DUT需要经过时钟沿处理后才能产生实际输出。两者到达Scoreboard的时间存在偏移。

**失败场景**（多操作连续执行时）：
```
Time:  T1          T2          T3          T4
Ref:   exp_op1     exp_op2
DUT:               act_op1     act_op2

方案A: write_golden(exp_op1) → push expect_q
       write_actual(act_op1) → pop expect_q → 比较 exp_op1 vs act_op1 ✓
       write_golden(exp_op2) → push expect_q
       write_actual(act_op2) → pop expect_q → 比较 exp_op2 vs act_op2 ✓
       ← 时序同步时工作正常

但如果 DUT 延迟更大：
Time:  T1          T2          T3          T4          T5
Ref:   exp_op1     exp_op2     exp_op3
DUT:                           act_op1     act_op2     act_op3

方案A: write_golden(exp_op1) → push
       write_golden(exp_op2) → push  ← expect_q 有2项
       write_actual(act_op1) → pop exp_op1, 比较 exp_op1 vs act_op1 ✓
       write_golden(exp_op3) → push  ← expect_q 有2项
       write_actual(act_op2) → pop exp_op2, 比较 exp_op2 vs act_op2 ✓
       write_actual(act_op3) → pop exp_op3, 比较 exp_op3 vs act_op3 ✓
       ← 队列模式天然处理了时序偏移
```

**关键洞察**：方案A在单操作测试中可能通过，但在多操作Smoke Test中会暴露问题。如果单独测试每种功能都PASS，但组合测试FAIL，多半是Scoreboard时序问题。

### 1.3 推荐方案B：队列 + run_phase ★★★ CRITICAL

```systemverilog
`uvm_analysis_imp_decl(_golden)
`uvm_analysis_imp_decl(_actual)

class xxx_scoreboard extends uvm_scoreboard;
  `uvm_component_param_utils(xxx_scoreboard)

  uvm_analysis_imp_golden #(out_seq_item_t, xxx_scoreboard) golden_imp;
  uvm_analysis_imp_actual #(out_seq_item_t, xxx_scoreboard) actual_imp;

  out_seq_item_t expect_q[$];
  out_seq_item_t actual_q[$];
  int match_count = 0;
  int mismatch_count = 0;
  int compare_count = 0;
  event data_available;

  // write函数只负责入队 + 触发event
  function void write_golden(out_seq_item_t tr);
    out_seq_item_t exp_clone;
    $cast(exp_clone, tr.clone());
    expect_q.push_back(exp_clone);
    ->data_available;
  endfunction

  function void write_actual(out_seq_item_t tr);
    out_seq_item_t act_clone;
    $cast(act_clone, tr.clone());
    actual_q.push_back(act_clone);
    ->data_available;
  endfunction

  // run_phase中顺序比较
  task run_phase(uvm_phase phase);
    out_seq_item_t exp_tr, act_tr;
    forever begin
      @(data_available);
      while (expect_q.size() > 0 && actual_q.size() > 0) begin
        exp_tr = expect_q.pop_front();
        act_tr = actual_q.pop_front();
        compare_count++;
        if (exp_tr.data !== act_tr.data) begin
          `uvm_error("SCB_MISMATCH", $sformatf("[%0d] Expected: 0x%h, Actual: 0x%h",
            compare_count, exp_tr.data, act_tr.data))
          mismatch_count++;
        end else begin
          match_count++;
        end
      end
    end
  endtask

  // check_phase: 检查队列是否为空
  function void check_phase(uvm_phase phase);
    super.check_phase(phase);
    if (expect_q.size() > 0)
      `uvm_warning("SCB_QUEUE", $sformatf("Expect queue not empty: %0d items", expect_q.size()))
    if (actual_q.size() > 0)
      `uvm_warning("SCB_QUEUE", $sformatf("Actual queue not empty: %0d items", actual_q.size()))
  endfunction

  // report_phase: 输出统计
  function void report_phase(uvm_phase phase);
    super.report_phase(phase);
    `uvm_info("SCB_REPORT", $sformatf("Scoreboard: %0d matches, %0d mismatches out of %0d comparisons",
      match_count, mismatch_count, compare_count), UVM_NONE)
  endfunction
endclass
```

### 1.4 clone-before-push 必须执行 ★★★ CRITICAL

无论使用哪种方案，write函数中必须先clone再push：

```systemverilog
function void write_golden(out_seq_item_t tr);
  out_seq_item_t exp_clone;
  $cast(exp_clone, tr.clone());  // ★ clone防止原始transaction被后续修改
  expect_q.push_back(exp_clone);
endfunction
```

**不clone的风险**：如果analysis_port连接了多个imp（如同时连scoreboard和coverage），后写入的transaction会覆盖先写入的，导致数据不一致。

---

## 2. op_in Driver 协议与 Sequence 规范

### 2.1 op_in_driver 三阶段协议 ★★★ CRITICAL

op_in_driver的`send()`任务遵循严格的三阶段协议：

```
时间线:  ──[start=1]──[start=0]──[data_en=1,data1,data2]×N──[data_en=0,ed=1]──[ed=0]──
                       ↑                                    ↑                    ↑
                   开始标志                            数据传输(N个周期)        结束标志
```

**关键代码路径**：
```systemverilog
task op_in_driver::send(op_in_seq_item tr);
  wait(vif.rst_n===1);
  @(`_DRV_VIF);          // ① 等一个时钟沿
  `_DRV_VIF.start  <= 1; // ② 发送 start=1
  @(`_DRV_VIF);          // ③ 等一个时钟沿
  `_DRV_VIF.start  <= 0; // ④ 拉低 start=0
  for(int i=0; i<tr.data_num; i++) begin  // ⑤ ★ 按 data_num 循环
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

### 2.2 op_in_monitor 采样行为 ★★★ CRITICAL

op_in_monitor是**批量发送**，不是逐个发送：

```
时间线:  ──[start=1]──[data_en=1]×N──[ed=1]──
          ↑ 开始采集    ↑ 累积data1_q/data2_q  ↑ analysis_port.write(tr_clone) 一次性发送
```

**关键行为**：
1. 检测到 `start=1` → 设置 `mon_flg=1`，开始采集
2. `mon_flg==1` 且 `data_en==1` → `tr.data1_q.push_back(data1)` / `tr.data2_q.push_back(data2)`
3. 检测到 `ed=1` → `analysis_port.write(tr_clone)` — **只发一次，包含本次全部数据**
4. Monitor **不填充** `data_num` 或 `logic_en` 字段

### 2.3 Reference Model 必须用 data1_q.size() ★★★ CRITICAL

由于monitor批量发送且不填充data_num，Reference Model的循环边界必须使用`tr.data1_q.size()`：

```systemverilog
function void write_op_in(op_in_seq_item#(W) tr);
  // ✗ 错误：tr.data_num 可能是0（monitor不填充）
  for (int i = 0; i < tr.data_num; i++) begin ...

  // ✓ 正确：使用data1_q的实际大小
  for (int i = 0; i < tr.data1_q.size(); i++) begin ...
endfunction
```

### 2.4 Sequence 中必须设置 data_num ★★★ CRITICAL

op_in_driver按`data_num`循环发送数据，sequence中必须显式设置：

```systemverilog
task drive_op_in(bit[3:0] d1[$], bit[3:0] d2[$]);
  op_in_sequence#(W) seq;
  seq = op_in_sequence#(W)::type_id::create("seq");
  seq.data_num = d1.size();   // ★ 必须设置
  seq.data1_q = d1;           // 直接赋值
  seq.data2_q = d2;
  seq.start(sequencer);
endtask
```

---

## 3. Sequence 与 Sequencer 架构

### 3.1 参数化 Sequence 必须在对应类型的 Sequencer 上启动 ★★★ CRITICAL

**问题**：`op_in_sequence#(W) start(m_sequencer)` 会导致 `start_item` 永远阻塞，因为 virtual sequencer 与参数化 sequence 的 sequencer 类型不匹配。

**正确模式**：
```systemverilog
task drive_op_in(bit[3:0] d1[$], bit[3:0] d2[$]);
  op_in_sequence#(W) op_in_seq;
  xxx_v_sequencer vsqr;
  op_in_sequencer#(W) op_in_sqr;

  if (!$cast(vsqr, m_sequencer)) begin
    `uvm_error("VSEQ", "Failed to cast m_sequencer")
    return;
  end
  op_in_sqr = vsqr.op_in_data_sqr;  // 获取 child sequencer

  op_in_seq = op_in_sequence#(W)::type_id::create("op_in_seq");
  op_in_seq.data_num = d1.size();
  op_in_seq.data1_q = d1;
  op_in_seq.data2_q = d2;
  op_in_seq.start(op_in_sqr);  // ★ 在正确的 sequencer 上启动
endtask
```

### 3.2 Virtual Sequence 使用 m_sequencer 前必须做类型转换 ★★ IMPORTANT

```systemverilog
xxx_v_sequencer vsqr;
if (!$cast(vsqr, m_sequencer)) begin
  `uvm_error("CAST", "m_sequencer is not xxx_v_sequencer")
  return;
end
// 现在可以访问 vsqr.child_sqr 等
```

### 3.3 推荐：在 base_v_sequence 中封装 helper ★★ IMPORTANT

```systemverilog
class xxx_base_v_sequence extends xxx_v_sequence_base;
  `uvm_object_utils(xxx_base_v_sequence)

  // Helper: 写寄存器
  task write_ctrl(bit[1:0] val);
    uvm_status_e status;
    env_cfg.rgm.CTRL_reg.field.set(val);
    env_cfg.rgm.CTRL_reg.update(status, UVM_FRONTDOOR, .parent(this));
    `uvm_info("VSEQ", $sformatf("Wrote CTRL.field = %0d", val), UVM_MEDIUM)
  endtask

  // Helper: 驱动多个数据对
  task drive_op_in(bit[3:0] d1[$], bit[3:0] d2[$]);
    op_in_sequence#(4) seq;
    seq = op_in_sequence#(4)::type_id::create("seq");
    seq.data_num = d1.size();
    seq.data1_q = d1;
    seq.data2_q = d2;
    seq.start(p_sequencer.op_in_data_sqr);
  endtask

  // Helper: 驱动单个数据对
  task drive_single(bit[3:0] in1, bit[3:0] in2);
    bit[3:0] q1[$], q2[$];
    q1.push_back(in1);
    q2.push_back(in2);
    drive_op_in(q1, q2);
  endtask

  // Helper: 等待复位完成
  task wait_for_reset(int delay_ns = 150);
    #(delay_ns * 1ns);
  endtask
endclass
```

### 3.4 禁止使用 uvm_do_on 驱动 op_in_sequence ★★★ CRITICAL

```systemverilog
// ✗ 禁止 — VCS不支持queue约束 + 嵌套randomize问题
`uvm_do_on_with(op_in_seq, sqr, { data1_q == local_q; })

// ✓ 正确 — 直接赋值 + start()
op_in_seq.data1_q = d1;
op_in_seq.start(sqr);
```

---

## 4. Package 结构与 Include 规范

### 4.1 _pkg.svh 文件分类

| 文件类型 | 包含内容 | 使用场景 |
|----------|----------|----------|
| 真正的 package 文件 | `package...endpackage` | 被 import 在其他 package 或 tb_top 的 import 区 |
| include 文件 | 只有 `include` 和类定义 | 被 `` `include`` 在 module 内（tb_top） |

### 4.2 Include 文件中的 import 原则

在 include 文件中，如果类需要引用其他 package 中的类型，在该类定义之前加 import：

```systemverilog
`ifndef XXX_BASE_V_SEQUENCE__SV
`define XXX_BASE_V_SEQUENCE__SV

import xxx_env_pkg::*;  // 在类定义之前 import

class xxx_base_v_sequence extends xxx_v_sequence_base;
  xxx_v_sequencer vsqr;
  ...
endclass
`endif
```

### 4.3 禁止在 Module 内 include 带 package 的文件

- 真正的 package 文件（`package...endpackage`）只能被 import，不能被 include
- 在 tb_top 内使用的 `_pkg.svh` 必须是 include 文件

### 4.4 Package include 顺序

```systemverilog
// all_testcases.svh
import xxx_vseq_base_pkg::*;
import xxx_base_test_pkg::*;

`include "xxx_v_sequence_pkg.svh"   // ★ vseq 必须先 include
`include "xxx_testcase_pkg.svh"     // ★ test 后 include（test 引用 vseq 类名）
```

依赖链：`reg_model_pkg → env_pkg → reg_seq_pkg → vseq_base_pkg → test_base_pkg → vseq_pkg → test_pkg`

---

## 5. Register Model 访问

### 5.1 使用 get_mirrored_value() 读取寄存器字段 ★★★ CRITICAL

**推荐方式**（立即获取，无总线延迟）：
```systemverilog
function void write_op_in(op_in_item_t tr);
  bit[1:0] cur_sel;
  cur_sel = env_cfg.rgm.CTRL_reg.field.get_mirrored_value();
  // 计算期望值...
endfunction
```

**为什么不用 UVM_FRONTDOOR 读取**：
- `uvm_reg::read()` 是 **task** 不是 function，不能在 `write()` 函数中调用
- FRONTDOOR 读取需要 APB 总线事务，有延迟
- `get_mirrored_value()` 直接返回镜像值，无总线访问

### 5.2 寄存器写入后需要等待 ★★ IMPORTANT

```systemverilog
write_ctrl(2'h0);  // 通过 APB 总线写寄存器
#100ns;            // ★ 等待写入完成 + DUT 内部更新
drive_data(...);   // 然后再发 stimulus
```

### 5.3 寄存器 configure 第三个参数 ★★ IMPORTANT

```systemverilog
// ✗ 错误
CTRL_reg.configure(this, "");

// ✓ 正确 — 第三个参数是 HDL path
CTRL_reg.configure(this, null, "RTL");
```

---

## 6. 已知模板 Bug 的修复

### 6.1 op_in_sequence 不需要 get_response ★★★ CRITICAL

op_in_sequence 的 body() 只用 `start_item/finish_item`，driver 调用 `item_done()` 完成后 sequence 就继续，不调用 `get_response`。

```systemverilog
task op_in_sequence::body();
  op_in_seq_item req;
  req = op_in_seq_item::type_id::create("req");
  start_item(req);
  req.data_num = this.data_num;
  req.data1_q = this.data1_q;
  req.data2_q = this.data2_q;
  finish_item(req);
  // ★ 不要调用 get_response(req) — driver只调用item_done()
endtask
```

### 6.2 PH_TIMEOUT 修复 ★★★ CRITICAL

APB slave memory sequence 在 run_phase 保持活跃，导致仿真挂死。使用 event 协调：

```systemverilog
class xxx_test_base extends uvm_test;
  event test_done;

  task run_phase(uvm_phase phase);
    super.run_phase(phase);
    phase.raise_objection(this);
    @test_done;           // 等待 post_main_phase 触发
    phase.drop_objection(this);
  endtask

  task post_main_phase(uvm_phase phase);
    super.post_main_phase(phase);
    ->test_done;          // 触发事件，让 run_phase 退出
  endtask
endclass
```

### 6.3 寄存器模型成员变量命名 ★★★ CRITICAL

模板生成的 reg_model 中，成员变量名可能与类名相同导致 scope resolution error：

```systemverilog
// ✗ 错误 — 成员变量名与类名冲突
class xxx_reg_model extends uvm_reg_block;
  rand CTRL CTRL;
endclass

// ✓ 正确 — 加 _reg 后缀
class xxx_reg_model extends uvm_reg_block;
  rand CTRL CTRL_reg;
endclass
```

修复后需要更新所有引用（使用 `grep -r "rgm\.CTRL[^_]"` 检查遗漏）。

### 6.4 final_phase report_server ★★ IMPORTANT

```systemverilog
function void xxx_test_base::final_phase(uvm_phase phase);
  uvm_report_server server;
  int err_num;
  super.final_phase(phase);
  server = uvm_report_server::get_server();  // ✓ 使用 get_server()
  // ✗ 不要用 server = new(); — 新建的server没有error计数
  err_num = server.get_severity_count(UVM_ERROR)
          + server.get_severity_count(UVM_FATAL);
  if(!err_num) `uvm_info("UVM_CASE_PASS", "**********UVM TEST PASSED*********", UVM_NONE)
  else         `uvm_info("UVM_CASE_FAIL", "**********UVM TEST FAILED*********", UVM_NONE)
endfunction
```

---

## 7. RTL Bug 识别模式

### 7.1 单一操作 PASS 但组合测试 FAIL → Scoreboard 时序问题

如果每种功能单独测试都PASS，但Smoke Test（多操作连续执行）FAIL，多半是Scoreboard时序偏移问题，不是RTL Bug。

**判断方法**：
- 检查 `SCB_MISMATCH` 报告的 expected 和 actual 值
- 如果 expected 值对应的是另一种操作的结果 → 队列错位
- 如果 expected 值与 Spec 一致但 actual 值不符合 Spec → 可能是 RTL Bug

### 7.2 运算符优先级导致的 RTL Bug ★★★ CRITICAL

Verilog 运算符优先级陷阱：

| 运算符 | 优先级 | 常见错误 |
|--------|--------|----------|
| `^~` (XNOR) | 高于 `\|` | `a ^~ b \| mask` 等价于 `(a ^~ b) \| mask`，不是 `a ^~ (b \| mask)` |
| `&` (AND) | 高于 `\|` | `a & b \| c` 等价于 `(a & b) \| c` |
| `==` | 高于 `?:` | `a == b ? c : d` 正确，但复杂表达式需加括号 |

**识别方法**：
- 如果DUT输出的LSB/某一位始终为0或1 → 检查是否有 `| constant` 或 `& constant` 操作
- 如果XNOR/XOR结果被"污染" → 检查运算符优先级

### 7.3 Bug 被掩盖的条件

当RTL Bug导致的结果恰好与正确结果相同时，Bug被掩盖。例如：
- XNOR Bug `| 4'b1` 强制LSB为1
- 当正确XNOR结果的LSB本身就是1时（如 `0xF ^~ 0xF = 0xF`），Bug不可见
- 只有当正确XNOR结果的LSB是0时（如 `0xA ^~ 0x5 = 0x0`），Bug才暴露

**测试设计建议**：选择测试向量时，确保覆盖Bug可能被掩盖和暴露两种情况。

---

## 8. 编译/仿真问题速查表

| 错误现象 | 可能原因 | 检查命令 |
|----------|----------|----------|
| `SCB_MISMATCH` 多操作场景 | Scoreboard时序偏移 | 改用队列+run_phase方案 |
| `SCB_MISMATCH` 单操作场景 | Reference Model计算错误 | 对比Spec，检查get_mirrored_value |
| 仿真挂死不退出 | PH_TIMEOUT | 检查`event test_done`和`post_main_phase` |
| `start_item` 永远阻塞 | sequence在错误的sequencer上启动 | 确认`start(child_sqr)`而非`start(m_sequencer)` |
| `token is 'initial'` | wire声明位置错误 | wire必须在itf_inst之后、DUT之前 |
| `Token 'XXX' is not a class` | 寄存器变量名与类名冲突 | 成员变量加`_reg`后缀 |
| `Syntax error: token is '#'` | 参数化类型声明语法错误 | 使用typedef |
| `Unsupported: 'package' inside module` | _pkg.svh包含package声明 | 确认是被import还是被include |
| 多位端口数据为X | 隐式wire默认1-bit | 显式声明`wire[N-1:0]` |
| `Invalid data_width('d32)` | APB VIP默认最大宽度8 | 编译添加`+define+SVT_APB_MAX_DATA_WIDTH=32` |
| `Could not find member 'write'` | uvm_analysis_imp后缀不匹配 | 确认`write_<suffix>`函数名 |
| `data_num` 为0导致无数据 | monitor不填充data_num | Reference Model用`data1_q.size()` |

---

## 版本历史

| 版本 | 日期 | 更新内容 |
|------|------|----------|
| v1.0 | 2026-05-16 | 初版 |
| v2.0 | 2026-05-28 | 全面重写：(1)泛化为通用规范，去除模块特定引用 (2)新增Scoreboard比较机制选型分析 (3)新增op_in Driver三阶段协议详解 (4)新增RTL Bug识别模式 (5)重组目录结构 |
