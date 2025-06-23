//interface assignment
//ahb : ahb
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hmastlock          = 0;
`ifdef USE_AHB_VIP_TO_REPLACE
initial begin
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_haddr          = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).haddr ;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_hburst         = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hburst;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_hsize          = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hsize ;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_htrans         = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).htrans;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_hwdata         = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hwdata;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_hwrite         = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hwrite;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m0_hprot          = `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hprot ;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_haddr          = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_hburst         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_hsize          = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_htrans         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_hwdata         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_hwrite         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m1_hprot          = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_haddr          = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_hburst         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_hsize          = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_htrans         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_hwdata         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_hwrite         = 0;
  force `HIER_SOC_TOP.x_cpu_top.cpu_hmain0_m2_hprot          = 0;
end
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hready     = `HIER_SOC_TOP.x_cpu_top.hmain0_cpu_m0_hready;//1;
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hresp      = `HIER_SOC_TOP.x_cpu_top.hmain0_cpu_m0_hresp ;//0;
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hrdata     = `HIER_SOC_TOP.x_cpu_top.hmain0_cpu_m0_hrdata;//0;
`else
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hready     = 1;
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hresp      = 0;
assign `MERGE_ITF_NAME(`SOC_TOP_ITF_PRE_NAME, ahb_vif).hrdata     = 0;
`endif

// Sub Environment Interface assignment 

