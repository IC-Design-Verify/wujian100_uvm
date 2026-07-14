# Timer （TIM） - Register Description

## Timer （TIM）

### Register Description
The following tables describe the memory map of the register in the Timers.

#### Register Memory map
In each TIMER IP，two timers are provided:Timer1 and Timer2.Following table describes the registers address map of Timer1 and the Timer2.
Table 2-2 Timers Memory Map

**Table:** (11 rows, 6 cols)

| Name | Address offset | Width | Access | Reset Value | Description |
|---|---|---|---|---|---|
| Timer1LoadCount | 0x00 | 32 | R/W | 32’b0 | Value to be loaded into Timer1. |
| Timer1CurrentValue | 0x04 | 32 | R | 32’b0 | Current Value of Timer1. |
| Timer1Control Reg | 0x08 | 4 | R/W | 4’b0 | Control Register for Timer1. |
| Timer1_int_clr | 0x0C | 1 | R | 1’b0 | Clears the interrupt from Timer1. |
| Timer1Int Status | 0x10 | 1 | R | 1’b0 | Contains the interrupt status for Timer1. |
| Timer2LoadCount | 0x14 | 32 | R/W | 32’b0 | Value to be loaded into Timer2. |
| Timer2CurrentValue | 0x18 | 32 | R | 32’b0 | Current Value of Timer2 |
| Timer2Control Reg | 0x1c | 4 | R/W | 4’b0 | Control Register for Timer2 |
| Timer2_int_clr | 0x20 | 1 | R | 1’b0 | Clears the interrupt from Timer2 |
| Timer2Int Status | 0x24 | 1 | R | 1’b0 | Contains the interrupt status for Timer2 |


#### Register Field Description
The following section mainly describes the Timer1 register field and the Timers system register field. Timer2 register field description just replicates that of Timer1 using the address offset specified in above table.
Timer1 Load Count
Address Offset: 0x00
Read/write access: read/write
Table 2-3 Timer1LoadCount Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Timer1 Load Count Register | R/W | Value to be loaded into Timer1. This is the value from which counting commences. Any value written to this register is loaded into the associated timer.  Reset Value: 32’b0 |

Timer1 Current Value
Address Offset: 0x04
Read/write access: read
Table 2-4 Timer1CurrentValue Register Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Timer1 Current Value Register | R | Current Value of Timer1.  Reset Value: 32’b0 |

Timer1 Control Reg
Address Offset: 0x08
Read/write access: read/write
Table 2-5 Timer1Control Register Field Description

**Table:** (7 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:5 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 4 | Timer Hardware Trigger enable | R/W | 0 (timer hardware trigger disable) 1 (timer hardware trigger enable) |
| 3 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 2 | Timer Interrupt Mask | R/W | 0 (timer interrupt not masked) 1 (timer interrupt masked) Reset Value: 1’b0 |
| 1 | Timer Mode Select | R/W | 0 (free-running) 1 (user-defined running) Reset Value: 1’b0 |
| 0 | Timer Enable Select | R/W | 0 (disabled) 1 (enabled) Reset Value: 1’b0 |

Timer1_int_clr
Address Offset: 0x0c
Read/write access: read
Table 2-6 Timer1 int clr  Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 0 | Timer1 int clr Register | R | Reading from this register returns all zeros(0) and clears the interrupt from Timer1 |

Timer1Int Status
Address Offset: 0x10
Read/write access: read
Table 2-7 Timer1Int Status Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 0 | Timer1Int Status Register | R | Contains the interrupt status for Timer1 |

Timer2 Load Count
Address Offset: 0x14
Read/write access: read/write
Table 2-8 Timer2LoadCount Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Timer2 Load Count Register | R/W | Value to be loaded into Timer2. This is the value from which counting commences. Any value written to this register is loaded into the associated timer.  Reset Value: 32’b0 |

Timer2 Current Value
Address Offset: 0x18
Read/write access: read
Table 2-9 Timer2CurrentValue Register Field Description

**Table:** (2 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:0 | Timer2 Current Value Register | R | Current Value of Timer2 Reset Value: 32’b0 |

Timer2 Control Reg
Address Offset: 0x1c
Read/write access: read/write
Table 2-10 Timer2 Control Register Field Description

**Table:** (7 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:5 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 4 | Timer Hardware Trigger enable | R/W | 0 (timer hardware trigger disable) 1 (timer hardware trigger enable) |
| 3 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 2 | Timer Interrupt Mask | R/W | 0 (timer interrupt not masked) 1 (timer interrupt masked) Reset Value: 1’b0 |
| 1 | Timer Mode Select | R/W | 0 (free-running) 1 (user-defined running) Reset Value: 1’b0 |
| 0 | Timer Enable Select | R/W | 0 (disabled) 1 (enabled) Reset Value: 1’b0 |

Timer2 int clr
Address Offset: 0x20
Read/write access: read
Table 2-11 Timer2 int clr Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 0 | Timer2 int clr Register | R | Reading from this register returns all zeros(0) and clears the interrupt from Timer2 |

Timer2Int Status
Address Offset: 0x24
Read/write access: read
Table 2-12 Timer2Int Status Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:1 | Reserved, read as zero | Reserved, read as zero | Reserved, read as zero |
| 0 | Timer2 Interrupt Status Register | R | Contains the interrupt status for Timer2. Reset Value: 32’b0 |


### Work Flow
Normal program of the timer is as follows:
1. Disable the timer and program its operating mode by writing to its control register, TimerN Control Reg.
2. Load the timer’s Timer Load Count register, TimerN Load Count.
3. Enable the timer by writing the TimerN Control Reg register.
