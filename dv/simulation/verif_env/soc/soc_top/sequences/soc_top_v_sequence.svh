`ifndef SOC_TOP_V_SEQUENCE__SV
`define SOC_TOP_V_SEQUENCE__SV
class soc_top_smoke_v_sequence extends soc_top_v_sequence_base;
  `uvm_object_utils(soc_top_smoke_v_sequence)
  //`uvm_declare_p_sequencer(soc_top_vsequencer)

  function new(string name = "soc_top_smoke_v_sequence");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on_with(api_write_seq, p_sequencer.apb_sqr, {addr == 32'h4000_000c; data == 32'h1234_5678;})
    //`uvm_do_on_with(api_read_seq , p_sequencer.apb_sqr, {addr == 32'h4000_0010;})
    //api_read_seq.get_response(apb_trans)
    //`uvm_info("APB_READ", $sformatf("data read from 32'h4000_0010 is %h", apb_trans.data), UVM_MEDIUM)

    //`ifdef UVM_VERSION
    //  `uvm_do({agent_name}_seq, p_sequencer.{agent_name}_sqr)
    //`else
    //  `uvm_do_on({agent_name}_seq, p_sequencer.{agent_name}_sqr)
    //`endif
//    fork
//      `uvm_do_on({agent_name}_seq, p_sequencer.{agent_name}_sqr)
//    join
  endtask
endclass: soc_top_smoke_v_sequence

`endif
