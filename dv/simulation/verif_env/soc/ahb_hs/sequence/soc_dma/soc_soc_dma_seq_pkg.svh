`ifndef SOC_SUBSYS_SOC_DMA_SEQ_PKG__SV
`define SOC_SUBSYS_SOC_DMA_SEQ_PKG__SV
//package soc_subsys_soc_dma_seq_pkg;
  //import uvm_pkg::*;
  //import soc_top_vseq_lib_pkg::*;
  //import svt_uvm_pkg::*;
  //import svt_ahb_uvm_pkg::*;    
  //import svt_ahb_sequence_pkg::*;
  import ahb_pkg::*;

  `include "soc_top_v_sequence_pkg.svh"
  `include "soc_subsys_ahb_hs_seq_pkg.svh"

  //
  `include "soc_dma/soc_soc_dma_sequence.svh"
  `include "soc_dma/soc_soc_dma_vseq.svh"

//endpackage
`endif
