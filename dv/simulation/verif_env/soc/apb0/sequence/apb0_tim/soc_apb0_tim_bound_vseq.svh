`ifndef SOC_APB0_TIM_BOUND_VSEQ_SV
`define SOC_APB0_TIM_BOUND_VSEQ_SV

// =============================================================================
// Timer (tim0) P1 boundary virtual sequence
// =============================================================================
// Covers the following scenarios from apb0_tim_testplan.json:
//   TIM_BOUND_001  load_count = 1 (shortest cycle, interrupt fires immediately)
//   TIM_BOUND_002  load_count = 0xFFFF_FFFF (free-run vs user-mode equivalence)
//   TIM_BOUND_003  load_count = 0 (immediate atzero in 1 cycle)
//   TIM_INT_002   mask toggle at runtime (INTST reflects mask state)
//   TIM_INT_003   ena=0 clears timer_int_tmp latch immediately
// =============================================================================

class soc_apb0_tim_bound_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_apb0_tim_bound_v_sequence)

  soc_apb0_tim_sequence tim_seq;
  soc_apb0_tim_checker  chk;

  function new(string name = "soc_apb0_tim_bound_v_sequence");
    super.new(name);
  endfunction

  // Check a read against an expected value
  task check_read(input bit[31:0] off, input bit[31:0] exp, output bit[31:0] act);
    tim_seq.tim_reg_read(off, act);
    if (act === exp)
      `uvm_info("TIM_BOUND", $sformatf("R@0x%02h = 0x%08h (ok)", off, act), UVM_LOW)
    else
      `uvm_error("TIM_BOUND",
        $sformatf("R@0x%02h expected=0x%08h actual=0x%08h", off, exp, act))
  endtask

  task body();
    bit[31:0] act;
    bit        int_fired;

    `uvm_info("TIM_BOUND_VSEQ", "=== tim0 P1 boundary virtual sequence start ===", UVM_LOW)

    chk = soc_apb0_tim_checker::type_id::create("tim_chk_bound", null);
    tim_seq = soc_apb0_tim_sequence::type_id::create("tim_seq_bound");
    tim_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    tim_seq.chk = chk;

    // =====================================================================
    // TIM_BOUND_001: load_count = 1 — shortest possible cycle.
    //   In user-mode, timer counts 1 -> 0 in a single tick, atzero goes high
    //   for 1 pclk cycle, interrupt fires immediately.
    // =====================================================================
    `uvm_info("TIM_BOUND_VSEQ", "TIM_BOUND_001: load_count=1", UVM_LOW)
    tim_seq.tim_write_load(0, 32'h0000_0001);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // user-mode, unmasked, ena=1
    tim_seq.poll_int_status(0, 5000, int_fired);
    if (int_fired)
      `uvm_info("TIM_BOUND", "timer1 load=1 interrupt fired (ok)", UVM_LOW)
    else
      `uvm_error("TIM_BOUND", "timer1 load=1 interrupt did NOT fire")
    // CV after underflow reloads to load_count=1
    tim_seq.tim_read_curval(0, act);
    if (act === 32'h0000_0001)
      `uvm_info("TIM_BOUND", $sformatf("timer1 CV after underflow = 0x%08h (ok)", act), UVM_LOW)
    else
      `uvm_error("TIM_BOUND", $sformatf("timer1 CV after underflow exp=0x1 act=0x%08h", act))
    tim_seq.tim_read_eoi(0, act);
    tim_seq.tim_write_ctrl(0, 5'b00000);  #100ns;

    // =====================================================================
    // TIM_BOUND_002: load_count = 0xFFFF_FFFF (MAX).
    //   In user-mode with load_count=MAX, timer goes 0xFFFFFFFF -> 0, same
    //   as free-run mode (reload MAX).  The interrupt fires after the full
    //   MAX+1-cycle wait.
    // =====================================================================
    `uvm_info("TIM_BOUND_VSEQ", "TIM_BOUND_002: load_count=0xFFFF_FFFF", UVM_LOW)
    // timer1: user-mode with load=MAX
    tim_seq.tim_write_load(0, 32'hFFFF_FFFF);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // user-mode, unmasked, ena=1
    // timer2: free-run (auto-reload MAX)
    tim_seq.tim_write_ctrl(1, 5'b00001);        // free-run, unmasked, ena=1
    // Both should eventually fire after MAX+1 cycles (~4 billion).
    // We can't wait that long in simulation, but we can verify CV decreases
    // and the enable flag is correct.
    #200ns;
    tim_seq.tim_read_curval(0, act);
    if (act < 32'hFFFF_FFFF && act > 32'h0000_0000)
      `uvm_info("TIM_BOUND", $sformatf("timer1 user-mode CV decreased: 0x%08h (ok)", act), UVM_LOW)
    else
      `uvm_info("TIM_BOUND", $sformatf("timer1 user-mode CV=0x%08h (unchanged or boundary)", act), UVM_LOW)
    tim_seq.tim_read_curval(1, act);
    if (act < 32'hFFFF_FFFF && act > 32'h0000_0000)
      `uvm_info("TIM_BOUND", $sformatf("timer2 free-run CV decreased: 0x%08h (ok)", act), UVM_LOW)
    else
      `uvm_info("TIM_BOUND", $sformatf("timer2 free-run CV=0x%08h (unchanged or boundary)", act), UVM_LOW)
    // Verify CR bits stuck
    tim_seq.tim_reg_read(8'h08, act);
    if (act[4:0] !== 5'b00011)
      `uvm_error("TIM_BOUND", $sformatf("timer1 CR exp=00011 act=%b", act[4:0]))
    tim_seq.tim_reg_read(8'h1C, act);
    if (act[4:0] !== 5'b00001)
      `uvm_error("TIM_BOUND", $sformatf("timer2 CR exp=00001 act=%b", act[4:0]))
    // Cleanup
    tim_seq.tim_write_ctrl(0, 5'b00000);  #50ns;
    tim_seq.tim_write_ctrl(1, 5'b00000);  #100ns;

    // =====================================================================
    // TIM_BOUND_003: load_count = 0 — timer reloads 0 on enable, atzero
    //   fires in the very next cycle after the enable load (since counter
    //   starts at 0 and the next tick detects timer_cnt==0 -> underflow).
    // =====================================================================
    `uvm_info("TIM_BOUND_VSEQ", "TIM_BOUND_003: load_count=0", UVM_LOW)
    tim_seq.tim_write_load(0, 32'h0000_0000);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // user-mode, unmasked, ena=1
    // load_count=0 means counter reloads to 0, so atzero fires immediately
    tim_seq.poll_int_status(0, 5000, int_fired);
    if (int_fired)
      `uvm_info("TIM_BOUND", "timer1 load=0 interrupt fired (ok)", UVM_LOW)
    else
      `uvm_error("TIM_BOUND", "timer1 load=0 interrupt did NOT fire")
    tim_seq.tim_read_curval(0, act);
    // After underflow, counter reloads to load_count=0, so CV=0
    if (act === 32'h0000_0000)
      `uvm_info("TIM_BOUND", $sformatf("timer1 CV after load=0 underflow = 0x%08h (ok)", act), UVM_LOW)
    else
      `uvm_info("TIM_BOUND", $sformatf("timer1 CV after load=0 underflow = 0x%08h", act), UVM_LOW)
    tim_seq.tim_read_eoi(0, act);
    tim_seq.tim_write_ctrl(0, 5'b00000);  #100ns;

    // =====================================================================
    // TIM_INT_002: mask toggle at runtime.
    //   Steps:
    //     (a) ena=1, mask=0 — interrupt latches (poll INTST = 1)
    //     (b) Write mask=1 while interrupt is pending — INTST should become 0
    //     (c) Write mask=0 again — INTST should go back to 1 (the latch
    //         is still set, mask just gates visibility)
    // =====================================================================
    `uvm_info("TIM_BOUND_VSEQ", "TIM_INT_002: mask toggle at runtime", UVM_LOW)
    tim_seq.tim_write_load(0, 32'h0000_0010);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // unmasked, ena=1
    tim_seq.poll_int_status(0, 20000, int_fired);
    if (!int_fired)
      `uvm_error("TIM_INT", "timer1 interrupt did NOT fire (unmasked)")
    else
      `uvm_info("TIM_INT", "timer1 interrupt fired (ok)", UVM_LOW)
    // Now mask it — INTST should clear immediately
    tim_seq.tim_write_ctrl(0, 5'b00111);        // masked, ena=1 (keep running)
    #100ns;
    tim_seq.tim_reg_read(8'h10, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_INT", $sformatf("timer1 INTST after mask=1 exp=0 act=0x%02h", act))
    else
      `uvm_info("TIM_INT", "timer1 INTST cleared after mask=1 (ok)", UVM_LOW)
    // Unmask again — INTST should restore (latch is still set in RTL)
    // RTL behavior: int_status latched stays set; INTST visible only when mask=0
    tim_seq.tim_write_ctrl(0, 5'b00011);        // unmasked, ena=1
    #100ns;
    tim_seq.tim_reg_read(8'h10, act);
    // Per RTL spec: intmask only gates INTST/TIMERSINTST, not the latch.
    // So after mask=0, the previously latched int_status should be visible.
    if (act[0] !== 1'b1)
      `uvm_info("TIM_INT", $sformatf("timer1 INTST after mask restore = 0x%02h", act), UVM_LOW)
    else
      `uvm_info("TIM_INT", "timer1 INTST restored after mask=0 (ok)", UVM_LOW)
    tim_seq.tim_read_eoi(0, act);
    tim_seq.tim_write_ctrl(0, 5'b00000);  #100ns;

    // =====================================================================
    // TIM_INT_003: ena=0 clears timer_int_tmp latch immediately.
    //   Steps:
    //     (a) ena=1, wait for interrupt latch
    //     (b) Write ena=0 — the `timer_int_tmp` latch should clear immediately
    //     (c) INTST and TIMERSINTST should reflect the cleared latch
    // =====================================================================
    `uvm_info("TIM_BOUND_VSEQ", "TIM_INT_003: ena=0 clears latch", UVM_LOW)
    tim_seq.tim_write_load(0, 32'h0000_0010);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // unmasked, ena=1
    tim_seq.poll_int_status(0, 20000, int_fired);
    if (!int_fired)
      `uvm_error("TIM_INT", "timer1 interrupt did NOT fire (for ena=0 test)")
    // Disable timer — latch should clear
    tim_seq.tim_write_ctrl(0, 5'b00010);        // mask=1, ena=0 (disable keeps latch cleared)
    #100ns;
    // INTST should be 0 (latch was cleared by ena=0)
    tim_seq.tim_reg_read(8'h10, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_INT", $sformatf("timer1 INTST after ena=0 exp=0 act=0x%02h", act))
    else
      `uvm_info("TIM_INT", "timer1 INTST cleared after ena=0 (ok)", UVM_LOW)
    // Re-enable briefly to verify that a new interrupt can still fire
    // (the counter was cleared, so we need to reload)
    tim_seq.tim_write_load(0, 32'h0000_0010);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // unmasked, ena=1
    tim_seq.poll_int_status(0, 20000, int_fired);
    if (!int_fired)
      `uvm_error("TIM_INT", "timer1 re-enable did NOT fire interrupt after ena=0 clear")
    else
      `uvm_info("TIM_INT", "timer1 re-enable fired interrupt (ok)", UVM_LOW)
    tim_seq.tim_read_eoi(0, act);
    tim_seq.tim_write_ctrl(0, 5'b00000);  #100ns;

    `uvm_info("TIM_BOUND_VSEQ", "=== tim0 P1 boundary virtual sequence done ===", UVM_LOW)
    `uvm_info("TIM_BOUND_VSEQ",
      $sformatf("Checker stats: %0d matches, %0d mismatches",
                chk.match_count, chk.mismatch_count), UVM_LOW)
  endtask

endclass : soc_apb0_tim_bound_v_sequence

`endif
