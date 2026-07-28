// =============================================================================
// tb_sim_soc_dma.v — DMA subsystem testbench wrapper (SoC-integrated mode)
// =============================================================================
// In `USE_AHB_VIP_TO_REPLACE + USE_SOC_DMA` mode, the full SoC
// (`wujian100_open_top dut`) is already instantiated by tb_top.sv, and
// `soc_top_interface_assignment.sv` replaces the CPU's AHB master port (m0)
// with `soc_top_ahb_vif` via force/assign. The SoC's `ahb_matrix_7_12_main`
// then routes VIP-issued AHB traffic to:
//   - s0  ISRAM       (0x0000_0000–0x0000_ffff)  — source region
//   - s2  DSRAM0      (0x2000_0000–0x2000_ffff)
//   - s3  DSRAM1      (0x2001_0000–0x2001_ffff)
//   - s4  DSRAM2      (0x2002_0000–0x2002_ffff)  — destination region
//   - s6  DMA slave   (0x4000_0000–0x4000_3fff)  — DMA register access
//
// The DMA (`dmac_top x_dmac_top`) is instantiated inside `wujian100_open_top`
// at `ahb_matrix_top.v` lines 870-976. Its master port (m3) reaches SRAM via
// the same matrix; its slave port (s6) is accessed by the VIP via m0.
//
// `soc_top_itf_inst.sv` already creates:
//   - clock_if #(300,0) soc_top_ehs_clk / els_clk / jtag_clk
//   - reset_if #(20000) soc_top_pad_rst
//   - ahb_if soc_top_ahb_vif(pad_core_clk, pad_core_rst_b)
//
// `soc_top_itf_config.sv` registers them in resource_db/interface_pool and
// config_db (`uvm_test_top.m_env.ahb_env.*` "vif"), so `soc_top_test_base`
// can read SOC_TOP_ehs_clk/els_clk/jtag_clk/pad_rst, and the AHB UVC mst/slv
// agents bind to soc_top_ahb_vif.
//
// Therefore this file intentionally contains NO DUT, clock/reset, interface,
// or config_db wiring — the SoC-level infrastructure is sufficient. The DMA
// sequences (soc_dma_*_v_sequence) drive all addresses (ISRAM, DSRAM, DMA
// regs) through the single `ahb_mst_sqr` bound to soc_top_ahb_vif; the SoC
// matrix decodes and routes each access to the proper slave.
//
// This file is included by `tb_sim_subsys_ahb_hs.v` under `ifdef USE_SOC_DMA`
// inside tb_top.sv (after `dut` instantiation and `soc_top_interface_assignment`
// has applied the force/assign). Keeping it empty preserves the build order
// while avoiding duplicate instantiation of clock_if/reset_if/ahb_if/dmac_top.
// =============================================================================
