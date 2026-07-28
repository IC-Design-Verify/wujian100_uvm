`ifndef SOC_SOC_DMA_SEQUENCE_SV
`define SOC_SOC_DMA_SEQUENCE_SV

// =============================================================================
// DMA base sequence — AHB-mediated register + memory access helpers
// =============================================================================
// Provides low-level tasks used by all DMA virtual sequences:
//   - wr_reg / rd_reg          : DMA register access via AHB slave port (s6)
//   - fill_mem / check_mem      : SRAM source prefill / destination verify
//                                via AHB master (CPU) — same bus the test uses
//   - dma_global_enable         : set DMACCFG[0]
//   - dma_ch_configure          : full per-channel setup (SAR/DAR/CTRL_A/CTRL_B/INT_MASK)
//   - dma_ch_enable / disable   : ENn[0]
//   - dma_ch_soft_trigger       : SOFT_REQn pulse
//   - dma_wait_tfr / dma_wait_htfr / dma_wait_idle
//                                : poll INT_STATUSn / CHSR
//   - dma_clear_int             : INT_CLEARn W1C
//
// All addresses use the AHB bus address space:
//   DMA base   : 0x4000_0000
//   ISRAM base: 0x0000_0000
//   DSRAM0     : 0x2000_0000  DSRAM1: 0x2001_0000  DSRAM2: 0x2002_0000
//
// Register bit layout follows RTL (dmac.v) — see dma_verification_plan.md.
// =============================================================================

