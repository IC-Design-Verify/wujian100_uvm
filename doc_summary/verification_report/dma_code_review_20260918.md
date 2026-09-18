# DMA 验证代码审查报告 (2026-09-18)

## 结论：可合入（所有 3 项重点审查 PASS）

---

## 审查项 1：注入 bug 注释准确性（dma_tr_width.c / dma_addr_mode.c）

**结论：PASS**

**证据**：对照 `wujian100_open_for_debug/soc/dmac.v:15707-15720`：

```
assign cntr_blk_decen = ( is_wr_ctrlstt ) & current_rcvvld & (~blk_nu_eql0);
assign cntr_blk_rlden = (hbus_stt[4:0]==IDLE_BUS) ;
assign chregc_fsmc_block_tl_plus[12:0] = chregc_fsmc_block_tl[11:0] + 12'b1;
always@(posedge  hclk or negedge hrst_n)
    if(~hrst_n) begin
        cntr_blk[12:0] <= 13'h1FFF ;
    else  if( cntr_blk_decen  )
        cntr_blk[12:0] <=  cntr_blk[12:0] - dst_trbyt[2:0] ;
    else if( cntr_blk_rlden )
        cntr_blk[12:0] <= chregc_fsmc_block_tl_plus[12:0] * dst_trbyt[2:0] ;
```

- reload 语句为 `* dst_trbyt[2:0]`（乘法），确认注释中 `cntr_blk reload '-'→'*'` 正确
- 语义：`cntr_blk_rlden` 时 `cntr_blk <= (block_tl+1) * dst_trbyt`，即 beats = BLOCK_TL+2
- 32B 配置 + 32-bit（4B/beat）：`(32+1)*4 = 132B`；8-bit：`33*1 = 33B`；16-bit：`33*2 = 66B`
- 两个 C 用例的注释（`dmac.v:15717` 行引用、beats=BLOCK_TL+2、8/16/32-bit 实传 33/66/132B）与 RTL 行为和实测结果一致
- 两个用例作为 bug 检测器设计上 FAIL（bug_hits 非零时 sim_fail），注释与代码行为自洽

---

## 审查项 2：dma_int_split.c 写后读回延迟

**结论：PASS — 无遗漏**

**逐条确认**（`dma_int_split.c`）：

| 行号 | 操作 | 延迟 | 读回 |
|------|------|------|------|
| 86 | `INT_CLEAR=0x2` | `dma_delay(50)` | 88 |
| 90 | `INT_CLEAR=0x4` | `dma_delay(50)` | 92 |
| 94 | `INT_CLEAR=0x8` | `dma_delay(50)` | 96 |
| 110 | `INT_MASK=0x1f` | `dma_delay(50)` | 112 |
| 114 | `INT_MASK=0x00` | `dma_delay(50)` | 116 |

所有 5 次写后紧跟 `dma_delay(50)`（asm-nop），再读回。3 拍寄存器延迟对策完整，无遗漏。

---

## 审查项 3：UVM 测试类与地址一致性

**结论：PASS**

**3a. 类数量**：`soc_top_dma_dfx_test.svh` 包含恰好 4 个类：
- `soc_top_dma_vic_test`
- `soc_top_dma_prot_test`
- `soc_top_dma_dual_arb_test`
- `soc_top_dma_etb_test`

无 `soc_top_dma_dbg_test` 残留。注释明确标注调试类已移除。

**3b. dual_arb classify 区域地址**：

| 通道 | classify 区域 | C 用例地址 |
|------|-------------|-----------|
| ch1 (区B) | `0x5600` / `0x2002A000` | `SRCB=0x00005600, DSTB=0x2002A000` |
| ch0 (区A) | `0x5500` / `0x2002C000` | `SRCA=0x00005500, DSTA=0x2002C000` |

`classify()` 函数与 `dma_dual_ch_arb.c` 中 `SRCA/DSTA/SRCB/DSTB` 定义完全一致。

---

## 最终结论

**可合入（PASS × 3 / 问题 × 0）**

- 注入 bug 注释与 RTL `dmac.v:15707-15720` 及实测语义完全一致
- `dma_int_split.c` 所有写后读回均有 `dma_delay(50)` asm-nop，无遗漏
- UVM 测试类无残留调试类，地址区域与 C 用例一致
