class ahb_cfg extends uvm_object;
  `uvm_object_utils(ahb_cfg)
  
  bit tr_print = TRUE; // default: monitor prints rcvd items

  bit is_slave = FALSE; // default: master mode

  uvm_active_passive_enum is_active = UVM_ACTIVE; // default: active mode, mirrored value of agent's attribute

  // Address-range filter for slave response generation.
  // When addr_filter_en == TRUE and a transaction's address falls within
  // [addr_lo, addr_hi] inclusive, the slave VIP will NOT drive hrdata for
  // READ responses (it still asserts hready/hresp to ack the transfer).
  // This allows another master/slave on the bus (e.g. DUT's AHB-to-APB
  // bridge) to provide the read data without contention.
  bit         addr_filter_en = FALSE;
  bit[31:0]   addr_lo        = 32'h0000_0000;
  bit[31:0]   addr_hi        = 32'hFFFF_FFFF;

  // master busy or slave latency attributes
  rand uint32_t min_rd_busy;
  rand uint32_t max_rd_busy;
  rand uint32_t min_wr_busy;
  rand uint32_t max_wr_busy;

  // Returns TRUE when the given address is inside the filtered range.
  function bit addr_in_filter(bit[31:0] a);
    return (addr_filter_en && (a >= addr_lo) && (a <= addr_hi));
  endfunction // addr_in_filter

  function new(string name = "ahb_mst_cfg");
    super.new(name);
  endfunction // new


  constraint slv_latencies_c {
    min_rd_busy dist { 0 :/ 50, [1:5]  :/ 50 };
    min_wr_busy dist { 0 :/ 50, [1:5]  :/ 50 };
    max_rd_busy dist { 0 :/ 25, [1:5]  :/ 25, [6:10] :/ 50 };
    max_wr_busy dist { 0 :/ 25, [1:5]  :/ 25, [6:10] :/ 50 };
    max_rd_busy >= min_rd_busy;
    max_wr_busy >= min_wr_busy;
  }

  function string convert2string();
    if (addr_filter_en)
      return $sformatf("AHB_CFG: Slave=%b, Active=%s, AddrFilter=[0x%x:0x%x], Busy: RD => [%0d : %0d], WR => [%0d : %0d]", is_slave, is_active.name(), addr_lo, addr_hi, min_rd_busy, max_rd_busy, min_wr_busy, max_wr_busy);
    else
      return $sformatf("AHB_CFG: Slave=%b, Active=%s, Busy: RD => [%0d : %0d], WR => [%0d : %0d]", is_slave, is_active.name(), min_rd_busy, max_rd_busy, min_wr_busy, max_wr_busy);
  endfunction // convert2string
  
endclass // ahb_cfg