`ifndef SOC_TOP_RTC_DFX_TEST__SV
`define SOC_TOP_RTC_DFX_TEST__SV

// F12: RTC 中断路由——配合 c_case/rtc_vic_route/rtc_vic_route.c
// C 侧：match 触发（ien=1, mask=0）→ 保持 → EOI 撤销
// UVM 侧：轮询 pad_vic_int_vld[26]（core_top.v:546 ip_cpu_int_vld[26]=rtc_wic_intr）
class soc_top_rtc_vic_route_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_rtc_vic_route_test)

  bit seen_bit26;
  bit seen_bit26_clear;

  extern function new(string name="soc_top_rtc_vic_route_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_rtc_vic_route_test::new(string name="soc_top_rtc_vic_route_test",
                                           uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_rtc_vic_route_test::run_phase(uvm_phase phase);
  uvm_hdl_data_t vic_val;
  fork
    begin : vic_monitor
      while (1) begin
        #20ns;
        if (!uvm_hdl_read("tb_top.dut.x_cpu_top.pad_vic_int_vld", vic_val)) begin
          `uvm_error("RTC_VIC_ROUTE",
            "uvm_hdl_read failed for tb_top.dut.x_cpu_top.pad_vic_int_vld")
        end
        else begin
          if (vic_val[26]) seen_bit26 = 1'b1;
          else if (seen_bit26) seen_bit26_clear = 1'b1;
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (!seen_bit26)
    `uvm_error("RTC_VIC_ROUTE",
      "RTC route missing: pad_vic_int_vld[26] never asserted")
  if (!seen_bit26_clear)
    `uvm_error("RTC_VIC_ROUTE",
      "RTC route clear missing: pad_vic_int_vld[26] never deasserted after EOI")
  if (seen_bit26 && seen_bit26_clear)
    `uvm_info("RTC_VIC_ROUTE",
      "RTC routed to pad_vic_int_vld[26], cleared after EOI", UVM_LOW)
endtask: run_phase

// F10: RTC ETB 触发输出——配合 c_case/rtc_etb_trig/rtc_etb_trig.c
// RTL: rtc_etb_trig = cmp_res 寄存在 i_rtc_ext_clk 域（rtc.v rtc_ig），
//      match 时产生 1 个 ext_clk 周期（~5ns）脉冲——不能用定时轮询（会漏），
//      用 wait(===) 电平敏感捕获（参照 soc_top_wdt_rpl_pulse_test 模式）。
// SoC 集成注：rtc_etb_trig 在 aou_top 为内部线网（aou_top.v:420/:586），
//      wujian100_open_top 未引出，无 ETB 消费者——差异记录。
class soc_top_rtc_etb_trig_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_rtc_etb_trig_test)

  int pulse_count;

  extern function new(string name="soc_top_rtc_etb_trig_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_rtc_etb_trig_test::new(string name="soc_top_rtc_etb_trig_test",
                                          uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_rtc_etb_trig_test::run_phase(uvm_phase phase);
  pulse_count = 0;
  fork
    begin : etb_monitor
      forever begin
        wait (tb_top.dut.x_aou_top.rtc_etb_trig === 1'b1);
        pulse_count++;
        wait (tb_top.dut.x_aou_top.rtc_etb_trig === 1'b0);
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (pulse_count < 2)
    `uvm_error("RTC_ETB_TRIG",
      $sformatf("rtc_etb_trig pulse count=%0d, expect >=2 (C 侧制造 2 次 match)",
        pulse_count))
  else
    `uvm_info("RTC_ETB_TRIG",
      $sformatf("rtc_etb_trig pulsed %0d times on match events", pulse_count),
      UVM_LOW)
endtask: run_phase

`endif
