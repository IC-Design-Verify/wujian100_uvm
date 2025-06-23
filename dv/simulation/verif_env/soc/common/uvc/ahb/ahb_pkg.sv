`include "ahb_macros.svh"

package ahb_pkg;
  timeunit 1ns;
  timeprecision 1ps;
  
  import uvm_pkg::*;
  import vutils_pkg::*;

  typedef class ahb_master_run_with_c_sequence;
  ahb_master_run_with_c_sequence core;

  import "DPI-C" context task test_start();

  export "DPI-C" mem_write32_ = task mem_write32_;
  export "DPI-C" mem_read32_  = task mem_read32_ ;
  export "DPI-C" cpu_delay    = task delay_time  ;
  export "DPI-C" sim_fail     = function sim_fail    ;
  export "DPI-C" sim_end      = function sim_end     ;

  task mem_write32_(longint addr, longint data);
    core.mem_write32_(addr, data);
  endtask
  task mem_read32_(longint addr, output longint data);
    bit[31:0] rdata;
    core.mem_read32_(addr, rdata);
    data = rdata;
  endtask
  task delay_time(int time_num);
    core.delay_time(time_num);
  endtask
  function void sim_fail();
    `uvm_error("AHB_RUN_WITH_C_FAIL", "C test run on AHB is failed")
  endfunction
  function void sim_end();
    `uvm_info("AHB_RUN_WITH_C_PASS", "UVM_TEST_PASS", UVM_LOW)
  endfunction



  `include "ahb_types.svh"
  `include "ahb_cfg.svh"
  `include "ahb_seq_item.svh"
  `include "ahb_driver.svh"
  `include "ahb_monitor.svh"
  `include "ahb_sequencer.svh"
  `include "ahb_agent.svh"
  `include "ahb_base_seq.svh"
  `include "ahb_mst_rand_seq.svh"
  `include "ahb_slv_resp_seq.svh"

  `include "ahb_env.svh"

endpackage // ahb_pkg
