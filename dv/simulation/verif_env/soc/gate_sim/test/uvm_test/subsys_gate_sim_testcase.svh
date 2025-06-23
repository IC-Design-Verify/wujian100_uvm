`ifndef GATE_SIM_TESTCASE__SV
`define GATE_SIM_TESTCASE__SV
class gate_sim_base_test extends soc_top_test_base;
  `uvm_component_utils(gate_sim_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "gate_sim_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : gate_sim_base_test


class gate_sim_smoke_test extends gate_sim_base_test;
  `uvm_component_utils(gate_sim_smoke_test)

  /** Class Constructor */
  function new(string name = "gate_sim_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_gate_sim_smoke_virtual_sequence    gate_sim_seq;
    super.main_phase(phase);
    gate_sim_seq= soc_gate_sim_smoke_virtual_sequence::type_id::create("gate_sim_seq", this);
    phase.raise_objection(this);
    gate_sim_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class gate_sim_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(gate_sim_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "gate_sim_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_gate_sim_smoke_virtual_sequence    gate_sim_seq;
//    super.main_phase(phase);
//    gate_sim_seq= soc_gate_sim_smoke_virtual_sequence::type_id::create("gate_sim_seq", this);
//    phase.raise_objection(this);
//    gate_sim_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
