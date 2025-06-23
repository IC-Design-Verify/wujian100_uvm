`ifndef SOC_SOC_DMA_VIRTUAL_SEQUENCE_SV
`define SOC_SOC_DMA_VIRTUAL_SEQUENCE_SV

class soc_soc_dma_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_soc_dma_smoke_virtual_sequence)

  soc_soc_dma_sequence soc_dma_seq;

  function new(string name = "soc_soc_dma_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(soc_dma_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_soc_dma_smoke_virtual_sequence



`endif
