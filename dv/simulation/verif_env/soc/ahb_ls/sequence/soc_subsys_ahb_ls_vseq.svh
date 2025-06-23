`ifndef SOC_AHB_LS_VIRTUAL_SEQUENCE_SV
`define SOC_AHB_LS_VIRTUAL_SEQUENCE_SV

class soc_ahb_ls_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_ahb_ls_smoke_virtual_sequence)

  soc_ahb_ls_sequence ahb_ls_seq;

  function new(string name = "soc_ahb_ls_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(ahb_ls_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_ahb_ls_smoke_virtual_sequence



`endif
