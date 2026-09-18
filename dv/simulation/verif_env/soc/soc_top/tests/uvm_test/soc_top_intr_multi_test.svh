`ifndef SOC_TOP_INTR_MULTI_TEST__SV
`define SOC_TOP_INTR_MULTI_TEST__SV

// ---------------------------------------------------------------------------
// SoC F4: 多中断源路由独立性 —— 配合 c_case/intr_multi_route/intr_multi_route.c
// C 侧：WDT(#27) + TIM1(#17) + PWM(#25) 三源同时 pending，逐个清
// UVM 侧：并行采样 pad_vic_int_vld[17]/[25]/[27]，要求：
//   1. 三条线各自经历 断言→撤销 完整电平窗口
//   2. 存在三线同时断言的重叠窗口（多源并发路由的证据）
// 路由表依据 core_top.v:537-556
// ---------------------------------------------------------------------------
class soc_top_intr_multi_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_intr_multi_test)

  bit seen17, seen25, seen27;
  bit clr17,  clr25,  clr27;
  bit overlap;          // 三线同高的窗口

  extern function new(string name="soc_top_intr_multi_test", uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_intr_multi_test::new(string name="soc_top_intr_multi_test",
                                      uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_intr_multi_test::run_phase(uvm_phase phase);
  uvm_hdl_data_t vic_val;
  seen17 = 0; seen25 = 0; seen27 = 0;
  clr17  = 0; clr25  = 0; clr27  = 0;
  overlap = 0;
  fork
    begin : vic_mon
      forever begin
        #50ns;
        if (!uvm_hdl_read("tb_top.dut.x_cpu_top.pad_vic_int_vld", vic_val)) begin
          `uvm_error("INTR_MULTI", "uvm_hdl_read failed for pad_vic_int_vld")
        end
        else begin
          if (vic_val[17] && vic_val[25] && vic_val[27]) overlap = 1'b1;
          if (vic_val[17]) seen17 = 1; else if (seen17) clr17 = 1;
          if (vic_val[25]) seen25 = 1; else if (seen25) clr25 = 1;
          if (vic_val[27]) seen27 = 1; else if (seen27) clr27 = 1;
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (!seen17) `uvm_error("INTR_MULTI", "TIM1 route missing: pad_vic_int_vld[17] never asserted")
  if (!seen25) `uvm_error("INTR_MULTI", "PWM route missing: pad_vic_int_vld[25] never asserted")
  if (!seen27) `uvm_error("INTR_MULTI", "WDT route missing: pad_vic_int_vld[27] never asserted")
  if (!clr17)  `uvm_error("INTR_MULTI", "pad_vic_int_vld[17] never deasserted")
  if (!clr25)  `uvm_error("INTR_MULTI", "pad_vic_int_vld[25] never deasserted")
  if (!clr27)  `uvm_error("INTR_MULTI", "pad_vic_int_vld[27] never deasserted")
  if (!overlap)
    `uvm_error("INTR_MULTI", "no window with all three lines asserted concurrently")
  if (seen17 && seen25 && seen27 && clr17 && clr25 && clr27 && overlap)
    `uvm_info("INTR_MULTI",
      "TIM1[17]/PWM[25]/WDT[27] all asserted concurrently, cleared independently", UVM_LOW)
endtask: run_phase

`endif
