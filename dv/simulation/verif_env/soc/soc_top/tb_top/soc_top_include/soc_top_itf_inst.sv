`ifdef SOC_TOP_ITF_PRE_NAME
//clock reset itf
clock_if #(300, 0) `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ehs_clk)();
clock_if #(200, 0) `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, els_clk)();
clock_if #(100, 0) `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, jtag_clk)();
reset_if #(20000)
  `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, pad_rst)(.clk(`MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ehs_clk.CLOCK)), .rst_n(`MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, pad_rst).RESET));

//interface
ahb_if `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif) (`HIER_SOC_TOP.x_cpu_top.pad_core_clk, `HIER_SOC_TOP.x_cpu_top.pad_core_rst_b);
  

// Sub Environment Interface Instance

`undef SOC_TOP_ITF_PRE_NAME
`endif

