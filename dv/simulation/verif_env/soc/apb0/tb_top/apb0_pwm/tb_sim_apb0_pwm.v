//  `define  PERIPH_UART  `HIER_PERIPH_SS.u_periph_uart_top.uart_wrapper_U0
//  
//  assign uart_clk         =  `PERIPH_UART.sclk; // pclk and uart_clk is sync
//
//  assign uart_dce_if.rst  =  ~`HIER_PERIPH_SS.periph_uart_0_srstn;
//
//  svt_uart_if           uart_dce_if(uart_clk);
//  svt_uart_bfm_wrapper  #(.UV_DEVICE_TYPE(`UV_DCE)) Bfm1 (uart_dce_if);
//
//  assign uart_dce_if.re       = 1'b1;
//  assign uart_dce_if.de       = 1'b0;
//  
//  assign u_soc_top.PAD_UART0SIN_GPIO1           = uart_dce_if.sin;
//  assign u_soc_top.PAD_UART0CTSN_GPIO0         = 0;
//  assign uart_dce_if.rts      =  u_soc_top.PAD_UART0RTSN_GPIO2;
//  assign uart_dce_if.sout     =  u_soc_top.PAD_UART0SOUT_GPIO3;
//  initial begin
//    uvm_config_db#(virtual svt_uart_if)::set(uvm_root::get(),"uvm_test_top.uart_dce_env","dce_vif", uart_dce_if);
//  end

