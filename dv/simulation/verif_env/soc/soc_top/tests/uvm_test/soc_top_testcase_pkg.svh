`ifndef SOC_TOP_TESTCASE__SV
`define SOC_TOP_TESTCASE__SV

  //vip package

  

  //reg_model package

  //m_env package
  //import soc_top_env_pkg::*;

  //import soc_top_intr_seq_pkg::*;
  //reg_seq package

  //include test_base
  `include "soc_top_test_base.svh"

  //include testcase
  `include "soc_top_test_lib.svh"
  `include "soc_top_timer_dfx_test.svh"
`include "soc_top_wdt_dfx_test.svh"
`include "soc_top_gpio_dfx_test.svh"
`include "soc_top_rtc_dfx_test.svh"
`include "soc_top_pwm_dfx_test.svh"
`include "soc_top_usi_dfx_test.svh"
`include "soc_top_dma_dfx_test.svh"

`endif
