`ifdef SOC_TOP_ITF_PRE_NAME
`ifndef SOC_TOP_ENV_INST_NAME
  `define SOC_TOP_ENV_INST_NAME m_env
`endif
//config interface

uvm_resource_db #(virtual clock_if#(300,0))::set("interface_pool", {`"`SOC_TOP_PRE_NAME`", "_ehs_clk"}, `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ehs_clk));
uvm_resource_db #(virtual clock_if#(200,0))::set("interface_pool", {`"`SOC_TOP_PRE_NAME`", "_els_clk"}, `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, els_clk));
uvm_resource_db #(virtual clock_if#(100,0))::set("interface_pool", {`"`SOC_TOP_PRE_NAME`", "_jtag_clk"}, `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, jtag_clk));


uvm_resource_db #(virtual reset_if#(20000))::set("interface_pool", {`"`SOC_TOP_PRE_NAME`", "_pad_rst"}, `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, pad_rst));




`undef SOC_TOP_ITF_PRE_NAME
`undef SOC_TOP_PRE_NAME
`endif

