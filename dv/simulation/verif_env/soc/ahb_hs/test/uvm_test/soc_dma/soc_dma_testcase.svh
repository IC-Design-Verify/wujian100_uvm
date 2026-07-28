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


// DMA-001: Register R/W test
class soc_dma_reg_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_reg_test)

  function new(string name = "soc_dma_reg_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_dma_reg_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_dma_reg_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass


// DMA-002: Single channel SRAM→SRAM transfer
class soc_dma_xfer_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_xfer_test)

  function new(string name = "soc_dma_xfer_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_dma_xfer_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_dma_xfer_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass


// DMA-003: Multi-channel concurrent transfer
class soc_dma_concur_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_concur_test)

  function new(string name = "soc_dma_concur_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_dma_concur_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_dma_concur_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass


// DMA-004/005/007: Interrupt tests
class soc_dma_int_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_int_test)

  function new(string name = "soc_dma_int_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_dma_int_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_dma_int_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass


// DMA-008/009/010/011/013/014: Misc tests
class soc_dma_misc_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_misc_test)

  function new(string name = "soc_dma_misc_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_dma_misc_v_sequence vseq;
    super.main_phase(phase);
    vseq = soc_dma_misc_v_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass


// DMA Smoke: Combined P0 tests
class soc_dma_smoke_test extends soc_dma_base_test;
  `uvm_component_utils(soc_dma_smoke_test)

  function new(string name = "soc_dma_smoke_test", uvm_component parent=null);
    super.new(name,parent);
  endfunction: new

  virtual task main_phase(uvm_phase phase);
    soc_soc_dma_smoke_virtual_sequence vseq;
    super.main_phase(phase);
    vseq = soc_soc_dma_smoke_virtual_sequence::type_id::create("vseq", this);
    phase.raise_objection(this);
    vseq.start(m_env.soc_top_vsqr);
    phase.drop_objection(this);
  endtask
endclass

`endif
