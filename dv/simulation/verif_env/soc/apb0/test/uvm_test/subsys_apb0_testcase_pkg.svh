`ifndef APB0_TESTCASE_PACKAGE__SV
`define APB0_TESTCASE_PACKAGE__SV
`include "soc_subsys_apb0_seq_pkg.svh"

//package apb0_testcase_pkg;
  //import uvm_pkg::*;

  //import svt_uvm_pkg::*;
  //import svt_ahb_uvm_pkg::*;    
  //import svt_ahb_sequence_pkg::*;


  
  //
  `include "subsys_apb0_testcase.svh"

  //import apb0_wdt_testcase_pkg::*;
  //import apb0_pwm_testcase_pkg::*;
  //import apb0_usi_testcase_pkg::*;

//endpackage

`include "apb0_wdt/apb0_wdt_testcase_pkg.svh"
`include "apb0_pwm/apb0_pwm_testcase_pkg.svh"
`include "apb0_usi/apb0_usi_testcase_pkg.svh"

`endif
