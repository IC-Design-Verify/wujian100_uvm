# Real-Time Clock （RTC） - Register Description

## Real-Time Clock （RTC）

### Register Description

#### Register Memory Map
The following table descripts the register memory map.
Table 7-1 RTC Memory Map

**Table:** (10 rows, 6 cols)

| Name | Address offset | Width | Access | Reset Value | Description |
|---|---|---|---|---|---|
| RTC_current_value | 0x00 | 32 | R | 0x0 | Current Counter Value Register |
| RTC_match_value | 0x04 | 32 | RW | 0x0 | Counter Match Register |
| RTC_load_value | 0x08 | 32 | RW | 0x0 | Counter Load Register |
| RTC_CCR | 0x0C | 4~2bits | RW | 0x0 | Counter Control Register |
| RTC_int_status | 0x10 | 32 | R | 0x0 | Interrupt Status Register |
| RTC_raw_int_status | 0x14 | 32 | R | 0x0 | Interrupt Raw Status Register |
| RTC_int_clr | 0x18 | 32 | R | 0x0 | End of Interrupt Register |
| RTC_COMP_VERSION | 0x1C | 32 | R | 0x0 | Component Version Register |
| RTC_DIV | 0x20 | 20 | RW | 0x4000 | Rtc clock divider value |


#### Registers Field Description
Current Counter Value Register (RTC_current_value)
Address Offset: 0x00
Read/write access: read-only
Table 7-2 RTC_current_value Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Current Counter Value | R | When read, this register is the current value of the internal counter. This value always is read coherently.  Reset Value: 0x0 |

Counter Match Register (RTC_match_value)
Address Offset: 0x04
Read/write access: read/write
Table 7-3 RTC_match_value Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Counter Match | R/W | Interrupt Match Register. When the internal counter matches this register, an interrupt is generated, provided interrupt generation is enabled. When appropriate, this value is written coherently. Only when all the bytes are written is the register used by the interrupt detection logic.  Reset Value: 0x0 |

Counter Load Register (RTC_load_value)
Address Offset: 0x08
Read/write access: read/write
Table 7-4 RTC_CLR Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Counter Load | R/W | Loaded into the counter as the loaded value, which is written coherently.  Reset Value: 0x0 |

Counter Control Register (RTC_CCR)
Address Offset: 0x0C
Read/write access: read/write
Table 7-5 RTC_CCR Fiesld Description

**Table:** (6 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:4 | N/A | N/A | Reserved and read as 0 |
| 3 | rtc_wen | R/W | Allows the user to force the counter to wrap when a match occurs instead of waiting until the maximum count is reached. 0 = Wrap disabled 1 = Wrap enabled Reset Value: 0x0 |
| 2 | Rtc_en | R/W | Allows the user to control counting in the counter. 0 = Counter disabled 1 = Counter enabled Reset Value: 0x0 |
| 1 | rtc_mask | R/W | Allows the user to mask interrupt generation. 0 = Interrupt unmasked 1 = Interrupt masked Reset Value: 0x0 |
| 0 | rtc_ien | R/W | Allows the user to disable interrupt generation. 0 = Interrupt disabled 1 = Interrupt enabled Reset Value: 0x0 |

Interrupt Status Register (RTC_int_status)
Address Offset: 0x10
Read/write access: read-only
Table 7-6 RTC_int_status Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | N/A | N/A | Reserved and read as 0 |
| 0 | rtc_stat | R | This register is the masked raw status 0 = Interrupt is inactive 1 = Interrupt is active (regardless of polarity) Reset Value: 0x0 |

Interrupt Raw Status Register (RTC_Rawint_status)
Address Offset: 0x14
Read/write access: read-only
Table 7-7 RTC_Rawint_status Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | N/A | N/A | Reserved and read as 0 |
| 0 | rtc_rstat | R | 0 = Interrupt is inactive 1 = Interrupt is active (regardless of polarity) Reset Value: 0x0 |

End of Interrupt Register (RTC_int_clr)
Address Offset: 0x18
Read/write access: read-only
Table 7-8 RTC_int_clr Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | N/A | N/A | Reserved and read as 0 |
| 0 | rtc_int_clr | R | By reading this location, the match interrupt is cleared. Performing read-to-clear on interrupts, the interrupt is cleared at the end of the read. Reset Value: 0x0 |

CLK DIV Register (RTC_DIV)
Address Offset: 0x20
Read/write access: read/write
Table 7-9 RTC_DIV Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | N/A | N/A | Reserved and read as 0 |
| 19:0 | rtc_div | R/W | Rtc clcok divider base on this register value Reset Value: 0x4000 |


### Work Flow
This section describe the configuration of the RTC for a simple use. The special configuration should refer to the RTC register description.
Configuration the RTC for use:
Set the control register (RTC_CCR) to reset value to disable the RTC first.
Set the RTC counter match register (RTC_match_value).
Set the RTC counter load register (RTC_load_value).
Set the RTC counter control register to the corresponding value (RTC_CCR).
