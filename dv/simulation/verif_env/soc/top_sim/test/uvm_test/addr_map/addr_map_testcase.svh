`ifndef ADDR_MAP_TESTCASE__SV
`define ADDR_MAP_TESTCASE__SV
class addr_map_base_test extends soc_top_test_base;
  `uvm_component_utils(addr_map_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "addr_map_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : addr_map_base_test


class addr_map_smoke_test extends addr_map_base_test;
  `uvm_component_utils(addr_map_smoke_test)

  /** Class Constructor */
  function new(string name = "addr_map_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_addr_map_smoke_virtual_sequence    addr_map_seq;
    super.main_phase(phase);
    addr_map_seq= soc_addr_map_smoke_virtual_sequence::type_id::create("addr_map_seq", this);
    phase.raise_objection(this);
    addr_map_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class addr_map_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(addr_map_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "addr_map_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_addr_map_smoke_virtual_sequence    addr_map_seq;
//    super.main_phase(phase);
//    addr_map_seq= soc_addr_map_smoke_virtual_sequence::type_id::create("addr_map_seq", this);
//    phase.raise_objection(this);
//    addr_map_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
