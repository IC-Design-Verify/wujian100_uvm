`ifndef SOC_APB0_TIM_VIRTUAL_SEQUENCE_SV
`define SOC_APB0_TIM_VIRTUAL_SEQUENCE_SV

// =============================================================================
// Timer (tim0) virtual sequence
// =============================================================================
// Drives the tim0 peripheral through the AHB VIP via the timer base sequence
// helper tasks, while binding a soc_apb0_tim_checker to keep the reference
// model in sync with stimulus.  Each smoke scenario enables a timer, polls
// the interrupt status, reads EOI to clear, and verifies the read-back
// values against the model.
// =============================================================================

class soc_apb0_tim_smoke_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_apb0_tim_smoke_v_sequence)

  // Timer base sequence (wraps AHB transactions)
  soc_apb0_tim_sequence tim_seq;

  // Self-checking checker (holds reference model + comparison stats)
  soc_apb0_tim_checker chk;

  function new(string name = "soc_apb0_tim_smoke_v_sequence");
    super.new(name);
    `uvm_info("TRACE", $sformatf("%m"), UVM_HIGH)
  endfunction

  // Run the timer base sequence on the AHB master sequencer.  We use a
  // fork-join to allow the inner sequence to run as a child while we
  // drive it through its helper tasks.
  task body();
    bit        int_fired_t0;
    bit        int_fired_t1;
    bit[31:0]  cv_t0;
    bit[31:0]  cv_t1;
    bit[31:0]  eoi_t0;
    bit[31:0]  eoi_t1;
    bit[31:0]  raw_intst;
    bit[31:0]  intst_combined;
    bit[31:0]  eoi_combined;

    `uvm_info("TIM_VSEQ", "=== tim0 smoke virtual sequence start ===", UVM_LOW)

    // ----- 1. Create checker (UVM component, parent=null top) -----
    chk = soc_apb0_tim_checker::type_id::create("tim_chk", null);

    // ----- 2. Create timer base sequence and bind checker -----
    tim_seq = soc_apb0_tim_sequence::type_id::create("tim_seq");
    tim_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    tim_seq.chk = chk;

    // ----- 3. Scenario: timer1 free-running mode, interrupt unmasked -----
    `uvm_info("TIM_VSEQ", "Scenario 1: timer1 free-run (unmasked)", UVM_LOW)
    tim_seq.tim_write_load(0, 32'h0000_0100);     // load count = 256
    #100ns;  // wait for write to complete
    tim_seq.tim_write_ctrl(0, 5'b00001);           // mode=free, intmask=0, ena=1
    tim_seq.poll_int_status(0, .fired(int_fired_t0));
    if (int_fired_t0)
      `uvm_info("TIM_VSEQ", "timer1 interrupt fired (free-run)", UVM_LOW)
    else
      `uvm_error("TIM_VSEQ", "timer1 interrupt did NOT fire within timeout (free-run)")
    tim_seq.tim_read_eoi(0, eoi_t0);
    tim_seq.tim_read_curval(0, cv_t0);

    // ----- 4. Scenario: timer1 user-defined mode with interrupt mask -----
    `uvm_info("TIM_VSEQ", "Scenario 2: timer1 user-mode (masked)", UVM_LOW)
    tim_seq.tim_write_load(0, 32'h0000_0040);     // load count = 64
    #100ns;
    tim_seq.tim_write_ctrl(0, 5'b00101);           // mode=user, intmask=1, ena=1
    // Masked: interrupt status should never latch; poll loop expected to time out
    tim_seq.poll_int_status(0, 5000, int_fired_t0);
    if (int_fired_t0)
      `uvm_error("TIM_VSEQ", "timer1 masked interrupt unexpectedly fired")
    else
      `uvm_info("TIM_VSEQ", "timer1 masked interrupt correctly stayed inactive", UVM_LOW)

    // ----- 5. Disable timer1 and re-enable in user-mode unmasked -----
    `uvm_info("TIM_VSEQ", "Scenario 3: timer1 user-mode (unmasked)", UVM_LOW)
    tim_seq.tim_write_ctrl(0, 5'b00000);           // disable
    #200ns;
    tim_seq.tim_write_load(0, 32'h0000_0020);     // load count = 32
    #100ns;
    tim_seq.tim_write_ctrl(0, 5'b00011);           // mode=user, intmask=0, ena=1
    tim_seq.poll_int_status(0, .fired(int_fired_t0));
    if (int_fired_t0)
      `uvm_info("TIM_VSEQ", "timer1 interrupt fired (user-mode)", UVM_LOW)
    else
      `uvm_error("TIM_VSEQ", "timer1 interrupt did NOT fire within timeout (user-mode)")
    tim_seq.tim_read_eoi(0, eoi_t0);

    // ----- 6. Scenario: timer2 free-running unmasked -----
    `uvm_info("TIM_VSEQ", "Scenario 4: timer2 free-run (unmasked)", UVM_LOW)
    tim_seq.tim_write_load(1, 32'h0000_0080);     // load count = 128
    #100ns;
    tim_seq.tim_write_ctrl(1, 5'b00001);           // mode=free, intmask=0, ena=1
    tim_seq.poll_int_status(1, .fired(int_fired_t1));
    if (int_fired_t1)
      `uvm_info("TIM_VSEQ", "timer2 interrupt fired (free-run)", UVM_LOW)
    else
      `uvm_error("TIM_VSEQ", "timer2 interrupt did NOT fire within timeout (free-run)")
    tim_seq.tim_read_eoi(1, eoi_t1);

    // ----- 7. Combined interrupt status registers -----
    `uvm_info("TIM_VSEQ", "Scenario 5: combined registers readback", UVM_LOW)
    tim_seq.tim_reg_read(32'hA0, intst_combined);  // TIMERSINTST
    tim_seq.tim_reg_read(32'hA4, eoi_combined);   // TIMERSEOI (clears both)
    tim_seq.tim_reg_read(32'hA8, raw_intst);       // TIMERSRAW

    `uvm_info("TIM_VSEQ", "=== tim0 smoke virtual sequence done ===", UVM_LOW)
    `uvm_info("TIM_VSEQ",
      $sformatf("Checker stats: %0d matches, %0d mismatches",
                chk.match_count, chk.mismatch_count), UVM_LOW)
  endtask

endclass : soc_apb0_tim_smoke_v_sequence

`endif
