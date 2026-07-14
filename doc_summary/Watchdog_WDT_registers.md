# Watchdog （WDT） - Register Description

## Watchdog （WDT）

### Register Description

#### Register Memory map
Table 5-1 Memory Map of WDT

**Table:** (7 rows, 6 cols)

| Name | Address Offset | Width | Access | Reset value | Description |
|---|---|---|---|---|---|
| WDT_CR | 0x00 | 5 | R/W | 5’h2 | WDT control register |
| WDT_time_out | 0x04 | 8 | R/W | 8’h0 | WDT timeout range register |
| WDT_current_value | 0x08 | 32 | R | 32’hffff | WDT current counter value register |
| WDT_restart | 0x0c | 8 | W | 8’h0 | WDT counter restart register |
| WDT_int_status | 0x10 | 1 | R | 1’b0 | WDT interrupt status register |
| WDT_int_clr | 0x14 | 1 | R | 1’b0 | WDT interrupt clear register |


#### Register Field Description
Control Register (WDT_CR)
Address Offset: 0x00
Read/write access: read/write
Table 5-2 WDT_CR Field Description

**Table:** (5 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:5 | N/A | Reserved and read as zero (0). | Reserved and read as zero (0). |
| 4:2 | RPL | R/W | This is used to select the number of pclk cycles for which the system reset stays asserted. The range of values available is 2 to 256 pclk cycles.  000 – 2 pclk cycles 001 – 4 pclk cycles 010 – 8 pclk cycles 011 – 16 pclk cycles 100 – 32 pclk cycles 101 – 64 pclk cycles 110 – 128 pclk cycles 111 – 256 pclk cycles Reset Value: 3’b0. |
| 1 | RMOD | R/W | 0 = Generate a system reset. 1 = First generate an interrupt and if it is not cleared by the time a second timeout occurs then generate a system reset. Reset Value: 1 |
| 0 | WDT_EN | R/W | Once this bit has been enabled, it can only be cleared by a system reset. 0 = WDT disabled. 1 = WDT enabled. Reset Value: 0 |

Timeout Range Register (WDT_time_out)
Address Offset: 0x04
Read/write access: read/write
Table 5-3 WDT_time_out Field Description

**Table:** (4 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:8 | Reserved and read as zero (0). | Reserved and read as zero (0). | Reserved and read as zero (0). |
| 7:4 | TOP_INIT | R/W | Used to select the timeout period that the watchdog counter restarts from for the first counter restart(kick). This register should be written after reset and before the WDT is enabled.  A change of the TOP_INIT is seen only once the WDT has been enabled, and any change after the first kick is not seen as subsequent kicks use the period specified by the TOP bits. The range of values available for a 32-bit watchdog counter are: Where i=TOP_INIT and t=timeout period  for i = 0 to 15, t=32’hffff Reset Value: 4’h0 |
| 3:0 | TOP | R/W | Timeout period. This field is used to select the timeout period from which the watchdog counter restarts. A change of the timeout period takes effect only after the next counter restart (kick). The range of values available are: where i = TOP and t = timeout period for i = 0 to 15, t =32’hffff Reset Value: 4’h0 |

Current Counter Value Register (WDT_current_value)
Address Offset: 0x08
Read/write access: read
Table 5-4 WDT_current_value Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Current Counter Value Register | R | This register, when read, is the current value of the internal counter. This value is read coherently whenever it is read.  Reset Value: 32’hffff |

Counter Restart Register (WDT_CRR)
Address Offset: 0x0c
Read/write access: write
Table 5-5 WDT_CRR Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:8 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 7:0 | Counter Restart Register | W | This register is used to restart the WDT counter. As a safety feature to prevent accidental restarts, the value 0x76 must be written. A restart also clears the WDT interrupt. Reading this register returns zero. Reset Value: 0 |

Interrupt Status Register (WDT_int_status)
Address Offset: 0x10
Read/write access: read
Table 5-6 WDT_STAT Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 0 | Interrupt Status Register | R | This register shows the interrupt status of the WDT. 1 = Interrupt is active regardless of polarity. 0 = Interrupt is inactive.  Reset Value: 0 |

Interrupt Clear Register (WDT_int_clr)
Address Offset: 0x14
Read/write access: read
Table 5-7 WDT_int_clr Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 0 | Interrupt Clear Register | R | Clears the watchdog interrupt. This can be used to clear the interrupt without restarting the watchdog counter. Reset Value: 0 |


### Work Flow
This section describes the configuration of the WDR for a simple use. The special configuration should refer to the WDT register description.
Configure the WDT for use:
Disable the WDT by writing watchdog control register WDT_CR.
Configure the WDT_time_out.
Enable the WDT by writing the WDT_CR.
