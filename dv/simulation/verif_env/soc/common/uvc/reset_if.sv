`ifndef RESET_IF__SV
`define RESET_IF__SV
interface reset_if #(parameter RESET_DELAY=100)(input clk, input rst_n);
  bit RESET;

  initial begin
    RESET = 0;
    repeat(2) @(posedge clk);
    RESET = 1;
    repeat(2) @(posedge clk);
    RESET = 0;
    $display("\\nINFO----------RESET(%m) set 0");
    #(RESET_DELAY*1.0ns) RESET = 1;
    $display("\\nINFO----------RESET(%m) set 1 @%0t", $time);
    forever begin
      @(RESET);
      if(RESET) $display("\\nINFO----------RESET(%m) set 1 @%0t", $time);
      else      $display("\\nINFO----------RESET(%m) set 0 @%0t", $time);
    end
  end

  task wait_for_reset();
    $display("Enter wait for reset.............");
    @(posedge clk);
    if(rst_n) @(posedge ~rst_n);
    $display("Rst_n = 0 Now!");
  endtask

  task wait_n_cycle(int n);
    @(posedge clk);
    assert(n>0);
    repeat(n-1) @(posedge clk);
  endtask

  task wait_for_reset_leave();
    $display("Enter wait for reset leave.....................");
    @(posedge clk);
    if(!rst_n) @(posedge rst_n);
    $display("Rst_n = 1 Now!");
  endtask
endinterface
`endif
