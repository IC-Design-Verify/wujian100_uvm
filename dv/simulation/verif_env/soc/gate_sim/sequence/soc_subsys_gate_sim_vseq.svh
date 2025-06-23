`ifndef SOC_GATE_SIM_VIRTUAL_SEQUENCE_SV
`define SOC_GATE_SIM_VIRTUAL_SEQUENCE_SV

class soc_gate_sim_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_gate_sim_smoke_virtual_sequence)

  soc_gate_sim_sequence gate_sim_seq;

  function new(string name = "soc_gate_sim_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(gate_sim_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_gate_sim_smoke_virtual_sequence



`endif
