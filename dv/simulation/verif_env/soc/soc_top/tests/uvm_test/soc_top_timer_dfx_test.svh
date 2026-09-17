/*
Copyright (c) 2019 Alibaba Group Holding Limited

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
`ifndef SOC_TOP_TIMER_DFX_TEST__SV
`define SOC_TOP_TIMER_DFX_TEST__SV

//====================================================================
// soc_top_timer_vic_route_test : F10 interrupt routing verification
//   Firmware : timer_dual_ch_parallel.c  (TIM0 Timer1 + Timer2 both fire)
//   DUT side : core_top.v  ip_cpu_int_vld[18:17] = tim0_wic_intr
//             intr17 = TIM0 Timer1 , intr18 = TIM0 Timer2
//             observed on x_cpu_top.pad_vic_int_vld[18:17]
//   Run cmd  : make all C_TEST=timer_dual_ch_parallel/timer_dual_ch_parallel.c \
//            UTEST=soc_top_timer_vic_route_test
//====================================================================
class soc_top_timer_vic_route_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_timer_vic_route_test)

  // observed routing flags
  bit seen_bit17;
  bit seen_bit18;

  extern function new(string name="soc_top_timer_vic_route_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_timer_vic_route_test::new(string name="soc_top_timer_vic_route_test",
                                           uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_timer_vic_route_test::run_phase(uvm_phase phase);
  uvm_hdl_data_t vic_val;
  fork
    begin : vic_monitor
      while (1) begin
        #50ns;
        if (!uvm_hdl_read("tb_top.dut.x_cpu_top.pad_vic_int_vld", vic_val)) begin
          `uvm_error("VIC_ROUTE",
            "uvm_hdl_read failed for tb_top.dut.x_cpu_top.pad_vic_int_vld")
        end
        else begin
          if (vic_val[17]) seen_bit17 = 1'b1;
          if (vic_val[18]) seen_bit18 = 1'b1;
        end
      end
    end
  join_none

  // parent (soc_top_for_c_case_test) blocks on cpu_c_finish_e
  // then drops its objection and returns => C side has PASSED/FAILED
  super.run_phase(phase);

  disable fork;

  if (!seen_bit17)
    `uvm_error("VIC_ROUTE",
      "TIM0 Timer1 route missing: pad_vic_int_vld[17] never asserted")
  if (!seen_bit18)
    `uvm_error("VIC_ROUTE",
      "TIM0 Timer2 route missing: pad_vic_int_vld[18] never asserted")

  if (seen_bit17 && seen_bit18)
    `uvm_info("VIC_ROUTE",
      "TIM0 Timer1/Timer2 routed to pad_vic_int_vld[18:17]", UVM_LOW)
endtask: run_phase

//====================================================================
// soc_top_timer_etb_trig_test : F3/F11 ETB hardware trigger
//   Firmware : timer_etb_hw_trig.c  (SW writes ControlReg=0x12 only;
//            enable driven by UVM force on tie-0 nets)
//   DUT side : apb0_sub_top.tie-0 nets:
//            etb_tim0_trig_en_on1, etb_tim0_trig_en_off1,
//            timer0_tim1_etb_trig  (F11 pulse)
//   Run cmd  : make all C_TEST=timer_etb_hw_trig/timer_etb_hw_trig.c \
//            UTEST=soc_top_timer_etb_trig_test
//====================================================================
class soc_top_timer_etb_trig_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_timer_etb_trig_test)

  bit  etb_trig_seen;
  bit  drv_done;
  bit  mon_done;
  event etb_trig_e;

  extern function new(string name="soc_top_timer_etb_trig_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_timer_etb_trig_test::new(string name="soc_top_timer_etb_trig_test",
                                          uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_timer_etb_trig_test::run_phase(uvm_phase phase);
  fork
    // ---- ETB enable driver: wait C config -> pulse on1 -> wait trig -> pulse off1 ----
    // NOTE: use language force/release with XMR (compile has plain -debug_access
    //       which grants read but not VPI force; uvm_hdl_force fails here)
    // NOTE: do NOT force at a fixed time — if ETB-on lands before the C firmware
    //       finishes writing ControlReg=0x12, the C writes wipe the enable and the
    //       test deadlocks. Poll the internal ControlReg instead.
    begin : etb_driver
      // wait until C firmware has written ControlReg=0x12 (hw_trig_en + user-defined,
      // enable=0). If the XMR path is wrong this times out and errors out.
      fork
        begin : poll_cfg
          // case-inequality: reg is X before reset deasserts; `!=` on X yields X
          // (=> loop would exit immediately at t=0 and force far too early)
          while (tb_top.dut.x_pdu_top.x_sub_apb0_top.x_tim0_sec_top.x_tim_top
                 .x_timers_top.U_TIMERS_APBIF.timer1controlreg !== 32'h12)
            #100ns;
        end
        begin : cfg_timeout
          #500us;
          `uvm_error("ETB_TRIG",
            "timeout waiting C config ControlReg==0x12 (XMR path suspect)")
        end
      join_any
      disable fork;

      // F3 enable
      force tb_top.dut.x_pdu_top.x_sub_apb0_top.etb_tim0_trig_en_on1 = 1'b1;
      #1us;
      release tb_top.dut.x_pdu_top.x_sub_apb0_top.etb_tim0_trig_en_on1;

      // wait for the trig pulse (or monitor end) before issuing off1
      wait (etb_trig_seen || mon_done);

      #2us;
      // F11 clear enable (fall)
      force tb_top.dut.x_pdu_top.x_sub_apb0_top.etb_tim0_trig_en_off1 = 1'b1;
      #1us;
      release tb_top.dut.x_pdu_top.x_sub_apb0_top.etb_tim0_trig_en_off1;
      drv_done = 1'b1;
    end

    // ---- ETB trig monitor: level-sensitive wait (pulse is only 1 pclk wide,
    //      polling at #100ns could miss it) with 200us timeout ----
    begin : etb_trig_monitor
      fork
        begin : mon_wait
          wait (tb_top.dut.x_pdu_top.x_sub_apb0_top.timer0_tim1_etb_trig === 1'b1);
          etb_trig_seen = 1'b1;
          -> etb_trig_e;
        end
        begin : mon_timeout
          #200us;
        end
      join_any
      disable fork;
      mon_done = 1'b1;
    end
  join_none

  // parent (soc_top_for_c_case_test) blocks on cpu_c_finish_e
  // then drops its objection and returns => C side has PASSED/FAILED
  super.run_phase(phase);

  disable fork;

  // defensive release in case C side finished mid-force
  release tb_top.dut.x_pdu_top.x_sub_apb0_top.etb_tim0_trig_en_on1;
  release tb_top.dut.x_pdu_top.x_sub_apb0_top.etb_tim0_trig_en_off1;

  if (!etb_trig_seen)
    `uvm_error("ETB_TRIG",
      "timer0_tim1_etb_trig pulse not observed within 200us (F11)")
  else
    `uvm_info("ETB_TRIG", "timer0_tim1_etb_trig pulse observed (F11)", UVM_LOW)
endtask: run_phase

`endif
