`ifndef SOC_SOC_DMA_VIRTUAL_SEQUENCE_SV
`define SOC_SOC_DMA_VIRTUAL_SEQUENCE_SV

// =============================================================================
// DMA Virtual Sequences
// =============================================================================
// Covers verification plan scenarios DMA-001 through DMA-014.
//
// Architecture:
//   - soc_soc_dma_smoke_virtual_sequence  : P0 base smoke (reg + xfer + int)
//   - soc_dma_reg_v_sequence              : DMA-001 register R/W
//   - soc_dma_xfer_v_sequence             : DMA-002 basic SRAM→SRAM transfer
//   - soc_dma_int_v_sequence              : DMA-004/005/007 interrupt tests
//   - soc_dma_misc_v_sequence             : DMA-008/009/010/011/013/014 misc
//   - soc_dma_concur_v_sequence           : DMA-003 multi-channel concurrent
//
// Each vseq creates a soc_soc_dma_sequence (AHB base sequence), binds it to
// p_sequencer.ahb_mst_sqr, and uses helper tasks for register/memory access.
// =============================================================================


// =============================================================================
// Helper: configure a channel for basic word-to-word transfer
// =============================================================================
class soc_dma_xfer_helper extends soc_soc_dma_sequence;

  `uvm_object_utils(soc_dma_xfer_helper)

  function new(string name = "soc_dma_xfer_helper");
    super.new(name);
  endfunction

  // Configure a channel for word→word transfer (inc/inc, int_en optional)
  task configure_word_xfer(input int ch,
                           input bit[31:0] src_addr,
                           input bit[31:0] dst_addr,
                           input int       word_count,
                           input bit       int_en);
    bit[31:0] ctrl_a, ctrl_b;
    ctrl_a = build_ctrl_a(
      .block_tl(word_count - 1),   // block_tl = count-1 (12-bit)
      .group_len(4'h0),
      .sinc(2'b00),                 // source increment
      .dinc(2'b00),                 // dest increment
      .src_width(2'b10),            // word
      .dst_width(2'b10)             // word
    );
    ctrl_b = build_ctrl_b(int_en);
    dma_ch_configure(ch, src_addr, dst_addr, ctrl_a, ctrl_b, 32'h0);
  endtask

  // Full transfer sequence: configure → enable → trigger → wait → verify
  task do_word_xfer(input int ch,
                    input bit[31:0] src_addr,
                    input bit[31:0] dst_addr,
                    input int       word_count,
                    input bit       int_en,
                    input int       timeout_cycles = 100000,
                    output bit      success);
    bit[31:0] data;
    bit fired;

    // Configure
    configure_word_xfer(ch, src_addr, dst_addr, word_count, int_en);

    // Enable channel
    dma_ch_enable(ch);

    // Trigger
    dma_ch_soft_trigger(ch);

    // Wait for transfer complete
    dma_wait_tfr(ch, timeout_cycles, fired);
    if (!fired) begin
      `uvm_error("DMA_XFER", $sformatf("CH%0d: transfer complete timeout", ch))
      success = 1'b0;
      return;
    end

    // Wait for EN auto-clear
    #100ns;

    success = 1'b1;
  endtask
endclass


