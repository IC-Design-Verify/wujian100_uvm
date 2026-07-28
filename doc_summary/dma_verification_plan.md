# DMA (dmac) Verification Plan

> Based on wujian100_open RTL (`wujian100_open/soc/dmac.v`)
> Generated: 2026-07-25

---

## 1. Overview

The DMA controller (`dmac_top`) is an AHB-Lite master/slave peripheral at base address **0x4000_0000** (16KB). It supports **16 channels** with independent block-transfer capability.

### Architecture

```
CPU (AHB master m0) ──┐
                       ├── AHB Matrix ── SRAM
DMA (AHB master m3) ──┘
                       │
CPU ── AHB Matrix ── DMA (slave s6, register access)
```

- **Slave port s6**: CPU reads/writes DMA registers (0x4000_0000–0x4000_3FFF)
- **Master port m3**: DMA reads source data from SRAM and writes destination data to SRAM
- No external VIP wiring needed — DMA operates entirely through internal AHB bus matrix

### SRAM Regions Used for Verification

| Region | Address Range | Size | Usage |
|--------|--------------|------|-------|
| ISRAM | 0x0000_0000–0x0000_FFFF | 64KB | DMA source/destination |
| DSRAM0 | 0x2000_0000–0x2000_FFFF | 64KB | DMA source/destination |
| DSRAM1 | 0x2001_0000–0x2001_FFFF | 64KB | DMA source/destination |
| DSRAM2 | 0x2002_0000–0x2002_FFFF | 64KB | DMA source/destination |

---

## 2. Register Map (RTL-Authoritative)

> **IMPORTANT**: The bit layout below is extracted directly from RTL (`dmac.v`), NOT from the spec document. The spec document has a different CTRL_A layout that does not match the actual RTL.

### 2.1 Global Registers

| Offset | Name | Width | Access | RTL Bits | Description |
|--------|------|-------|--------|----------|-------------|
| 0x330 | CHPENDIFR | 32 | R | [31:0] | Channel pending interrupt status (read-only) |
| 0x338 | CHSR | 32 | R | [31:0] | Channel busy status (read-only) |
| 0x33C | DMACCFG | 32 | RW | [0] | bit[0]=dmacen: global DMA enable |

### 2.2 Per-Channel Registers (offset = channel × 0x40)

| Offset | Name | Width | Access | Description |
|--------|------|-------|--------|-------------|
| 0x00 | SARn | 32 | RW | Source address |
| 0x04 | DARn | 32 | RW | Destination address |
| 0x08 | CTRL_An | 32 | RW | Transfer control A |
| 0x0C | CTRL_Bn | 32 | RW | Transfer control B |
| 0x10 | INT_MASKn | 32 | RW | Interrupt mask |
| 0x14 | INT_STATUSn | 32 | R | Interrupt status (read-only) |
| 0x18 | INT_CLEARn | 32 | W | Interrupt clear (W1C) |
| 0x1C | SOFT_REQn | 32 | W | Software trigger (pulse, self-clearing) |
| 0x20 | ENn | 32 | RW | Channel enable |
| 0x24 | GRP_LEN_EXTn | 32 | RW | Group length extension |

### 2.3 CTRL_An Bit Fields (RTL)

| Bits | Name | Description |
|------|------|-------------|
| [31] | dgrpaddrc | Destination group address control |
| [29] | sgrpaddrc | Source group address control |
| [28] | grpmc | Group mode control |
| [26:24] | chintmdc | Channel interrupt mode control |
| [23:12] | block_tl | Block transfer length (actual = block_tl+1, range 1–4096) |
| [11:8] | group_len | Group length |
| [7:6] | sinc | Source address increment: 00=inc, 01=dec, 1x=const |
| [5:4] | dinc | Destination address increment: 00=inc, 01=dec, 1x=const |
| [3:2] | src_tr_width | Source transfer width: 00=byte, 01=half, 10=word, 11=word |
| [1:0] | dst_tr_width | Destination transfer width: 00=byte, 01=half, 10=word, 11=word |

### 2.4 CTRL_Bn Bit Fields (RTL)

| Bits | Name | Description |
|------|------|-------------|
| [18:15] | protctl | Protection control (write-once) |
| [14] | endlan | Endian mode: 0=little, 1=big |
| [13] | srcdtlgc | Source data log control |
| [2:1] | trgtmdc | Target mode control |
| [0] | int_en | Interrupt enable: 1=enable channel interrupt |

### 2.5 INT_STATUSn / INT_CLEARn Bit Fields

| Bit | Name | Description |
|-----|------|-------------|
| [4] | trgetcmpfr | Target complete |
| [3] | htfr | Half-transfer complete |
| [2] | tfr | Transfer complete |
| [1] | err | Error |
| [0] | pend | Pending |

