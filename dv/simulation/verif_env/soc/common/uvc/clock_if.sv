`ifndef CLOCK_IF__SV
`define CLOCK_IF__SV
`resetall
`timescale 1ns/10ps
interface clock_if #(parameter CLK_FREQ=100, parameter START_PHASE=0)();
  bit CLOCK;
  real half_cycle;
  int start_phase;
  real clock_freq_set;
  bit gate_en = 1'b1;
  initial begin
    CLOCK = 0;
    half_cycle = 1000.000/CLK_FREQ/2.000;
    $display("\nINFO----------CLOCK(%m) %0dMHz, half cycle is initial: %0f", CLK_FREQ, half_cycle);
    if(START_PHASE==0)
      start_phase = $urandom_range(0, int'(half_cycle*2.00));
    else
      start_phase = START_PHASE;
    fork
      begin
        #(start_phase*1.0ns);
        CLOCK = ~CLOCK;
        forever #(half_cycle*1.0ns) begin
          CLOCK = ~CLOCK;
          CLOCK = CLOCK & gate_en;
        end
      end
      begin
        forever begin
          @(half_cycle);
          clock_freq_set = 1000.000/half_cycle/2.000;
          $display("\nINFO---------CLOCK(%m) %0fMHz, half cycle set by: %0f @%0t", clock_freq_set, half_cycle, $time);
        end
      end
      begin
        forever begin
          @(gate_en);
          if(gate_en) $display("\nINFO----------CLOCK(%m) GATE is opened @%0t", $time);
          else        $display("\nINFO----------CLOCK(%m) GATE is closed @%0t", $time);
        end
      end
    join
  end
endinterface
`endif

