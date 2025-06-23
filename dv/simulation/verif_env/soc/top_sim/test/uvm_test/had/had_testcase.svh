`ifndef HAD_TESTCASE__SV
`define HAD_TESTCASE__SV
class had_base_test extends soc_top_test_base;
  `uvm_component_utils(had_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "had_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : had_base_test


class had_smoke_test extends had_base_test;
  `uvm_component_utils(had_smoke_test)

  /** Class Constructor */
  function new(string name = "had_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_had_smoke_virtual_sequence    had_seq;
    super.main_phase(phase);
    had_seq= soc_had_smoke_virtual_sequence::type_id::create("had_seq", this);
    phase.raise_objection(this);
    had_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class had_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(had_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "had_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_had_smoke_virtual_sequence    had_seq;
//    super.main_phase(phase);
//    had_seq= soc_had_smoke_virtual_sequence::type_id::create("had_seq", this);
//    phase.raise_objection(this);
//    had_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