class soc_soc_dma_sequence extends ahb_master_base_sequence;

  `uvm_object_utils(soc_soc_dma_sequence)

  // DMA base address (slave port s6 in AHB matrix)
  localparam bit[31:0] DMA_BASE = 32'h4000_0000;

  // Global register offsets
  localparam bit[31:0] OFF_CHPENDIFR = 32'h330;
  localparam bit[31:0] OFF_CHSR      = 32'h338;
  localparam bit[31:0] OFF_DMACCFG   = 32'h33C;

  // Per-channel register offsets (channel N: base = CHN_BAS_ADR[N], see
  // dmac.v chregc0..15 — NON-UNIFORM banked layout).
  // The RTL uses 10-bit s_haddr[9:0] decode against CHN_BAS_ADR parameters,
  // so each channel's registers live at CHN_BAS_ADR[ch] + 0x00..0x24, NOT
  // at ch*0x40 + offset.
  localparam bit[31:0] OFF_SAR       = 32'h00;
  localparam bit[31:0] OFF_DAR       = 32'h04;
  localparam bit[31:0] OFF_CTRL_A    = 32'h08;
  localparam bit[31:0] OFF_CTRL_B    = 32'h0C;
  localparam bit[31:0] OFF_INT_MASK  = 32'h10;
  localparam bit[31:0] OFF_INT_STAT  = 32'h14;
  localparam bit[31:0] OFF_INT_CLEAR = 32'h18;
  localparam bit[31:0] OFF_SOFT_REQ  = 32'h1C;
  localparam bit[31:0] OFF_EN        = 32'h20;
  localparam bit[31:0] OFF_GRP_LEN_EXT = 32'h24;

  // Per-channel base addresses (RTL CHN_BAS_ADR parameters, dmac.v lines
  // 3781/4377/4973/5569/6165/6761/7357/7953/8549/9145/9741/10337/10933/11529/12125/12721).
  // Index = channel number, value = 10-bit base offset within DMA window.
  // NOTE: channel base addresses are NON-UNIFORM — ch0=0x000, ch1=0x1E0,
  // ch2=0x210, ... ch7=0x030, ch8=0x060, ... ch15=0x1B0.  Using ch*0x40
  // is WRONG; this lookup matches the RTL parameter table.
  function bit[9:0] chn_bas_adr(input int ch);
    case (ch)
       0: return 10'h000;  1: return 10'h1E0;  2: return 10'h210;  3: return 10'h240;
       4: return 10'h270;  5: return 10'h2A0;  6: return 10'h2D0;  7: return 10'h030;
       8: return 10'h060;  9: return 10'h090; 10: return 10'h0C0; 11: return 10'h0F0;
      12: return 10'h120; 13: return 10'h150; 14: return 10'h180; 15: return 10'h1B0;
      default: return 10'h000;
    endcase
  endfunction

  // INT_STATUS/INT_CLEAR bit positions (RTL dmac.v chn_int_status_r)
  // RTL: assign chn_int_status_r = {28'h0, statustrgetcmpfr, statushtfr, statustfr, statuserr};
  // NOTE: statuspend is NOT visible in INT_STATUS (only feeds chnc_gbc_pdvld internally).
  localparam bit[4:0] BIT_ERR          = 5'd0;
  localparam bit[4:0] BIT_TFR          = 5'd1;
  localparam bit[4:0] BIT_HTFR         = 5'd2;
  localparam bit[4:0] BIT_TRGETCMPFR   = 5'd3;

  // Optional: checker handle (set by vseq).  If assigned, observe_write /
  // observe_read are called for every register access.
  soc_dma_checker chk;

  function new(string name = "soc_soc_dma_sequence");
    super.new(name);
  endfunction

  virtual task body();
    super.body();
    `uvm_info("body", "soc_soc_dma_sequence base body (no-op)", UVM_LOW)
  endtask

  // -------------------------------------------------------------------------
  // Register access helpers (32-bit AHB CPU port)
  // -------------------------------------------------------------------------
  task wr_reg(input bit[31:0] offset, input bit[31:0] data);
    bit[31:0] addr = DMA_BASE + offset;
    mem_write32_(addr, data);
    if (chk != null) chk.observe_write(addr, data);
  endtask

  task rd_reg(input bit[31:0] offset, output bit[31:0] data);
    bit[31:0] addr = DMA_BASE + offset;
    mem_read32_(addr, data);
    if (chk != null) chk.observe_read(addr, data);
  endtask

  // Per-channel register helpers (use RTL's CHN_BAS_ADR lookup, not ch*0x40)
  task wr_ch_reg(input int ch, input bit[31:0] offset, input bit[31:0] data);
    wr_reg({22'h0, chn_bas_adr(ch)} + offset, data);
  endtask

  task rd_ch_reg(input int ch, input bit[31:0] offset, output bit[31:0] data);
    rd_reg({22'h0, chn_bas_adr(ch)} + offset, data);
  endtask

  // -------------------------------------------------------------------------
  // Memory access helpers (AHB master / CPU port accesses SRAM)
  // -------------------------------------------------------------------------
  task fill_mem(input bit[31:0] base, input int words, input bit[31:0] pattern);
    for (int i = 0; i < words; i++)
      mem_write32_(base + i*4, pattern + i);
  endtask

  // Verify destination memory word-by-word; returns mismatch count via chk
  task check_mem(input bit[31:0] base, input int words, input bit[31:0] pattern,
                 output int mismatch_count);
    bit[31:0] data;
    mismatch_count = 0;
    for (int i = 0; i < words; i++) begin
      mem_read32_(base + i*4, data);
      if (data !== pattern + i) begin
        mismatch_count++;
        `uvm_error("DMA_MEMCHK",
          $sformatf("@0x%08h exp=0x%08h act=0x%08h", base + i*4, pattern + i, data))
      end
    end
  endtask

  // -------------------------------------------------------------------------
  // DMA configuration helpers
  // -------------------------------------------------------------------------

  // Global enable/disable: DMACCFG[0]
  task dma_global_enable(input bit en);
    wr_reg(OFF_DMACCFG, en ? 32'h1 : 32'h0);
  endtask

  // Full per-channel configure (writes SAR, DAR, CTRL_A, CTRL_B, INT_MASK).
  // Caller supplies raw 32-bit CTRL_A / CTRL_B values per RTL bit layout.
  task dma_ch_configure(input int ch,
                        input bit[31:0] sar,
                        input bit[31:0] dar,
                        input bit[31:0] ctrl_a,
                        input bit[31:0] ctrl_b,
                        input bit[31:0] int_mask);
    wr_ch_reg(ch, OFF_SAR,      sar);
    wr_ch_reg(ch, OFF_DAR,      dar);
    wr_ch_reg(ch, OFF_CTRL_A,   ctrl_a);
    wr_ch_reg(ch, OFF_CTRL_B,   ctrl_b);
    wr_ch_reg(ch, OFF_INT_MASK, int_mask);
  endtask

  task dma_ch_enable(input int ch);
    wr_ch_reg(ch, OFF_EN, 32'h1);
  endtask

  task dma_ch_disable(input int ch);
    wr_ch_reg(ch, OFF_EN, 32'h0);
  endtask

  task dma_ch_soft_trigger(input int ch);
    wr_ch_reg(ch, OFF_SOFT_REQ, 32'h1);
  endtask

  task dma_clear_int(input int ch, input bit[4:0] bits);
    wr_ch_reg(ch, OFF_INT_CLEAR, {27'h0, bits});
  endtask

  // -------------------------------------------------------------------------
  // Status polling helpers
  // -------------------------------------------------------------------------

  // Poll a specific bit of INT_STATUSn until set or timeout (AHB cycles)
  task poll_int_bit(input int ch, input bit[4:0] bitn,
                    input int max_cycles, output bit fired);
    bit[31:0] data;
    int poll_cnt = 0;
    fired = 1'b0;
    while (poll_cnt < max_cycles) begin
      rd_ch_reg(ch, OFF_INT_STAT, data);
      if (data[bitn]) begin
        fired = 1'b1;
        return;
      end
      // Each AHB read takes >=1 cycle; small extra delay to let DMA progress
      #10ns;
      poll_cnt++;
    end
  endtask

  task dma_wait_tfr(input int ch, input int max_cycles, output bit fired);
    poll_int_bit(ch, BIT_TFR, max_cycles, fired);
  endtask

  task dma_wait_htfr(input int ch, input int max_cycles, output bit fired);
    poll_int_bit(ch, BIT_HTFR, max_cycles, fired);
  endtask

  // Poll CHSR until channel's busy bit clears (or timeout)
  task dma_wait_ch_idle(input int ch, input int max_cycles, output bit idle);
    bit[31:0] chsr;
    int poll_cnt = 0;
    idle = 1'b0;
    while (poll_cnt < max_cycles) begin
      rd_reg(OFF_CHSR, chsr);
      if (!chsr[ch]) begin
        idle = 1'b1;
        return;
      end
      #10ns;
      poll_cnt++;
    end
  endtask

  // Read channel's ENn — auto-cleared by RTL on block completion
  task dma_read_ch_en(input int ch, output bit[31:0] data);
    rd_ch_reg(ch, OFF_EN, data);
  endtask

  // -------------------------------------------------------------------------
  // CTRL_A field builder helpers (RTL bit layout)
  // -------------------------------------------------------------------------
  // block_tl: 12-bit, transfer count = block_tl + 1
  // group_len: 4-bit
  // sinc/dinc: 2-bit (00=inc, 01=dec, 1x=const)
  // src/dst width: 2-bit (00=byte, 01=half, 10=word, 11=word)
  function bit[31:0] build_ctrl_a(input bit[11:0] block_tl,
                                   input bit[3:0]  group_len,
                                   input bit[1:0]  sinc,
                                   input bit[1:0]  dinc,
                                   input bit[1:0]  src_width,
                                   input bit[1:0]  dst_width);
    return {1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 3'h0,        // [31:24]
            block_tl,                                          // [23:12]
            group_len,                                         // [11:8]
            sinc, dinc, src_width, dst_width};                // [7:0]
  endfunction

  function bit[31:0] build_ctrl_b(input bit int_en);
    return {14'h0, 1'b0, 1'b0, 1'b0, 4'h0, 2'b10, int_en};   // block-level transfer
  endfunction

endclass

`endif
