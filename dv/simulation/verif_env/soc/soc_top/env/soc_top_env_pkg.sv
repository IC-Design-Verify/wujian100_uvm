`ifndef SOC_TOP_ENV_PACKAGE__SV
`define SOC_TOP_ENV_PACKAGE__SV
package soc_top_env_pkg;
  import uvm_pkg::*;

  //agent package

  

  //sub_env package

  //reg_model package

  //vip items

  `include "soc_top_env_config.svh"
  //include reference_model

  //include scoreboard

  `include "soc_top_v_sequencer.svh"
  `include "soc_top_coverage.svh"
  `include "soc_top_env.svh"

endpackage
`endif

