# System Overview - Register Description

## System Overview

### Functional Features
CPU Part
32-bit general purpose CPU E902
CoreTim
The circulating decrement counter is 24-bit count width
VIC
Supports 64 Interrupts nesting
Each interrupt has corresponding priority
Memory
64KB ISRAM and 3×64KB DSRAM on-chip SRAM
Characteristics of Peripherals
DMAC(x1)
16 channels
Supports block/group trigger mode transactions
TIM (×8)
connects to APB1 and APB0
All of timers are 32-bit count width
All of timers support for two operation modes: free-running and user-defined count
GPIO (×1)
Connects to APB
GPIO supports 32-bit width;
Each bit of GPIO supports interrupt generation
USI (×3)
Universal Asynchronous Receiver/Transmitter(UART)
Inter-integrated Circuit(I2C)
Serial Peripheral Interface(SPI)
RTC
RTC connects to APB
32-bit count width
Incrementing counter and comparator for interrupt generation
PWM
Connects to APB
12 input/output channels
6 PWM generators, each with 1 32-bit counter, 2 PWM comparator, 1 PWM signal generator, 1 interrupt generator
Each PWM signal generator contains 2 channels
PWM output enable or disable of each PWM signal
Optional output inversion of each PWM signal (polarity control)
6 32-bit counter and each has the following characteristics:
Up or Up/Down mode
Output frequency controlled by a 16-bit load value
32-bit input capture modes
Input edge count mode
Input edge time mode
WDT
One WDT in APB
Each WDT has 32 count width
Counter counts down from a pre-set value to zero to indicate the occurrence of a timeout

### Address Map
For this chip, the memory address is divided into several parts: on-chip memory, off-chip memory, TCIP and peripherals. The detailed address map is shown as below.

**Table:** (8 rows, 3 cols)

| Address Range | Size | Usage |
|---|---|---|
| 0x0000_0000~0x0000_7FFF | 64KB | Internal SRAM（inst） |
| 0x2000_0000~0x2002_FFFF | 192KB | Internal SRAM(data) |
| 0x4000_0000~0x401F_FFFF | 2MB | MAIN BUS Peripherals |
| 0x4020_0000~0x0x7FFF_FFFF | 1024M-2M | Low speed Peripherals |
| 0x8000_0000~0x9FFF_FFFF | 768M | MAIN BUS Peripherals |
| 0xE000_E000~0xE000_EFFF | 4KB | TCIP |
| Other | - | Reserved |

Table 1-1Memory Address Map

#### Peripheral Address Map
The register blocks for all on-chip peripheral devices are located on 4KB boundaries. Those modules that may require additional address space are assigned additional 4KB blocks. Within a 4KB block, peripheral registers may be incompletely decoded. The description of each peripheral will define the result of accesses to unimplemented registers. For registers that do not implement all 32 bits, the unimplemented bits will return zero when read, and write to the unimplemented bits will have no effect. In general, unimplemented bits should return zero to ensure future compatibility.
Table 1-2Peripheral Address Map

**Table:** (60 rows, 6 cols)

