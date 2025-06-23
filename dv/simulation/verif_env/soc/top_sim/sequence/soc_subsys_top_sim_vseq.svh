`ifndef SOC_TOP_SIM_VIRTUAL_SEQUENCE_SV
`define SOC_TOP_SIM_VIRTUAL_SEQUENCE_SV

class soc_top_sim_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_top_sim_smoke_virtual_sequence)

  soc_top_sim_sequence top_sim_seq;

  function new(string name = "soc_top_sim_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(top_sim_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_top_sim_smoke_virtual_sequence



`endif
