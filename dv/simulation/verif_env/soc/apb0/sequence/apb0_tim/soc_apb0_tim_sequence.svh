`ifndef SOC_APB0_TIM_SEQUENCE_SV
`define SOC_APB0_TIM_SEQUENCE_SV

// =============================================================================
// Timer (tim0) base sequence
// =============================================================================
// Provides helper tasks for driving the tim0 peripheral (base 0x5000_0000)
// through the AHB VIP DPI. Each task wraps mem_write32_/mem_read32_ and
// forwards observations to the checker when it is bound.
// =============================================================================

class soc_apb0_tim_sequence extends ahb_master_base_sequence;

  `uvm_object_utils(soc_apb0_tim_sequence)

  // Optional checker handle.  When bound, every write/read is forwarded
  // so the reference model stays in sync with the stimulus.
  soc_apb0_tim_checker chk;

  // Base address of tim0
  localparam bit[31:0] TIM0_BASE = 32'h5000_0000;

  function new(string name = "soc_apb0_tim_sequence");
    super.new(name);
  endfunction

  // ---- Helper: write a tim0 register ----
  task tim_reg_write(bit[31:0] offset, bit[31:0] data);
    bit[31:0] addr;
    addr = TIM0_BASE + offset;
    mem_write32_(addr, data);
    if (chk != null) chk.observe_write(addr, data);
  endtask

  // ---- Helper: read a tim0 register ----
  task tim_reg_read(bit[31:0] offset, output bit[31:0] data);
    bit[31:0] addr;
    int       rd;
    addr = TIM0_BASE + offset;
    mem_read32_(addr, rd);
    data = rd;
    if (chk != null) chk.observe_read(addr, data);
  endtask

  // ---- Helper: write timer control register ----
  // ctrl_bits = {hwen, intmask, mode, ena}
  task tim_write_ctrl(int tidx, bit[4:0] ctrl_bits);
    bit[31:0] off;
    off = (tidx == 0) ? 32'h08 : 32'h1C;
    tim_reg_write(off, 32'h0 | ctrl_bits);
  endtask

  // ---- Helper: write timer load count ----
  task tim_write_load(int tidx, bit[31:0] load);
    bit[31:0] off;
    off = (tidx == 0) ? 32'h00 : 32'h14;
    tim_reg_write(off, load);
  endtask

  // ---- Helper: poll interrupt status until set or timeout ----
  // fired=1 if interrupt fired, 0 on timeout
  task poll_int_status(int tidx, int max_polls = 20000, output bit fired);
    bit[31:0] off_intst;
    bit[31:0] intst_val;
    int       polls;
    off_intst = (tidx == 0) ? 32'h10 : 32'h24;
    polls = 0;
    intst_val = 32'h0;
    while (intst_val[0] == 1'b0 && polls < max_polls) begin
      tim_reg_read(off_intst, intst_val);
      polls++;
    end
    fired = (intst_val[0] == 1'b1);
  endtask

  // ---- Helper: read EOI to clear interrupt ----
  task tim_read_eoi(int tidx, output bit[31:0] eoi_val);
    bit[31:0] off;
    off = (tidx == 0) ? 32'h0C : 32'h20;
    tim_reg_read(off, eoi_val);
  endtask

  // ---- Helper: read current value ----
  task tim_read_curval(int tidx, output bit[31:0] cv);
    bit[31:0] off;
    off = (tidx == 0) ? 32'h04 : 32'h18;
    tim_reg_read(off, cv);
  endtask

  // ---- Body: placeholder for compatibility ----
  virtual task body();
    super.body();
    `uvm_info("TIM_SEQ", "soc_apb0_tim_sequence body (base) — extend or use helpers", UVM_LOW)
  endtask

endclass

`endif
