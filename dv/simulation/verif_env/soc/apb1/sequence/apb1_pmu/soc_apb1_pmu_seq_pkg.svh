`ifndef SOC_SUBSYS_APB1_PMU_SEQ_PKG__SV
`define SOC_SUBSYS_APB1_PMU_SEQ_PKG__SV
//package soc_subsys_apb1_pmu_seq_pkg;
  //import uvm_pkg::*;
  //import soc_top_vseq_lib_pkg::*;
  //import svt_uvm_pkg::*;
  //import svt_ahb_uvm_pkg::*;    
  //import svt_ahb_sequence_pkg::*;
  import ahb_pkg::*;

  `include "soc_top_v_sequence_pkg.svh"
  `include "soc_subsys_apb1_seq_pkg.svh"

  //
  `include "apb1_pmu/soc_apb1_pmu_sequence.svh"
  `include "apb1_pmu/soc_apb1_pmu_vseq.svh"

//endpackage
`endif