| Address Range | IP name | Size | Master/Slave | Master/Slave | Description |
|---|---|---|---|---|---|
|  | E902 |  | M0/M2 | M0/M2 | Core |
|  | DMA |  | M3 | M3 | DMA |
|  | MDummy0 |  | M4 | M4 | main_mdummy_top0 |
|  | MDummy1 |  | M5 | M5 | main_mdummy_top1 |
|  | MDummy2 |  | M6 | M6 | main_mdummy_top2 |
| 0x0000_0000~0x0000_27FF | ISRAM | 64K | S0 | S0 | ROM |
| 0x1000_0000~0x1007_FFFF | MemDummy | 512K | S1 | S1 | instmem_dummy top0 |
| 0x2000_0000~0x2000_FFFF | SRAM | 64K | S2 | S2 | DATA SRAM |
| 0x2001_0000~0x2001_FFFF | SRAM | 64K | S3 | S3 | DATA SRAM |
| 0x2002_0000~0x2002_FFFF | SRAM | 64K | S4 | S4 | DATA SRAM |
| 0x3000_0000~0x3007_FFFF | MemDummy | 512K | S5 | S5 | datamem_dummy_top1 |
| 0x4000_0000~0x4000_3FFF | DMA | 16K | S6 | S6 | DMA Controller |
| 0x4001_0000~0x4001_FFFF | Dummy | 64K | S7 | S7 | main_dummy_top0 |
| 0x4002_0000~0x4002_FFFF | Dummy | 64K | S8 | S8 | main_dummy_top1 |
| 0x4010_0000~0x401F_FFFF | Dummy | 1M | S9 | S9 | main_dummy_top2 |
| 0x4020_0000~0x7FFF_FFFF | LSBUS | 1024M-2M | S10 | S10 | AHB LS BUS |
| 0x8000_0000~0x9FFF_FFFF | Dummy | 512M | S11 | S11 | main_dummy_top3 |
| Other | REV | - | REV | REV | Reserved |
| AHB LS BUS | AHB LS BUS | AHB LS BUS | AHB LS BUS | AHB LS BUS | AHB LS BUS |
| 0x4020_0000~0x4020_0FFF | Dummy | 4K | S0 | S0 | lsbus_dummy_top0 |
| 0x4030_0000~0x403F_FFFF | Dummy | 1M | S1 | S1 | lsbus_dummy_top1 |
| 0x5000_0000~0x5004_FFFF | APB0 | 320K | S2 | S2 | APB0 |
| 0x6000_0000~0x6004_FFFF | APB1 | 320K | S3 | S3 | APB1 |
| 0x7000_0000~0x77FF_FFFF | Dummy | 128M | S4 | S4 | lsbus_dummy_top2 |
| 0x7800_0000~0x7FFF_FFFF | Dummy | 128M | S5 | S5 | lsbus_dummy_top3 |
| APB0 | APB0 | APB0 | APB0 | APB0 | APB0 |
| 0x5000_0000~0x5000_03FF | TIM0 | 1K | 1K | P0 | Timer 0 |
| 0x5000_0400~0x5000_07FF | TIM2 | 1K | 1K | P1 | Timer 2 |
| 0x5000_0800~0x5000_0BFF | TIM4 | 1K | 1K | P2 | Timer 4 |
| 0x5000_0C00~0x5000_0FFF | TIM6 | 1K | 1K | P3 | Timer 6 |
| 0x5002_8000~0x5002_8FFF | USI0 | 16K | 16K | P4 | USI0 |
| 0x5002_9000~0x5002_9FFF | USI2 | 16K | 16K | P5 | USI2 |
| 0x5000_4000~0x5000_7FFF | Dummy | 16K | 16K | P6 | apb0_dummy_top1 |
| 0x5000_8000~0x5000_BFFF | WDT | 16K | 16K | P7 | WDT |
| 0x5000_C000~0x5000_FFFF | Dummy | 16K | 16K | P8 | apb0_dummy_top2 |
| 0x5001_0000~0x5001_3FFF | Dummy | 16K | 16K | P9 | apb0_dummy_top3 |
| 0x5001_4000~0x5001_7FFF | Dummy | 16K | 16K | P10 | apb0_dummy_top4 |
| 0x5001_8000~0x5001_BFFF | Dummy | 16K | 16K | P11 | apb0_dummy_top5 |
| 0x5001_C000~0x5001_FFFF | PWM | 16K | 16K | P12 | PWM |
| 0x5002_0000~0x5002_3FFF | Dummy | 16K | 16K | P13 | apb0_dummy_top7 |
| 0x5002_4000~0x5002_7FFF | Dummy | 16K | 16K | P14 | apb0_dummy_top8 |
| 0x5003_0000~0x5003_3FFF | Dummy | 16K | 16K | P15 | apb0_dummy_top9 |
| APB1 | APB1 | APB1 | APB1 | APB1 | APB1 |
| 0x6000_0000~0x6000_03FF | TIM1 | 1K | 1K | P0 | Timer 1 |
| 0x6000_0400~0x6000_07FF | TIM3 | 1K | 1K | P1 | Timer 3 |
| 0x6000_0800~0x6000_0BFF | TIM5 | 1K | 1K | P2 | Timer 5 |
| 0x6000_0C00~0x6000_0FFF | TIM7 | 1K | 1K | P3 | Timer 7 |
| 0x6002_8000~0x6002_BFFF | USI1 | 16K | 16K | P4 | USI1 |
| 0x6001_8000~0x6001_BFFF | GPIO | 16K | 16K | P5 | GPIO |
| 0x6000_4000~0x6000_7FFF | RTC | 16K | 16K | P6 | RTC |
| 0x6000_8000~0x6000_BFFF | Dummy | 16K | 16K | P7 | apb1_dummy_top1 |
| 0x6000_C000~0x6000_FFFF | Dummy | 16K | 16K | P8 | apb1_dummy_top2 |
| 0x6001_0000~0x6001_3FFF | Dummy | 16K | 16K | P9 | apb1_dummy_top3 |
| 0x6001_4000~0x6001_7FFF | Dummy | 16K | 16K | P10 | apb1_dummy_top4 |
| 0x6001_C000~0x6001_FFFF | Dummy | 16K | 16K | P11 | apb1_dummy_top5 |
| 0x6002_0000~0x6002_3FFF | Dummy | 16K | 16K | P12 | apb1_dummy_top6 |
| 0x6002_4000~0x6002_7FFF | Dummy | 16K | 16K | P13 | apb1_dummy_top7 |
| 0x6002_C000~0x6002_FFFF | Dummy | 16K | 16K | P14 | apb1_dummy_top8 |
| 0x6003_0000~0x6003_3FFF | PMU | 16K | 16K | P15 | Power Management Unit(dummy) |