- **INT_STATUSn**: Set by hardware on events, cleared via INT_CLEARn (W1C)
- **INT_CLEARn**: W-only, write 1 to clear corresponding status bit

### 2.6 ENn Bit Fields

| Bit | Name | Description |
|-----|------|-------------|
| [0] | chn_en | Channel enable |

- Channel active only when **both** DMACCFG[0]=1 AND ENn[0]=1
- Auto-clears on block completion (`fsmc_regc_chnen_clr = blk_evtend`)

### 2.7 SOFT_REQn

- W-only pulse, bit[0]
- Self-clears after generating single-cycle trigger
- Must be written after channel is enabled

---

## 3. Verification Scenarios

### 3.1 Priority Definitions

| Priority | Definition | Target Coverage |
|----------|-----------|-----------------|
| **P0** | Basic function, must pass | Line/Cond/Branch > 95% |
| **P1** | Extended function, recommended | Line/Cond > 80% |
| **P2** | Advanced/boundary, conditional | Per project schedule |

### 3.2 Scenario List

#### DMA-001: Register Read/Write (P0)

**Objective**: Verify all DMA registers are readable and writable with correct reset values.

**Reset Values**:
- DMACCFG = 0x0 (DMA disabled)
- SARn/DARn = 0x0
- CTRL_An = 0x0, CTRL_Bn = 0x0
- INT_MASKn = 0x0 (all interrupts unmasked)
- INT_STATUSn = 0x0
- ENn = 0x0 (all channels disabled)
- CHPENDIFR = 0x0
- CHSR = 0x0

**Steps**:
1. After reset, read all global registers and verify reset values
2. For channel 0: write known values to SAR, DAR, CTRL_A, CTRL_B, INT_MASK, EN; read back and compare
3. Repeat for at least 2 other channels (e.g., channel 1, channel 15)
4. Verify DMACCFG[0] controls global enable
5. Verify read-only registers (INT_STATUSn, CHPENDIFR, CHSR) ignore writes

**Register Access Pattern** (via AHB UVC):
```systemverilog
// Write register
ahb_seq.mem_write32_(32'h4000_0000 + offset, data);
// Read register
ahb_seq.mem_read32_(32'h4000_0000 + offset, data);
```

---

#### DMA-002: Single Channel SRAM→SRAM Transfer (P0)

**Objective**: Configure channel 0 to transfer data from ISRAM to DSRAM2, verify data integrity.

**Configuration**:
```
DMACCFG    = 0x01          // enable DMA
SAR0       = 0x0000_1000   // source: ISRAM offset 4KB
DAR0       = 0x2002_0000   // dest: DSRAM2 base
CTRL_A0    = (0x03F << 12) | (2'b00 << 7) | (2'b00 << 5) | (2'b10 << 2) | 2'b10
             // block_tl=0x03F (64 words), src_inc=inc, dst_inc=inc, width=word
CTRL_B0    = 0x01          // int_en=1
INT_MASK0  = 0x00          // unmask all
EN0        = 0x01          // enable channel 0
SOFT_REQ0  = 0x01          // trigger transfer
```

**Steps**:
1. Pre-fill source memory (ISRAM offset 0x1000) with known pattern via AHB writes
2. Configure DMA channel 0 as above
3. Wait for interrupt or poll INT_STATUS0 for tfr bit
4. Read destination memory (DSRAM2) and compare with source pattern
5. Verify INT_STATUS0[2] (tfr) is set
6. Verify EN0[0] auto-cleared after completion

**Expected Result**: 64 words (256 bytes) copied correctly, interrupt fired, channel auto-disabled.

---

#### DMA-003: Multi-Channel Concurrent Transfer (P1)

**Objective**: Configure channels 0 and 1 simultaneously, verify independent completion.

**Configuration**:
- Channel 0: ISRAM → DSRAM0, 32 words
- Channel 1: ISRAM → DSRAM1, 64 words (longer, should complete second)

**Steps**:
1. Fill source regions with distinct patterns
2. Enable both channels simultaneously
3. Trigger both with SOFT_REQ
4. Verify each channel completes independently
5. Verify data integrity for both destinations
6. Verify CHSR shows both channels busy during transfer

---

#### DMA-004: Transfer Complete Interrupt (P0)

**Objective**: Verify interrupt fires when block transfer completes.

**Steps**:
1. Enable DMA globally, configure channel 0 with int_en=1
2. Enable channel, trigger transfer
3. Poll INT_STATUS0 until tfr bit set
4. Verify interrupt is visible at CPU level (if IRQ routing is wired)

**Expected Result**: INT_STATUS0[2]=1 after transfer completes.

---

#### DMA-005: Half-Transfer Interrupt (P1)

**Objective**: Verify half-transfer interrupt fires at midpoint.

