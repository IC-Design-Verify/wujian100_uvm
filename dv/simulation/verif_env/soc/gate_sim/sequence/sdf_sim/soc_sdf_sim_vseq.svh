`ifndef SOC_SDF_SIM_VIRTUAL_SEQUENCE_SV
`define SOC_SDF_SIM_VIRTUAL_SEQUENCE_SV

class soc_sdf_sim_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_sdf_sim_smoke_virtual_sequence)

  soc_sdf_sim_sequence sdf_sim_seq;

  function new(string name = "soc_sdf_sim_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(sdf_sim_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_sdf_sim_smoke_virtual_sequence



`endif
