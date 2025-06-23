`ifndef SOC_APB0_PWM_VIRTUAL_SEQUENCE_SV
`define SOC_APB0_PWM_VIRTUAL_SEQUENCE_SV

class soc_apb0_pwm_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_apb0_pwm_smoke_virtual_sequence)

  soc_apb0_pwm_sequence apb0_pwm_seq;

  function new(string name = "soc_apb0_pwm_vseq");
    super.new(name);
    `uvm_info("TRACE",$sformatf("%m"), UVM_HIGH)
  endfunction: new

  task body();
    //`uvm_do_on(apb0_pwm_seq, p_sequencer.ahb_sqr.master_sequencer[0])
  endtask
endclass: soc_apb0_pwm_smoke_virtual_sequence



`endif
