`ifndef SOC_TOP_V_SEQ_BASE__SV
`define SOC_TOP_V_SEQ_BASE__SV
class soc_top_v_sequence_base extends uvm_sequence;
  // UVM Factory Registration Macro
  //
  `uvm_object_utils(soc_top_v_sequence_base)
  `uvm_declare_p_sequencer(soc_top_v_sequencer)

  //uvc's sequences

  //------------------------------------------
  // Data Members
  //------------------------------------------
  soc_top_env_config env_cfg;

  // intr_sequence inst

  //------------------------------------------
  // Methods
  //------------------------------------------

  // Standard UVM Methods:
  extern function new(string name = "soc_top_v_sequence_base");
  extern task pre_body();
  extern task post_body();

endclass: soc_top_v_sequence_base

function soc_top_v_sequence_base::new(string name = "soc_top_v_sequence_base");
  super.new(name);
  `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
endfunction: new

task soc_top_v_sequence_base::pre_body();
  if(!uvm_config_db#(soc_top_env_config)::get(get_sequencer(), "", "soc_top_env_config", env_cfg))
    `uvm_fatal("CFGERR", "environment config is not set to virtual sequence!")
  `ifdef UVM_VERSION_1_1
    if(get_parent_sequence() == null && starting_phase != null) begin
      uvm_objection objection = starting_phase.get_objection();
      objection.set_drain_time(this,25ns);
      starting_phase.raise_objection(this);
    end
  `endif
  fork
    begin
      env_cfg.pad_rst.wait_for_reset_leave();
      env_cfg.pad_rst.wait_for_reset();
      env_cfg.pad_rst.wait_for_reset_leave();
    end
  join
  // Add for intr_sequence execution

endtask

task soc_top_v_sequence_base::post_body();
  `ifdef UVM_VERSION_1_1
    if(get_parent_sequence() == null && starting_phase != null) begin 
      starting_phase.drop_objection(this);
    end
  `endif
endtask

class soc_vip_run_with_c_v_sequence extends soc_top_v_sequence_base;
  `uvm_object_utils(soc_vip_run_with_c_v_sequence)
  //`uvm_declare_p_sequencer(soc_vsequencer)

  //ahb_master_run_with_c_sequence vip_run_c_seq;
  function new(string name = "soc_vip_run_with_c_v_sequence");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //vip_run_c_seq = new();
    //`uvm_do_on(vip_run_c_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_vip_run_with_c_v_sequence
`endif

