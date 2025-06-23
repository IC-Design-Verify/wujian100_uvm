`ifndef SOC_SUBSYS_APB0_WDT_SEQ_PKG__SV
`define SOC_SUBSYS_APB0_WDT_SEQ_PKG__SV
//package soc_subsys_apb0_wdt_seq_pkg;
  //import uvm_pkg::*;
  //import soc_top_vseq_lib_pkg::*;
  //import svt_uvm_pkg::*;
  //import svt_ahb_uvm_pkg::*;    
  //import svt_ahb_sequence_pkg::*;

  `include "soc_top_v_sequence_pkg.svh"
  `include "soc_subsys_apb0_seq_pkg.svh"

  //
  `include "apb0_wdt/soc_apb0_wdt_sequence.svh"
  `include "apb0_wdt/soc_apb0_wdt_vseq.svh"

//endpackage
`endif
