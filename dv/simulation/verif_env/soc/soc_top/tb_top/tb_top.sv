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

`define SYNTHESIS
`define MAX_SIM_TIME 1500000000
`timescale 1ns/100ps
`ifndef USE_AHB_VIP_TO_REPLACE
`include "soc_top_include/busmnt.v"
`include "soc_top_include/virtual_counter.v"
`endif
module tb_top();
  import uvm_pkg::*;
  import ahb_pkg::*;


  //vip package, sub_env needs

  `include "soc_top_include/all_testcases.svh"

  `define  TB_MODULE            tb_top
  `define  TOP_MODULE           `TB_MODULE.dut
  `define  WJ2_TB_MODULE        wj2_tb_top
  `define  WJ2_TOP_MODULE       `TB_MODULE.dut
           
  `define CORE_JTG_TCLK_DURATION 500
  
  `define CLKMUX_EHS_CLK_DURATION 25 
  
  `define CLKMUX_ELS_CLK_DURATION 15258.789
  
  `ifdef iverilog
    integer FILE;
  `else
    static integer FILE;
  `endif
  
  reg     [31:0]  cpuclk_counter;           
  reg             i_ext_pad_clkmux_ehs_clk; 
  reg             i_ext_pad_clkmux_els_clk; 
  reg             i_ext_pad_rstgen_i_mcurst; 
  reg             jtag_clk;                 
  
  
  wire            PAD_GPIO_0;               
  wire            PAD_GPIO_1;               
  wire            PAD_GPIO_10;              
  wire            PAD_GPIO_11;              
  wire            PAD_GPIO_12;              
  wire            PAD_GPIO_13;              
  wire            PAD_GPIO_14;              
  wire            PAD_GPIO_15;              
  wire            PAD_GPIO_16;              
  wire            PAD_GPIO_17;              
  wire            PAD_GPIO_18;              
  wire            PAD_GPIO_19;              
  wire            PAD_GPIO_2;               
  wire            PAD_GPIO_20;              
  wire            PAD_GPIO_21;              
  wire            PAD_GPIO_22;              
  wire            PAD_GPIO_23;              
  wire            PAD_GPIO_24;              
  wire            PAD_GPIO_25;              
  wire            PAD_GPIO_26;              
  wire            PAD_GPIO_27;              
  wire            PAD_GPIO_28;              
  wire            PAD_GPIO_29;              
  wire            PAD_GPIO_3;               
  wire            PAD_GPIO_30;              
  wire            PAD_GPIO_31;              
  wire            PAD_GPIO_4;               
  wire            PAD_GPIO_5;               
  wire            PAD_GPIO_6;               
  wire            PAD_GPIO_7;               
  wire            PAD_GPIO_8;               
  wire            PAD_GPIO_9;               
  wire            PAD_JTAG_TCLK;            
  wire            PAD_JTAG_TMS;             
  wire            PAD_MCURST;               
  wire            PAD_PWM_CH0;              
  wire            PAD_PWM_CH1;              
  wire            PAD_PWM_CH10;             
  wire            PAD_PWM_CH11;             
  wire            PAD_PWM_CH2;              
  wire            PAD_PWM_CH3;              
  wire            PAD_PWM_CH4;              
  wire            PAD_PWM_CH5;              
  wire            PAD_PWM_CH6;              
  wire            PAD_PWM_CH7;              
  wire            PAD_PWM_CH8;              
  wire            PAD_PWM_CH9;              
  wire            PAD_PWM_FAULT;            
  wire            PAD_USI0_NSS;             
  wire            PAD_USI0_SCLK;            
  wire            PAD_USI0_SD0;             
  wire            PAD_USI0_SD1;             
  wire            PAD_USI1_NSS;             
  wire            PAD_USI1_SCLK;            
  wire            PAD_USI1_SD0;             
  wire            PAD_USI1_SD1;             
  wire            PAD_USI2_NSS;             
  wire            PAD_USI2_SCLK;            
  wire            PAD_USI2_SD0;             
  wire            PAD_USI2_SD1;             
  wire            PIN_EHS;                  
  wire            PIN_ELS;                  
  wire            PIN_TCLK;                 
  wire            PI_IO_JTAG_MODE;          
  wire            PI_MCURST;                
  wire            PI_MODE_SEL0;             
  wire            PI_MODE_SEL1;             
  wire            PI_SOC_32KCLK;            
  wire            PI_SOC_CLK;               
  wire            PI_SOC_RST_B;             
  wire            PI_TEST_MODE;             
  wire            POUT_EHS;                 
  wire            POUT_ELS;                 
  wire            flash_bist_en;            


  ////////////////////////////////////clock define/////////////////////////////////
  ////////////////////////////////////ehs clock define////////////////////////////
  assign PIN_EHS = i_ext_pad_clkmux_ehs_clk;
  assign PI_SOC_CLK = i_ext_pad_clkmux_ehs_clk;
  assign i_ext_pad_clkmux_ehs_clk = `MERGE_ITF_NAME(soc_top, ehs_clk.CLOCK);
  //initial
  //begin
  //  i_ext_pad_clkmux_ehs_clk = 1'b0;
  //  forever begin
  //    #`CLKMUX_EHS_CLK_DURATION;
  //    i_ext_pad_clkmux_ehs_clk = ~i_ext_pad_clkmux_ehs_clk;
  //  end
  //end
  
  ////////////////////////////////////els clock define////////////////////////////                   
  assign PIN_ELS = i_ext_pad_clkmux_els_clk;
  assign i_ext_pad_clkmux_els_clk = `MERGE_ITF_NAME(soc_top, els_clk.CLOCK);
  //initial
  //begin
  //  i_ext_pad_clkmux_els_clk = 1'b0;
  //  forever begin
  //    #`CLKMUX_ELS_CLK_DURATION;
  //    i_ext_pad_clkmux_els_clk = ~i_ext_pad_clkmux_els_clk;
  //  end
  //end

  ////////////////////////////////////jtag clock define////////////////////////////
  assign PIN_TCLK = jtag_clk;
  assign PAD_JTAG_TCLK = jtag_clk;
  assign jtag_clk = `MERGE_ITF_NAME(soc_top, jtag_clk.CLOCK);
  
  initial begin
    force PAD_JTAG_TMS = 1'b1;
  end
  
  //initial begin
  //  jtag_clk = 1'b0;
  //  forever begin
  //    #`CORE_JTG_TCLK_DURATION;
  //    jtag_clk = ~jtag_clk;
  //  end
  //end

  /////////////////////////////////pad reset define/////////////////////////////
  assign PI_MCURST = i_ext_pad_rstgen_i_mcurst;
  assign PAD_MCURST = i_ext_pad_rstgen_i_mcurst;
  assign PI_SOC_RST_B = i_ext_pad_rstgen_i_mcurst;
  assign i_ext_pad_rstgen_i_mcurst = `MERGE_ITF_NAME(soc_top, pad_rst).RESET;
  
  //initial begin
  //  i_ext_pad_rstgen_i_mcurst = 1'b0;
  //  #200;
  //  i_ext_pad_rstgen_i_mcurst = 1'b0;
  //  #20000;
  //  i_ext_pad_rstgen_i_mcurst = 1'b1;
  //end
  
  /////////////////////////////test mode define////////////////////////////////


  //cpuclk counter
  initial begin 
    cpuclk_counter[31:0] = 32'b0;
  end
  
  always@(posedge i_ext_pad_clkmux_ehs_clk) begin 
    cpuclk_counter[31:0] = cpuclk_counter[31:0] + 1;
  end

`ifndef USE_AHB_VIP_TO_REPLACE
  busmnt x_busmnt();
  virtual_counter  x_virtual_counter ();
