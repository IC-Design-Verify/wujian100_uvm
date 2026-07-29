`ifndef SOC_DMA_CHECKER__SV
`define SOC_DMA_CHECKER__SV

// =============================================================================
// DMA Reference Model + Self-Checking Checker
// =============================================================================
// Tracks per-channel register state and predicts AHB read values.
// Called by soc_soc_dma_sequence helper tasks via observe_write / observe_read.
//
// Architecture mirrors the timer checker (soc_apb0_tim_checker.svh):
//   ref_model  — shadow register bank, predicts read values
//   checker    — compares actual read vs prediction, tracks stats
// =============================================================================

class soc_dma_ref_model extends uvm_object;

  `uvm_object_utils(soc_dma_ref_model)

  // DMA base
  localparam bit[31:0] DMA_BASE = 32'h4000_0000;

  // ---- Per-channel shadow registers ----
  bit [31:0] sar       [16];   // 0x00
  bit [31:0] dar       [16];   // 0x04
  bit [31:0] ctrl_a    [16];   // 0x08
  bit [31:0] ctrl_b    [16];   // 0x0C
  bit [31:0] int_mask  [16];   // 0x10
  bit [31:0] int_status[16];   // 0x14
  bit [31:0] en        [16];   // 0x20
  bit [31:0] grp_len_ext[16];  // 0x24

  // ---- Global registers ----
  bit [31:0] dmaccfg;           // 0x33C
  bit [31:0] chpendifr;         // 0x330 (read-only, derived from int_status)
  bit [31:0] chsr;              // 0x338 (read-only, derived from en)

  function new(string name = "soc_dma_ref_model");
    super.new(name);
    reset();
  endfunction

  function void reset();
    foreach (sar[i]) begin
      sar[i]        = 32'h0;
      dar[i]        = 32'h0;
      ctrl_a[i]     = 32'h0;
      ctrl_b[i]     = 32'h0;
      int_mask[i]   = 32'h0;
      int_status[i] = 32'h0;
      en[i]         = 32'h0;
      grp_len_ext[i]= 32'h0;
    end
    dmaccfg   = 32'h0;
    chpendifr = 32'h0;
    chsr      = 32'h0;
  endfunction

  // Reverse-lookup of RTL CHN_BAS_ADR (non-uniform per-channel base).
  // Matches soc_soc_dma_sequence::chn_bas_adr() exactly.
  localparam bit[9:0] CHN_BAS_ADR[16] = '{
    10'h000, 10'h1E0, 10'h210, 10'h240,
    10'h270, 10'h2A0, 10'h2D0, 10'h030,
    10'h060, 10'h090, 10'h0C0, 10'h0F0,
    10'h120, 10'h150, 10'h180, 10'h1B0
  };

  // Decode addr[9:0] into channel index and register offset.
  // Uses range check instead of exact match because addr[9:0] for non-SAR
  // registers includes the register offset (e.g., ch0 DAR = 0x004, not 0x000).
  function void decode_addr(input bit[9:0] addr_low, output int ch, output bit[5:0] reg_off);
    ch = -1;
    reg_off = 6'h0;
    for (int i = 0; i < 16; i++) begin
      if (addr_low >= CHN_BAS_ADR[i] && addr_low < (CHN_BAS_ADR[i] + 10'h030)) begin
        ch = i;
        reg_off = (addr_low - CHN_BAS_ADR[i]);
        return;
      end
    end
  endfunction

  // Record a register write into the shadow model.
  // addr is the full bus address (not offset).
  function void apply_write(bit[31:0] addr, bit[31:0] data);
    bit[6:0] offset;
    int ch;

    // Global registers
    if (addr[15:0] == 16'h033C) begin
      dmaccfg = data;
      return;
    end
    if (addr[15:0] == 16'h0330 || addr[15:0] == 16'h0338) begin
      return;  // read-only derived registers
    end

    // Per-channel: non-uniform CHN_BAS_ADR decode using range check
    decode_addr(addr[9:0], ch, offset[5:0]);
    if (ch < 0) return;

    case (offset[5:0])
      6'h00: sar[ch]        = data;
      6'h04: dar[ch]        = data;
      6'h08: ctrl_a[ch]     = data;
      6'h0C: ctrl_b[ch]     = data;
      6'h10: int_mask[ch]   = data;
      6'h18: int_status[ch] = int_status[ch] & ~data[3:0]; // W1C (bits 0..3)
      6'h1C: ;  // SOFT_REQ — W-only pulse, no state
      6'h20: en[ch]         = data;
      6'h24: grp_len_ext[ch]= data;
      default: ;
    endcase
  endfunction

  // Predict value for a register read.  addr is the full bus address.
  function bit[31:0] predict_read(bit[31:0] addr);
    bit[6:0] offset;
    int ch;
    bit[31:0] result;

    // Global registers
    if (addr[15:0] == 16'h0330) begin
      // CHPENDIFR — derived: bit[n] = |(int_status[n] & ~int_mask[n])
      result = 32'h0;
      for (int i = 0; i < 16; i++)
        result[i] = |(int_status[i] & ~int_mask[i]);
      return result;
    end
    if (addr[15:0] == 16'h0338) begin
      // CHSR — derived: bit[n] = en[n][0]
      result = 32'h0;
      for (int i = 0; i < 16; i++)
        result[i] = en[i][0];
      return result;
    end
    if (addr[15:0] == 16'h033C) return dmaccfg;

    // Per-channel: non-uniform CHN_BAS_ADR decode using range check
    decode_addr(addr[9:0], ch, offset[5:0]);
    if (ch < 0) return 32'h0;

    case (offset[5:0])
      6'h00: return sar[ch];
      6'h04: return dar[ch];
      6'h08: return ctrl_a[ch];
      6'h0C: return ctrl_b[ch];
      6'h10: return int_mask[ch];
      6'h14: return int_status[ch];
      6'h18: return 32'h0;          // INT_CLEAR is W-only
      6'h1C: return 32'h0;          // SOFT_REQ is W-only
      6'h20: return en[ch];
      6'h24: return grp_len_ext[ch];
      default: return 32'h0;
    endcase
  endfunction

  // Returns 1 if addr points to a register whose read value is dynamic
  // (set by HW, not just by APB writes) or write-only (no read side-effect).
  // The checker skips comparison for these addresses.
  function bit is_dynamic_or_wonly(bit[31:0] addr);
    int ch;
    bit[5:0] offset;
    // Global CHSR — derived from busy status, not en[]
    if (addr[15:0] == 16'h0338) return 1'b1;
    decode_addr(addr[9:0], ch, offset);
    if (ch < 0) return 1'b0;
    case (offset)
      6'h14, 6'h18, 6'h1C, 6'h20: return 1'b1;  // INT_STATUS / INT_CLEAR / SOFT_REQ / EN
      default: return 1'b0;
    endcase
  endfunction

  // Hardware event: set int_status bit for a channel (called by vseq after
  // detecting a transfer completion in DUT).
  function void set_int_status(input int ch, input bit[4:0] bits);
    int_status[ch] = int_status[ch] | {27'h0, bits};
  endfunction

  // Auto-clear ENn when block completes (mirrors RTL behavior)
  function void auto_clear_en(input int ch);
    en[ch] = 32'h0;
  endfunction

endclass : soc_dma_ref_model


// =============================================================================
// Self-checking checker
// =============================================================================

class soc_dma_checker extends uvm_object;

  `uvm_object_utils(soc_dma_checker)

  soc_dma_ref_model refm;

  int match_count    = 0;
  int mismatch_count = 0;

  function new(string name = "soc_dma_checker");
    super.new(name);
    refm = soc_dma_ref_model::type_id::create("refm");
  endfunction

  function void reset();
    match_count    = 0;
    mismatch_count = 0;
    refm = soc_dma_ref_model::type_id::create("refm");
  endfunction

  // Called by soc_soc_dma_sequence::wr_reg after write
  function void observe_write(bit[31:0] addr, bit[31:0] data);
    refm.apply_write(addr, data);
  endfunction

  // Called by soc_soc_dma_sequence::rd_reg after read — compare vs prediction
  function void observe_read(bit[31:0] addr, bit[31:0] actual);
    bit[31:0] expected;
    expected = refm.predict_read(addr);

    // Skip dynamic / write-only registers (INT_STATUS, INT_CLEAR, SOFT_REQ,
    // EN, CHSR) — their read value depends on HW state, not just APB writes.
    if (refm.is_dynamic_or_wonly(addr)) begin
      `uvm_info("DMA_CHK_SKIP",
        $sformatf("skip dynamic/W-only @0x%08h: actual=0x%08h", addr, actual), UVM_HIGH)
      return;
    end
    // Skip CHSR read — derived from chNc_gbc_chbsy (busy status), not en[]
    if (addr[15:0] == 16'h0338) begin
      `uvm_info("DMA_CHK_SKIP",
        $sformatf("skip CHSR @0x%08h: actual=0x%08h", addr, actual), UVM_HIGH)
      return;
    end

    if (actual === expected) begin
      match_count++;
      `uvm_info("DMA_CHK",
        $sformatf("MATCH @0x%08h: 0x%08h", addr, actual), UVM_HIGH)
    end else begin
      mismatch_count++;
      `uvm_error("DMA_MISMATCH",
        $sformatf("@0x%08h expected=0x%08h actual=0x%08h", addr, expected, actual))
    end
  endfunction

  function void report_summary();
    `uvm_info("DMA_CHK_REPORT",
      $sformatf("DMA checker: %0d matches, %0d mismatches", match_count, mismatch_count),
      UVM_NONE)
  endfunction

endclass : soc_dma_checker

`endif
