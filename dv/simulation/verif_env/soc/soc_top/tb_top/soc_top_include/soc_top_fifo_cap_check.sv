  parameter START_FRAME=1;

  //wire xxx_almf;
  //wire xxx_full;
  //wire xxx_ovfl;
  //wire xxx_udfl;
  //wire xxx_push_clk;
  //wire xxx_pop_clk;
  //wire xxx_start;

  //assign xxx_start = `XXX_TOP.xxx;
  //bit assert_xxx_check_en = 0;
  //int frame_start = START_FRAME;
  
  //property fifo_check(bit check_en, bit signal, bit clock);
  //  @(posedge clock) disable iff(!check_en) signal == 0;
  //endproperty
  
  //assign xxx_full     = `HIE.xxx_full;
  //assign xxx_ovfl     = `HIE.xxx_ovfl;
  //assign xxx_udfl     = `HIE.xxx_udfl;
  //assign xxx_push_clk = `HIE.xxx_push_clk;
  //assign xxx_pop_clk  = `HIE.xxx_pop_clk;
  
  //assert property(fifo_check(assert_xxx_check_en, xxx_ovfl, xxx_push_clk))
  //else begin 
  //  `uvm_error("FIFO_ASSERT", "assertion fail xxx overflow Error!") 
  //  assert_xxx_check_en=0;
  //end
    
  //assert property(fifo_check(assert_xxx_check_en, xxx_udfl, xxx_pop_clk))
  //else begin 
  //  `uvm_error("FIFO_ASSERT", "assertion fail xxx overflow Error!")  //  assert_xxx_check_en=0; 
  //end
    
  //initial begin
  //  repeat(frame_start) @(posedge xxx_start);
  //  #100ns;
  //  assert_xxx_check_en = 1;
  //end
