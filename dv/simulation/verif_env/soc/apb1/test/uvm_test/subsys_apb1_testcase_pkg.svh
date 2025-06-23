`ifndef APB1_TESTCASE_PACKAGE__SV
`define APB1_TESTCASE_PACKAGE__SV
`include "soc_subsys_apb1_seq_pkg.svh"

//package apb1_testcase_pkg;
  //import uvm_pkg::*;

  //import svt_uvm_pkg::*;
  //import svt_ahb_uvm_pkg::*;    
  //import svt_ahb_sequence_pkg::*;


  
  //
  `include "subsys_apb1_testcase.svh"

  //import apb1_rtc_testcase_pkg::*;
  //import apb1_gpio_testcase_pkg::*;
  //import apb1_pmu_testcase_pkg::*;

//endpackage

`include "apb1_rtc/apb1_rtc_testcase_pkg.svh"
`include "apb1_gpio/apb1_gpio_testcase_pkg.svh"
`include "apb1_pmu/apb1_pmu_testcase_pkg.svh"

`endif
