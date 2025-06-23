`ifndef SOC_TOP_V_SEQUENCER
`define SOC_TOP_V_SEQUENCER
class soc_top_v_sequencer extends uvm_sequencer;

  // UVM Factory Registration Macro
  //
  `uvm_component_utils(soc_top_v_sequencer)

  //------------------------------------------
  // Data Members
  //------------------------------------------
  soc_top_env_config env_cfg;

  //------------------------------------------
  // Component Members
  //------------------------------------------
  //agent seqr
  ahb_sequencer ahb_mst_sqr;
  ahb_sequencer ahb_slv_sqr;

  //sub_env v_sqr

  //------------------------------------------
  // Methods
  //------------------------------------------

  // Standard UVM Methods:
  extern function new(string name, uvm_component parent);

endclass

function soc_top_v_sequencer::new(string name, uvm_component parent);
  super.new(name, parent);
  if (!uvm_config_db#(soc_top_env_config)::get(this, "", "soc_top_env_config", env_cfg))
    `uvm_error("CFGERR", "environment config is not set to virtual sequencer!")
  //set sub_env_cfg to sub_env's vseqr

  //create sub_v_sqr

endfunction

`endif

