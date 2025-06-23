`ifndef SOC_ADDR_MAP_SEQUENCE_SV
`define SOC_ADDR_MAP_SEQUENCE_SV

class soc_addr_map_sequence extends uvm_sequence;//ahb_master_base_sequence;

  /** UVM Object Utility macro */
  `uvm_object_utils(soc_addr_map_sequence)

  /** Class Constructor */
  function new(string name="soc_addr_map_sequence");
    super.new(name);
  endfunction

  virtual task body();
    super.body();
    //mem_write32_('hb10,  32'h1a010000);
    `uvm_info("body", "soc_addr_map_sequence...", UVM_LOW)
    #25us;
  endtask: body

endclass

`endif
