# Unified Serial Interface (USI) - Register Description

## Unified Serial Interface (USI)

### Overview
The unified serial interface (USI) module is a serial interface useful for communication with other peripheral or microcontroller devices by the serial interface in USI. There are APB, DMA interface and interrupt interface in USI, it can be integrated in SOC as an APB device easily, transfer data with DMA and interrupt to CPU by the interrupt interface. The USI module can be configured as one function of UART or I2C or SPI. The USI’s function and the value of control registers in USI cannot be changed when the USI is working, Otherwise the data may be lost. The change can take place when the USI module is disable.

### PAD ports mapping
PAD ports mapping with internal serial module’s ports:

**Table:** (5 rows, 4 cols)

| Serial Ports(PAD) | UART | I2C | SPI |
|---|---|---|---|
| SCLK | RXD | SCL | SCK |
| SD0 | TXD | SDA | MOSI |
| SD1 | CTS |  | MISO |
| NSS | RTS |  | NSS |


### Register
Register and field descriptions
USI_CTRL
Name: USI module enable control register.
Address offset: 0x000
All of sub modules (TX FIFO, RX FIFO, UART, I2C and SPI) are controlled by this register.

**Table:** (6 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 4 |  | 0x0 | RO | Reserved. |
| 3 | RX_FIFO_EN | 0x0 | R/W | RX FIFO enable controller. 0x0: disable RX FIFO. 0x1: enable RX FIFO. |
| 2 | TX_FIFO_EN | 0x0 | R/W | TX FIFO enable controller. 0x0: disable TX FIFO. 0x1: enable TX FIFO. |
| 1 | FM_EN | 0x0 | R/W | Function modules (UART I2C SPI) enable controller. 0x0: disable function modules. 0x1: enable function modules. |
| 0 | USI_EN | 0x0 | R/W | All modules in USI enable controller. 0x0: disable USI module. 0x1: enable USI module. |

MODE_SEL
Name: mode select control register.
Address offset: 0x004
This register is used to select one module of three sub modules (UART I2C and SPI).

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 2 |  | 0x0 | RO | Reserved. |
| 1: 0 | MODE_SEL | 0x0 | R/W | 0x0: UART. 0x1: I2C. 0x2: SPI. 0x3: mode keep the last set value unchanged. |

TX_FIFO
Name: the transmitter FIFO
Address offset: 0x008
The TX data is stored in this FIFO, then the USI will read from this FIFO, and send the data.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 16 |  | 0x0 | RO | Reserved. |
| 15: 0 | TX_DATA |  | WO | Transmitting data |

RX_FIFO
Name: the receiver FIFO
Address offset: 0x008
If USI received the data, the USI will store the data into RX FIFO.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 16 |  | 0x0 | RO | Reserved. |
| 15: 0 | RX_DATA |  | RO | Received data |

FIFO_STA
Name: the status of receive FIFO
Address offset: 0x00C
This register lists the FULL or EMTPY data counter status for TX and RX FIFO.

**Table:** (10 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 20 |  | 0x0 | RO | Reserved. |
| 19: 16 | RX_NUM | 0x0 | RO | The counter indicates how many data word is stored in the RX FIFO. |
| 15: 12 |  | 0x0 | RO | Reserved. |
| 11: 8 | TX_NUM | 0x0 | RO | The counter indicates how many data words is stored in the TX FIFO. |
| 7: 4 |  | 0x0 | RO | Reserved. |
| 3 | RX_FULL | 0x0 | RO | 0x0: the TX FIFO is not full. 0x1: the RX FIFO is full. |
| 2 | RX_EMPTY | 0x1 | RO | 0x0: the RX FIFO is not empty. 0x1: the RX FIFO is empty |
| 1 | TX_FULL | 0x0 | RO | 0x0: the TX FIFO is not full 0x1: TX FIFO is full |
| 0 | TX_EMPTY | 0x1 | RO | 0x0: the TX FIFO is not empty. 0x1: the TX FIFO is empty |

CLK_DIV0
Name: I2C SCL high counter register.
Address offset: 0x010
Description: The clock divider register. In UART mode, the register set the baud rate; in I2C mode, the register sets the SCL clock high-period count; in the SPI mode, the register sets the SCK clock period count.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 24 |  | 0x0 | RO | Reserved. |
| 23: 0 | CLK_DIV0 | 0x20 | R/W | System clock divided register. UART: baud rate. Baud rate = FPCLK/(16 * (CLK_DIV0+1)) I2C: the register sets SCL clock high-period count.  FSCL = FPCLK/(CLK_DIV0+CLK_DIV1+2) SPI: divider for SCK clock. FSCK = FPCLK/CLK_DIV0, CLK_DIV0 is even value |

CLK_DIV1
Name: I2C SCL low counter register.
Address offset: 0x014
Description: This register is only for I2C, the register set the I2C SCL clock low-period count.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 24 |  | 0x0 | RO | Reserved. |
| 23: 0 | CLK_DIV1 | 0x30 | R/W | I2C: sets SCL clock low-period counter. UART, SPI: no used. |

UART_CTRL
Name: UART control register
Address offset: 0x018
Description: All of the features (Data length, STOP bit length and parity) can be configured by this register.

**Table:** (6 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 6 |  | 0x0 | RO | Reserved. |
| 5 | EPS | 0x0 | R/W | Even parity select. This bit is used to select between even and odd parity, when parity is enable (PEN set to 1).  0x0: an odd number of logic 1s is transmitted or checked. 0x1: an even number of logic 1s is transmitted or checked. |
| 4 | PEN | 0x0 | R/W | Parity enable. This bit is used to enable and disable parity generation and detection in transmitted and received serial character respectively. 0x0: parity disable. 0x1: parity enable. |
| 3: 2 | PBIT | 0x0 | R/W | Number of stop bits. This is used to select the number of stop bits per character that the peripheral transmits and receives. 0x0: 1 stop bit. 0x1: 1.5 stop pbits. 0x2: 2 stop bits. 0x3: number of stop bits keep the last set value unchanged. |
| 1: 0 | DBIT | 0x3 | R/W | Data bits select. This is used to select the number of data bits per character that the peripheral transmits and receive. The number of bit that may be selected areas follows: 0x0: 5-bit. 0x1: 6-bit. 0x2: 7-bit. 0x3: 8-bit. |

UART_STA
Name: UART status register.
Address offset: 0x01C
Description: The RXD and TXD working state can be found form this state register.

**Table:** (4 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 2 |  | 0x0 | RO | Reserved. |
| 1 | RXD_WORK | 0x0 | RO | The UART RXD is receiving. |
| 0 | TXD_WORK | 0x0 | RO | The UART TXD is transmitting. |

I2C_MODE
Name: I2C mode control register
Address offset: 0x020
Description: The I2C can work in only one mode of two (master or slave) mode.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 1 |  | 0x0 | RO | Reserved. |
| 0 | MS_MODE | 0x1 | R/W | 0x0: I2CS mode. 0x1: I2CM mode. |

I2C_ADDR
Name: I2C address register.
Address offset: 0x024
Description: Both I2CM and I2CS use this register as I2CS address.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 10 |  | 0x0 | RO | Reserved. |
| 9: 0 | I2C_ADDR | 0x133 | R/W | The 10-bit I2C slave address. |

I2CM_CTRL
Name: I2C master mode control register
Address offset: 0x028
Description: The I2CM will implement I2C protocol in I2CM mode by configure this control register.

**Table:** (7 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 5 |  | 0 | RO | Reserved. |
| 4 | GCALL | 0x0 | R/W | 0x0: the first address is not general call address 0x1: the first address is general call address If user want to use the general call address, he has to clear the control bit of ADDR_MODE. |
| 3 | SBYTE | 0x0 | R/W | 0x0: do not generate START byte. 0x1: generate START byte. |
| 2 | HS_MODE | 0x0 | R/W | High speed mode. |
| 1 | STOP | 0x0 | R/W | 0: if TX FIFO is empty, the I2CM holds SCL low. 1: if TX FIFO is empty, the I2CM terminates transfer, then generating STOP condition. |
| 0 | ADDR_MODE | 0x0 | R/W | 0: 7-bit address mode. 1: 10-bit address mode. If both of GCALL and ADDR_MODE is set by “1”, I2CM only work in 10-bit I2C address mode. |

I2CM_CODE
Name: I2C master code register.
Address offset: 0x02C
Description: The I2C master code is used in high-speed mode.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 3 |  | 0x0 | RO | Reserved. |
| 2: 0 | MCODE | 0x1 | R/W | I2CM code. |

I2CS_CTRL
Name: I2C slave control register
Address offset: 0x030
Description: Many special I2C protocol can be configured by this control register.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 1 |  | 0x0 | RO | Reserved. |
| 0 | GCALL_MODE | 0x0 | R/W | 0x0: receive general call data. 0x1: don’t receive the data of general call. |

I2C_FM_DIV
Name: I2C fast-mode clock divider register.
Address offset: 0x034
Description: The I2C fast mode divisor. this divisor is used in I2C master code and START byte transfer.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 8 |  | 0x0 | RO | Reserved. |
| 7: 0 | I2C_FM_DIV | 0x5 | R/W | The I2C fast mode divisor. |

I2C_HOLD
Name: the hold time register.
Address offset: 0x038
Description: This register controls the amount of hold time on the SDA signal after a negative of SCL in both master and slave mode.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 8 |  | 0x0 | RO | Reserved. |
| 7: 0 | I2C_HOLD | 0x5 | R/W | The I2C hold time register. The hold time after SCL fall edge, used in both I2CM and I2CS. TI2C_HOLD = (I2C_HOLD+1)*TPCLK TPCLK: the PCLK period. |

I2C_STA
Name: I2C status register
Address offset: 0x03C
Description: I2CM and I2CS state of working can be found in this state register.

**Table:** (6 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 9 |  | 0x0 | RO | Reserved. |
| 8 | I2CS_WORK | 0x0 | RO | 1’b0: I2C slave is idle. 1’b1: I2C slave is active. |
| 7: 2 |  | 0x0 | RO | Reserved. |
| 1 | I2CM_DATA | 0x0 | RO | If this bit equal 1’b1 that indicates I2C master is transmitting or receiving data. |
| 0 | I2CM_WORK | 0x0 | RO | 1’b0: I2C master is idle. 1’b1: I2C master is active. |

SPI_MODE
Name: SPI (master or slave) mode select register.
Address offset: 0x040
Description: The SPI master or slave can be selected by write this register.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 1 |  | 0x0 | RO | Reserved. |
| 0 | MS_MODE | 0x1 | R/W | 0x0: SPI slave mode. 0x1: SPI master mode. |

SPI_CTRL
Name: SPI control register
Address offset: 0x044
Description: User can control phase, polarity and data length by this control register.

**Table:** (9 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 10 |  | 0x0 | RO | Reserved. |
| 9 | NSS_CTRL | 0x0 | R/W | Infect the NSS output  0X0: the output NSS is controlled by the NSS_TOGGLE 0x1: the output NSS is controlled by the NSS_DATA |
| 8 | NSS_TOGGLE | 0x0 | R/W | 0x0: the NSS toggle at end of each data item. 0x0: the NSS is not change until the SPI master stop. |
| 7 | CPOL | 0x0 | R/W | Clock polarity 0x0: Inactive state of serial clock is low. 0x1: Inactive state of serial clock is high. |
| 6 | CPHA | 0x0 | R/W | Clock phase 0x0: Serial clock toggles in middle of first data bit. 0x1: Serial clock toggles at start of first data bit. |
| 5: 4 | TMOD | 0x0 | R/W | 0x0: Transmit & Receive. 0x1: Transmit Only. 0x2: Receive Only. 0x3: TMOD keep the last set value unchanged. |
| 3: 0 | DATA_SIZE | 0x7 | R/W | Data frame size. |
|  |  |  |  |  |

DATA_SIZE

**Table:** (17 rows, 2 cols)

| DATA_SIZE value | Description |
|---|---|
| 0x0 | Reserved. If user set this value, it will be replaced by the default value 0x7. |
| 0x1 | Reserved. If user set this value, it will be replaced by the default value 0x7. |
| 0x2 | Reserved. If user set this value, it will be replaced by the default value 0x7. |
| 0x3 | 4-bit data length for each transfer. |
| 0x4 | 5-bit data length for each transfer. |
| 0x5 | 6-bit data length for each transfer. |
| 0x6 | 7-bit data length for each transfer. |
| 0x7 | 8-bit data length for each transfer. |
| 0x8 | 9-bit data length for each transfer. |
| 0x9 | 10-bit data length for each transfer. |
| 0xA | 11-bit data length for each transfer. |
| 0xB | 12-bit data length for each transfer. |
| 0xC | 13-bit data length for each transfer. |
| 0xD | 14-bit data length for each transfer. |
| 0xE | 15-bit data length for each transfer. |
| 0xF | 16-bit data length for each transfer. |

SPI_STA
Name: SPI status register
Address offset: 0x048
Description: The SPI working status can be found in this register.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 1 |  | 0x0 | RO | Reserved. |
| 0 | SPI_WORKING | 0x0 | RO | SPI master or slave is working. |

INTR_CTRL
Name: interrupts control register.
Address offset: 0x04C
Description: This register controls the threshold value for the transmit and receive FIFO memory.

**Table:** (7 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 17 |  | 0x0 | RO | Reserved. |
| 16 | TH_MODE | 0x0 | R/W | 0x1: not less than threshold. 0x0: not larger than threshold. |
| 15: 11 |  | 0x0 | RO | Reserved. |
| 10: 8 | RX_FIFO_TH | 0x1 | R/W | RX FIFO threshold level control bits. If the received data in RX FIFO equal the threshold value, an interrupt will be generated. 0x0: threshold value keep the last set value unchanged. 0x1: 1-byte data 0x2: 2-bye data 0x3: 3-byte data 0x4: 4-byte data 0x5: 5-byte data 0x6: 6-byte data 0x7: 7-byte data |
| 7: 3 |  | 0x0 | RO | Reserved. |
| 2: 0 | TX_FIFO_TH | 0x1 | R/W | TX FIFO threshold control bits. 0x0: threshold value keep the last set value unchanged. 0x1: 1-byte 0x2: 2-byte 0x3: 3-byte 0x4: 4-byte 0x5: 5-byte 0x6: 6-byte 0x7: 7-byte |

INTR_EN
Name: interrupts enable control register.
Address offset: 0x050
Description: the interrupt can be enable by set the control bit.

**Table:** (21 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 19 |  | 0x0 | RO | Reserved. |
| 18 | SPI_STOP_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 17 | I2C_AERR_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 16 | I2CS_GCALL_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 15 | I2CM_LOSE_ARBI_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 14 | I2C_NACK_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 13 | I2C_STOP_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 12 | UART_PERR_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 11 | UART_RX_STOP_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 10 | UART_TX_STOP_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 9 | RX_WERR_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 8 | RX_RERR_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 7 | RX_FULL_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 6 | RX_EMPTY_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 5 | RX_THOLD_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 4 | TX_WERR_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 3 | TX_RERR_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 2 | TX_FULL_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 1 | TX_EMPTY_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |
| 0 | TX_THOLD_EN | 0x0 | R/W | The interrupt enable control bit. 0x0: disable the interrupt. 0x1: enable the interrupt. |

INTR_STA
Name: interrupts status register.
Address offset: 0x054
Description: which interrupt will be found in this register, all the state of the interrupt can be masked.

**Table:** (21 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 19 |  | 0x0 | RO | Reserved. |
| 18 | SPI_STOP | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 17 | I2C_AERR | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 16 | I2CS_GCALL | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 15 | I2CM_LOSE_ARBI | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 14 | I2C_NACK | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 13 | I2C_STOP | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 12 | UART_PERR | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 11 | UART_RX_STOP | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 10 | UART_TX_STOP | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 9 | RX_WERR | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 8 | RX_RERR | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 7 | RX_FULL | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 6 | RX_EMPTY | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 5 | RX_THOLD | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 4 | TX_WERR | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 3 | TX_RERR | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 2 | TX_FULL | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 1 | TX_EMPTY | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |
| 0 | TX_THOLD | 0x0 | RO | See “RAW_INTR_STA” on page 35 for a detailed description of these bits. |

RAW_INTR_STA
Name: interrupts status register.
Address offset: 0x058
Description: The raw of interrupts status register.

**Table:** (21 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 19 |  | 0x0 | RO | Reserved. |
| 18 | RAW_SPI_STOP | 0x0 | RO | SPI master: if the TX FIFO is empty, or the RX FIFO is full the interrupt will be generated before the FSM enter the SPIM_IDLE state. SPI slave: the interrupt will be generated after one data transfer. |
| 17 | RAW_I2C_AERR | 0x0 | RO | If the I2C_ADDR is loaded a reserved address or a special address in 7-bit address mode, the I2CM will generate a interrupt. |
| 16 | RAW_I2CS_GCALL | 0x0 | RO | When the I2CS received a general call, the interrupt will be generated. |
| 15 | RAW_I2CM_LOSE_ARBI | 0x0 | RO | When the I2CM loses arbitration, the interrupt will be generated. |
| 14 | RAW_I2C_NACK | 0x0 | RO | The interrupt will generated either the I2CM or I2CS receives NACK. |
| 13 | RAW_I2C_STOP | 0x0 | RO | The interrupt will be generated when the I2CM or I2CS finished working before the FSM enter the IDLE state. |
| 12 | RAW_UART_PERR | 0x0 | RO | The interrupt will be generated if the parity check fails when the UART RXD received the data. |
| 11 | RAW_UART_RX_STOP | 0x0 | RO | UART RXD: the interrupt will be generated after the data is received. |
| 10 | RAW_UART_TX_STOP | 0x0 | RO | UART TXD: the interrupt will be generated at the STOP bit. |
| 9 | RAW_RX_WERR | 0x0 | RO | The interrupt will be generated, when the RX FIFO is written after the RX FIFO is full. |
| 8 | RAW_RX_RERR | 0x0 | RO | The interrupt will be generated, when the RX FIFO is read after the RX FIFO is empty. |
| 7 | RAW_RX_FULL | 0x0 | RO | The interrupt will be generated when RX FIFO is full. |
| 6 | RAW_RX_EMPTY | 0x0 | RO | The interrupt will be generated when RX FIFO is empty. |
| 5 | RAW_RX_THOLD | 0x0 | RO | TH_MODE = 0 (TH_MODE is field of INTR_CTRL register) The interrupt will be generated, when the RX FIFO depth of data not larger than threshold value of RX_FIFO_TH (the field in INTR_CTRL register).  TH_MODE = 1 The interrupt will be generated, when the RX FIFO depth of data not less than threshold value of RX_FIFO_TH. |
| 4 | RAW_TX_WERR | 0x0 | RO | The interrupt will be generated, when the TX FIFO is written after the TX FIFO is full. |
| 3 | RAW_TX_RERR | 0x0 | RO | The interrupt will be generated, when the TX FIFO is read after the TX FIFO is empty. |
| 2 | RAW_TX_FULL | 0x0 | RO | The interrupt will be generated when TX FIFO is full. |
| 1 | RAW_TX_EMPTY | 0x0 | RO | The interrupt will be generated when TX FIFO is empty. |
| 0 | RAW_TX_THOLD | 0x0 | RO | TH_MODE = 0 The interrupt will be generated, when the TX FIFO depth of data not larger than threshold value of TX_FIFO_TH (the field in INTR_CTRL register). TH_MODE = 1 The interrupt will be generated, when the TX FIFO depth of data not less than threshold value of TX_FIFO_TH. |

INTR_UNMASK
Name: interrupts unmask register.
Address offset: 0x05C
Description: the interrupt can be masked by this control register.

**Table:** (21 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 19 |  | 0x0 | RO | Reserved. |
| 18 | SPI_STOP_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 17 | I2C_AERR_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 16 | I2CS_GCALL_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 15 | I2CM_LOSE_ARBI_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 14 | I2C_NACK_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 13 | I2C_STOP_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 12 | UART_PERR_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 11 | UART_RX_STOP_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 10 | UART_TX_STOP_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 9 | RX_WERR_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 8 | RX_RERR_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 7 | RX_FULL_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 6 | RX_EMPTY_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 5 | RX_THOLD_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 4 | TX_WERR_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 3 | TX_RERR_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 2 | TX_FULL_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 1 | TX_EMPTY_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |
| 0 | TX_THOLD_MASK | 0x0 | R/W | Interrupt unmask register 0x0: masked 0x1: unmasked. |

INTR_CLR
Name: interrupts clear register.
Address offset: 0x060
Description: the control register be use to clear interrupt.

**Table:** (21 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 19 |  | 0x0 | RO | Reserved. |
| 18 | SPI_STOP_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_SPI_STOP interrupt of the RAW_INTR_STA register. |
| 17 | I2C_AERR_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_I2C_AERR interrupt of the RAW_INTR_STA register. |
| 16 | I2CS_GCALL_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_I2CS_GCALL interrupt of the RAW_INTR_STA register. |
| 15 | I2CM_LOSE_ARBI_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_I2CM_LOSE_ARBI interrupt of the RAW_INTR_STA register. |
| 14 | I2C_NACK_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_I2C_NACK interrupt of the RAW_INTR_STA register. |
| 13 | I2C_STOP_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_I2C_STOP interrupt of the RAW_INTR_STA register. |
| 12 | UART_PERR_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_UART_PERR interrupt of the RAW_INTR_STA register. |
| 11 | UART_RX_STOP_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_UART_RX_STOP interrupt of the RAW_INTR_STA register. |
| 10 | UART_TX_STOP_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_UART_TX_STOP interrupt of the RAW_INTR_STA register. |
| 9 | RX_WERR_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_RX_WERR interrupt of the RAW_INTR_STA register. |
| 8 | RX_RERR_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_RX_RERR interrupt of the RAW_INTR_STA register. |
| 7 | RX_FULL_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_RX_FULL interrupt of the RAW_INTR_STA register. |
| 6 | RX_EMPTY_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_RX_EMPTY interrupt of the RAW_INTR_STA register. |
| 5 | RX_THOLD_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_RX_THOLD interrupt of the RAW_INTR_STA register. |
| 4 | TX_WERR_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_TX_WERR interrupt of the RAW_INTR_STA register. |
| 3 | TX_RERR_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_TX_RERR interrupt of the RAW_INTR_STA register. |
| 2 | TX_FULL_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_TX_FULL interrupt of the RAW_INTR_STA register. |
| 1 | TX_EMPTY_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_TX_EMPTY interrupt of the RAW_INTR_STA register. |
| 0 | TX_THOLD_CLR | 0x0 | WO | Write 1’b1 to clear the RAW_TX_THOLD interrupt of the RAW_INTR_STA register. |

DMA_CTRL
Name: DMA interface control register.
Address offset: 0x064
Description: this register is design for controlling the TX FIFO or RX FIFO DMA interface.

**Table:** (4 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 2 |  | 0x0 | RO | Reserved. |
| 1 | RX_DMA_EN | 0x0 | R/W | RX FIFO DMA interface enable. 0x0: disable the RX FIFO DMA interface. 0x1: enable the RX FIFO DMA interface. |
| 0 | TX_DMA_EN | 0x0 | R/W | TX FIFO DMA interface enable. 0x0: disable the TX FIFO DMA interface. 0x1: enable the TX FIFO DMA interface. |

DMA_THRESHOLD
Name: DMA interface threshold control register.
Address offset: 0x068
Description: the threshold value for DMA trigger.

**Table:** (5 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 12 |  | 0x0 | RO | Reserved. |
| 11: 8 | RX_DMA_TH | 0x4 | R/W | The counter is the threshold level for RX FIFO DMA interface. If the RX FIFO DMA interface is enable, and the number of received data words in RX FIFO not less than the threshold value, then the RX FIFO DMA interface requests that DMA read data from RX FIFO. 0x0: the threshold keep the last set value unchanged. 0x1: 1-data frame … 0x5: 5-data frame 0x6: 6-data frame 0x7: 7-data frame 0x8: 8-data frame 0x9-0xF: Reserved. |
| 7: 4 |  | 0x0 | RO | Reserved. |
| 3: 0 | TX_DMA_TH | 0x4 | R/W | The counter is the threshold level for TX FIFO DMA interface. If the TX FIFO DMA interface is enable, and the number of stored data words in TX FIFO not larger than the threshold value, then the TX FIFO DMA interface requests that DMA write data to TX FIFO. 0x0: the threshold keep the last set value unchanged. 0x1: 1-data frame … 0x5: 5-data frame 0x6: 6-data frame 0x7: 7-data frame 0x8: 8-data frame 0x9-0xF: Reserved. |

SPI_NSS_DATA
Name: SPI NSS data register.
Address offset: 0x06C
Description: User can input NSS data by this nss data register.

**Table:** (3 rows, 5 cols)

| Field | Name | Default | Access | Description |
|---|---|---|---|---|
| 31: 1 |  | 0x0 | RO | Reserved. |
| 0 | NSS_DATA | 0x0 | R/W | 0x0: if NSS_CTRL is set 1, the “0” will be propagated to NSS 0x1: if NSS_CTRL is set 1, the “1” will be propagated to NSS. |

