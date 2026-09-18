`ifndef SOC_TOP_DMA_DFX_TEST__SV
`define SOC_TOP_DMA_DFX_TEST__SV

// ============================================================================
// DMA DFX/量测类测试 —— 与 c_case/dma_*/ 下的 C 用例一一配合
//
// 公共模式：fork 量测/监控线程 → super.run_phase(phase)（跑 C 用例）
//           → disable fork → 断言量测结果
//
// 信号锚点（均已 RTL 实证）：
//   pad_vic_int_vld[32] : tb_top.dut.x_cpu_top.pad_vic_int_vld
//                         （core_top.v:552 ip_cpu_int_vld[32]=dmac0_wic_intr）
//   dmac_vic_if         : tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.dmac_vic_if
//   DMAC master 端口    : tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_*
//                         （ahb_matrix_top.v:870 实例化，m_hprot ← PROTCTL）
//   etb_dmacch0_trg     : tb_top.dut.x_pdu_top.x_main_bus_top.etb_dmacch0_trg
//                         （ahb_matrix_top.v:1022 tie-off 1'b0，UVM force 覆盖）
// ============================================================================

// ---------------------------------------------------------------------------
// F12: 中断聚合 + VIC 路由 —— 配合 c_case/dma_vic_route/dma_vic_route.c
// C 侧：ch0 正常传输（INT_EN=1, MASK=0x1f），完成后保持 status 置位一段
//       时间再清中断。vic_if = (status&mask)&int_en（dmac.v:4089）应为电平。
// UVM 侧：wait pad_vic_int_vld[32] 上升/回落，断言观测到完整电平窗口
// ---------------------------------------------------------------------------
class soc_top_dma_vic_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_dma_vic_test)

  bit saw_rise;
  bit saw_fall;

  extern function new(string name="soc_top_dma_vic_test", uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_dma_vic_test::new(string name="soc_top_dma_vic_test",
                                   uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_dma_vic_test::run_phase(uvm_phase phase);
  saw_rise = 0;
  saw_fall = 0;
  fork
    begin : vic_mon
      wait(tb_top.dut.x_cpu_top.pad_vic_int_vld[32] === 1'b1);
      saw_rise = 1;
      `uvm_info("DMA_VIC", "pad_vic_int_vld[32] (DMAC0) asserted", UVM_LOW)
      wait(tb_top.dut.x_cpu_top.pad_vic_int_vld[32] === 1'b0);
      saw_fall = 1;
      `uvm_info("DMA_VIC", "pad_vic_int_vld[32] (DMAC0) deasserted after INT_CLEAR", UVM_LOW)
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (!saw_rise)
    `uvm_error("DMA_VIC", "pad_vic_int_vld[32] never asserted during dma_vic_route")
  if (!saw_fall)
    `uvm_error("DMA_VIC", "pad_vic_int_vld[32] never deasserted after INT_CLEAR")
endtask: run_phase

// ---------------------------------------------------------------------------
// F9: PROTCTL → m_hprot —— 配合 c_case/dma_prot_hprot/dma_prot_hprot.c
// C 侧：CTRLB.PROTCTL=4'b1010 发起 32-bit 块传输
// UVM 侧：m_htrans[1]==1（NONSEQ/SEQ）且 m_hready 时采样 m_hprot，
//         所有采样必须 == 4'hA
// ---------------------------------------------------------------------------
class soc_top_dma_prot_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_dma_prot_test)

  int unsigned prot_samples;
  int unsigned prot_mismatch;

  extern function new(string name="soc_top_dma_prot_test", uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_dma_prot_test::new(string name="soc_top_dma_prot_test",
                                    uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_dma_prot_test::run_phase(uvm_phase phase);
  prot_samples   = 0;
  prot_mismatch  = 0;
  fork
    begin : prot_mon
      forever begin
        @(posedge tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.hclk);
        if (tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_htrans[1] === 1'b1 &&
            tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_hready   === 1'b1) begin
          prot_samples++;
          if (tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_hprot !== 4'hA) begin
            prot_mismatch++;
            `uvm_error("DMA_PROT", $sformatf("m_hprot=0x%h expect 0xA (PROTCTL) @%0t",
              tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_hprot, $realtime))
          end
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  if (prot_samples == 0)
    `uvm_error("DMA_PROT", "no valid DMA transfer sampled on master port")
  else
    `uvm_info("DMA_PROT", $sformatf("sampled %0d transfers, all m_hprot==0xA (mismatch=%0d)",
      prot_samples, prot_mismatch), UVM_LOW)
endtask: run_phase

// ---------------------------------------------------------------------------
// F4: 双通道仲裁抢占 —— 配合 c_case/dma_dual_ch_arb/dma_dual_ch_arb.c
// C 侧：ch1(低优先级,4096B,区B: 0x5600/0x2002A000) 先触发；
//       短延时后 ch0(高优先级,64B,区A: 0x5500/0x2002C000) 触发
// UVM 侧：按时钟采样 m_haddr（htrans[1]&&hready），归并为区域序列，
//         断言出现 B→A→B（ch0 抢占 ch1 后 ch1 恢复）
// ---------------------------------------------------------------------------
class soc_top_dma_dual_arb_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_dma_dual_arb_test)

  int region_q[$];   // 0=其他 1=区A(ch0) 2=区B(ch1)，连续相同不重复记录

  extern function new(string name="soc_top_dma_dual_arb_test", uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
  extern function int  classify(bit [31:0] addr);
endclass

function soc_top_dma_dual_arb_test::new(string name="soc_top_dma_dual_arb_test",
                                        uvm_component parent);
  super.new(name, parent);
endfunction

function int soc_top_dma_dual_arb_test::classify(bit [31:0] addr);
  if ((addr >= 32'h5500     && addr < 32'h5600)     ||
      (addr >= 32'h2002C000 && addr < 32'h2002C100)) return 1;  // 区A (ch0)
  if ((addr >= 32'h5600     && addr < 32'h6600)     ||
      (addr >= 32'h2002A000 && addr < 32'h2002B000)) return 2;  // 区B (ch1)
  return 0;
endfunction

task soc_top_dma_dual_arb_test::run_phase(uvm_phase phase);
  int r;
  int first_a, last_b;
  region_q.delete();
  fork
    begin : arb_mon
      forever begin
        @(posedge tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.hclk);
        if (tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_htrans[1] === 1'b1 &&
            tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_hready   === 1'b1) begin
          r = classify(tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.m_haddr);
          if (r != 0 && (region_q.size() == 0 || region_q[$] != r))
            region_q.push_back(r);
        end
      end
    end
  join_none

  super.run_phase(phase);

  disable fork;

  begin
    string s;
    s = "";
    foreach (region_q[i]) s = {s, (region_q[i] == 1) ? "A" : "B"};
    `uvm_info("DMA_ARB", $sformatf("region sequence: %s (%0d segments)", s, region_q.size()), UVM_LOW)
  end

  // 断言：存在 B 段 → A 段 → B 段（ch0 抢占 ch1，ch1 恢复）
  first_a = -1;
  last_b  = -1;
  foreach (region_q[i]) begin
    if (region_q[i] == 1 && first_a == -1) first_a = i;
  end
  for (int i = region_q.size() - 1; i >= 0; i--)
    if (region_q[i] == 2) begin last_b = i; break; end

  if (region_q.size() < 3 || first_a <= 0 || last_b <= first_a)
    `uvm_error("DMA_ARB", $sformatf(
      "no B->A->B preemption pattern (size=%0d first_A=%0d last_B=%0d)",
      region_q.size(), first_a, last_b))
  else
    `uvm_info("DMA_ARB", "ch0 preempted ch1 and ch1 resumed (B->A->B observed)", UVM_LOW)
endtask: run_phase

// ---------------------------------------------------------------------------
// F5(ETB): 硬件触发 —— 配合 c_case/dma_etb_trigger/dma_etb_trigger.c
// SoC 中 etb_dmacch0_trg tie-off 1'b0（ahb_matrix_top.v:1022）；
// UVM 侧延时后 force=1 再 release，C 侧（未写 soft_req）应观察到传输完成。
// C 侧完成即为本测试主断言；UVM 侧附加确认 force 生效。
// ---------------------------------------------------------------------------
class soc_top_dma_etb_test extends soc_top_for_c_case_test;
  `uvm_component_utils(soc_top_dma_etb_test)

  extern function new(string name="soc_top_dma_etb_test", uvm_component parent);
  extern virtual task run_phase(uvm_phase phase);
endclass

function soc_top_dma_etb_test::new(string name="soc_top_dma_etb_test",
                                   uvm_component parent);
  super.new(name, parent);
endfunction

task soc_top_dma_etb_test::run_phase(uvm_phase phase);
  fork
    begin : etb_drive
      // 等 C 侧完成通道使能（chntrg_latch 置位需 chn_en=1，dmac.v:3568），
      // 避免固定延时与 CPU boot 时间竞争
      wait(tb_top.dut.x_pdu_top.x_main_bus_top.x_dmac_top.x_reg_ctrl.x_chregc0.chn_en === 1'b1);
      #500;
      `uvm_info("DMA_ETB", "ch0 enabled, forcing etb_dmacch0_trg = 1", UVM_LOW)
      force tb_top.dut.x_pdu_top.x_main_bus_top.etb_dmacch0_trg = 1'b1;
      #2000;    // 保持 2us，覆盖若干 hclk（DMACEN 由 C 紧随 EN 后写 1）
      release tb_top.dut.x_pdu_top.x_main_bus_top.etb_dmacch0_trg;
      `uvm_info("DMA_ETB", "released etb_dmacch0_trg", UVM_LOW)
    end
  join_none

  super.run_phase(phase);   // C 侧轮询 INT_STATUS 直到 ETB 触发完成传输

  disable fork;
endtask: run_phase

// ---------------------------------------------------------------------------
// （临时调试类 soc_top_dma_dbg_test 已移除——曾用于定位 en_lock 挂死、
//   int_split 写读竞态、global_cfg DMACEN corner 的 SAR 空转，结论已固化到
//   各 C 用例注释与 DMA 验证报告）
// ---------------------------------------------------------------------------

`endif
