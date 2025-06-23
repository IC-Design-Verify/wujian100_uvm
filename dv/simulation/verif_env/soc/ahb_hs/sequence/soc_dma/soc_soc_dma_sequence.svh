`ifndef SOC_SOC_DMA_SEQUENCE_SV
`define SOC_SOC_DMA_SEQUENCE_SV

class soc_soc_dma_sequence extends ahb_master_base_sequence;

  /** UVM Object Utility macro */
  `uvm_object_utils(soc_soc_dma_sequence)

  /** Class Constructor */
  function new(string name="soc_soc_dma_sequence");
    super.new(name);
  endfunction

  virtual task body();
    super.body();
    //mem_write32_('hb10,  32'h1a010000);
    `uvm_info("body", "soc_soc_dma_sequence...", UVM_LOW)
    #25us;
  endtask: body

endclass

`endif