**Steps**:
1. Configure channel with block_tl=0xFF (256 words, 1024 bytes)
2. Enable int_en, trigger transfer
3. Poll INT_STATUS0 until htfr bit set
4. Verify htfr fires when roughly half the data is transferred
5. Verify tfr fires after full completion

---

#### DMA-006: Error Interrupt (P1)

**Objective**: Verify error interrupt on invalid access.

**Note**: In the current RTL, the error path is limited. The error interrupt may only trigger on specific bus error conditions. This scenario may require bus error injection or may be limited to checking the error status bit behavior.

**Steps**:
1. Configure channel with err interrupt unmasked
2. Attempt transfer to invalid address region (if accessible)
3. Check INT_STATUS0[1] (err) if bus error occurs

---

#### DMA-007: Interrupt Mask (P1)

**Objective**: Verify INT_MASKn blocks interrupt generation.

**Steps**:
1. Configure channel 0 with int_en=1, trigger transfer
2. Verify INT_STATUS0[2]=1 (transfer complete)
3. Set INT_MASK0=0x04 (mask tfr)
4. Verify channel_interrupt output is suppressed
5. Clear mask, verify interrupt re-asserts

---

#### DMA-008: Software Trigger (P0)

**Objective**: Verify SOFT_REQ generates a single-cycle pulse and self-clears.

**Steps**:
1. Configure channel 0, enable it
2. Read SOFT_REQ0 — should be 0x00
3. Write 0x01 to SOFT_REQ0
4. Read SOFT_REQ0 immediately — should be 0x00 (self-cleared)
5. Verify transfer started (poll CHSR or INT_STATUS)

---

#### DMA-009: Address Increment/Decrement/Constant (P1)

**Objective**: Verify three address modes for source and destination.

**Test Matrix**:

| Test | sinc | dinc | Source Pattern | Expected |
|------|------|------|---------------|----------|
| 9a | 00 (inc) | 00 (inc) | addr[i] = i | addr[i] = i |
| 9b | 00 (inc) | 01 (dec) | addr[i] = i | addr[N-1-i] = i |
| 9c | 10 (const) | 00 (inc) | addr[i] = const | addr[i] = const |

**Steps for each**:
1. Fill source with pattern
2. Configure CTRL_A with appropriate sinc/dinc
3. Transfer and verify destination matches expected pattern

---

#### DMA-010: Transfer Width Combinations (P1)

**Objective**: Test all 9 combinations of src_tr_width × dst_tr_width.

**Valid Combinations** (src_width, dst_width):

| Combo | src | dst | Description |
|-------|-----|-----|-------------|
| 1 | 00 (byte) | 00 (byte) | Byte to byte |
| 2 | 00 (byte) | 01 (half) | Byte to halfword |
| 3 | 00 (byte) | 10 (word) | Byte to word |
| 4 | 01 (half) | 00 (byte) | Halfword to byte |
| 5 | 01 (half) | 01 (half) | Halfword to halfword |
| 6 | 01 (half) | 10 (word) | Halfword to word |
| 7 | 10 (word) | 00 (byte) | Word to byte |
| 8 | 10 (word) | 01 (half) | Word to halfword |
| 9 | 10 (word) | 10 (word) | Word to word |

**Note**: Mixed-width transfers require careful address alignment and data packing verification.

---

#### DMA-011: Block Size Boundary Test (P1)

**Objective**: Test block_tl values at boundaries: 0, 1, 0x7FE (max), and powers of 2.

**block_tl values to test**: 0x000 (1 byte), 0x001 (2 bytes), 0x003 (4 bytes), 0x00F (16 bytes), 0x03F (64 bytes), 0x0FF (256 bytes), 0x7FE (4096 bytes max)

**Steps for each**:
1. Configure block_tl
2. Transfer and verify data count matches block_tl+1

---

#### DMA-012: Endian Mode (P2)

**Objective**: Verify big-endian data transfer when CTRL_B[14]=1.

**Steps**:
1. Configure channel with endlan=1
2. Transfer word data
3. Verify byte ordering in destination matches big-endian format

---

#### DMA-013: Transfer Abort (P1)

**Objective**: Verify disabling ENn during transfer stops the channel.

**Steps**:
1. Configure channel with large block_tl
2. Enable and trigger transfer
3. Before completion, write ENn=0x00
4. Verify CHSR shows channel no longer busy
5. Verify partial data may be written

---

#### DMA-014: Enable Protection (P1)

**Objective**: Verify SAR/DAR/CTRL registers are write-protected when ENn[0]=1.

**Steps**:
1. Configure channel 0 completely
2. Enable channel (EN0=0x01)
3. Attempt to write SAR0, DAR0, CTRL_A0, CTRL_B0
4. Read back — values should be unchanged (write-protected)
5. Disable channel, re-write, verify changes take effect

