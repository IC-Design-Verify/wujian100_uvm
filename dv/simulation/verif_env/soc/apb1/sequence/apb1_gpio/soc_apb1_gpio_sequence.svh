`ifndef SOC_APB1_GPIO_SEQUENCE_SV
`define SOC_APB1_GPIO_SEQUENCE_SV

class soc_apb1_gpio_sequence extends ahb_master_base_sequence;

  /** UVM Object Utility macro */
  `uvm_object_utils(soc_apb1_gpio_sequence)

  /** Class Constructor */
  function new(string name="soc_apb1_gpio_sequence");
    super.new(name);
  endfunction

  virtual task body();
    super.body();
    //mem_write32_('hb10,  32'h1a010000);
    `uvm_info("body", "soc_apb1_gpio_sequence...", UVM_LOW)
    #25us;
  endtask: body

endclass

`endif
