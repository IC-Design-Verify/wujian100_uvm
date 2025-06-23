`ifndef SOC_TOP_ENV__SV
`define SOC_TOP_ENV__SV
class soc_top_env extends uvm_env;

  // UVM Factory Registration Macro
  //
  `uvm_component_utils(soc_top_env)

  //------------------------------------------
  // Data Members
  //------------------------------------------
  soc_top_env_config  env_cfg;

  //------------------------------------------
  // TLM/Component Members
  //------------------------------------------
  //scb fifo inst

  //refm fifo inst

  //tlm inst

  //env inst

  //agent inst

  //virtual sequencer inst
  soc_top_v_sequencer soc_top_vsqr;
  

  //scb inst

  //refm inst

  //func cov inst
  soc_top_coverage  cov;

  //reg_model inst


  //------------------------------------------
  // Methods
  //------------------------------------------
  // Standard UVM Methods:
  extern function new(string name, uvm_component parent);
  extern function void build_phase(uvm_phase phase);
  extern virtual function void connect_phase(uvm_phase phase);

  // User_defined Methods

endclass: soc_top_env

function soc_top_env::new(string name, uvm_component parent);
  super.new(name, parent);
  `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
endfunction: new

function void soc_top_env::build_phase(uvm_phase phase);
  super.build_phase(phase);
  `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  if (!uvm_config_db #(soc_top_env_config)::get(this, "", "soc_top_env_config", env_cfg)) begin
    `uvm_error("build_phase", "environment config not found");
  end
  //vip config get

  //set s_env config to s_env

  //build reg_model


  //sub_env reg_model connect
  //Example:
  //if (env_cfg.has_sub_env) begin
  //  env_cfg.sub_env_cfg.rgm = rgm.sub_env_blk;
  //  env_cfg.sub_env_cfg.sync_rgm = env_cfg.sync_rgm.sub_env_blk;
  //end

  //scb fifo new

  //refm fifo new

  //v_seq new
  if (env_cfg.has_soc_top_vsqr) begin
    soc_top_vsqr = soc_top_v_sequencer::type_id::create("soc_top_vsqr", this);
  end

  //env exists or not

  //agent exists or not

  //reference_model new

  //scoreboard new

  //refm config

  //function coverage config
  if (env_cfg.has_functional_coverage) begin
    cov = soc_top_coverage::type_id::create("cov",this);
  end
endfunction: build_phase

function void soc_top_env::connect_phase(uvm_phase phase);
  super.connect_phase(phase);
  `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)

  //reg_model connect
  //env_cfg.xxx_s_env_cfg.rgm.map.set_base_addr('h5000_0000);

  //function coverage connect
  //if (env_cfg.has_functional_coverage) begin
  //  xxx_agt.analysis_port.connect(cov.cov_imp);
  //end

  //v_seqr connect

  //tlm connect

  //v_seqr config

  //refm connect

  //scoreboard connect

endfunction: connect_phase

`endif
