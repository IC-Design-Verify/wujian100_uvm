`ifndef SOC_APB1_PMU_VIRTUAL_SEQUENCE_SV
`define SOC_APB1_PMU_VIRTUAL_SEQUENCE_SV

class soc_apb1_pmu_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_apb1_pmu_smoke_virtual_sequence)

  soc_apb1_pmu_sequence apb1_pmu_seq;

  function new(string name = "soc_apb1_pmu_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(apb1_pmu_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_apb1_pmu_smoke_virtual_sequence



`endif
