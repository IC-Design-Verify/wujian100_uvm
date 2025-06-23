`ifndef SOC_TOP_TB_TOP__SV
`define SOC_TOP_TB_TOP__SV



`ifdef USE_APB0
  `include "tb_subsys_apb0_pre_module.v"
`endif
`ifdef USE_APB1
  `include "tb_subsys_apb1_pre_module.v"
`endif
`ifdef USE_GATE_SIM
  `include "tb_subsys_gate_sim_pre_module.v"
`endif
`ifdef USE_AHB_HS
  `include "tb_subsys_ahb_hs_pre_module.v"
`endif
`ifdef USE_AHB_LS
  `include "tb_subsys_ahb_ls_pre_module.v"
`endif
`ifdef USE_TOP_SIM
  `include "tb_subsys_top_sim_pre_module.v"
`endif
`include "uvm_macros.svh"
module tb_top();
  import uvm_pkg::*;


  //vip package, sub_env needs

  `include "soc_top_include/all_testcases.svh"
         
  initial begin
  	$timeformat(-9, 3, " ns", 10);
    //set virtual interface to environment
  	run_test();
  end

  //type_width

  //`ifdef RUN_SOC_TOP
  //clock reset itf


  //`ifdef TB_USE_ASSIGN
  //interface
  `define SOC_TOP_ITF_PRE_NAME soc_top
  //`define SOC_TOP_PATH
  `include "soc_top_include/soc_top_itf_inst.sv"
  //`endif

  //dut inst
  wujian100_open_top dut (
      );


  //use for download program to instr memory
  `include "soc_top_include/soc_top_program_download.sv"

  //`ifdef TB_USE_ASSIGN
  //itf_assignment
	`define SOC_TOP_ITF_PRE_NAME soc_top
  `define SOC_TOP_PRE_NAME SOC_TOP
  `include "soc_top_include/soc_top_interface_assignment.sv"
	`undef SOC_TOP_ITF_PRE_NAME
  `undef SOC_TOP_PRE_NAME
  //`endif

  //`endif

  //sub_env test_top param
  //parameter xxx_top_name = "xxx_uvm_test_top";

  //>>>>>>> subsys TB >>>>>>>
  `ifdef USE_APB0
    `include "tb_sim_subsys_apb0.v"
  `endif
  `ifdef USE_APB1
    `include "tb_sim_subsys_apb1.v"
  `endif
  `ifdef USE_GATE_SIM
    `include "tb_sim_subsys_gate_sim.v"
  `endif
  `ifdef USE_AHB_HS
    `include "tb_sim_subsys_ahb_hs.v"
  `endif
  `ifdef USE_AHB_LS
    `include "tb_sim_subsys_ahb_ls.v"
  `endif
  `ifdef USE_TOP_SIM
    `include "tb_sim_subsys_top_sim.v"
  `endif
  //<<<<<<< subsys TB <<<<<<<


  initial begin
  //`ifdef TB_USE_ASSIGN
    //`ifdef RUN_SOC_TOP
    //config interface
		`define SOC_TOP_ITF_PRE_NAME soc_top
    `define SOC_TOP_PRE_NAME SOC_TOP
    `define SOC_TOP_ENV_INST_NAME m_env
    `include "soc_top_include/soc_top_itf_config.sv"
	  `undef SOC_TOP_ITF_PRE_NAME
    `undef SOC_TOP_PRE_NAME
    `undef SOC_TOP_ENV_INST_NAME
    //`endif
  //`endif
  end

  `ifdef DEMO_MAKEFILE
  initial begin
    $fsdbDumpfile("tb.fsdb");
    $fsdbDumpvars("+all");
  end
  `endif

  //`ifdef POST_SIM 
  //initial begin
  //  `ifdef SDF_MAX
  //    $sdf_annotate("../../xxx_max.sdf", xxx_tb_top.u_dut, 
  //                   "TYPICAL", "1:1:1", "FROM_MTM");
  //  `elsif SDF_MIN
  //    $sdf_annotate("../../xxx_min.sdf", xxx_tb_top.u_dut, 
  //                   "TYPICAL", "1:1:1", "FROM_MTM");
  //  `endif
  //end
  //`endif

endmodule
//`endif 

`ifdef USE_APB0
  `include "tb_subsys_apb0_post_module.v"
`endif
`ifdef USE_APB1
  `include "tb_subsys_apb1_post_module.v"
`endif
`ifdef USE_GATE_SIM
  `include "tb_subsys_gate_sim_post_module.v"
`endif
`ifdef USE_AHB_HS
  `include "tb_subsys_ahb_hs_post_module.v"
`endif
`ifdef USE_AHB_LS
  `include "tb_subsys_ahb_ls_post_module.v"
`endif
`ifdef USE_TOP_SIM
  `include "tb_subsys_top_sim_post_module.v"
`endif

//`ifndef TB_USE_ASSIGN
//Include Bind File
//`define SOC_TOP_NAME soc_top
//`define SOC_TOP_INST_PATH soc_top_tb_top.dut
//`define SOC_TOP_PRE_NAME SOC_TOP
//`include "soc_top_include/soc_top_bind.sv"
//`endif

`endif