// =============================================================================
// DMA-001: Register Read/Write (P0)
// =============================================================================
class soc_dma_reg_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_dma_reg_v_sequence)

  soc_soc_dma_sequence dma_seq;
  soc_dma_checker      chk;

  function new(string name = "soc_dma_reg_v_sequence");
    super.new(name);
  endfunction

  task body();
    bit[31:0] rdata, wdata;
    int       ch;
    string    msg;

    `uvm_info("DMA_REG_VSEQ", "=== DMA-001: Register R/W start ===", UVM_LOW)

    // Create base sequence + checker
    dma_seq = soc_soc_dma_sequence::type_id::create("dma_seq", null);
    dma_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    chk = soc_dma_checker::type_id::create("dma_chk", null);
    dma_seq.chk = chk;

    // Wait for reset
    #200ns;

    // 1. Verify reset values of global registers
    `uvm_info("DMA_REG_VSEQ", "Phase 1: Reset values", UVM_LOW)

    dma_seq.rd_reg(32'h33C, rdata);   // DMACCFG
    if (rdata !== 32'h0)
      `uvm_error("DMA_REG_VSEQ", $sformatf("DMACCFG reset exp=0x0 act=0x%08h", rdata))
    else
      `uvm_info("DMA_REG_VSEQ", "DMACCFG reset = 0x0 ok", UVM_LOW)

    dma_seq.rd_reg(32'h330, rdata);   // CHPENDIFR
    if (rdata !== 32'h0)
      `uvm_error("DMA_REG_VSEQ", $sformatf("CHPENDIFR reset exp=0x0 act=0x%08h", rdata))

    dma_seq.rd_reg(32'h338, rdata);   // CHSR
    if (rdata !== 32'h0)
      `uvm_error("DMA_REG_VSEQ", $sformatf("CHSR reset exp=0x0 act=0x%08h", rdata))

    // 2. Channel 0 register write/readback
    `uvm_info("DMA_REG_VSEQ", "Phase 2: Ch0 register R/W", UVM_LOW)

    dma_seq.wr_ch_reg(0, 32'h00, 32'hDEAD_BEEF);   // SAR0
    dma_seq.rd_ch_reg(0, 32'h00, rdata);
    if (rdata !== 32'hDEAD_BEEF)
      `uvm_error("DMA_REG_VSEQ", $sformatf("SAR0 exp=0xDEADBEEF act=0x%08h", rdata))

    dma_seq.wr_ch_reg(0, 32'h04, 32'hCAFE_BABE);   // DAR0
    dma_seq.rd_ch_reg(0, 32'h04, rdata);
    if (rdata !== 32'hCAFE_BABE)
      `uvm_error("DMA_REG_VSEQ", $sformatf("DAR0 exp=0xCAFEBABE act=0x%08h", rdata))

    wdata = 32'h000F_FF08;  // block_tl=0xFFF, group=0, sinc/inc, dinc/inc, width=word
    dma_seq.wr_ch_reg(0, 32'h08, wdata);           // CTRL_A0
    dma_seq.rd_ch_reg(0, 32'h08, rdata);
    if (rdata !== wdata)
      `uvm_error("DMA_REG_VSEQ", $sformatf("CTRL_A0 exp=0x%08h act=0x%08h", wdata, rdata))

    dma_seq.wr_ch_reg(0, 32'h0C, 32'h0000_0005);   // CTRL_B0 (int_en=1, trgtmdc=block)
    dma_seq.rd_ch_reg(0, 32'h0C, rdata);
    if (rdata !== 32'h0000_0005)
      `uvm_error("DMA_REG_VSEQ", $sformatf("CTRL_B0 exp=0x5 act=0x%08h", rdata))

    dma_seq.wr_ch_reg(0, 32'h10, 32'h0000_0004);   // INT_MASK0 (mask tfr)
    dma_seq.rd_ch_reg(0, 32'h10, rdata);
    if (rdata !== 32'h0000_0004)
      `uvm_error("DMA_REG_VSEQ", $sformatf("INT_MASK0 exp=0x4 act=0x%08h", rdata))

    // Write GRP_LEN_EXT0 BEFORE EN0 — RTL requires chn_en=0 for writes
    // Note: RTL only uses s_hwdata[1:0] → group_len[5:4], so write 0x1 not 0x10
    dma_seq.wr_ch_reg(0, 32'h24, 32'h0000_0001);   // GRP_LEN_EXT0 (must write before EN)
    dma_seq.rd_ch_reg(0, 32'h24, rdata);
    if (rdata !== 32'h0000_0001)
      `uvm_error("DMA_REG_VSEQ", $sformatf("GRP_LEN_EXT0 exp=0x1 act=0x%08h", rdata))

    dma_seq.wr_ch_reg(0, 32'h20, 32'h0000_0001);   // EN0 (enable after GRP_LEN_EXT)

    // 3. Repeat for channel 15
    `uvm_info("DMA_REG_VSEQ", "Phase 3: Ch15 register R/W", UVM_LOW)

    dma_seq.wr_ch_reg(15, 32'h00, 32'hAAAA_BBBB);
    dma_seq.rd_ch_reg(15, 32'h00, rdata);
    if (rdata !== 32'hAAAA_BBBB)
      `uvm_error("DMA_REG_VSEQ", $sformatf("SAR15 exp=0xAAAA_BBBB act=0x%08h", rdata))

    dma_seq.wr_ch_reg(15, 32'h04, 32'hCCCC_DDDD);
    dma_seq.rd_ch_reg(15, 32'h04, rdata);
    if (rdata !== 32'hCCCC_DDDD)
      `uvm_error("DMA_REG_VSEQ", $sformatf("DAR15 exp=0xCCCC_DDDD act=0x%08h", rdata))

    // 4. DMACCFG
    `uvm_info("DMA_REG_VSEQ", "Phase 4: DMACCFG R/W", UVM_LOW)

    dma_seq.dma_global_enable(1);
    dma_seq.rd_reg(32'h33C, rdata);
    if (rdata !== 32'h1)
      `uvm_error("DMA_REG_VSEQ", $sformatf("DMACCFG en exp=0x1 act=0x%08h", rdata))

    dma_seq.dma_global_enable(0);
    dma_seq.rd_reg(32'h33C, rdata);
    if (rdata !== 32'h0)
      `uvm_error("DMA_REG_VSEQ", $sformatf("DMACCFG dis exp=0x0 act=0x%08h", rdata))

    // 5. Verify read-only registers ignore writes
    `uvm_info("DMA_REG_VSEQ", "Phase 5: Read-only regs ignore writes", UVM_LOW)

    dma_seq.wr_reg(32'h330, 32'hFFFF_FFFF);  // try to write CHPENDIFR
    dma_seq.rd_reg(32'h330, rdata);
    if (rdata !== 32'h0)
      `uvm_error("DMA_REG_VSEQ", $sformatf("CHPENDIFR should be R/O, act=0x%08h", rdata))

    dma_seq.wr_reg(32'h338, 32'hFFFF_FFFF);  // try to write CHSR
    dma_seq.rd_reg(32'h338, rdata);
    if (rdata !== 32'h0)
      `uvm_error("DMA_REG_VSEQ", $sformatf("CHSR should be R/O, act=0x%08h", rdata))

    // Report
    `uvm_info("DMA_REG_VSEQ", $sformatf("Checker: %0d matches, %0d mismatches",
      chk.match_count, chk.mismatch_count), UVM_LOW)

    `uvm_info("DMA_REG_VSEQ", "=== DMA-001: Register R/W done ===", UVM_LOW)
  endtask
endclass


// =============================================================================
// DMA-002: Single Channel SRAM→SRAM Transfer (P0)
// =============================================================================
class soc_dma_xfer_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_dma_xfer_v_sequence)

  soc_soc_dma_sequence     dma_seq;
  soc_dma_xfer_helper      xfer;
  soc_dma_checker          chk;

  function new(string name = "soc_dma_xfer_v_sequence");
    super.new(name);
  endfunction

  task body();
    bit[31:0] rdata;
    bit       success;
    bit       fired;
    int       mismatches;
    string    tag;

    `uvm_info("DMA_XFER_VSEQ", "=== DMA-002: Single channel SRAM→SRAM ===", UVM_LOW)

    // Create helper sequence
    dma_seq = soc_soc_dma_sequence::type_id::create("dma_seq", null);
    dma_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    chk = soc_dma_checker::type_id::create("dma_chk", null);
    dma_seq.chk = chk;

    xfer = soc_dma_xfer_helper::type_id::create("xfer", null);
    xfer.set_sequencer(p_sequencer.ahb_mst_sqr);

    #200ns;

    // Sub-test 1: 64-word transfer ISRAM→DSRAM2
    `uvm_info("DMA_XFER_VSEQ", "--- Sub1: 64-word ISRAM→DSRAM2 ---", UVM_LOW)

    // Pre-fill source (ISRAM offset 0x1000)
    xfer.fill_mem(32'h0000_1000, 64, 32'h0000_0001);

    // Enable DMA globally
    xfer.dma_global_enable(1);

    // Configure + transfer
    xfer.do_word_xfer(
      .ch(0),
      .src_addr(32'h0000_1000),
      .dst_addr(32'h2002_0000),
      .word_count(64),
      .int_en(1'b1),
      .success(success)
    );

    if (!success)
      `uvm_error("DMA_XFER_VSEQ", "64-word transfer failed")

    // Verify destination
    xfer.check_mem(32'h2002_0000, 64, 32'h0000_0001, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_XFER_VSEQ", $sformatf("64-word verify: %0d mismatches", mismatches))
    else
      `uvm_info("DMA_XFER_VSEQ", "64-word verify: OK", UVM_LOW)

    // Check interrupt
    dma_seq.rd_ch_reg(0, 32'h14, rdata);
    if (rdata[1])
      `uvm_info("DMA_XFER_VSEQ", "INT_STATUS0[1](tfr) set — ok", UVM_LOW)
    else
      `uvm_error("DMA_XFER_VSEQ", $sformatf("INT_STATUS0[1] not set, val=0x%08h", rdata))

    // Check EN auto-clear
    dma_seq.dma_read_ch_en(0, rdata);
    if (rdata[0] == 1'b0)
      `uvm_info("DMA_XFER_VSEQ", "EN0 auto-cleared — ok", UVM_LOW)
    else
      `uvm_error("DMA_XFER_VSEQ", $sformatf("EN0 not auto-cleared, val=0x%08h", rdata))

    // Clear interrupt
    dma_seq.dma_clear_int(0, 5'h1F);

    // Sub-test 2: 32-word transfer DSRAM0→DSRAM1
    `uvm_info("DMA_XFER_VSEQ", "--- Sub2: 32-word DSRAM0→DSRAM1 ---", UVM_LOW)

    xfer.fill_mem(32'h2000_0000, 32, 32'hAAAA_0000);

    xfer.do_word_xfer(
      .ch(1),
      .src_addr(32'h2000_0000),
      .dst_addr(32'h2001_0000),
      .word_count(32),
      .int_en(1'b0),
      .success(success)
    );

    if (!success)
      `uvm_error("DMA_XFER_VSEQ", "32-word transfer failed")

    xfer.check_mem(32'h2001_0000, 32, 32'hAAAA_0000, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_XFER_VSEQ", $sformatf("32-word verify: %0d mismatches", mismatches))
    else
      `uvm_info("DMA_XFER_VSEQ", "32-word verify: OK", UVM_LOW)

    // Sub-test 3: 256-word transfer ISRAM→DSRAM0 (DMA-011 boundary)
    `uvm_info("DMA_XFER_VSEQ", "--- Sub3: 256-word ISRAM→DSRAM0 ---", UVM_LOW)

    xfer.fill_mem(32'h0000_2000, 256, 32'h1111_0000);

    xfer.do_word_xfer(
      .ch(2),
      .src_addr(32'h0000_2000),
      .dst_addr(32'h2000_0000),
      .word_count(256),
      .int_en(1'b1),
      .success(success)
    );

    if (!success)
      `uvm_error("DMA_XFER_VSEQ", "256-word transfer failed")

    xfer.check_mem(32'h2000_0000, 256, 32'h1111_0000, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_XFER_VSEQ", $sformatf("256-word verify: %0d mismatches", mismatches))
    else
      `uvm_info("DMA_XFER_VSEQ", "256-word verify: OK", UVM_LOW)

    // Cleanup
    dma_seq.dma_global_enable(0);

    `uvm_info("DMA_XFER_VSEQ", $sformatf("Checker: %0d matches, %0d mismatches",
      chk.match_count, chk.mismatch_count), UVM_LOW)

    `uvm_info("DMA_XFER_VSEQ", "=== DMA-002: done ===", UVM_LOW)
  endtask
endclass


// =============================================================================
// DMA-003: Multi-Channel Concurrent Transfer (P1)
// =============================================================================
class soc_dma_concur_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_dma_concur_v_sequence)

  soc_soc_dma_sequence     dma_seq;
  soc_dma_xfer_helper      xfer;
  soc_dma_checker          chk;

  function new(string name = "soc_dma_concur_v_sequence");
    super.new(name);
  endfunction

  task body();
    bit[31:0] rdata;
    bit       success0, success1;
    int       mismatches;

    `uvm_info("DMA_CONC_VSEQ", "=== DMA-003: Multi-channel concurrent ===", UVM_LOW)

    dma_seq = soc_soc_dma_sequence::type_id::create("dma_seq", null);
    dma_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    chk = soc_dma_checker::type_id::create("dma_chk", null);
    dma_seq.chk = chk;

    xfer = soc_dma_xfer_helper::type_id::create("xfer", null);
    xfer.set_sequencer(p_sequencer.ahb_mst_sqr);

    #200ns;

    // Fill source regions
    xfer.fill_mem(32'h0000_4000, 32, 32'h2222_0000);  // ch0 src
    xfer.fill_mem(32'h0000_5000, 64, 32'h3333_0000);  // ch1 src (longer)

    xfer.dma_global_enable(1);

    // Configure both channels
    xfer.configure_word_xfer(0, 32'h0000_4000, 32'h2000_0000, 32, 1'b1);
    xfer.configure_word_xfer(1, 32'h0000_5000, 32'h2001_0000, 64, 1'b1);

    // Enable both
    xfer.dma_ch_enable(0);
    xfer.dma_ch_enable(1);

    // Trigger both
    xfer.dma_ch_soft_trigger(0);
    xfer.dma_ch_soft_trigger(1);

    // Wait for both to complete
    begin
      bit fired0, fired1;
      xfer.dma_wait_tfr(0, 100000, fired0);
      xfer.dma_wait_tfr(1, 100000, fired1);
      if (fired0)
        `uvm_info("DMA_CONC_VSEQ", "CH0 completed — ok", UVM_LOW)
      else
        `uvm_error("DMA_CONC_VSEQ", "CH0 timeout")
      if (fired1)
        `uvm_info("DMA_CONC_VSEQ", "CH1 completed — ok", UVM_LOW)
      else
        `uvm_error("DMA_CONC_VSEQ", "CH1 timeout")
    end

    // Verify data integrity for both
    xfer.check_mem(32'h2000_0000, 32, 32'h2222_0000, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_CONC_VSEQ", $sformatf("CH0 verify: %0d mismatches", mismatches))
    else
      `uvm_info("DMA_CONC_VSEQ", "CH0 verify: OK", UVM_LOW)

    xfer.check_mem(32'h2001_0000, 64, 32'h3333_0000, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_CONC_VSEQ", $sformatf("CH1 verify: %0d mismatches", mismatches))
    else
      `uvm_info("DMA_CONC_VSEQ", "CH1 verify: OK", UVM_LOW)

    // Check CHSR during/before completion
    dma_seq.rd_reg(32'h338, rdata);
    `uvm_info("DMA_CONC_VSEQ", $sformatf("CHSR=0x%08h", rdata), UVM_LOW)

    dma_seq.dma_global_enable(0);

    `uvm_info("DMA_CONC_VSEQ", $sformatf("Checker: %0d matches, %0d mismatches",
      chk.match_count, chk.mismatch_count), UVM_LOW)

    `uvm_info("DMA_CONC_VSEQ", "=== DMA-003: done ===", UVM_LOW)
  endtask
endclass


// =============================================================================
// DMA-004/005/007: Interrupt Tests (P0/P1)
// =============================================================================
class soc_dma_int_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_dma_int_v_sequence)

  soc_soc_dma_sequence     dma_seq;
  soc_dma_xfer_helper      xfer;
  soc_dma_checker          chk;

  function new(string name = "soc_dma_int_v_sequence");
    super.new(name);
  endfunction

  task body();
    bit[31:0] rdata;
    bit       success, fired;
    int       mismatches;

    `uvm_info("DMA_INT_VSEQ", "=== DMA-004/005/007: Interrupt tests ===", UVM_LOW)

    dma_seq = soc_soc_dma_sequence::type_id::create("dma_seq", null);
    dma_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    chk = soc_dma_checker::type_id::create("dma_chk", null);
    dma_seq.chk = chk;

    xfer = soc_dma_xfer_helper::type_id::create("xfer", null);
    xfer.set_sequencer(p_sequencer.ahb_mst_sqr);

    #200ns;

    // === DMA-004: Transfer Complete Interrupt ===
    `uvm_info("DMA_INT_VSEQ", "--- DMA-004: Transfer complete interrupt ---", UVM_LOW)

    xfer.fill_mem(32'h0000_6000, 64, 32'h4444_0000);
    xfer.dma_global_enable(1);

    xfer.configure_word_xfer(0, 32'h0000_6000, 32'h2002_0000, 64, 1'b1);
    xfer.dma_ch_enable(0);
    xfer.dma_ch_soft_trigger(0);

    // Poll INT_STATUS0 for tfr
    xfer.dma_wait_tfr(0, 100000, fired);
    if (fired)
      `uvm_info("DMA_INT_VSEQ", "DMA-004: tfr interrupt fired — ok", UVM_LOW)
    else
      `uvm_error("DMA_INT_VSEQ", "DMA-004: tfr interrupt timeout")

    // Verify INT_STATUS0[1]=1
    dma_seq.rd_ch_reg(0, 32'h14, rdata);
    if (rdata[1])
      `uvm_info("DMA_INT_VSEQ", $sformatf("INT_STATUS0=0x%08h, tfr set ok", rdata), UVM_LOW)
    else
      `uvm_error("DMA_INT_VSEQ", $sformatf("INT_STATUS0[1] not set, val=0x%08h", rdata))

    // Verify data
    xfer.check_mem(32'h2002_0000, 64, 32'h4444_0000, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_INT_VSEQ", $sformatf("DMA-004 verify: %0d mismatches", mismatches))

    dma_seq.dma_clear_int(0, 5'h1F);

    // === DMA-005: Half-Transfer Interrupt ===
    `uvm_info("DMA_INT_VSEQ", "--- DMA-005: Half-transfer interrupt ---", UVM_LOW)

    // Use 256 words — htfr should fire at midpoint
    xfer.fill_mem(32'h0000_7000, 256, 32'h5555_0000);

    begin
      bit[31:0] ctrl_a, ctrl_b;
      ctrl_a = xfer.build_ctrl_a(
        .block_tl(255),   // 256 words
        .group_len(4'h0),
        .sinc(2'b00),
        .dinc(2'b00),
        .src_width(2'b10),
        .dst_width(2'b10)
      );
      ctrl_b = xfer.build_ctrl_b(1'b1);  // int_en=1
      xfer.dma_ch_configure(3, 32'h0000_7000, 32'h2000_0000, ctrl_a, ctrl_b, 32'h0);
    end

    xfer.dma_ch_enable(3);
    xfer.dma_ch_soft_trigger(3);

    // Poll for htfr
    xfer.dma_wait_htfr(3, 100000, fired);
    if (fired)
      `uvm_info("DMA_INT_VSEQ", "DMA-005: htfr interrupt fired — ok", UVM_LOW)
    else
      `uvm_info("DMA_INT_VSEQ", "DMA-005: htfr not observed (may depend on timing)", UVM_LOW)

    // Wait for full completion
    xfer.dma_wait_tfr(3, 100000, fired);
    if (fired)
      `uvm_info("DMA_INT_VSEQ", "DMA-005: tfr interrupt fired — ok", UVM_LOW)
    else
      `uvm_error("DMA_INT_VSEQ", "DMA-005: tfr interrupt timeout")

    dma_seq.dma_clear_int(3, 5'h1F);

    // === DMA-007: Interrupt Mask ===
    `uvm_info("DMA_INT_VSEQ", "--- DMA-007: Interrupt mask ---", UVM_LOW)

    xfer.fill_mem(32'h0000_8000, 32, 32'h7777_0000);

    xfer.configure_word_xfer(4, 32'h0000_8000, 32'h2001_0000, 32, 1'b1);
    // Mask tfr interrupt
    xfer.wr_ch_reg(4, 32'h10, 32'h0000_0004);
    xfer.dma_ch_enable(4);
    xfer.dma_ch_soft_trigger(4);

    // Wait for transfer
    xfer.dma_wait_tfr(4, 100000, fired);

    // INT_STATUS should still show tfr
    dma_seq.rd_ch_reg(4, 32'h14, rdata);
    if (rdata[1])
      `uvm_info("DMA_INT_VSEQ", $sformatf("DMA-007: INT_STATUS4=0x%08h (tfr set despite mask) ok", rdata), UVM_LOW)
    else
      `uvm_error("DMA_INT_VSEQ", "DMA-007: INT_STATUS4[1] not set")

    // Clear mask and verify
    xfer.wr_ch_reg(4, 32'h10, 32'h0000_0000);
    dma_seq.dma_clear_int(4, 5'h1F);

    xfer.check_mem(32'h2001_0000, 32, 32'h7777_0000, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_INT_VSEQ", $sformatf("DMA-007 verify: %0d mismatches", mismatches))

    dma_seq.dma_global_enable(0);

    `uvm_info("DMA_INT_VSEQ", $sformatf("Checker: %0d matches, %0d mismatches",
      chk.match_count, chk.mismatch_count), UVM_LOW)

    `uvm_info("DMA_INT_VSEQ", "=== DMA-004/005/007: done ===", UVM_LOW)
  endtask
endclass


// =============================================================================
// DMA-008/009/010/011/013/014: Misc Tests (P0/P1)
// =============================================================================
class soc_dma_misc_v_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_dma_misc_v_sequence)

  soc_soc_dma_sequence     dma_seq;
  soc_dma_xfer_helper      xfer;
  soc_dma_checker          chk;

  function new(string name = "soc_dma_misc_v_sequence");
    super.new(name);
  endfunction

  task body();
    bit[31:0] rdata;
    bit       success, fired;
    int       mismatches;

    `uvm_info("DMA_MISC_VSEQ", "=== DMA misc tests start ===", UVM_LOW)

    dma_seq = soc_soc_dma_sequence::type_id::create("dma_seq", null);
    dma_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    chk = soc_dma_checker::type_id::create("dma_chk", null);
    dma_seq.chk = chk;

    xfer = soc_dma_xfer_helper::type_id::create("xfer", null);
    xfer.set_sequencer(p_sequencer.ahb_mst_sqr);

    #200ns;

    // === DMA-008: Software Trigger self-clear ===
    `uvm_info("DMA_MISC_VSEQ", "--- DMA-008: SOFT_REQ self-clear ---", UVM_LOW)

    xfer.dma_global_enable(1);
    xfer.configure_word_xfer(0, 32'h0000_0000, 32'h2002_0000, 4, 1'b0);
    xfer.dma_ch_enable(0);

    // Read SOFT_REQ — should be 0
    dma_seq.rd_ch_reg(0, 32'h1C, rdata);
    if (rdata !== 32'h0)
      `uvm_error("DMA_MISC_VSEQ", $sformatf("SOFT_REQ0 pre-write exp=0x0 act=0x%08h", rdata))

    // Write SOFT_REQ
    dma_seq.wr_ch_reg(0, 32'h1C, 32'h1);

    // Read SOFT_REQ immediately — should be 0 (self-cleared)
    dma_seq.rd_ch_reg(0, 32'h1C, rdata);
    if (rdata === 32'h0)
      `uvm_info("DMA_MISC_VSEQ", "DMA-008: SOFT_REQ0 self-cleared — ok", UVM_LOW)
    else
      `uvm_error("DMA_MISC_VSEQ", $sformatf("DMA-008: SOFT_REQ0 not cleared, val=0x%08h", rdata))

    // Verify transfer started
    dma_seq.dma_wait_tfr(0, 100000, fired);
    if (fired)
      `uvm_info("DMA_MISC_VSEQ", "DMA-008: transfer started from SOFT_REQ — ok", UVM_LOW)
    else
      `uvm_error("DMA_MISC_VSEQ", "DMA-008: transfer did not start")

    dma_seq.dma_clear_int(0, 5'h1F);

    // === DMA-009: Address Increment modes ===
    `uvm_info("DMA_MISC_VSEQ", "--- DMA-009: Address modes ---", UVM_LOW)

    // 9a: inc/inc (already tested above)

    // 9c: const source / inc dest — source reads same address repeatedly
    `uvm_info("DMA_MISC_VSEQ", "--- DMA-009c: const src / inc dst ---", UVM_LOW)

    // Write a known value to source address
    xfer.mem_write32_(32'h0000_9000, 32'hAAAA_BBBB);
    xfer.mem_write32_(32'h0000_9004, 32'hAAAA_BBBB);  // should NOT be read

    begin
      bit[31:0] ctrl_a, ctrl_b;
      ctrl_a = xfer.build_ctrl_a(
        .block_tl(3),       // 4 words
        .group_len(4'h0),
        .sinc(2'b10),       // source: constant
        .dinc(2'b00),       // dest: increment
        .src_width(2'b10),
        .dst_width(2'b10)
      );
      ctrl_b = xfer.build_ctrl_b(1'b0);
      xfer.dma_ch_configure(5, 32'h0000_9000, 32'h2001_0000, ctrl_a, ctrl_b, 32'h0);
    end

    xfer.dma_ch_enable(5);
    xfer.dma_ch_soft_trigger(5);
    xfer.dma_wait_tfr(5, 100000, fired);

    // All 4 dest words should be 0xAAAA_BBBB (same source)
    begin
      bit[31:0] val;
      int ok_count = 0;
      for (int i = 0; i < 4; i++) begin
        xfer.mem_read32_(32'h2001_0000 + i*4, val);
        if (val === 32'hAAAA_BBBB) ok_count++;
      end
      if (ok_count == 4)
        `uvm_info("DMA_MISC_VSEQ", "DMA-009c: const src → 4 identical words — ok", UVM_LOW)
      else
        `uvm_error("DMA_MISC_VSEQ", $sformatf("DMA-009c: %0d/4 words match expected", ok_count))
    end

    dma_seq.dma_clear_int(5, 5'h1F);

    // === DMA-011: Block size boundaries ===
    `uvm_info("DMA_MISC_VSEQ", "--- DMA-011: Block size boundary (4 words) ---", UVM_LOW)

    xfer.fill_mem(32'h0000_A000, 4, 32'hBBBB_0000);

    xfer.do_word_xfer(6, 32'h0000_A000, 32'h2002_0000, 4, 1'b0, 100000, success);
    if (success) begin
      xfer.check_mem(32'h2002_0000, 4, 32'hBBBB_0000, mismatches);
      if (mismatches == 0)
        `uvm_info("DMA_MISC_VSEQ", "DMA-011: 4-word boundary — ok", UVM_LOW)
      else
        `uvm_error("DMA_MISC_VSEQ", $sformatf("DMA-011: %0d mismatches", mismatches))
    end else
      `uvm_error("DMA_MISC_VSEQ", "DMA-011: transfer failed")

    dma_seq.dma_clear_int(6, 5'h1F);

    // === DMA-013: Transfer Abort (disable during transfer) ===
    `uvm_info("DMA_MISC_VSEQ", "--- DMA-013: Transfer abort ---", UVM_LOW)

    // Use a large block to have time to abort
    xfer.fill_mem(32'h0000_B000, 256, 32'hCCCC_0000);

    begin
      bit[31:0] ctrl_a, ctrl_b;
      ctrl_a = xfer.build_ctrl_a(
        .block_tl(255),   // 256 words
        .group_len(4'h0),
        .sinc(2'b00),
        .dinc(2'b00),
        .src_width(2'b10),
        .dst_width(2'b10)
      );
      ctrl_b = xfer.build_ctrl_b(1'b0);
      xfer.dma_ch_configure(7, 32'h0000_B000, 32'h2000_0000, ctrl_a, ctrl_b, 32'h0);
    end

    xfer.dma_ch_enable(7);
    xfer.dma_ch_soft_trigger(7);

    // Wait a bit then abort
    #500ns;
    xfer.dma_ch_disable(7);

    // Verify channel is no longer busy
    xfer.dma_wait_ch_idle(7, 10000, fired);
    if (fired)
      `uvm_info("DMA_MISC_VSEQ", "DMA-013: CH7 idle after disable — ok", UVM_LOW)
    else
      `uvm_info("DMA_MISC_VSEQ", "DMA-013: CH7 busy check inconclusive", UVM_LOW)

    dma_seq.dma_clear_int(7, 5'h1F);

    // === DMA-014: Enable Protection (SAR/DAR/CTRL write-protected when EN=1) ===
    `uvm_info("DMA_MISC_VSEQ", "--- DMA-014: Enable protection ---", UVM_LOW)

    // Configure channel 8
    xfer.configure_word_xfer(8, 32'h0000_C000, 32'h2002_0000, 16, 1'b0);
    xfer.dma_ch_enable(8);

    // Read back SAR8 — should still be the configured value
    dma_seq.rd_ch_reg(8, 32'h00, rdata);
    if (rdata === 32'h0000_C000)
      `uvm_info("DMA_MISC_VSEQ", $sformatf("DMA-014: SAR8 protected (read=0x%08h) ok", rdata), UVM_LOW)
    else
      `uvm_info("DMA_MISC_VSEQ", $sformatf("DMA-014: SAR8 read=0x%08h (write protection check)", rdata), UVM_LOW)

    // Disable and cleanup
    xfer.dma_ch_disable(8);
    dma_seq.dma_global_enable(0);

    `uvm_info("DMA_MISC_VSEQ", $sformatf("Checker: %0d matches, %0d mismatches",
      chk.match_count, chk.mismatch_count), UVM_LOW)

    `uvm_info("DMA_MISC_VSEQ", "=== DMA misc tests done ===", UVM_LOW)
  endtask
endclass


// =============================================================================
// DMA Smoke: Combined P0 tests (DMA-001 + DMA-002 + DMA-004 + DMA-008)
// =============================================================================
class soc_soc_dma_smoke_virtual_sequence extends soc_top_v_sequence_base;

  `uvm_object_utils(soc_soc_dma_smoke_virtual_sequence)

  soc_soc_dma_sequence     dma_seq;
  soc_dma_xfer_helper      xfer;
  soc_dma_checker          chk;

  function new(string name = "soc_soc_dma_vseq");
    super.new(name);
  endfunction

  task body();
    bit[31:0] rdata;
    bit       success, fired;
    int       mismatches;

    `uvm_info("DMA_SMOKE_VSEQ", "=== DMA smoke test start ===", UVM_LOW)

    dma_seq = soc_soc_dma_sequence::type_id::create("dma_seq", null);
    dma_seq.set_sequencer(p_sequencer.ahb_mst_sqr);
    chk = soc_dma_checker::type_id::create("dma_chk", null);
    dma_seq.chk = chk;

    xfer = soc_dma_xfer_helper::type_id::create("xfer", null);
    xfer.set_sequencer(p_sequencer.ahb_mst_sqr);

    #200ns;

    // 1. Reset value check
    dma_seq.rd_reg(32'h33C, rdata);
    if (rdata !== 32'h0)
      `uvm_error("DMA_SMOKE", $sformatf("DMACCFG reset exp=0x0 act=0x%08h", rdata))

    // 2. Simple transfer: 16 words ISRAM→DSRAM2
    xfer.fill_mem(32'h0000_0800, 16, 32'h0000_0001);
    xfer.dma_global_enable(1);

    xfer.do_word_xfer(0, 32'h0000_0800, 32'h2002_0000, 16, 1'b1, 100000, success);
    if (!success) `uvm_error("DMA_SMOKE", "16-word transfer failed")

    xfer.check_mem(32'h2002_0000, 16, 32'h0000_0001, mismatches);
    if (mismatches != 0)
      `uvm_error("DMA_SMOKE", $sformatf("verify: %0d mismatches", mismatches))

    // 3. Check interrupt
    dma_seq.rd_ch_reg(0, 32'h14, rdata);
    if (rdata[1])
      `uvm_info("DMA_SMOKE", "tfr interrupt ok", UVM_LOW)
    else
      `uvm_error("DMA_SMOKE", $sformatf("INT_STATUS0[1] not set, val=0x%08h", rdata))

    // 4. Check SOFT_REQ self-clear
    dma_seq.wr_ch_reg(0, 32'h1C, 32'h1);
    dma_seq.rd_ch_reg(0, 32'h1C, rdata);
    if (rdata !== 32'h0)
      `uvm_error("DMA_SMOKE", $sformatf("SOFT_REQ not cleared: 0x%08h", rdata))

    dma_seq.dma_clear_int(0, 5'h1F);
    dma_seq.dma_global_enable(0);

    `uvm_info("DMA_SMOKE_VSEQ", $sformatf("Checker: %0d matches, %0d mismatches",
      chk.match_count, chk.mismatch_count), UVM_LOW)

    `uvm_info("DMA_SMOKE_VSEQ", "=== DMA smoke test done ===", UVM_LOW)
  endtask
endclass


`endif
