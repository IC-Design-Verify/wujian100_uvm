`ifndef AHB_LS_TESTCASE__SV
`define AHB_LS_TESTCASE__SV
class ahb_ls_base_test extends soc_top_test_base;
  `uvm_component_utils(ahb_ls_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "ahb_ls_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : ahb_ls_base_test


class ahb_ls_smoke_test extends ahb_ls_base_test;
  `uvm_component_utils(ahb_ls_smoke_test)

  /** Class Constructor */
  function new(string name = "ahb_ls_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_ahb_ls_smoke_virtual_sequence    ahb_ls_seq;
    super.main_phase(phase);
    ahb_ls_seq= soc_ahb_ls_smoke_virtual_sequence::type_id::create("ahb_ls_seq", this);
    phase.raise_objection(this);
    ahb_ls_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class ahb_ls_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(ahb_ls_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "ahb_ls_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_ahb_ls_smoke_virtual_sequence    ahb_ls_seq;
//    super.main_phase(phase);
//    ahb_ls_seq= soc_ahb_ls_smoke_virtual_sequence::type_id::create("ahb_ls_seq", this);
//    phase.raise_objection(this);
//    ahb_ls_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
