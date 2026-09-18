`ifndef SOC_TOP_INTR_NESTING_TEST__SV
`define SOC_TOP_INTR_NESTING_TEST__SV

// ---------------------------------------------------------------------------
// SoC F5: CLIC 中断嵌套 —— bug 检测器（配合 c_case/intr_nesting/intr_nesting.c）
//
// RTL 实证背景（2026-09-19，多轮设计迭代取证）：
//   嵌套一旦发生（高级 #18 级别3 抢占低级 #17 级别1 的 ISR 窗口），
//   系统即进入不可逆劣化：
//     - GPR/内存倒计时的窗口循环在 ch2 重触发下永不收敛（29790 次嵌套）
//     - 最终在 handler 恢复序列出现假异常（lw a1,4(sp) 假 load fault）
//       → crt0 向量表 __dummy(0x600) 非法指令 → 异常风暴
//     - main 轮询计数被破坏，既不成功也不超时（C 侧失去自报告能力）
//   无嵌套时（ch2 在窗口后才到期/被停）系统完全正常（干净完成）。
//   结论：E902 CLIC 嵌套入口路径疑似 RTL 缺陷，记录为 SoC 级发现。
//
// UVM 侧职责（回归安全）：
//   1. 全局看门狗 set_timeout(45ms)：C 侧挂死时 UVM_FATAL 终止仿真
//      （正确 RTL 下 C 侧 ~23ms 内完成并 sim_end）
//   2. 监视 tim0_wic_intr[1]：确认嵌套尝试确实发生过（区分
//      "嵌套劣化" 与 "高级源根本没触发"）
// ---------------------------------------------------------------------------
class soc_top_intr_nesting_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_intr_nesting_test)

  bit       saw_high_line;     // tim0_wic_intr[1] 曾断言（嵌套尝试证据）

  extern function new(string name="soc_top_intr_nesting_test", uvm_component parent);
  extern virtual function void build_phase(uvm_phase phase);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_intr_nesting_test::new(string name="soc_top_intr_nesting_test",
                                        uvm_component parent);
  super.new(name, parent);
endfunction

function void soc_top_intr_nesting_test::build_phase(uvm_phase phase);
  super.build_phase(phase);
  // 看门狗：boot ~20ms + 窗口/清理 + 轮询；超时 = 嵌套劣化（bug 检测器触发）
  uvm_top.set_timeout(45ms, 1);
endfunction

task soc_top_intr_nesting_test::run_phase(uvm_phase phase);
  fork
    begin : line_mon
      forever begin
        #50ns;
        if (tb_top.dut.x_cpu_top.tim0_wic_intr[1] === 1'b1)
          saw_high_line = 1'b1;
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (saw_high_line)
    `uvm_info("INTR_NEST",
      "nest attempt observed on tim0_wic_intr[1]; C case completed - see C-side verdict", UVM_LOW)
  else
    `uvm_warning("INTR_NEST",
      "tim0_wic_intr[1] never asserted - high-priority source did not fire (check TIM2 config)")
endtask: run_phase

`endif
