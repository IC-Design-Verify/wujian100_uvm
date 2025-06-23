`ifndef SOC_DMA_TESTCASE__SV
`define SOC_DMA_TESTCASE__SV
class soc_dma_base_test extends soc_top_test_base;
  `uvm_component_utils(soc_dma_base_test)
  uvm_cmdline_processor clp = uvm_cmdline_processor::get_inst();

  function new(string name = "soc_dma_base_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual function void build_phase(uvm_phase phase);
    `uvm_info("build_phase", "Entered...", UVM_LOW)
    super.build_phase(phase);
    `uvm_info("build_phase", "Exiting...", UVM_LOW)
  endfunction: build_phase
endclass : soc_dma_base_test


class soc_dma_smoke_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_smoke_test)

  /** Class Constructor */
  function new(string name = "soc_dma_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_soc_dma_smoke_virtual_sequence    soc_dma_seq;
    super.main_phase(phase);
    soc_dma_seq= soc_soc_dma_smoke_virtual_sequence::type_id::create("soc_dma_seq", this);
    phase.raise_objection(this);
    soc_dma_seq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

//class soc_dma_txt_test extends soc_ahb_vip_run_with_txt_test;
//  `uvm_component_utils(soc_dma_txt_test)
//
//  /** Class Constructor */
//  function new(string name = "soc_dma_txt_test", uvm_component parent=null);
//    super.new(name,parent);
//  endfunction: new
//
//  virtual task main_phase(uvm_phase phase);
//    soc_soc_dma_smoke_virtual_sequence    soc_dma_seq;
//    super.main_phase(phase);
//    soc_dma_seq= soc_soc_dma_smoke_virtual_sequence::type_id::create("soc_dma_seq", this);
//    phase.raise_objection(this);
//    soc_dma_seq.start(m_env.soc_top_vsqr);
//    phase.drop_objection(this);
//  endtask
//endclass
`endif
