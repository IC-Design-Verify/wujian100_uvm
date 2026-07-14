# General-purpose I/O （GPIO） - Register Description

## General-purpose I/O （GPIO）

### Register description

#### Register Memory Map
Table 8-1 Address Map of GPIO Registers

**Table:** (12 rows, 6 cols)

| Name | Description | Offset | Initial  Value | R/W | Access Size/bit |
|---|---|---|---|---|---|
| gpio_output_data | GPIO output data register | 0x00 | 0x0 | R/W | 32 |
| gpio_direction | GPIO direction register | 0x04 | 0x0 | R/W | 32 |
| gpio_ctl | GPIO control register | 0x08 | 0x0 | R/W | 32 |
| gpio_inten | Interrupt enable register | 0x30 | 0x0 | R/W | 32 |
| gpio_intmask | Interrupt mask register | 0x34 | 0x0 | R/W | 32 |
| gpio_inttype_level | Interrupt level register | 0x38 | 0x0 | R/W | 32 |
| gpio_int_polarity | Interrupt polarity register | 0x3c | 0x0 | R/W | 32 |
| gpio_intstatus | Interrupt status of GPIO | 0x40 | 0x0 | R | 32 |
| gpio_rawintstatus | Raw interrupt status of GPIO (premasking) | 0x44 | 0x0 | R | 32 |
| gpio_porta_int_clr | GPIO clear interrupt register | 0x4c | 0x0 | W | 32 |
| gpio_input_data | GPIO input data register | 0x50 | 0x0 | R | 32 |


#### Registers Filed Description
Gpio_output_data:
Name: GPIO Data Register
Size: 32 bits
Address Offset: 0x00
Read/write access: read/write
Table 8-2 gpio_output_data Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | GPIO Data Register | Values written to this register are output on the I/O signals. For GPIO if the corresponding data direction bits for GPIO are set to Output mode and the corresponding control bit for GPIO is set to Software mode. The value read back is equal to the last value written to this register. | 32’b0 |

gpio_direction:
Name: GPIO Data Direction Register
Size: 32 bits
Address Offset: 0x04
Read/write access: read/write
Table 8-3 gpio_direction Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | GPIO Data Direction Register | Values written to this register independently control the direction of the corresponding data bit in GPIO.  0 – Input 1 – Output for each bit | 32’b0 |

gpio_ctl:
Name: GPIO data source register
Size: 32 bits
Address Offset: 0x08
Read/write access: read/write
Table 8-4 gpio_ctl Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | GPIO Data  Source  Register | The data and control source for a signal can come from either software or hardware; this bit selects between them.  0-Software mode for each bit 1-Hardware mode for each bit Reset Value: 28’h0 | 32’b0 |

gpio_inten:
Name: Interrupt enable
Size: 32 bits
Address Offset: 0x30
Read/write access: read/write
Table 8-5 gpio_inten Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Interrupt enable | Allows each bit of GPIO to be configured for interrupts. By default the generation of interrupts is disabled. Whenever a 1 is written to a bit of this register, it configures the corresponding bit on GPIO to become an interrupt; otherwise, GPIO operates as a normal GPIO signal. Interrupts are disabled on the corresponding bits of GPIO if the corresponding data direction register is set to Output or if GPIO mode is set to Hardware.  0 – Configure GPIO bit as normal GPIO signal  1 – Configure GPIO bit as interrupt | 32’b0 |

gpio_intmask:
Name: Interrupt mask
Size: 32 bits
Address Offset: 0x34
Read/write access: read/write
Table 8-6 gpio_intmask Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Interrupt mask | Controls whether an interrupt on GPIO can create an interrupt for the interrupt controller by not masking it. By default, all interrupts bits are unmasked. Whenever a 1 is written to a bit in this register, it masks the interrupt generation capability for this signal; otherwise interrupts are allowed through. The unmasked status can be read as well as the resultant status after masking.  0 – Interrupt bits are unmasked (default) 1 – Mask interrupt | 32’b0 |

gpio_inttype_level
Name: Interrupt level
Size: 32 bits
Address Offset: 0x38
Read/write access: read/write
Table 8-7 gpio_inttype_level Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Interrupt level | Controls the type of interrupt that can occur on GPIO.  Whenever a 0 is written to a bit of this register, it configures the interrupt type to be level-sensitive; otherwise, it is edge-sensitive.  0 – Level-sensitive (default) 1 – Edge-sensitive  Reset Value:0x0 | 32’b0 |

gpio_int_polarity:
Name: Interrupt polarity
Size: 32 bits
Address Offset: 0x3c
Read/write access: read/write
Table 8-8 gpio_int_polarity Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Interrupt polarity | Controls the polarity of edge or level sensitivity that can occur on input of GPIO. Whenever a 0 is written to a bit of this register, it configures the interrupt type to falling-edge or active-low sensitive; otherwise, it is rising-edge or active-high sensitive.  0 – Active-low (default) 1 – Active-high | 32’b0 |

gpio_intstatus:
Name: Interrupt status
Size: 32 bits
Address Offset: 0x40
Read/write access: read only
Table 8-9 gpio_intstatus Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Interrupt status | Interrupt status of GPIO | 32’b0 |

gpio_rawintstatus:
Name: Raw interrupt status
Size: 32 bits
Address Offset: 0x44
Read/write access: read only
Table 8-10 gpio_rawintstatus Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Raw interrupt status | Raw interrupt of status of GPIO (pre-masking bits) | 32’b0 |

gpio_int_clr:
Name: Clear interrupt
Size: 32 bits
Address Offset: 0x4c
Read/write access: write only
Table 8-11 gpio_int_clr Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | Clear interrupt | Controls the clearing of edge type interrupts from GPIO. When a 1 is written into a corresponding bit of this register, the interrupt is cleared. All interrupts are cleared when GPIO is not configured for interrupts.  0 – No interrupt clear (default) 1 – Clear interrupt | 32’b0 |

gpio_input_data:
Name: External GPIO
Size: 32 bits
Address Offset: 0x50
Read/write access: read only
Table 8-12 gpio_input_data Field Description

**Table:** (2 rows, 4 cols)

| Bits | Field Name | Description | Initial value |
|---|---|---|---|
| 31:0 | External GPIO | When GPIO is configured as Input, then reading this location reads the values on the signal. When the data direction of GPIO is set as Output, reading this location reads the data register for GPIO. | 32’b0 |


#### Work Flow
Configure GPIO output：
Set GPIO. gpio_swporta_ctl. gpio_swporta_ctl[0] = 1’b1, configure GPIO[0] use software mode
Set GPIO. gpio_swporta_ddr. gpio_swporta_ddr[0] = 1’b1, configure GPIO[0] direct is input
Set GPIO. gpio_swporta_dr. gpio_swporta_dr[0] = 1’b1, write data 1’b1 to gpio
Monitor PAD GPIO[0] == 1
Configure GPIO input:
Set GPIO. gpio_swporta_ctl. gpio_swporta_ctl[0] = 1’b1, configure GPIO[0] use software mode
Set GPIO. gpio_swporta_ddr. gpio_swporta_ddr[1] = 1’b0, configure GPIO[0] direct is output
Connect PAD GPIO[0] to high level
Read GPIO. GPIO external port register. GPIO external port register[0] == 1. Check GPIO[0] input data = 1
