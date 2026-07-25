`ifndef SOC_APB0_TIM_REG_VSEQ_SV
`define SOC_APB0_TIM_REG_VSEQ_SV

// =============================================================================
// Timer (tim0) P1 register-access virtual sequence
// =============================================================================
// Covers the following scenarios from apb0_tim_testplan.json:
//   TIM_REG_001  default values after reset
//   TIM_REG_002  load-count write + readback (timer1 & timer2)
//   TIM_REG_003  control register all bit combinations readback
//   TIM_REG_004  EOI self-clear on read
//   TIM_REG_005  TIMERSEOI combined clear (both timers)
// =============================================================================
// This vseq reuses soc_apb0_tim_sequence helper tasks and the
// soc_apb0_tim_checker reference model.  All AHB reads are checked by the
// checker via tim_seq.tim_reg_read().
// =============================================================================

class soc_apb0_tim_reg_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_apb0_tim_reg_v_sequence)

  soc_apb0_tim_sequence tim_seq;
  soc_apb0_tim_checker  chk;

  function new(string name = "soc_apb0_tim_reg_v_sequence");
    super.new(name);
  endfunction

  // ---- Helper: read a register and report pass/fail against an expected value ----
  // The checker records mismatches via uvm_error; this only adds a UART log line.
  task check_read(input bit[31:0] off, input bit[31:0] exp, output bit[31:0] act);
    tim_seq.tim_reg_read(off, act);
    if (act === exp)
      `uvm_info("TIM_REG", $sformatf("R@0x%02h = 0x%08h (ok)", off, act), UVM_LOW)
    else
      `uvm_error("TIM_REG",
        $sformatf("R@0x%02h expected=0x%08h actual=0x%08h", off, exp, act))
  endtask

  task body();
    bit[31:0] act;
    bit[31:0] exp;
    int       t;
    bit       fired;
    bit[4:0]  ctrl_walk[16];

    `uvm_info("TIM_REG_VSEQ", "=== tim0 P1 register virtual sequence start ===", UVM_LOW)

    // ----- 1. Create checker and bind to base sequence -----
    chk = soc_apb0_tim_checker::type_id::create("tim_chk_reg", null);
    tim_seq = soc_apb0_tim_sequence::type_id::create("tim_seq_reg");
    tim_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    tim_seq.chk = chk;

    // =====================================================================
    // TIM_REG_001: default values after reset
    //   TIMER1LC/2LC mirror in RTL is `ri_timerNloadcount` which resets to 0.
    //   TIMER1CV/2CV come from `timers_frc` counter which resets to all-ones.
    //   TIMER1CR/2CR reset to 0.
    //   TIMER1INTST/2INTST, TIMERSINTST, TIMERSRAW reset to 0.
    // =====================================================================
    `uvm_info("TIM_REG_VSEQ", "TIM_REG_001: default values after reset", UVM_LOW)
    // CR reset to 0
    exp = 32'h0;  check_read(8'h08, exp, act);   // TIMER1CR
    exp = 32'h0;  check_read(8'h1C, exp, act);   // TIMER2CR
    // LC reset to 0 (W-only but readable via ri_timerNloadcount mirror)
    exp = 32'h0;  check_read(8'h00, exp, act);   // TIMER1LC
    exp = 32'h0;  check_read(8'h14, exp, act);   // TIMER2LC
    // INTST reset to 0
    exp = 32'h0;  check_read(8'h10, exp, act);   // TIMER1INTST
    exp = 32'h0;  check_read(8'h24, exp, act);   // TIMER2INTST
    // CV: internal counter resets to all-ones, but current_value output is
    // gated by timer_en (0 after reset), so the bus reads back 0x0.
    exp = 32'h0;  check_read(8'h04, exp, act);   // TIMER1CV
    exp = 32'h0;  check_read(8'h18, exp, act);   // TIMER2CV
    // Combined status reset to 0
    exp = 32'h0;  check_read(8'hA0, exp, act);   // TIMERSINTST
    exp = 32'h0;  check_read(8'hA8, exp, act);   // TIMERSRAW
    // EOI should read 0 in idle (no pending IRQ)
    exp = 32'h0;  check_read(8'h0C, exp, act);   // TIMER1EOI
    exp = 32'h0;  check_read(8'h20, exp, act);   // TIMER2EOI

    // =====================================================================
    // TIM_REG_002: load-count write + readback.  LC is W-only in the spec,
    // but RTL exposes a readback mirror (`ri_timerNloadcount`).  Consecutive
    // writes overwrite the previously-loaded value.
    // =====================================================================
    `uvm_info("TIM_REG_VSEQ", "TIM_REG_002: load-count write + readback", UVM_LOW)
    // timer1: write 0xDEADBEEF then 0xCAFEBABE — last write should stick
    tim_seq.tim_write_load(0, 32'hDEAD_BEEF);  #50ns;
    tim_seq.tim_reg_read(8'h00, act);
    if (act === 32'hDEAD_BEEF)
      `uvm_info("TIM_REG", $sformatf("timer1 LC after 1st write = 0x%08h (ok)", act), UVM_LOW)
    else
      `uvm_error("TIM_REG", $sformatf("timer1 LC after 1st write: exp=0xDEADBEEF act=0x%08h", act))
    tim_seq.tim_write_load(0, 32'hCAFE_BABE);  #50ns;
    check_read(8'h00, 32'hCAFE_BABE, act);
    // timer2: write 0x12345678 then 0xAAAAAAAA
    tim_seq.tim_write_load(1, 32'h1234_5678);  #50ns;
    tim_seq.tim_reg_read(8'h14, act);
    if (act === 32'h1234_5678)
      `uvm_info("TIM_REG", $sformatf("timer2 LC after 1st write = 0x%08h (ok)", act), UVM_LOW)
    else
      `uvm_error("TIM_REG", $sformatf("timer2 LC after 1st write: exp=0x12345678 act=0x%08h", act))
    tim_seq.tim_write_load(1, 32'hAAAA_AAAA);  #50ns;
    check_read(8'h14, 32'hAAAA_AAAA, act);

    // =====================================================================
    // TIM_REG_003: control register all 16 useful bit combinations
    //   Bits [4:0] = {hwen, intmask, mode, ena}, bit [3] reserved.
    //   We walk the 16 combos of {hwen, intmask, mode, ena} ignoring bit[3].
    //   Each write is followed by a readback to confirm the bits stick.
    // =====================================================================
    `uvm_info("TIM_REG_VSEQ", "TIM_REG_003: control register all bit combinations", UVM_LOW)
    ctrl_walk = '{
      5'b00000, 5'b00001, 5'b00010, 5'b00011,
      5'b00100, 5'b00101, 5'b00110, 5'b00111,
      5'b10000, 5'b10001, 5'b10010, 5'b10011,
      5'b10100, 5'b10101, 5'b10110, 5'b10111
    };
    // Note: enabling the timer (ena=1) and then polling is not done here —
    // we only verify CR readback.  Test TIM_BOUND/TIM_INT covers running.
    for (int i = 0; i < 16; i++) begin
      bit[4:0] c = ctrl_walk[i];
      // timer1
      tim_seq.tim_write_ctrl(0, c);  #50ns;
      tim_seq.tim_reg_read(8'h08, act);
      if (act[4:0] !== c)
        `uvm_error("TIM_REG", $sformatf("timer1 CR walk i=%0d exp=%b act=%b", i, c, act[4:0]))
      else
        `uvm_info("TIM_REG", $sformatf("timer1 CR walk i=%0d = %b (ok)", i, c), UVM_HIGH)
      // timer2
      tim_seq.tim_write_ctrl(1, c);  #50ns;
      tim_seq.tim_reg_read(8'h1C, act);
      if (act[4:0] !== c)
        `uvm_error("TIM_REG", $sformatf("timer2 CR walk i=%0d exp=%b act=%b", i, c, act[4:0]))
      else
        `uvm_info("TIM_REG", $sformatf("timer2 CR walk i=%0d = %b (ok)", i, c), UVM_HIGH)
    end
    // Disable both timers before moving on
    tim_seq.tim_write_ctrl(0, 5'b00000);  #50ns;
    tim_seq.tim_write_ctrl(1, 5'b00000);  #50ns;

    // =====================================================================
    // TIM_REG_004 + TIM_REG_005: EOI self-clear and TIMERSEOI combined clear.
    //   Per RTL (tim.v timers_apbif prdata mux), EOI registers have NO read
    //   path — they fall into the `default: prdata <= 32'b0` branch. EOI is a
    //   side-effect-only register: reading it clears the latched interrupt
    //   (timer_int_tmp), but the read data is always 0.
    //   Verification strategy: read INTST before/after EOI to observe the
    //   side-effect:
    //     (a) INTST=1 → read EOI (returns 0) → INTST=0
    //     (b) same for timer2
    //     (c) TIMERSINTST=3 → read TIMERSEOI (returns 0) → TIMERSINTST=0
    // =====================================================================
    `uvm_info("TIM_REG_VSEQ", "TIM_REG_004: EOI self-clear on read", UVM_LOW)
    // ---- (a) timer1 self-clear ----
    tim_seq.tim_write_load(0, 32'h0000_0001);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);        // user-mode, mask=0, ena=1
    // Wait for underflow (very short — load=1 fires immediately)
    tim_seq.poll_int_status(0, 200, fired);
    // Confirm INTST=1 before EOI
    tim_seq.tim_reg_read(8'h10, act);
    if (act[0] !== 1'b1)
      `uvm_error("TIM_REG", $sformatf("timer1 INTST before EOI exp=0x1 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer1 INTST before EOI = 0x1 (ok)", UVM_LOW)
    // First EOI read — must return 0 AND clear the latch
    tim_seq.tim_reg_read(8'h0C, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_REG", $sformatf("timer1 EOI 1st read exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer1 EOI 1st read = 0x0 (ok)", UVM_LOW)
    // Second EOI read — still 0 (no pending interrupt)
    tim_seq.tim_reg_read(8'h0C, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_REG", $sformatf("timer1 EOI 2nd read exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer1 EOI 2nd read = 0x0 (ok)", UVM_LOW)
    // INTST should also be clear now (side-effect of first EOI read)
    tim_seq.tim_reg_read(8'h10, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_REG", $sformatf("timer1 INTST after EOI exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer1 INTST after EOI = 0x0 (ok)", UVM_LOW)
    // Disable timer1
    tim_seq.tim_write_ctrl(0, 5'b00000);  #100ns;

    // ---- (b) timer2 self-clear ----
    tim_seq.tim_write_load(1, 32'h0000_0001);  #50ns;
    tim_seq.tim_write_ctrl(1, 5'b00011);
    tim_seq.poll_int_status(1, 200, fired);
    tim_seq.tim_reg_read(8'h24, act);
    if (act[0] !== 1'b1)
      `uvm_error("TIM_REG", $sformatf("timer2 INTST before EOI exp=0x1 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer2 INTST before EOI = 0x1 (ok)", UVM_LOW)
    tim_seq.tim_reg_read(8'h20, act);           // first TIMER2EOI
    if (act[0] !== 1'b0)
      `uvm_error("TIM_REG", $sformatf("timer2 EOI 1st read exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer2 EOI 1st read = 0x0 (ok)", UVM_LOW)
    tim_seq.tim_reg_read(8'h20, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_REG", $sformatf("timer2 EOI 2nd read exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer2 EOI 2nd read = 0x0 (ok)", UVM_LOW)
    tim_seq.tim_reg_read(8'h24, act);
    if (act[0] !== 1'b0)
      `uvm_error("TIM_REG", $sformatf("timer2 INTST after EOI exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "timer2 INTST after EOI = 0x0 (ok)", UVM_LOW)
    tim_seq.tim_write_ctrl(1, 5'b00000);  #100ns;

    // ---- (c) TIMERSEOI combined clear ----
    `uvm_info("TIM_REG_VSEQ", "TIM_REG_005: TIMERSEOI combined clear", UVM_LOW)
    // Re-trigger both timers
    tim_seq.tim_write_load(0, 32'h0000_0001);  #50ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);  #50ns;
    tim_seq.tim_write_load(1, 32'h0000_0001);  #50ns;
    tim_seq.tim_write_ctrl(1, 5'b00011);  #50ns;
    tim_seq.poll_int_status(0, 200, fired);
    tim_seq.poll_int_status(1, 200, fired);
    // TIMERSINTST should be 0x3
    tim_seq.tim_reg_read(8'hA0, act);
    if (act[1:0] !== 2'b11)
      `uvm_error("TIM_REG", $sformatf("TIMERSINTST before combined EOI exp=0x3 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", $sformatf("TIMERSINTST before combined EOI = 0x%02h (ok)", act), UVM_LOW)
    // TIMERSEOI clears both — read data is 0 (no read path), but side-effect clears both latches
    tim_seq.tim_reg_read(8'hA4, act);
    if (act[1:0] !== 2'b00)
      `uvm_error("TIM_REG", $sformatf("TIMERSEOI 1st read exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "TIMERSEOI 1st read = 0x0 (ok)", UVM_LOW)
    // Second read should still be 0
    tim_seq.tim_reg_read(8'hA4, act);
    if (act[1:0] !== 2'b00)
      `uvm_error("TIM_REG", $sformatf("TIMERSEOI 2nd read exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "TIMERSEOI 2nd read = 0x0 (ok)", UVM_LOW)
    // TIMERSINTST should now be 0x0 (cleared by first TIMERSEOI read)
    tim_seq.tim_reg_read(8'hA0, act);
    if (act[1:0] !== 2'b00)
      `uvm_error("TIM_REG", $sformatf("TIMERSINTST after combined EOI exp=0x0 act=0x%02h", act))
    else
      `uvm_info("TIM_REG", "TIMERSINTST after combined EOI = 0x0 (ok)", UVM_LOW)
    // Disable timers
    tim_seq.tim_write_ctrl(0, 5'b00000);  #50ns;
    tim_seq.tim_write_ctrl(1, 5'b00000);  #100ns;

    `uvm_info("TIM_REG_VSEQ", "=== tim0 P1 register virtual sequence done ===", UVM_LOW)
    `uvm_info("TIM_REG_VSEQ",
      $sformatf("Checker stats: %0d matches, %0d mismatches",
                chk.match_count, chk.mismatch_count), UVM_LOW)
  endtask

endclass : soc_apb0_tim_reg_v_sequence

`endif
