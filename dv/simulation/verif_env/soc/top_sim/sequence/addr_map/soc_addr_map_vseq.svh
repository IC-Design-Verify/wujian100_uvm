`ifndef SOC_ADDR_MAP_VIRTUAL_SEQUENCE_SV
`define SOC_ADDR_MAP_VIRTUAL_SEQUENCE_SV

class soc_addr_map_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_addr_map_smoke_virtual_sequence)

  soc_addr_map_sequence addr_map_seq;

  function new(string name = "soc_addr_map_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(addr_map_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_addr_map_smoke_virtual_sequence



`endif
