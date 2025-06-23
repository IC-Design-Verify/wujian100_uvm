`ifndef SOC_TOP_V_SEQUENCE__SV
`define SOC_TOP_V_SEQUENCE__SV

`ifdef USE_AHB_VIP_TO_REPLACE
class soc_top_smoke_reg_sequence extends ahb_master_base_sequence;
  `uvm_object_utils(soc_top_smoke_reg_sequence)

  function new(string name = "soc_top_smoke_reg_sequence");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    int int_tim_flag=0;
    int int_tim_eoi=0;
    `uvm_info("reg_seq", "Start send ahb_write trans...", UVM_LOW)
    $display("Hello Friend!");
    mem_write32_(32'h50000008, 32'h2);
    mem_write32_(32'h50000000, 32'h400);
    mem_write32_(32'h50000008, 32'h3);
    mem_read32_ (32'h50000010, int_tim_flag);
    while (int_tim_flag != 'h1) begin
      mem_read32_ (32'h50000010, int_tim_flag);
    end
    mem_read32_(32'h5000000c, int_tim_eoi);
    $display("\ntimer test successfully");
    `uvm_info("reg_seq", "Start send ahb_write trans, Done...", UVM_LOW)
  endtask

endclass

`endif

class soc_top_smoke_v_sequence extends soc_top_v_sequence_base;
  `uvm_object_utils(soc_top_smoke_v_sequence)
  //`uvm_declare_p_sequencer(soc_top_vsequencer)

  function new(string name = "soc_top_smoke_v_sequence");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    `ifdef USE_AHB_VIP_TO_REPLACE
      soc_top_smoke_reg_sequence reg_seq;
      `uvm_do_on(reg_seq, p_sequencer.ahb_mst_sqr)
    `endif
  endtask
endclass: soc_top_smoke_v_sequence

`endif
