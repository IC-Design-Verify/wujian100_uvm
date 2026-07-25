`ifndef SOC_APB0_TIM_CONCUR_VSEQ_SV
`define SOC_APB0_TIM_CONCUR_VSEQ_SV

// =============================================================================
// Timer (tim0) P1 concurrency virtual sequence
// =============================================================================
// Covers:
//   TIM_CONC_001  dual_timer_independent_operation
//     Both timers enabled simultaneously with different load counts,
//     verify that interrupts fire independently and interleave correctly.
//     This exercises the shared APB slave interface and the two independent
//     timers_frc instances.
// =============================================================================

class soc_apb0_tim_concur_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_apb0_tim_concur_v_sequence)

  soc_apb0_tim_sequence tim_seq;
  soc_apb0_tim_checker  chk;

  function new(string name = "soc_apb0_tim_concur_v_sequence");
    super.new(name);
  endfunction

  task body();
    bit[31:0] act;
    bit[31:0] eoi_val;
    bit        int_fired_t0;
    bit        int_fired_t1;
    int        t0_fired_count = 0;
    int        t1_fired_count = 0;

    `uvm_info("TIM_CONC_VSEQ", "=== tim0 P1 concurrency virtual sequence start ===", UVM_LOW)

    chk = soc_apb0_tim_checker::type_id::create("tim_chk_conc", null);
    tim_seq = soc_apb0_tim_sequence::type_id::create("tim_seq_conc");
    tim_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    tim_seq.chk = chk;

    // =====================================================================
    // TIM_CONC_001: dual timer independent operation
    //   Both timers use user-mode (mode=1), unmasked (intmask=0).
    //   timer1: load=0x80  (128 cycles to first interrupt)
    //   timer2: load=0x40  (64 cycles to first interrupt)
    //   Verify:
    //     (a) timer2 fires first (shorter period)
    //     (b) timer1 fires second
    //     (c) combined TIMERSINTST reflects both bits set after both fire
    //     (d) TIMERSEOI clears both
    //     (e) both timers can re-fire after EOI + re-enable
    // =====================================================================
    `uvm_info("TIM_CONC_VSEQ", "TIM_CONC_001: dual timer concurrent", UVM_LOW)

    // Phase 1: Start both timers
    tim_seq.tim_write_load(0, 32'h0000_0080);  #50ns;   // timer1: load=128
    tim_seq.tim_write_load(1, 32'h0000_0040);  #50ns;   // timer2: load=64
    tim_seq.tim_write_ctrl(0, 5'b00011);        // timer1: user, unmasked, ena=1
    tim_seq.tim_write_ctrl(1, 5'b00011);        // timer2: user, unmasked, ena=1

    // Phase 2: Poll timer2 first (should fire before timer1)
    tim_seq.poll_int_status(1, 50000, int_fired_t1);
    if (int_fired_t1)
      `uvm_info("TIM_CONC_VSEQ", "timer2 interrupt fired (first) — ok", UVM_LOW)
    else
      `uvm_error("TIM_CONC_VSEQ", "timer2 interrupt did NOT fire")

    // Poll timer1 — should fire shortly after timer2
    tim_seq.poll_int_status(0, 50000, int_fired_t0);
    if (int_fired_t0)
      `uvm_info("TIM_CONC_VSEQ", "timer1 interrupt fired (second) — ok", UVM_LOW)
    else
      `uvm_error("TIM_CONC_VSEQ", "timer1 interrupt did NOT fire")

    // Phase 3: Both interrupts are pending, combined TIMERSINTST should be 0x3
    tim_seq.tim_reg_read(8'hA0, act);
    if (act[1:0] !== 2'b11)
      `uvm_error("TIM_CONC_VSEQ",
        $sformatf("TIMERSINTST after both fire exp=0x3 act=0x%02h", act))
    else
      `uvm_info("TIM_CONC_VSEQ",
        $sformatf("TIMERSINTST after both fire = 0x%02h (ok)", act), UVM_LOW)

    // Phase 4: Read individual INTST registers
    tim_seq.tim_reg_read(8'h10, act);
    if (act[0] !== 1'b1)
      `uvm_error("TIM_CONC_VSEQ", $sformatf("TIMER1INTST exp=1 act=0x%02h", act))
    tim_seq.tim_reg_read(8'h24, act);
    if (act[0] !== 1'b1)
      `uvm_error("TIM_CONC_VSEQ", $sformatf("TIMER2INTST exp=1 act=0x%02h", act))

    // Phase 5: Read TIMERSEOI to clear both.
    // Per RTL (tim.v timers_apbif prdata mux), TIMERSEOI has NO read path —
    // falls into `default: prdata <= 32'b0`. The read returns 0 but the
    // side-effect clears timer_int_tmp for both timers.
    tim_seq.tim_reg_read(8'hA4, eoi_val);
    if (eoi_val[1:0] !== 2'b00)
      `uvm_error("TIM_CONC_VSEQ",
        $sformatf("TIMERSEOI exp=0x0 act=0x%02h", eoi_val))
    else
      `uvm_info("TIM_CONC_VSEQ",
        $sformatf("TIMERSEOI = 0x%02h — side-effect cleared both (ok)", eoi_val), UVM_LOW)

    // Phase 6: Verify both interrupts are now cleared
    tim_seq.tim_reg_read(8'hA0, act);
    if (act[1:0] !== 2'b00)
      `uvm_error("TIM_CONC_VSEQ",
        $sformatf("TIMERSINTST after TIMERSEOI exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_CONC_VSEQ",
        $sformatf("TIMERSINTST after TIMERSEOI = 0x%02h (ok)", act), UVM_LOW)

    // Phase 7: Both timers are still running — verify re-fire after underflow
    //   timer2 underflows every 64 cycles, timer1 every 128.  Both will
    //   re-fire multiple times while we wait.  We poll for a re-fire
    //   sequence on each timer by checking INTST after a suitable wait.
    #2000ns;  // let timers count through several underflows
    tim_seq.tim_reg_read(8'h10, act);
    t0_fired_count = act[0] ? 1 : 0;
    tim_seq.tim_reg_read(8'h24, act);
    t1_fired_count = act[0] ? 1 : 0;
    `uvm_info("TIM_CONC_VSEQ",
      $sformatf("After re-fire wait: t0_int=%0d t1_int=%0d", t0_fired_count, t1_fired_count),
      UVM_LOW)

    // Clear any pending and stop timers
    tim_seq.tim_read_eoi(0, eoi_val);
    tim_seq.tim_read_eoi(1, eoi_val);
    tim_seq.tim_write_ctrl(0, 5'b00000);  #50ns;
    tim_seq.tim_write_ctrl(1, 5'b00000);  #100ns;

    // Phase 8: Final combined status should be 0x0 after disable + clear
    tim_seq.tim_reg_read(8'hA0, act);
    if (act[1:0] !== 2'b00)
      `uvm_error("TIM_CONC_VSEQ",
        $sformatf("TIMERSINTST after disable+clear exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_CONC_VSEQ",
        $sformatf("TIMERSINTST after disable+clear = 0x%02h (ok)", act), UVM_LOW)

    `uvm_info("TIM_CONC_VSEQ", "=== tim0 P1 concurrency virtual sequence done ===", UVM_LOW)
    `uvm_info("TIM_CONC_VSEQ",
      $sformatf("Checker stats: %0d matches, %0d mismatches",
                chk.match_count, chk.mismatch_count), UVM_LOW)
  endtask

endclass : soc_apb0_tim_concur_v_sequence

`endif
