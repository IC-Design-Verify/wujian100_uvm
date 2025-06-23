`ifndef SOC_SUBSYS_ADDR_MAP_SEQ_PKG__SV
`define SOC_SUBSYS_ADDR_MAP_SEQ_PKG__SV
//package soc_subsys_addr_map_seq_pkg;
  //import uvm_pkg::*;
  //import soc_top_vseq_lib_pkg::*;
  //import svt_uvm_pkg::*;
  //import svt_ahb_uvm_pkg::*;    
  //import svt_ahb_sequence_pkg::*;
  import ahb_pkg::*;

  `include "soc_top_v_sequence_pkg.svh"
  `include "soc_subsys_top_sim_seq_pkg.svh"

  //
  `include "addr_map/soc_addr_map_sequence.svh"
  `include "addr_map/soc_addr_map_vseq.svh"

//endpackage
`endif