### PAD I/O
Each of IO can be shared by less than 4 peripherals at one time. The details of AFIO function as follows.
Table 1-3PIN List

**Table:** (64 rows, 5 cols)

| Num | Pin name | I/O | Width | Description |
|---|---|---|---|---|
| 1 | PIN_EHS | I | 1 | External high speed osc clock |
| 2 | POUT_EHS | O | 1 | External high speed osc clock |
| 3 | PIN_ELS | I | 1 | External low speed osc clock |
| 4 | POUT_ELS | O | 1 | External low speed osc clock |
| 5 | PAD_MCURST | I | 1 | PAD system reset，active low |
| 6 | PAD_JTAG_TCLK | I | 1 | CPU JTAG TCLK |
| 7 | PAD_JTAG_TMS | I/O | 1 | CPU JTAG TMS |
| 8 | PAD_GPIO_0 | I/O | 1 | GPIO0_0 signal |
| 9 | PAD_GPIO_1 | I/O | 1 | GPIO0_1 signal |
| 10 | PAD_GPIO_2 | I/O | 1 | GPIO0_2 signal |
| 11 | PAD_GPIO_3 | I/O | 1 | GPIO0_3 signal |
| 12 | PAD_GPIO_4 | I/O | 1 | GPIO0_4 signal |
| 13 | PAD_GPIO_5 | I/O | 1 | GPIO0_5 signal |
| 14 | PAD_GPIO_6 | I/O | 1 | GPIO0_6 signal |
| 15 | PAD_GPIO_7 | I/O | 1 | GPIO0_7 signal |
| 16 | PAD_GPIO_8 | I/O | 1 | GPIO0_8 signal |
| 17 | PAD_GPIO_9 | I/O | 1 | GPIO0_9 signal |
| 18 | PAD_GPIO_10 | I/O | 1 | GPIO0_10 signal |
| 19 | PAD_GPIO_11 | I/O | 1 | GPIO0_11 signal |
| 20 | PAD_GPIO_12 | I/O | 1 | GPIO0_12 signal |
| 21 | PAD_GPIO_13 | I/O | 1 | GPIO0_13 signal |
| 22 | PAD_GPIO_14 | I/O | 1 | GPIO0_14 signal |
| 23 | PAD_GPIO_15 | I/O | 1 | GPIO0_15 signal |
| 24 | PAD_GPIO_16 | I/O | 1 | GPIO0_16 signal |
| 25 | PAD_GPIO_17 | I/O | 1 | GPIO0_17 signal |
| 26 | PAD_GPIO_18 | I/O | 1 | GPIO0_18 signal |
| 27 | PAD_GPIO_19 | I/O | 1 | GPIO0_19 signal |
| 28 | PAD_GPIO_20 | I/O | 1 | GPIO0_20 signal |
| 29 | PAD_GPIO_21 | I/O | 1 | GPIO0_21 signal |
| 30 | PAD_GPIO_22 | I/O | 1 | GPIO0_22 signal |
| 31 | PAD_GPIO_23 | I/O | 1 | GPIO0_23 signal |
| 32 | PAD_GPIO_24 | I/O | 1 | GPIO0_24 signal |
| 33 | PAD_GPIO_25 | I/O | 1 | GPIO0_25 signal |
| 34 | PAD_GPIO_26 | I/O | 1 | GPIO0_26 signal |
| 35 | PAD_GPIO_27 | I/O | 1 | GPIO0_27 signal |
| 36 | PAD_GPIO_28 | I/O | 1 | GPIO0_28 signal |
| 37 | PAD_GPIO_29 | I/O | 1 | GPIO0_29 signal |
| 38 | PAD_GPIO_30 | I/O | 1 | GPIO0_30 signal |
| 39 | PAD_GPIO_31 | I/O | 1 | GPIO0_31 signal |
| 40 | PAD_PWM_CH0 | I/O | 1 | PWM channel0 signal |
| 41 | PAD_PWM_CH1 | I/O | 1 | PWM channel1 signal |
| 42 | PAD_PWM_CH2 | I/O | 1 | PWM channel2 signal |
| 43 | PAD_PWM_CH3 | I/O | 1 | PWM channel3 signal |
| 44 | PAD_PWM_CH4 | I/O | 1 | PWM channel4 signal |
| 45 | PAD_PWM_CH5 | I/O | 1 | PWM channel5 signal |
| 46 | PAD_PWM_CH6 | I/O | 1 | PWM channel6 signal |
| 47 | PAD_PWM_CH7 | I/O | 1 | PWM channel7 signal |
| 48 | PAD_PWM_CH8 | I/O | 1 | PWM channel8 signal |
| 49 | PAD_PWM_CH9 | I/O | 1 | PWM channel9 signal |
| 50 | PAD_PWM_CH10 | I/O | 1 | PWM channel10 signal |
| 51 | PAD_PWM_CH11 | I/O | 1 | PWM channel11 signal |
| 52 | PAD_USI0_NSS | I/O | 1 | USI0 Slave select |
| 53 | PAD_USI0_SCLK | I/O | 1 | USI0 Serial clock |
| 54 | PAD_USI0_SD0 | I/O | 1 | USI0 Serial data0 |
| 55 | PAD_USI0_SD1 | I/O | 1 | USI0 Serial data1 |
| 56 | PAD_USI1_NSS | I/O | 1 | USI1 Slave select |
| 57 | PAD_USI1_SCLK | I/O | 1 | USI1 Serial clock |
| 58 | PAD_USI1_SD0 | I/O | 1 | USI1 Serial data0 |
| 59 | PAD_USI1_SD1 | I/O | 1 | USI1 Serial data1 |
| 60 | PAD_USI2_NSS | I/O | 1 | USI2 Slave select |
| 61 | PAD_USI2_SCLK | I/O | 1 | USI2 Serial clock |
| 62 | PAD_USI2_SD0 | I/O | 1 | US2 Serial data0 |
| 63 | PAD_USI2_SD1 | I/O | 1 | US2 Serial data1 |


