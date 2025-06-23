////////////////////////program download///////////////////////////////////
////Read frameware to CPU
////string frmware  = "test.pat";
////string frm_path = ".";
//initial begin : load_program
//  integer j;
//  reg [31:0] one_word;
//  reg [31:0]  temp_mem[16384];
//  if ($value$plusargs("CASEPATH=%s",frm_path))begin
//    `uvm_info("TESTCASE", $sformatf("CASEPATH=%s",frm_path), UVM_LOW)
//  end
//  uvm_config_db#(string)::wait_modified(null, "uvm_test_top", "c_case");
//  uvm_config_db#(string)::get(null, "uvm_test_top", "c_case", frmware);
//  $readmemh(<frm_path, "/", frmware, ".verilog">, temp_mem);
//
//  @(posedge PI_SOC_RST_B);
//  for(int k=0; k<32'h4000; k=k+1) begin
//    one_word[31:0] = temp_mem[k];
//    `SRAM_HIERARCHY.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte3_spram.mem[k][7:0] = one_word[7:0];
//    `SRAM_HIERARCHY.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte2_spram.mem[k][7:0] = one_word[15:8];
//    `SRAM_HIERARCHY.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte1_spram.mem[k][7:0] = one_word[23:16];
//    `SRAM_HIERARCHY.x_isram_top.x_sms_sram.x_fpga_spram.x_fpga_byte0_spram.mem[k][7:0] = one_word[31:24];
//  end
//end
//initial begin : load_data
//  integer j;
//  @(posedge PI_SOC_RST_B);
//  $display("\t******START TO LOAD PROGRAM******\n");
//  for(j=0;j<32'h4000;j=j+1) begin
//    `SRAM_HIERARCHY.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte3_spram.mem[j][7:0] = 8'h0;
//    `SRAM_HIERARCHY.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte2_spram.mem[j][7:0] = 8'h0;
//    `SRAM_HIERARCHY.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte1_spram.mem[j][7:0] = 8'h0;
//    `SRAM_HIERARCHY.x_sms0_top.x_sms_sram.x_fpga_spram.x_fpga_byte0_spram.mem[j][7:0] = 8'h0;
//  end
//end
