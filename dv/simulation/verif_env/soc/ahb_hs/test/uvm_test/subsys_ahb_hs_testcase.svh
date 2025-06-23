`ifndef AHB_HS_TESTCASE__SV
`define AHB_HS_TESTCASE__SV
class ahb_hs_base_test extends soc_top_test_base;
  `uvm_component_utils(ahb_hs_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "ahb_hs_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : ahb_hs_base_test


class ahb_hs_smoke_test extends ahb_hs_base_test;
  `uvm_component_utils(ahb_hs_smoke_test)

  /** Class Constructor */
  function new(string name = "ahb_hs_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_ahb_hs_smoke_virtual_sequence    ahb_hs_seq;
    super.main_phase(phase);
    ahb_hs_seq= soc_ahb_hs_smoke_virtual_sequence::type_id::create("ahb_hs_seq", this);
    phase.raise_objection(this);
    ahb_hs_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class ahb_hs_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(ahb_hs_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "ahb_hs_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_ahb_hs_smoke_virtual_sequence    ahb_hs_seq;
//    super.main_phase(phase);
//    ahb_hs_seq= soc_ahb_hs_smoke_virtual_sequence::type_id::create("ahb_hs_seq", this);
//    phase.raise_objection(this);
//    ahb_hs_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
