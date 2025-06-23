`ifndef APB0_TESTCASE__SV
`define APB0_TESTCASE__SV
class apb0_base_test extends soc_top_test_base;
  `uvm_component_utils(apb0_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "apb0_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : apb0_base_test


class apb0_smoke_test extends apb0_base_test;
  `uvm_component_utils(apb0_smoke_test)

  /** Class Constructor */
  function new(string name = "apb0_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_apb0_smoke_virtual_sequence    apb0_seq;
    super.main_phase(phase);
    apb0_seq= soc_apb0_smoke_virtual_sequence::type_id::create("apb0_seq", this);
    phase.raise_objection(this);
    apb0_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class apb0_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(apb0_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "apb0_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_apb0_smoke_virtual_sequence    apb0_seq;
//    super.main_phase(phase);
//    apb0_seq= soc_apb0_smoke_virtual_sequence::type_id::create("apb0_seq", this);
//    phase.raise_objection(this);
//    apb0_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
