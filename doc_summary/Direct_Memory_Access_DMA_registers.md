# Direct Memory Access （DMA） - Register Description

## Direct Memory Access （DMA）

### Overview
DMAC is compliant with AMBA2.0 AHB-lite Specification, Support max 16 channels. Every channel only supports block trigger mode.
There are mask registers to mask the interrupts and clean registers to clear the interrupts. Block transfer completion interrupt will generate after all block transfer done.

### Function Desription
16 channels, each channel has a source addr and a dest addr,data will be transfered from source addr to dest addr, both the source addr and the dest addr can be programed
Support 16 priority level ,Channel 0 has highest priority than other channels.
Support enable protect,DMA config can not be changed once enable is set to 1.
The source and dest Address have 3 types:increase, decrease , or no change
Programmable block transaction size for each channel with max 4096bytes
Support source and destination transaction with different width,8/16/32bits can be selected
Programmable enable and disable of DMA channel
The low priority channel can be interrupt by high priority channel
Data litte-endian or big-endian
Interrupt，Interrupt can be generated when:
a.Block transfer completion
b.Half Block transfer completion
c.Trigger event finish
d.Error condition
Each interrupt can be masked or cleared by registers.

### Register description

#### DMA channel base address
Include 17 base address:（16 Channel registers and 1 global registers ）
Channel 0 registers include registers(internal base offset address is:0x0)： SAR0 ，DAR0 ，CH0_CTRL_A ，CH0_CTRL_B ，CH0_INT_MASK ，CH0_INT_STATUS ， CH0_INT_CLEAR ，CH0_SOFT_REQ ，CH0_EN
Channel 1 registers include registers(internal base offset address is:0x30)：SAR1 ，DAR1 ，CH1_CTRL_A ，CH1_CTRL_B ，CH1_INT_MASK ，CH1_INT_STATUS ， CH1_INT_CLEAR ，CH1_SOFT_REQ ，CH1_EN
......
Channel n registers include registers(internal base offset address is:0x30*n)：SARn ，DARn ，CHn_CTRL_A ，CHn_CTRL_B ，CHn_INT_MASK ，CHn_INT_STATUS ， CHn_INT_CLEAR ，CHn_SOFT_REQ ，CHn_EN
......
Channel 15 registers include registers(internal base offset address is:0x2D0)：SAR15 ，DAR15 ，CH15_CTRL_A ，CH15_CTRL_B ，CH15_INT_MASK ，CH15_INT_STATUS ， CH15_INT_CLEAR ，CH15_SOFT_REQ ，CH15_EN
16 channels internal offset address is the same.
Global register (internal base offset address is :0x330):CHSR,DMACCFG

**Table:** (9 rows, 4 cols)

| Channel 0 base address | 0x0 | Channel 1 base address | 0x30 |
|---|---|---|---|
| Channel 2 base address | 0x60 | Channel 3 base address | 0x90 |
| Channel 4 base address | 0xC0 | Channel 5 base address | 0xF0 |
| Channel 6 base address | 0x120 | Channel 7 base address | 0x150 |
| Channel 8 base address | 0x180 | Channel 9 base address | 0x1B0 |
| Channel 10 base address | 0x1E0 | Channel 11 base address | 0x210 |
| Channel 12 base address | 0x240 | Channel 13 base address | 0x270 |
| Channel 14 base address | 0x2A0 | Channel 15 base address | 0x2D0 |
| Global register base address | 0x330 | - | - |


#### Address map
The following table describe the register memory map for one channel
Table 3-1 DMA Memory Map for one channel

**Table:** (10 rows, 5 cols)

| Register | Offset | Width | Describe | Reset Value |
|---|---|---|---|---|
| SARn | 0x0 | 32 bits | Channel n Source Address Register | 0x0000_0000 |
| DARn | 0x4 | 32 bits | Channel n Destination Address Register | 0x0000_0000 |
| CHn_CTRL_A | 0x8 | 32 bits | Channel n Control Register A | 0x0000_0000 |
| CHn_CTRL_B | 0xc | 32 bits | Channel n Control Register B | 0x0000_0000 |
| CHn_INT_MASK | 0x10 | 32 bits | Channel n Mask Interrupt Register | 0x0000_0000 |
| CHn_INT_STATUS | 0x14 | 32 bits | Channel n Status Interrupt Register | 0x0000_0000 |
| CHn_INT_CLEAR | 0x18 | 32 bits | Channel n Clear Interrupt Register | 0x0000_0000 |
| CHn_SOFT_REQ | 0x1c | 32 bits | Channel n Software Handshaking Request Register | 0x0000_0000 |
| CHn_EN | 0x20 | 32 bits | Channel n Enable Control Register | 0x0000_0000 |

