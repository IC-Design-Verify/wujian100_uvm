`ifndef SOC_TOP_ENV_CONFIG__SV
`define SOC_TOP_ENV_CONFIG__SV
class soc_top_env_config extends uvm_object;

  // UVM Factory Registration Macro
  //
  `uvm_object_utils(soc_top_env_config)

  //------------------------------------------
  // Virtual Interface
  //------------------------------------------
  virtual clock_if#(300,0) ehs_clk;
  virtual clock_if#(200,0) els_clk;
  virtual clock_if#(100,0) jtag_clk;
  virtual reset_if#(20000) pad_rst;

  //------------------------------------------
  // Data Members
  //------------------------------------------
  //use for interrupt process
  process main_process;

  //func_cov active
  bit has_functional_coverage = 0;

  //refm active

  //scoreboard active

  //virtual sequencer active
  bit has_soc_top_vsqr = 1;

  //have env or not


  //have agent or not
  bit has_ahb_agent = 1;

  //add env config

  //add agent config
  ahb_cfg       ahb_master_cfg;
  ahb_cfg       ahb_slave_cfg;

  //register model


  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
  extern function new(string name="soc_top_env_config");

  // User_defined Methods
  extern virtual function config_env(string name="SOC_TOP");
    //vip config function


endclass: soc_top_env_config

function soc_top_env_config::new(string name="soc_top_env_config");
  super.new(name);
  //create env config

  //create agent config
  ahb_master_cfg = ahb_cfg::type_id::create("ahb_master_cfg");
  ahb_slave_cfg  = ahb_cfg::type_id::create("ahb_slave_cfg");
  `check_rand(ahb_master_cfg.randomize())
  `check_rand(ahb_slave_cfg .randomize())
        
endfunction: new

function soc_top_env_config::config_env(string name="SOC_TOP");
  `uvm_info("CONFIG_ENV", "Starting config Environment", UVM_MEDIUM)

  //Get Interface To Agent Config
  if (!uvm_resource_db #(virtual reset_if#(20000))::read_by_name("interface_pool", {name, "_pad_rst"}, pad_rst)) begin
    `uvm_fatal("VIF_NOT_FOUND", {"Fail to get ", name, "_pad_rst from resource_db: interface_pool"})
  end


    

  if (name=="SOC_TOP") begin
  //Agent Configuration
  //End Agent Configuration
  end

  //call vip config function

  `ifndef PREFIX
    `define PREFIX SOC_TOP
  `endif
  //Sub Environment Configuration

  `undef PREFIX
endfunction

// vip function here -->
`endif