`endif  


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
                                    .PAD_GPIO_0    (PAD_GPIO_0   ),
                                    .PAD_GPIO_1    (PAD_GPIO_1   ),
                                    .PAD_GPIO_10   (PAD_GPIO_10  ),
                                    .PAD_GPIO_11   (PAD_GPIO_11  ),
                                    .PAD_GPIO_12   (PAD_GPIO_12  ),
                                    .PAD_GPIO_13   (PAD_GPIO_13  ),
                                    .PAD_GPIO_14   (PAD_GPIO_14  ),
                                    .PAD_GPIO_15   (PAD_GPIO_15  ),
                                    .PAD_GPIO_16   (PAD_GPIO_16  ),
                                    .PAD_GPIO_17   (PAD_GPIO_17  ),
                                    .PAD_GPIO_18   (PAD_GPIO_18  ),
                                    .PAD_GPIO_19   (PAD_GPIO_19  ),
                                    .PAD_GPIO_2    (PAD_GPIO_2   ),
                                    .PAD_GPIO_20   (PAD_GPIO_20  ),
                                    .PAD_GPIO_21   (PAD_GPIO_21  ),
                                    .PAD_GPIO_22   (PAD_GPIO_22  ),
                                    .PAD_GPIO_23   (PAD_GPIO_23  ),
                                    .PAD_GPIO_24   (PAD_GPIO_24  ),
                                    .PAD_GPIO_25   (PAD_GPIO_25  ),
                                    .PAD_GPIO_26   (PAD_GPIO_26  ),
                                    .PAD_GPIO_27   (PAD_GPIO_27  ),
                                    .PAD_GPIO_28   (PAD_GPIO_28  ),
                                    .PAD_GPIO_29   (PAD_GPIO_29  ),
                                    .PAD_GPIO_3    (PAD_GPIO_3   ),
                                    .PAD_GPIO_30   (PAD_GPIO_30  ),
                                    .PAD_GPIO_31   (PAD_GPIO_31  ),
                                    .PAD_GPIO_4    (PAD_GPIO_4   ),
                                    .PAD_GPIO_5    (PAD_GPIO_5   ),
                                    .PAD_GPIO_6    (PAD_GPIO_6   ),
                                    .PAD_GPIO_7    (PAD_GPIO_7   ),
                                    .PAD_GPIO_8    (PAD_GPIO_8   ),
                                    .PAD_GPIO_9    (PAD_GPIO_9   ),
                                    .PAD_JTAG_TCLK (PAD_JTAG_TCLK),
                                    .PAD_JTAG_TMS  (PAD_JTAG_TMS ),
                                    .PAD_MCURST    (PAD_MCURST   ),
                                    .PAD_PWM_CH0   (PAD_PWM_CH0  ),
                                    .PAD_PWM_CH1   (PAD_PWM_CH1  ),
                                    .PAD_PWM_CH10  (PAD_PWM_CH10 ),
                                    .PAD_PWM_CH11  (PAD_PWM_CH11 ),
                                    .PAD_PWM_CH2   (PAD_PWM_CH2  ),
                                    .PAD_PWM_CH3   (PAD_PWM_CH3  ),
                                    .PAD_PWM_CH4   (PAD_PWM_CH4  ),
                                    .PAD_PWM_CH5   (PAD_PWM_CH5  ),
                                    .PAD_PWM_CH6   (PAD_PWM_CH6  ),
                                    .PAD_PWM_CH7   (PAD_PWM_CH7  ),
                                    .PAD_PWM_CH8   (PAD_PWM_CH8  ),
                                    .PAD_PWM_CH9   (PAD_PWM_CH9  ),
                                    .PAD_PWM_FAULT (PAD_PWM_FAULT),
                                    .PAD_USI0_NSS  (PAD_USI0_NSS ),
                                    .PAD_USI0_SCLK (PAD_USI0_SCLK),
                                    .PAD_USI0_SD0  (PAD_USI0_SD0 ),
                                    .PAD_USI0_SD1  (PAD_USI0_SD1 ),
                                    .PAD_USI1_NSS  (PAD_USI1_NSS ),
                                    .PAD_USI1_SCLK (PAD_USI1_SCLK),
                                    .PAD_USI1_SD0  (PAD_USI1_SD0 ),
                                    .PAD_USI1_SD1  (PAD_USI1_SD1 ),
                                    .PAD_USI2_NSS  (PAD_USI2_NSS ),
                                    .PAD_USI2_SCLK (PAD_USI2_SCLK),
                                    .PAD_USI2_SD0  (PAD_USI2_SD0 ),
                                    .PAD_USI2_SD1  (PAD_USI2_SD1 ),
                                    .PIN_EHS       (PIN_EHS      ),
                                    .PIN_ELS       (PIN_ELS      ),
                                    .POUT_EHS      (POUT_EHS     ),
                                    .POUT_ELS      (POUT_ELS     )
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
