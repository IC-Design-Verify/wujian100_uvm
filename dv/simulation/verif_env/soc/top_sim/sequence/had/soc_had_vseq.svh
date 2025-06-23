`ifndef SOC_HAD_VIRTUAL_SEQUENCE_SV
`define SOC_HAD_VIRTUAL_SEQUENCE_SV

class soc_had_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_had_smoke_virtual_sequence)

  soc_had_sequence had_seq;

  function new(string name = "soc_had_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(had_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_had_smoke_virtual_sequence



`endif