### Interrupt source
Table 1-4 Interrupt source

**Table:** (44 rows, 2 cols)

| Number | Interrupt Source |
|---|---|
| 6:0 | REV |
| 1 | CoreTim |
| 15:8 | REV |
| 16 | GPIO0 |
| 18:17 | TIM0[1:0] |
| 20:19 | TIM1[1:0] |
| 22:21 | TIM2[1:0] |
| 24:23 | TIM3[1:0] |
| 25 | PWM |
| 26 | RTC |
| 27 | WDT |
| 28 | USI0 |
| 29 | USI1 |
| 30 | USI2 |
| 31 | PMU |
| 32 | DMAC0 |
| 34:33 | TIM4[1:0] |
| 36:35 | TIM5[1:0] |
| 38:37 | TIM6[1:0] |
| 40:39 | TIM7[1:0] |
| 41 | IMEMDUMMY0 | DMEMDUMMY0 |
| 42 | MAIN_DUMMY0 |
| 43 | MAIN_DUMMY1 |
| 44 | MAIN_DUMMY2 |
| 45 | MAIN_DUMMY3 |
| 46 | LSBUS_DUMMY0 |
| 47 | LSBUS_DUMMY1 |
| 48 | LSBUS_DUMMY2 |
| 49 | LSBUS_DUMMY3 |
| 50 | APB0_DUMMY1 |
| 51 | APB0_DUMMY2 |
| 52 | APB0_DUMMY3 |
| 53 | APB0_DUMMY4 |
| 54 | APB0_DUMMY5 |
| 55 | APB0_DUMMY7 |
| 56 | APB0_DUMMY8 |
| 57 | APB0_DUMMY9 |
| 58 | APB1_DUMMY1 |
| 59 | APB1_DUMMY2 |
| 60 | APB1_DUMMY3 |
| 61 | APB1_DUMMY4 |
| 62 | APB1_DUMMY5 | APB1_DUMMY6 |
| 63 | APB1_DUMMY7 | APB1_DUMMY8 |

