`ifndef SOC_TOP_USI_DFX_TEST__SV
`define SOC_TOP_USI_DFX_TEST__SV

// ============================================================================
// USI DFX/量测类测试
// ============================================================================

// ---------------------------------------------------------------------------
// F15: CLK_DIV0 波特率 —— 配合 c_case/usi_clk_div_boundary/usi_clk_div_boundary.c
// C 侧：USI0 UART 阶段1 DIV=0x81 发 16×0x55；阶段2 DIV=0x40 发 16×0x55
//       （8-N-1 + 0x55：start+8 数据+stop 每 bit 都翻转 → 每字节恰好 10 个沿，
//        bit = 16×(DIV+1) pclk → 阶段1/阶段2 = 130/65 = 2.0）
// UVM 侧：记录 PAD_USI0_SD0 有效沿（0↔1 跳变；X 毛刺沿事件域滤除），
//         前 160 沿=阶段1、后 160 沿=阶段2（沿数不符回退最大间隔切分），
//         各取最小沿间隔（=1 bit 宽度），断言比 ≈ 2
// ---------------------------------------------------------------------------
class soc_top_usi_uart_baud_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_usi_uart_baud_test)

  real edge_q[$];

  extern function new(string name="soc_top_usi_uart_baud_test",
                     uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_usi_uart_baud_test::new(string name="soc_top_usi_uart_baud_test",
                                         uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_usi_uart_baud_test::run_phase(uvm_phase phase);
  fork
    begin : edge_rec
      bit prev;
      bit prev_valid;
      prev_valid = 0;
      forever begin
        @(tb_top.PAD_USI0_SD0);
        if (tb_top.PAD_USI0_SD0 === 1'b1 || tb_top.PAD_USI0_SD0 === 1'b0) begin
          // 只记录 0↔1 有效跳变（X 毛刺沿：prev 不更新，X→0/1 只算一次）
          if (prev_valid && tb_top.PAD_USI0_SD0 !== prev)
            edge_q.push_back($realtime);
          prev = tb_top.PAD_USI0_SD0;
          prev_valid = 1;
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (edge_q.size() < 200) begin
    `uvm_error("USI_BAUD", $sformatf("too few edges recorded: %0d (expect 320)", edge_q.size()))
  end
  else begin
    real d[$];
    real min1, min2, maxd;
    int  split;
    min1 = 1.0e18; min2 = 1.0e18; maxd = 0.0;
    for (int i = 1; i < edge_q.size(); i++) d.push_back(edge_q[i] - edge_q[i-1]);
    // 主切分：C 侧 16 字节/阶段 × 10 沿/字节 → 沿 160 为阶段边界
    if (edge_q.size() == 320) begin
      split = 160;
    end
    else begin
      // 回退：最大间隔切分
      split = 0;
      for (int i = 0; i < d.size(); i++)
        if (d[i] > maxd) begin maxd = d[i]; split = i + 1; end
      `uvm_info("USI_BAUD", $sformatf("edge count %0d != 320, fallback max-gap split at %0d",
        edge_q.size(), split), UVM_LOW)
    end
    for (int i = 0; i < split - 1; i++)
      if (d[i] < min1) min1 = d[i];
    for (int i = split; i < d.size(); i++)
      if (d[i] < min2) min2 = d[i];
    if (min1 > 1.0e17 || min2 > 1.0e17 || min2 == 0.0) begin
      `uvm_error("USI_BAUD", $sformatf("cluster split failed: split=%0d/%0d min1=%0f min2=%0f",
        split, d.size(), min1, min2))
    end
    else begin
      real ratio;
      ratio = min1 / min2;
      if (ratio < 1.85 || ratio > 2.15)
        `uvm_error("USI_BAUD", $sformatf("baud ratio %0.3f not ~2.0 (DIV 0x81 vs 0x40)", ratio))
      else
        `uvm_info("USI_BAUD", $sformatf("bit width %0.1fns -> %0.1fns, ratio=%0.3f (~2x), edges=%0d",
          min1, min2, ratio, edge_q.size()), UVM_LOW)
    end
  end
endtask: run_phase

`endif
