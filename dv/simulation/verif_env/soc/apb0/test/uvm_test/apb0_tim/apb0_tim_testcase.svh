`ifndef APB0_TIM_TESTCASE__SV
`define APB0_TIM_TESTCASE__SV

class apb0_tim_base_test extends soc_top_test_base;
  `uvm_component_utils(apb0_tim_base_test)

  function new(string name = "apb0_tim_base_test", uvm_component parent=null);
    super.new(name, parent);
  endfunction

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("TIM_TEST", "apb0_tim_base_test build_phase Entered...", UVM_LOW)
    super.build_phase(phase);
    // Enable address-range filter on the AHB slave VIP so it does NOT
    // drive hrdata for addresses owned by the DUT's AHB-to-APB bridge.
    // APB0 peripherals (timers, PWM, USI, WDT) live at 0x5000_0000 - 0x5000_0FFF.
    // The DUT's bridge provides read data on these addresses; the VIP slave
    // must not create a multi-driver conflict by also driving hrdata.
    env_cfg.ahb_slave_cfg.addr_filter_en = 1;
    env_cfg.ahb_slave_cfg.addr_lo        = 32'h5000_0000;
    env_cfg.ahb_slave_cfg.addr_hi        = 32'h5000_0FFF;
    `uvm_info("TIM_TEST", $sformatf("AHB slave addr filter enabled: [0x%08h:0x%08h]",
      env_cfg.ahb_slave_cfg.addr_lo, env_cfg.ahb_slave_cfg.addr_hi), UVM_LOW)
    `uvm_info("TIM_TEST", "apb0_tim_base_test build_phase Exiting...", UVM_LOW)
  endfunction

endclass : apb0_tim_base_test


class apb0_tim_smoke_test extends apb0_tim_base_test;
  `uvm_component_utils(apb0_tim_smoke_test)

  function new(string name = "apb0_tim_smoke_test", uvm_component parent=null);
    super.new(name, parent);
  endfunction

  virtual task main_phase(uvm_phase phase);
    soc_apb0_tim_smoke_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_apb0_tim_smoke_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask

endclass

// ─────────────────────────────────────────────────────────────────────
// P1: register-access test (TIM_REG_001..005)
// ─────────────────────────────────────────────────────────────────────

class apb0_tim_reg_test extends apb0_tim_base_test;
  `uvm_component_utils(apb0_tim_reg_test)

  function new(string name = "apb0_tim_reg_test", uvm_component parent=null);
    super.new(name, parent);
  endfunction

  virtual task main_phase(uvm_phase phase);
    soc_apb0_tim_reg_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_apb0_tim_reg_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask

endclass

// ─────────────────────────────────────────────────────────────────────
// P1: boundary + interrupt test (TIM_BOUND_001..003, TIM_INT_002..003)
// ─────────────────────────────────────────────────────────────────────

class apb0_tim_bound_test extends apb0_tim_base_test;
  `uvm_component_utils(apb0_tim_bound_test)

  function new(string name = "apb0_tim_bound_test", uvm_component parent=null);
    super.new(name, parent);
  endfunction

  virtual task main_phase(uvm_phase phase);
    soc_apb0_tim_bound_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_apb0_tim_bound_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask

endclass

// ─────────────────────────────────────────────────────────────────────
// P1: dual-timer concurrency test (TIM_CONC_001)
// ─────────────────────────────────────────────────────────────────────

class apb0_tim_conc_test extends apb0_tim_base_test;
  `uvm_component_utils(apb0_tim_conc_test)

  function new(string name = "apb0_tim_conc_test", uvm_component parent=null);
    super.new(name, parent);
  endfunction

  virtual task main_phase(uvm_phase phase);
    soc_apb0_tim_concur_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_apb0_tim_concur_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask

endclass

`endif
