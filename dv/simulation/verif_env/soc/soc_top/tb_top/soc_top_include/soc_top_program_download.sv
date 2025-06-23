////////////////////////program download///////////////////////////////////
////Read frameware to CPU
initial
begin : load_program
  uvm_object_string_pool #(uvm_event#(bit[31:0])) evt_pool;
  uvm_event#(bit[31:0])                           e902_reload_program;
  evt_pool = uvm_object_string_pool #(uvm_event#(bit[31:0]))::get_global_pool();
  e902_reload_program = evt_pool.get("e902_reload_program");

   @( posedge PI_SOC_RST_B);
  load_pgm;

  e902_reload_program.wait_ptrigger();
  force tb_top.dut.x_cpu_top.pad_core_rst_b = 0;
  //`MERGE_ITF_NAME(soc_top, sys_hs_rst).RESET = 0;
  #100ns;
  force tb_top.dut.x_cpu_top.pad_core_rst_b = 1;
  //`MERGE_ITF_NAME(soc_top, sys_hs_rst).RESET = 1;
  $display("\t******START TO RELOAD PROGRAM******\n");
  load_pgm;
  $display("\t******RELOAD PROGRAM DONE******\n");
end


task load_pgm;
integer j;
integer k;
  reg [31:0] one_word;
`ifdef iverilog
  reg [31:0]  temp_mem[16384];
`else
  reg [31:0]  temp_mem[integer];
`endif
`ifndef USE_AHB_VIP_TO_REPLACE
  $readmemh("test.pat", temp_mem);
 for(k=0; k<32'h4000; k=k+1)
    begin
      one_word[31:0] = temp_mem[k];
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte3_spram.mem[k][7:0] = one_word[7:0];
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte2_spram.mem[k][7:0] = one_word[15:8];
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte1_spram.mem[k][7:0] = one_word[23:16];
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte0_spram.mem[k][7:0] = one_word[31:24];
    end
`endif
endtask


initial
begin : load_data
  integer j;
   @( posedge PI_SOC_RST_B);
  $display("\t******START TO LOAD PROGRAM******\n");
  for(j=0;j<32'h4000;j=j+1)
  begin
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte3_spram.mem[j][7:0] = 8'h0;
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte2_spram.mem[j][7:0] = 8'h0;
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte1_spram.mem[j][7:0] = 8'h0;
      `TOP_MODULE.x_retu_top.x_smu_top.x_sms_top.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte0_spram.mem[j][7:0] = 8'h0;
  end
end