Every channel has a base of offset address which is described in 3.3.1.
DMAC global registers base address is 0x330,these registers are the configs or
status of all 16 channels. Such as enable, busy status or pending state

**Table:** (5 rows, 5 cols)

| Register | Offset | Width | Describe | Reset Value |
|---|---|---|---|---|
| Reserved | 0x0 | - | - | - |
| Reserved | 0x4 | - | - | - |
| CHSR | 0x8 | 32 bits | Channel busy status register | 0x0000_0000 |
| DMACCFG | 0xC | 32bits | DMAC configure register | 0x0000_0000 |


#### Register field description

##### Channel n Source Address Register（SARn）
Address Offset: 0x0

**Table:** (2 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 31:0 | R/W | SARN[31:0] | Channel n Source read Address Register reset value:0x0 |


##### Channel n Destination Address Register（DARn）
Address Offset: 0x4

**Table:** (2 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 31:0 | R/W | DARN[31:0] | Channel n Destination write Address Register reset value:0x0 |


##### Channel n Control Register A（CHn_CTRL_A)
Address Offset: 0x8

**Table:** (8 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 31:24 | R/W | - | Reserved |
| 23:12 | R/W | BLOCK_TL[11:0] | Block Transfer Length writes this field before the channel is enabled in order to indicate the block Length (It’s bytes total number : BLOCK_TL+1  it’s max length is 4096 bytes) reset value:0x0 |
| 11：8 | R/W | - | Reserved |
| 7:6 | R/W | SINC[1:0] | Source Address Increment Indicates whether to increment or decrease the source address on every transfer.  00:increment 01: decrease 1x: no change reset value: 0x0 |
| 5:4 | R/W | DINC[1:0] | Destination Address Increment Indicates whether to increment or decrease the destination address on every transfer.  00: increment 01:  decrease 1x:  no change reset value: 0x0 |
| 3:2 | R/W | SRC_TR_WIDTH[1:0] | Source Transfer Width mapped to AHB bus “hsize.” 00: 8bit 01: 16bit 10: 32bit 11：reserved reset value: 0x0 |
| 1:0 | R/W | DST_TR_WIDTH[1:0] | Destination Transfer Width mapped to AHB bus “hsize.” 00: 8bit 01: 16bit 10: 32bit 11：reserved reset value: 0x0 |


##### Channel n Control Register B（CHn_CTRL_B)
Address Offset: 0xc

**Table:** (8 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 31:19 | Reserved | Reserved | Reserved |
| 18:15 | R/W | PROTCTL[3:0] | Channel n Protection Control used PROTCTL[3:0] to drive the AHB HPROT. PROTCTL[2]:controls the secure access. 1:secure access(It only support secure access) 0:normal access(It support secure/no secure access) reset value:0x0 |
| 14 | R/W | DSTDTLGC | Destination Write data Little-Big Endian change control 0: little endian  1: big endian  reset value:0x0 Detail infomation can be found Function Description |
| 13 | R/W | SRCDTLGC | Source read  data Little-Big Endian change control 0: little endian  1: big endian  reset value:0x0 Detail infomation can be found Function Description |
| 12:3 | Reserved | Reserved | Reserved |
| 2:1 | R/W | TRGTMDC[1:0] | Trigger transfer mode control register 2’b00:rev 2’b01:rev 2’b1x:Block trigger mode Reset value:0x0 These bits should be set to 2’b10 or 2’b11 |
| 0 | R/W | INT_EN | Interrupt Enable 0: all interrupt-generating sources are disabled. 1: all interrupt-generating sources are enabled. reset value: 0x0 |


##### Channel n Mask Interrupt Register（CHn_INT_MASK）
Address Offset:0x10

**Table:** (5 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 3 | R/W | masktrgetcmpfr | Mask for trigger event complete Interrupt of channel n  0: mask 1: not mask reset value: 0x0 |
| 2 | R/W | maskhtfr | Mask for transfer half complete Interrupt of channel n  0:  mask 1: not mask reset value: 0x0 |
| 1 | R/W | masktfr | Mask for transfer complete Interrupt of channel n 0:  mask 1: not mask reset value: 0x0 |
| 0 | R/W | maskErr | Mask for Transfer Error Interrupt of Channel n 0:  mask 1: not mask reset value: 0x0 |


##### Channel n Status Interrupt Register（CHn_INT_STATUS）
Address Offset:0x14

**Table:** (5 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 3 | R | statustrgetcmpfr | Status for Transfer Trigger event complete . Reset value:0x0 |
| 2 | R | statushtfr | Status for Transfer half of block length Complete Interrupt of Channel n reset value: 0x0 |
| 1 | R | statustfr | Status for Transfer Block Length Complete Interrupt of Channel n reset value: 0x0 |
| 0 | R | statusErr | Status for Transfer Error Interrupt of Channel n(It’s only detect AHB bus HRESP error condition) reset value: 0x0 |


##### Channel n Clear Interrupt Register（CHn_INT_CLEAR）
Address Offset:0x18

**Table:** (5 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 3 | W | cleartrgetcmpfr | Clear for statustrgetcmpif 0: no clear 1: clear reset value: 0x0 |
| 2 | W | clearhtfr | Clear for statushtfr 0: no clear 1: clear reset value: 0x0 |
| 1 | W | cleartfr | Clear for Statustfr 0: no clear 1: clear reset value: 0x0 |
| 0 | W | clearErr | Clear for StatusErr 0: no clear 1: clear reset value: 0x0 |


##### Channel n Software Handshaking Request Register（CHn_SOFT_REQ）
Address Offset:0x1c

**Table:** (2 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 0 | W | soft_req | Software Request Register reset value: 0x0 |


##### Channel n Enable control register （CHn_EN)
Address Offset:0x20

**Table:** (2 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 0 | R/W | chn_en | Channel 0 Enable Register When this bit set 1,the ADRn，SARn,CHANn_CTRLA,CHANn_CTRLB can’t be modified. reset value: 0x0 After transfer is over,this bit will auto cleared to 0 by hardware |


##### Global registers
Channel busy status register

**Table:** (18 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 23:16 | - | - | Reserved |
| 15 | R | Ch15bsy | Channel 15 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 14 | R | Ch14bsy | Channel 14 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 13 | R | ch12bsy | Channel 13 busy ,It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 12 | R | Ch12bsy | Channel 12 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 11 | R | Ch11bsy | Channel 11 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 10 | R | ch10bsy | Channel 10 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 9 | R | Ch9bsy | Channel 9 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 8 | R | Ch8bsy | Channel 8 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 7 | R | Ch7bsy | Channel 7 busy ,It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 6 | R | Ch6bsy | Channel 6 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 5 | R | Ch5bsy | Channel 5 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 4 | R | Ch4bsy | Channel 4 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 3 | R | Ch3bsy | Channel 3 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 2 | R | ch2bsy | Channel 2 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 1 | R | ch1bsy | Channel 1 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |
| 0 | R | ch0bsy | Channel 0 busy , It owns the data bus 1 : data transfer busy valid 0 : data transfer busy invalid |

DMACCFG

**Table:** (3 rows, 4 cols)

| bits | R/W | Describe | Describe |
|---|---|---|---|
| 31:1 | - | - | Reserved |
| 0 | R/W | DMACEN | Global DMAC enable. After this bit is set ,all channels can be work. |


### Work Flow
1.Configure register: SARn,DARn,CHn_CTRL_A,CHn_CTRL_B ,and any other register.And set DMACCFG.DMACEN  and  CHn_EN.chnen 1’b1 at last.
2.Configure soft_req for trigger transfer.
3.Wait int flag,The CHn_INT_STATUS.statustfr interrupt flag will clear CHn_EN.chn_en to 0.
4.Afer receive CHn_INT_CLEAR.cleartrf clear interrupt flag.
Note:SARn,DARn,CHn_CTRL_A,CHn_CTRL_B is enable protected（use channel internal enable protect ,not DMACEN）.Before set DMACCFG.DMACEN 1’b1,user should configure it’s channel registers.
