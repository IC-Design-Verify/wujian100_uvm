`ifdef USI_I2C
  `include "apb0_usi/usi_i2c_test.v"
`elsif  USI_SPI
  `include "apb0_usi/usi_spi_test.v"
`elsif  USI_UART
  `include "apb0_usi/usi_uart_test.v"
`endif