---

## 4. Reference Model Design

The checker uses a lightweight reference model approach:

### 4.1 Register Model

```systemverilog
// Track per-channel state
bit [31:0] sar[16];      // source address
bit [31:0] dar[16];      // destination address
bit [31:0] ctrl_a[16];   // control A
bit [31:0] ctrl_b[16];   // control B
bit [31:0] int_mask[16]; // interrupt mask
bit [31:0] int_status[16]; // interrupt status
bit [31:0] en[16];       // channel enable
bit [31:0] dmaccfg;      // global config
```

### 4.2 Verification Strategy

- **Register R/W**: Reference model tracks writes, predicts reads; checker compares
- **Data Transfer**: Pre-fill source memory via AHB writes, trigger DMA, poll until complete, read destination and compare byte-by-byte
- **Interrupts**: Check INT_STATUSn bits against expected events

### 4.3 Memory Model

Since DMA operates through AHB bus matrix to SRAM, the reference model uses the same AHB UVC to pre-fill source and verify destination:

```systemverilog
// Pre-fill source memory
task fill_memory(bit[31:0] base_addr, int word_count, bit[31:0] pattern);
  for (int i = 0; i < word_count; i++)
    mem_write32_(base_addr + i*4, pattern + i);
endtask

// Verify destination memory
task check_memory(bit[31:0] base_addr, int word_count, bit[31:0] expected_pattern);
  bit[31:0] data;
  for (int i = 0; i < word_count; i++) begin
    mem_read32_(base_addr + i*4, data);
    if (data !== expected_pattern + i)
      `uvm_error("DMA_CHK", $sformatf("MISMATCH at 0x%08h: exp=0x%08h act=0x%08h",
                 base_addr + i*4, expected_pattern + i, data));
  end
endtask
```

---

## 5. Coverage Points

### 5.1 Register Coverage

- DMACCFG write/read with dmacen=0 and dmacen=1
- All CTRL_A field values: sinc/dinc (3 modes each), src_tr_width/dst_tr_width (4 values each), block_tl boundaries
- CTRL_B fields: int_en, endlan, protctl
- ENn write with channel enabled vs disabled

### 5.2 Transfer Coverage

- All 16 channels tested (at least channels 0, 1, 15)
- Block sizes: 1, 2, 4, 16, 64, 256, 4096 bytes
- Address modes: inc/inc, inc/dec, const/inc
- Transfer widths: word/word (primary), byte/byte, mixed widths

### 5.3 Interrupt Coverage

- tfr interrupt: set and clear
- htfr interrupt: set and clear
- INT_MASK on/off effect
- Channel interrupt output: (OR of masked status) AND int_en

### 5.4 FSM Coverage

- Channel enable → trigger → busy → complete → auto-disable
- Channel enable → abort (EN=0) → idle
- Multiple channels concurrent activity

---

## 6. Test Mapping

| Scenario | Test Class | vseq Class | Priority |
|----------|-----------|------------|----------|
| DMA-001 | soc_dma_reg_test | soc_dma_reg_vseq | P0 |
| DMA-002 | soc_dma_xfer_test | soc_dma_xfer_vseq | P0 |
| DMA-003 | soc_dma_concur_test | soc_dma_concur_vseq | P1 |
| DMA-004 | soc_dma_int_test | soc_dma_int_vseq | P0 |
| DMA-005 | (included in DMA-004) | (included in DMA-004) | P1 |
| DMA-007 | (included in DMA-004) | (included in DMA-004) | P1 |
| DMA-008 | (included in DMA-002) | (included in DMA-002) | P0 |
| DMA-009 | soc_dma_misc_test | soc_dma_misc_vseq | P1 |
| DMA-010 | (included in DMA-002) | (included in DMA-002) | P1 |
| DMA-011 | (included in DMA-002) | (included in DMA-002) | P1 |
| DMA-013 | (included in DMA-002) | (included in DMA-002) | P1 |
| DMA-014 | (included in DMA-001) | (included in DMA-001) | P1 |

---

## 7. Compilation Requirements

- Add `USE_SOC_DMA` to VCS compile flags
- Existing file lists already include `dmac.v` via `rtl.f`
- Existing test package (`soc_dma_testcase_pkg.svh`) and sequence package (`soc_soc_dma_seq_pkg.svh`) will be extended

---

## 8. Risk Assessment

| Risk | Mitigation |
|------|-----------|
| RTL CTRL_A differs from spec doc | Use RTL bit layout as authoritative source |
| DMA error interrupt path may be limited | Focus on tfr/htfr interrupts; error test is best-effort |
| Block completion auto-clears EN | Account for this in register R/W tests (re-enable before readback) |
| Mixed-width transfers complex | Prioritize word→word transfers; test byte→byte as secondary |
