# Pulse Width Modulation （PWM） - Register Description

## Pulse Width Modulation （PWM）

### Register Description

#### Register Memory Map
Table 6-3 all internal registers

**Table:** (31 rows, 5 cols)

| Name | Address Offset | Width | R/W | Description |
|---|---|---|---|---|
| PWMCFG | 0x00 | 32 bits | R/W | PWM configure Reset Value: 0x0 |
| PWMINVERTTRIG | 0x04 | 32 bits | R/W | PWM signal is inverted  Reset Value: 0x0 |
| PWM01TRIG PWM23TRIG PWM45TRIG | 0x08 0x0c 0x10 | 32 bits | R/W | Contain the trigger generate compare value  Reset Value: 0x0 |
| PWMINTEN1 | 0x14 | 32 bits | R/W | Interrupt enable for group2/1/0  Reset Value: 0x0 |
| PWMINTEN2 | 0x18 | 32 bits | R/W | Interrupt enable for group5/4/3  Reset Value: 0x0 |
| PWMRIS1 | 0x1c | 32 bits | RO | Raw interrupt status for group2/1/0  Reset Value: 0x0 |
| PWMRIS2 | 0x20 | 32 bits | RO | Raw interrupt status for group5/4/3  Reset Value: 0x0 |
| PWMIC1 | 0x24 | 32 bits | R/W | Interrupt clear for group2/1/0  Reset Value: 0x0 |
| PWMIC2 | 0x28 | 32 bits | R/W | Interrupt clear for group5/4/3  Reset Value: 0x0 |
| PWMIS1 | 0x2c | 32 bits | RO | Interrupt status for group2/1/0  Reset Value: 0x0 |
| PWMIS2 | 0x30 | 32 bits | RO | Interrupt status for group5/4/3  Reset Value: 0x0 |
| PWMCTL | 0x34 | 32 bits | R/W | Configure the PWM generation blocks  Reset Value: 0x0 |
| PWM01LOAD PWM23LOAD PWM45LOAD | 0x38 0x3c 0x40 | 32 bits | R/W | Contain the load value of the PWM counter  Reset Value: 0x0 |
| PWM01COUNT PWM23COUNT PWM45COUNT | 0x44 0x48 0x4c | 32 bits | RO | Contain the current value of the PWM counter  Reset Value: 0x0 |
| PWM0CMP PWM1CMP PWM2CMP PWM3CMP PWM4CMP PWM5CMP | 0x50 0x54 0x58 0x5c 0x60 0x64 | 32 bits | R/W | Contain a value to be compared against the counter  Reset Value: 0x0 |
| PWM01DB PWM23DB PWM45DB | 0x68 0x6c 0x70 | 32 bits | R/W | Contain the number of clock ticks to delay  Reset Value: 0x0 |
| CAPCTL | 0x74 | 32 bits | R/W | Input capture control  Reset Value: 0x0 |
| CAPINTEN | 0x78 | 32 bits | R/W | Input capture interrupt enable  Reset Value: 0x0 |
| CAPRIS | 0x7c | 32 bits | RO | Input capture raw interrupt status  Reset Value: 0x0 |
| CAPIC | 0x80 | 32 bits | R/W | Input capture interrupt clear  Reset Value: 0x0 |
| CAPIS | 0x84 | 32 bits | RO | Input capture interrupt status  Reset Value: 0x0 |
| CAP01T CAP23T CAP45T | 0x88 0x8c 0x90 | 32 bits | RO | Input capture counter value  Reset Value: 0x0 |
| Cap01match Cap23match Cap45match | 0x94 0x98 0x9c | 32 bits | R/W | Capture match value low 16bits Reset Value:0x0 |
| Tim_int_en | 0xa0 | 32 bits | R/W | Tim int enable Reset Value:0x0 |
| timris | 0xa4 | 32 bits | RO | Timer interrupt status Reset Value:0x0 |
| Tim_int_clr | 0xa8 | 32 bits | R/W | Timer interrupt clear Reset Value:0x0 |
| timis | 0xac | 32 bits | RO | Timer interrupt status Reset Value:0x0 |
| Tim01load Tim23load Tim45load | 0xb0 0xb4 0xb8 | 32 bits | R/W | Timer load value low 16bits Reset Value:0x0 |
| Tim01count Tim23count Tim45count | 0xbc 0xc0 0xc4 | 32 bits | RO | Timer count value low 16bits Reset Value:0x0 |
| Cnt01val Cnt23val Cnv45val | 0xc8 0xcc 0xd0 | 32 bits | RO | Current count value low 16bits Reset Value:0x0 |


#### Register Field Description
PWMCFG
This register configures the global operation of the PWM module.
Name: PWM configuration
Address Offset: 0x00
Table 6–1 PWMCFG Field Description

**Table:** (28 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:28 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 27 | cntdiven | R/W | Counter frequency division configuration 1 : frequency division enable 0 : frequency division disable, so the counter clock source is system clock |
| 26:24 | cntdiv | R/W | Counter frequency division selection 111 : divide by 128 110 : divide by 128 101 : divide by 64 100 : divide by 32 011 : divide by 16 010 : divide by 8 001 : divide by 4 000 : divide by 2 |
| 23 | tim5en | R/W | Group 5 output compare configuration 1 : output compare enable 0 : output compare disable |
| 22 | tim4en | R/W | Group 4 output compare configuration 1 : output compare enable 0 : output compare disable |
| 21 | tim3en | R/W | Group 3 output compare configuration 1 : output compare enable 0 : output compare disable |
| 20 | tim2en | R/W | Group 2 output compare configuration 1 : output compare enable 0 : output compare disable |
| 19 | tim1en | R/W | Group 1 output compare configuration 1 : output compare enable 0 : output compare disable |
| 18 | tim0en | R/W | Group 0 output compare configuration 1 : output compare enable 0 : output compare disable |
| 17 | cap5en | R/W | Group 5-channel 10 input capture configuration 1 : input capture enable 0 : input capture disable |
| 16 | cap4en | R/W | Group 4-channel 8 input capture configuration 1 : input capture enable 0 : input capture disable |
| 15 | cap3en | R/W | Group 3-channel 6 input capture configuration 1 : input capture enable 0 : input capture disable |
| 14 | cap2en | R/W | Group 2-channel 4 input capture configuration 1 : input capture enable 0 : input capture disable |
| 13 | cap1en | R/W | Group 1-channel 2 input capture configuration 1 : input capture enable 0 : input capture disable |
| 12 | cap0en | R/W | Group 0-channel 0 input capture configuration 1 : input capture enable 0 : input capture disable |
| 11 | pwm11en | R/W | Channel 11 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 10 | pwm10en | R/W | Channel 10 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 9 | pwm9en | R/W | Channel 9 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 8 | pwm8en | R/W | Channel 8 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 7 | pwm7en | R/W | Channel 7 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 6 | pwm6en | R/W | Channel 6 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 5 | pwm5en | R/W | Channel 5 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 4 | pwm4en | R/W | Channel 4 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 3 | pwm3en | R/W | Channel 3 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 2 | pwm2en | R/W | Channel 2 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 1 | pwm1en | R/W | Channel 1 PWM configuration 1 : PWM output enable 0 : PWM output disable |
| 0 | pwm0en | R/W | Channel 0 PWM configuration 1 : PWM output enable 0 : PWM output disable |

PWMINVERTTRIG
This register controls the ADC trigger generation capabilities of the PWM generator (group) and provides a master control of the polarity of the PWM signals on the device pins.
Name: PWM Output Inversion and trigger signal
Address Offset: 0x04
Table 6–2 PWMINVERTTRIG Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:12 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 11 | pwm11inv | R/W | Invert PWM11 signal (channel11) 1 : PWM5 signal is inverted 0 : PWM5 signal is not inverted |
| 10 | pwm10inv | R/W | Invert PWM10 signal (channel10) 1 : PWM4 signal is inverted 0 : PWM4 signal is not inverted |
| 9 | pwm9inv | R/W | Invert PWM9 signal (channel9) 1 : PWM3 signal is inverted 0 : PWM3 signal is not inverted |
| 8 | pwm8inv | R/W | Invert PWM8 signal (channel8) 1 : PWM2 signal is inverted 0 : PWM2 signal is not inverted |
| 7 | pwm7inv | R/W | Invert PWM7 signal (channel7) 1 : PWM1 signal is inverted 0 : PWM1 signal is not inverted |
| 6 | pwm6inv | R/W | Invert PWM6 signal (channel6) 1 : PWM0 signal is inverted 0 : PWM0 signal is not inverted |
| 5 | pwm5inv | R/W | Invert PWM5 signal (channel5) 1 : PWM5 signal is inverted 0 : PWM5 signal is not inverted |
| 4 | pwm4inv | R/W | Invert PWM4 signal (channel4) 1 : PWM4 signal is inverted 0 : PWM4 signal is not inverted |
| 3 | pwm3inv | R/W | Invert PWM3 signal (channel3) 1 : PWM3 signal is inverted 0 : PWM3 signal is not inverted |
| 2 | pwm2inv | R/W | Invert PWM2 signal (channel2) 1 : PWM2 signal is inverted 0 : PWM2 signal is not inverted |
| 1 | pwm1inv | R/W | Invert PWM1 signal (channel1) 1 : PWM1 signal is inverted 0 : PWM1 signal is not inverted |
| 0 | pwm0inv | R/W | Invert PWM0 signal (channel0) 1 : PWM0 signal is inverted 0 : PWM0 signal is not inverted |

PWM01TRIG
This register contains the value generating trigger signals.
Name: PWM group 0 and group 1 trigger compare value
Address Offset: 0x08
Table 6–3 PWM01TRIG Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | pwm1trig | R/W | The value generating group 1 trigger signal |
| 15:0 | pwm0trig | R/W | The value generating group 0 trigger signal |

PWM23TRIG
This register contains the value generating trigger signals.
Name: PWM group 2 and group 3 trigger compare value
Address Offset: 0x0c
Table 6–4 PWM23TRIG Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | pwm3trig | R/W | The value generating group 3 trigger signal |
| 15:0 | pwm2trig | R/W | The value generating group 2 trigger signal |

PWM45TRIG
This register contains the value generating trigger signals.
Name: PWM group 4 and group 5 trigger compare value
Address Offset: 0x10
Table 6–5 PWM45TRIG Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | pwm5trig | R/W | The value generating group 5 trigger signal |
| 15:0 | pwm4trig | R/W | The value generating group 4 trigger signal |

PWMINTEN1
This register controls the global interrupt generation capabilities of the PWM module.
Name: PWM Interrupt enable for group 2/1/0
Address Offset: 0x14
Table 6–6 PWMNTEN1 Field Description

**Table:** (23 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:30 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 29 | Int2encmpbd | R/W | Interrupt for counter=comparator B Down (channel5) 1 : a raw interrupt occurs when the counter matches the value in the PWM2CMP register value when counting down 0 : no interrupt |
| 28 | Int2encmpad | R/W | Interrupt for counter=comparator A Down (channel4) 1 : a raw interrupt occurs when the counter matches the value in the PWM2CMP register value when counting down 0 : no interrupt |
| 27 | Int2encmpbu | R/W | Interrupt for counter=comparator B Up (channel5) 1 : a raw interrupt occurs when the counter matches the value in the PWM2CMP register value when counting up 0 : no interrupt |
| 26 | Int2encmpau | R/W | Interrupt for counter=comparator A Up (channel4) 1 : a raw interrupt occurs when the counter matches the value in the PWM2CMP register value when counting up 0 : no interrupt |
| 25 | Int2encntload | R/W | Interrupt for counter=Load (channel5/4) 1 : a raw interrupt occurs when the counter matches the values in the PWM23LOAD register value 0 : no interrupt |
| 24 | Int2encntzero | R/W | Interrupt for counter=0 (channel5/4) 1 : a raw interrupt occurs when the counter is zero 0 : no interrupt |
| 23:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Int1encmpbd | R/W | Interrupt for counter=comparator B Down (channel3) 1 : a raw interrupt occurs when the counter matches the value in the PWM1CMP register value when counting down 0 : no interrupt |
| 20 | Int1encmpad | R/W | Interrupt for counter=comparator A Down (channel2) 1 : a raw interrupt occurs when the counter matches the value in the PWM1CMP register value when counting down 0 : no interrupt |
| 19 | Int1encmpbu | R/W | Interrupt for counter=comparator B Up (channel3) 1 : a raw interrupt occurs when the counter matches the value in the PWM1CMP register value when counting up 0 : no interrupt |
| 18 | Int1encmpau | R/W | Interrupt for counter=comparator A Up (channel2) 1 : a raw interrupt occurs when the counter matches the value in the PWM1CMP register value when counting up 0 : no interrupt |
| 17 | Int1encntload | R/W | Interrupt for counter=Load (channel3/2) 1 : a raw interrupt occurs when the counter matches the values in the PWM01LOAD register value 0 : no interrupt |
| 16 | Int1encntzero | R/W | Interrupt for counter=0 (channel3/2) 1 : a raw interrupt occurs when the counter is zero 0 : no interrupt |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Int0encmpbd | R/W | Interrupt for counter=comparator B Down (channel1) 1 : a raw interrupt occurs when the counter matches the value in the PWM0CMP register value when counting down 0 : no interrupt |
| 12 | Int0encmpad | R/W | Interrupt for counter=comparator A Down (channel0) 1 : a raw interrupt occurs when the counter matches the value in the PWM0CMP register value when counting down 0 : no interrupt |
| 11 | Int0encmpbu | R/W | Interrupt for counter=comparator B Up (channel1) 1 : a raw interrupt occurs when the counter matches the value in the PWM0CMP register value when counting up 0 : no interrupt |
| 10 | Int0encmpau | R/W | Interrupt for counter=comparator A Up (channel0) 1 : a raw interrupt occurs when the counter matches the value in the PWM0CMP register value when counting up 0 : no interrupt |
| 9 | Int0encntload | R/W | Interrupt for counter=Load (channel1/0) 1 : a raw interrupt occurs when the counter matches the values in the PWM01LOAD register value 0 : no interrupt |
| 8 | Int0encntzero | R/W | Interrupt for counter=0 (channel1/0) 1 : a raw interrupt occurs when the counter is zero 0 : no interrupt |
| 7:0 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |

PWMINTEN2
This register controls the global interrupt generation capabilities of the PWM module.
Name: PWM Interrupt enable for group 5/4/3
Address Offset: 0x18
Table 6–7 PWMINTEN2 Field Description

**Table:** (22 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Int5encmpbd | R/W | Interrupt for counter=comparator B Down (group5-channel11) 1 : a raw interrupt occurs when the counter matches the value in the PWM5CMP register value when counting down 0 : no interrupt |
| 20 | Int5encmpad | R/W | Interrupt for counter=comparator A Down (group5-channel10) 1 : a raw interrupt occurs when the counter matches the value in the PWM5CMP register value when counting down 0 : no interrupt |
| 19 | Int5encmpbu | R/W | Interrupt for counter=comparator B Up (group5-channel11) 1 : a raw interrupt occurs when the counter matches the value in the PWM5CMP register value when counting up 0 : no interrupt |
| 18 | Int5encmpau | R/W | Interrupt for counter=comparator A Up (group5-channel10) 1 : a raw interrupt occurs when the counter matches the value in the PWM5CMP register value when counting up 0 : no interrupt |
| 17 | Int5encntload | R/W | Interrupt for counter=Load (group5-channel11/10) 1 : a raw interrupt occurs when the counter matches the values in the PWM45LOAD register value 0 : no interrupt |
| 16 | Int5encntzero | R/W | Interrupt for counter=0 (group5-channel11/10) 1 : a raw interrupt occurs when the counter is zero 0 : no interrupt |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Int4encmpbd | R/W | Interrupt for counter=comparator B Down (group4-channel9) 1 : a raw interrupt occurs when the counter matches the value in the PWM4CMP register value when counting down 0 : no interrupt |
| 12 | Int4encmpad | R/W | Interrupt for counter=comparator A Down (group4-channel8) 1 : a raw interrupt occurs when the counter matches the value in the PWM4CMP register value when counting down 0 : no interrupt |
| 11 | Int4encmpbu | R/W | Interrupt for counter=comparator B Up (group4-channel9) 1 : a raw interrupt occurs when the counter matches the value in the PWM4CMP register value when counting up 0 : no interrupt |
| 10 | Int4encmpau | R/W | Interrupt for counter=comparator A Up (group4-channel8) 1 : a raw interrupt occurs when the counter matches the value in the PWM4CMP register value when counting up 0 : no interrupt |
| 9 | Int4encntload | R/W | Interrupt for counter=Load (group4-channel8/9) 1 : a raw interrupt occurs when the counter matches the values in the PWM45LOAD register value 0 : no interrupt |
| 8 | Int4encntzero | R/W | Interrupt for counter=0 (group4-channel8/9) 1 : a raw interrupt occurs when the counter is zero 0 : no interrupt |
| 7:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | Int3encmpbd | R/W | Interrupt for counter=comparator B Down (group3-channel7) 1 : a raw interrupt occurs when the counter matches the value in the PWM3CMP register value when counting down 0 : no interrupt |
| 4 | Int3encmpad | R/W | Interrupt for counter=comparator A Down (group3-channel6) 1 : a raw interrupt occurs when the counter matches the value in the PWM3CMP register value when counting down 0 : no interrupt |
| 3 | Int3encmpbu | R/W | Interrupt for counter=comparator B Up (group3-channel7) 1 : a raw interrupt occurs when the counter matches the value in the PWM3CMP register value when counting up 0 : no interrupt |
| 2 | Int3encmpau | R/W | Interrupt for counter=comparator A Up (group3-channel6) 1 : a raw interrupt occurs when the counter matches the value in the PWM3CMP register value when counting up 0 : no interrupt |
| 1 | Int3encntload | R/W | Interrupt for counter=Load (group3-channel7/6) 1 : a raw interrupt occurs when the counter matches the values in the PWM23LOAD register value 0 : no interrupt |
| 0 | Int3encntzero | R/W | Interrupt for counter=0 (group3-channel7/6) 1 : a raw interrupt occurs when the counter is zero 0 : no interrupt |

PWMRIS1
This register provides the current set of interrupt sources that asserted, regardless of whether they cause an interrupt to be asserted to the controller.
Name: PWM raw interrupt status for gourp 2/1/0
Address Offset: 0x1c
Table 6–8 PWMRIS1 Field Description

**Table:** (23 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:30 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 29 | Intris2cmpbd | RO | Raw Interrupt for counter=comparator B Down (group2-channel5) 1 : a raw interrupt asserted when the counter matches the value in the PWM2CMP register value when counting down 0 : no interrupt |
| 28 | Intris2cmpad | RO | Raw Interrupt for counter=comparator A Down (group2-channel4) 1 : a raw interrupt asserted when the counter matches the value in the PWM2CMP register value when counting down 0 : no interrupt |
| 27 | Intris2cmpbu | RO | Raw Interrupt for counter=comparator B Up (group2-channel5) 1 : a raw interrupt asserted when the counter matches the value in the PWM2CMP register value when counting up 0 : no interrupt |
| 26 | Intris2cmpau | RO | Raw Interrupt for counter=comparator A Up (group2-channel4) 1 : a raw interrupt asserted when the counter matches the value in the PWM2CMP register value when counting up 0 : no interrupt |
| 25 | Intris2cntload | RO | Raw Interrupt for counter=Load (group2-channel5/4) 1 : a raw interrupt asserted when the counter matches the values in the PWM23LOAD register value 0 : n5o interrupt |
| 24 | Intris2cntzero | RO | Raw Interrupt for counter=0 (group2-channel5/4) 1 : a raw interrupt asserted when the counter is zero 0 : no interrupt |
| 23:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Intris1cmpbd | RO | Raw Interrupt for counter=comparator B Down (group1-channel3) 1 : a raw interrupt asserted when the counter matches the value in the PWM1CMP register value when counting down 0 : no interrupt |
| 20 | Intris1cmpad | RO | Raw Interrupt for counter=comparator A Down (group1-channel2) 1 : a raw interrupt asserted when the counter matches the value in the PWM1CMP register value when counting down 0 : no interrupt |
| 19 | Intris1cmpbu | RO | Raw Interrupt for counter=comparator B Up (group1-channel3) 1 : a raw interrupt asserted when the counter matches the value in the PWM1CMP register value when counting up 0 : no interrupt |
| 18 | Intris1cmpau | RO | Raw Interrupt for counter=comparator A Up (group1-channel2) 1 : a raw interrupt asserted when the counter matches the value in the PWM1CMP register value when counting up 0 : no interrupt |
| 17 | Intris1cntload | RO | Raw Interrupt for counter=Load (group1-channel3/2) 1 : a raw interrupt asserted when the counter matches the values in the PWM01LOAD register value 0 : no interrupt |
| 16 | Intris1cntzero | RO | Raw Interrupt for counter=0 (group1-channel3/2) 1 : a raw interrupt asserted when the counter is zero 0 : no interrupt |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Intris0cmpbd | RO | Raw Interrupt for counter=comparator B Down (group0-channel1) 1 : a raw interrupt asserted when the counter matches the value in the PWM0CMP register value when counting down 0 : no interrupt |
| 12 | Intris0cmpad | RO | Raw Interrupt for counter=comparator A Down (group0-channel0) 1 : a raw interrupt asserted when the counter matches the value in the PWM0CMP register value when counting down 0 : no interrupt |
| 11 | Intris0cmpbu | RO | Raw Interrupt for counter=comparator B Up (group0-channel1) 1 : a raw interrupt asserted when the counter matches the value in the PWM0CMP register value when counting up 0 : no interrupt |
| 10 | Intris0cmpau | RO | Raw Interrupt for counter=comparator A Up (group0-channel0) 1 : a raw interrupt asserted when the counter matches the value in the PWM0CMP register value when counting up 0 : no interrupt |
| 9 | Intris0cntload | RO | Raw Interrupt for counter=Load (group0-channel1/0) 1 : a raw interrupt asserted when the counter matches the values in the PWM01LOAD register value 0 : no interrupt |
| 8 | Intris0cntzero | RO | Raw Interrupt for counter=0 (group0-channel1/0) 1 : a raw interrupt asserted when the counter is zero 0 : no interrupt |
| 7:0 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |

PWMRIS2
This register controls the global interrupt generation capabilities of the PWM module.
Name: PWM raw interrupt status for group 5/4/3
Address Offset: 0x20
Table 6–9 PWMRIS2 Field Description

**Table:** (22 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Intris5cmpbd | RO | Raw Interrupt for counter=comparator B Down (group5-channel11) 1 : a raw interrupt asserted when the counter matches the value in the PWM5CMP register value when counting down 0 : no interrupt |
| 20 | Intris5cmpad | RO | Raw Interrupt for counter=comparator A Down (group5-channel10) 1 : a raw interrupt asserted when the counter matches the value in the PWM5CMP register value when counting down 0 : no interrupt |
| 19 | Intris5cmpbu | RO | Raw Interrupt for counter=comparator B Up (group5-channel11) 1 : a raw interrupt asserted when the counter matches the value in the PWM5CMP register value when counting up 0 : no interrupt |
| 18 | Intris5cmpau | RO | Raw Interrupt for counter=comparator A Up (group5-channel10) 1 : a raw interrupt asserted when the counter matches the value in the PWM5CMP register value when counting up 0 : no interrupt |
| 17 | Intris5cntload | RO | Raw Interrupt for counter=Load (group5-channel11/10) 1 : a raw interrupt asserted when the counter matches the values in the PWM45LOAD register value 0 : no interrupt |
| 16 | Intris5cntzero | RO | Raw Interrupt for counter=0 (group5-channel11/10) 1 : a raw interrupt asserted when the counter is zero 0 : no interrupt |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Intris4cmpbd | RO | Raw Interrupt for counter=comparator B Down (group4-channel9) 1 : a raw interrupt asserted when the counter matches the value in the PWM4CMP register value when counting down 0 : no interrupt |
| 12 | Intris4cmpad | RO | Raw Interrupt for counter=comparator A Down (group4-channel8) 1 : a raw interrupt asserted when the counter matches the value in the PWM4CMP register value when counting down 0 : no interrupt |
| 11 | Intris4cmpbu | RO | Raw Interrupt for counter=comparator B Up (group4-channel9) 1 : a raw interrupt asserted when the counter matches the value in the PWM4CMP register value when counting up 0 : no interrupt |
| 10 | Intris4cmpau | RO | Raw Interrupt for counter=comparator A Up (group4-channel8) 1 : a raw interrupt asserted when the counter matches the value in the PWM4CMP register value when counting up 0 : no interrupt |
| 9 | Intris4cntload | RO | Raw Interrupt for counter=Load (group4-channel9/8) 1 : a raw interrupt asserted when the counter matches the values in the PWM45LOAD register value 0 : no interrupt |
| 8 | Intris4cntzero | RO | Raw Interrupt for counter=0 (group4-channel9/8) 1 : a raw interrupt asserted when the counter is zero 0 : no interrupt |
| 7:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | Intris3cmpbd | RO | Raw Interrupt for counter=comparator B Down (group3-channel7) 1 : a raw interrupt asserted when the counter matches the value in the PWM3CMP register value when counting down 0 : no interrupt |
| 4 | Intris3cmpad | RO | Raw Interrupt for counter=comparator A Down (group3-channel6) 1 : a raw interrupt asserted when the counter matches the value in the PWM3CMP register value when counting down 0 : no interrupt |
| 3 | Intris3cmpbu | RO | Raw Interrupt for counter=comparator B Up (group3-channel7) 1 : a raw interrupt asserted when the counter matches the value in the PWM3CMP register value when counting up 0 : no interrupt |
| 2 | Intris3cmpau | RO | Raw Interrupt for counter=comparator A Up (group3-channel6) 1 : a raw interrupt asserted when the counter matches the value in the PWM3CMP register value when counting up 0 : no interrupt |
| 1 | Intris3cntload | RO | Raw Interrupt for counter=Load (group3-channel7/6) 1 : a raw interrupt asserted when the counter matches the values in the PWM23LOAD register value 0 : no interrupt |
| 0 | Intris3cntzero | RO | Raw Interrupt for counter=0 (group3-channel7/6) 1 : a raw interrupt asserted when the counter is zero 0 : no interrupt |

PWMIC1
This register clears the interrupt.
Name: PWM interrupt clear for gourp 2/1/0
Address Offset: 0x24
Table 6–10 PWMIC1 Field Description

**Table:** (23 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:30 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 29 | Intic2cmpbd | R/W | Clear Interrupt for counter=comparator B Down (group2-channel5) 1 : clear 0 : not clear |
| 28 | Intic2cmpad | R/W | Clear Interrupt for counter=comparator A Down (group2-channel4) 1 : clear 0 : not clear |
| 27 | Intic2cmpbu | R/W | Clear Interrupt for counter=comparator B Up (group2-channel5) 1 : clear 0 : not clear |
| 26 | Intic2cmpau | R/W | Clear Interrupt for counter=comparator A Up (group2-channel4) 1 : clear 0 : not clear |
| 25 | Intic2cntload | R/W | Clear Interrupt for counter=Load (group2-channel5/4) 1 : clear 0 : not clear |
| 24 | Intic2cntzero | R/W | Clear Interrupt for counter=0 (group2-channel5/4) 1 : clear 0 : not clear |
| 23:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Intic1cmpbd | R/W | Clear Interrupt for counter=comparator B Down (group1-channel3) 1 : clear 0 : not clear |
| 20 | Intic1cmpad | R/W | Clear Interrupt for counter=comparator A Down (group1-channel2) 1 : clear 0 : not clear |
| 19 | Intic1cmpbu | R/W | Clear Interrupt for counter=comparator B Up (group1-channel3) 1 : clear 0 : not clear |
| 18 | Intic1cmpau | R/W | Clear Interrupt for counter=comparator A Up (group1-channel2) 1 : clear 0 : not clear |
| 17 | Intic1cntload | R/W | Clear Interrupt for counter=Load (group1-channel3/2) 1 : clear 0 : not clear |
| 16 | Intic1cntzero | R/W | Clear Interrupt for counter=0 (group1-channel3/2) 1 : clear 0 : not clear |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Intic0cmpbd | R/W | Clear Interrupt for counter=comparator B Down (group0-channel1/0) 1 : clear 0 : not clear |
| 12 | Intic0cmpad | R/W | Clear Interrupt for counter=comparator A Down (group0-channel0) 1 : clear 0 : not clear |
| 11 | Intic0cmpbu | R/W | Clear Interrupt for counter=comparator B Up (group0-channel1) 1 : clear 0 : not clear |
| 10 | Intic0cmpau | R/W | Clear Interrupt for counter=comparator A Up (group0-channel0) 1 : clear 0 : not clear |
| 9 | Intic0cntload | R/W | Clear Interrupt for counter=Load (group0-channel1/0) 1 : clear 0 : not clear |
| 8 | Intic0cntzero | R/W | Clear Interrupt for counter=0 (group0-channel1/0) 1 : clear 0 : not clear |
| 7:0 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |

PWMIC2
This register clears the interrupt.
Name: PWM interrupt clear for group 5/4/3
Address Offset: 0x28
Table 6–11 PWMIC2 Field Description

**Table:** (22 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Intic5cmpbd | R/W | Clear Interrupt for counter=comparator B Down (group5-channel11) 1 : clear 0 : not clear |
| 20 | Intic5cmpad | R/W | Clear Interrupt for counter=comparator A Down (group5-channel10) 1 : clear 0 : not clear |
| 19 | Intic5cmpbu | R/W | Clear Interrupt for counter=comparator B Up (group5-channel11) 1 : clear 0 : not clear |
| 18 | Intic5cmpau | R/W | Clear Interrupt for counter=comparator A Up (group5-channel10) 1 : clear 0 : not clear |
| 17 | Intic5cntload | R/W | Clear Interrupt for counter=Load (group5-channel11/10) 1 : clear 0 : not clear |
| 16 | Intic5cntzero | R/W | Clear Interrupt for counter=0 (group5-channel11/10) 1 : clear 0 : not clear |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Intic4cmpbd | R/W | Clear Interrupt for counter=comparator B Down (group4-channel9) 1 : clear 0 : not clear |
| 12 | Intic4cmpad | R/W | Clear Interrupt for counter=comparator A Down (group4-channel8) 1 : clear 0 : not clear |
| 11 | Intic4cmpbu | R/W | Clear Interrupt for counter=comparator B Up (group4-channel9) 1 : clear 0 : not clear |
| 10 | Intic4cmpau | R/W | Clear Interrupt for counter=comparator A Up (group4-channel8) 1 : clear 0 : not clear |
| 9 | Intic4cntload | R/W | Clear Interrupt for counter=Load (group4-channel9/8) 1 : clear 0 : not clear |
| 8 | Intic4cntzero | R/W | Clear Interrupt for counter=0 (group4-channel9/8) 1 : clear 0 : not clear |
| 7:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | Intic3cmpbd | R/W | Clear Interrupt for counter=comparator B Down (group3-channel7) 1 : clear 0 : not clear |
| 4 | Intic3cmpad | R/W | Clear Interrupt for counter=comparator A Down (group3-channel6) 1 : clear 0 : not clear |
| 3 | Intic3cmpbu | R/W | Clear Interrupt for counter=comparator B Up (group3-channel7) 1 : clear 0 : not clear |
| 2 | Intic3cmpau | R/W | Clear Interrupt for counter=comparator A Up (group3-channel6) 1 : clear 0 : not clear |
| 1 | Intic3cntload | R/W | Clear Interrupt for counter=Load (group3-channel7/6) 1 : clear 0 : not clear |
| 0 | Intic3cntzero | R/W | Clear Interrupt for counter=0 (group3-channel7/6) 1 : clear 0 : not clear |

PWMIS1
This register provides the current set of interrupt sources that asserted, regardless of whether they cause an interrupt to be asserted to the controller.
Name: PWM interrupt status for gourp 2/1/0
Address Offset: 0x2c
Table 6–12 PWMIS1 Field Description

**Table:** (23 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:30 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 29 | Intis2cmpbd | RO | Interrupt for counter=comparator B Down (group2-channel5) 1 : a interrupt asserted when the counter matches the value in the PWM2CMP register value when counting down 0 : no interrupt |
| 28 | Intis2cmpad | RO | Interrupt for counter=comparator A Down (group2-channel4) 1 : a interrupt asserted when the counter matches the value in the PWM2CMP register value when counting down 0 : no interrupt |
| 27 | Intis2cmpbu | RO | Interrupt for counter=comparator B Up (group2-channel5) 1 : a interrupt asserted when the counter matches the value in the PWM2CMP register value when counting up 0 : no interrupt |
| 26 | Intis2cmpau | RO | Interrupt for counter=comparator A Up (group2-channel4) 1 : a interrupt asserted when the counter matches the value in the PWM2CMP register value when counting up 0 : no interrupt |
| 25 | Intis2cntload | RO | Interrupt for counter=Load (group2-channel5/4) 1 : a interrupt asserted when the counter matches the values in the PWM23LOAD register value 0 : no interrupt |
| 24 | Intis2cntzero | RO | Interrupt for counter=0 (group2-channel5/4) 1 : a interrupt asserted when the counter is zero 0 : no interrupt |
| 23:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Intis1cmpbd | RO | Interrupt for counter=comparator B Down (group1-channel3) 1 : a interrupt asserted when the counter matches the value in the PWM1CMP register value when counting down 0 : no interrupt |
| 20 | Intis1cmpad | RO | Interrupt for counter=comparator A Down (group1-channel2) 1 : a interrupt asserted when the counter matches the value in the PWM1CMP register value when counting down 0 : no interrupt |
| 19 | Intis1cmpbu | RO | Interrupt for counter=comparator B Up (group1-channel3) 1 : a interrupt asserted when the counter matches the value in the PWM1CMP register value when counting up 0 : no interrupt |
| 18 | Intis1cmpau | RO | Interrupt for counter=comparator A Up (group1-channel2) 1 : a interrupt asserted when the counter matches the value in the PWM1CMP register value when counting up 0 : no interrupt |
| 17 | Intis1cntload | RO | Interrupt for counter=Load (group1-channel3/2) 1 : a interrupt asserted when the counter matches the values in the PWM01LOAD register value 0 : no interrupt |
| 16 | Intis1cntzero | RO | Interrupt for counter=0 (group1-channel3/2) 1 : a interrupt asserted when the counter is zero 0 : no interrupt |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Intis0cmpbd | RO | Interrupt for counter=comparator B Down (group0-channel1) 1 : a interrupt asserted when the counter matches the value in the PWM0CMP register value when counting down 0 : no interrupt |
| 12 | Intis0cmpad | RO | Interrupt for counter=comparator A Down (group0-channel0) 1 : a interrupt asserted when the counter matches the value in the PWM0CMP register value when counting down 0 : no interrupt |
| 11 | Intis0cmpbu | RO | Interrupt for counter=comparator B Up (group0-channel1) 1 : a interrupt asserted when the counter matches the value in the PWM0CMP register value when counting up 0 : no interrupt |
| 10 | Intis0cmpau | RO | Interrupt for counter=comparator A Up (group0-channel0) 1 : a interrupt asserted when the counter matches the value in the PWM0CMP register value when counting up 0 : no interrupt |
| 9 | Intis0cntload | RO | Interrupt for counter=Load (group0-channel1/0) 1 : a interrupt asserted when the counter matches the values in the PWM01LOAD register value 0 : no interrupt |
| 8 | Intis0cntzero | RO | Interrupt for counter=0 (group0-channel1/0) 1 : a interrupt asserted when the counter is zero 0 : no interrupt |
| 7:0 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |

PWMIS2
This register controls the global interrupt generation capabilities of the PWM module.
Name: PWM raw interrupt status for group 5/4/3
Address Offset: 0x30
Table 6–13 PWMIS2 Field Description

**Table:** (22 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:22 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 21 | Intis5cmpbd | RO | Interrupt for counter=comparator B Down (group5-channel11) 1 : a interrupt asserted when the counter matches the value in the PWM5CMP register value when counting down 0 : no interrupt |
| 20 | Intis5cmpad | RO | Interrupt for counter=comparator A Down (group5-channel10) 1 : a interrupt asserted when the counter matches the value in the PWM5CMP register value when counting down 0 : no interrupt |
| 19 | Intis5cmpbu | RO | Interrupt for counter=comparator B Up (group5-channel11) 1 : a interrupt asserted when the counter matches the value in the PWM5CMP register value when counting up 0 : no interrupt |
| 18 | Intis5cmpau | RO | Interrupt for counter=comparator A Up (group5-channel10) 1 : a interrupt asserted when the counter matches the value in the PWM5CMP register value when counting up 0 : no interrupt |
| 17 | Intis5cntload | RO | Interrupt for counter=Load (group5-channel11/10) 1 : a interrupt asserted when the counter matches the values in the PWM45LOAD register value 0 : no interrupt |
| 16 | Intis5cntzero | RO | Interrupt for counter=0 (group5-channel11/10) 1 : a interrupt asserted when the counter is zero 0 : no interrupt |
| 15:14 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 13 | Intis4cmpbd | RO | Interrupt for counter=comparator B Down (group4-channel9) 1 : a interrupt asserted when the counter matches the value in the PWM4CMP register value when counting down 0 : no interrupt |
| 12 | Intis4cmpad | RO | Interrupt for counter=comparator A Down (group4-channel8) 1 : a interrupt asserted when the counter matches the value in the PWM4CMP register value when counting down 0 : no interrupt |
| 11 | Intis4cmpbu | RO | Interrupt for counter=comparator B Up (group4-channel9) 1 : a interrupt asserted when the counter matches the value in the PWM4CMP register value when counting up 0 : no interrupt |
| 10 | Intis4cmpau | RO | Interrupt for counter=comparator A Up (group4-channel8) 1 : a interrupt asserted when the counter matches the value in the PWM4CMP register value when counting up 0 : no interrupt |
| 9 | Intis4cntload | RO | Interrupt for counter=Load (group4-channel9/8) 1 : a interrupt asserted when the counter matches the values in the PWM45LOAD register value 0 : no interrupt |
| 8 | Intis4cntzero | RO | Interrupt for counter=0 (group4-channel9/8) 1 : a interrupt asserted when the counter is zero 0 : no interrupt |
| 7:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | Intis3cmpbd | RO | Interrupt for counter=comparator B Down (group3-channel7) 1 : a interrupt asserted when the counter matches the value in the PWM3CMP register value when counting down 0 : no interrupt |
| 4 | Intis3cmpad | RO | Interrupt for counter=comparator A Down (group3-channel6) 1 : a interrupt asserted when the counter matches the value in the PWM3CMP register value when counting down 0 : no interrupt |
| 3 | Intis3cmpbu | RO | Interrupt for counter=comparator B Up (group3-channel7) 1 : a interrupt asserted when the counter matches the value in the PWM3CMP register value when counting up 0 : no interrupt |
| 2 | Intis3cmpau | RO | Interrupt for counter=comparator A Up (group3-channel6) 1 : a interrupt asserted when the counter matches the value in the PWM3CMP register value when counting up 0 : no interrupt |
| 1 | Intis3cntload | RO | Interrupt for counter=Load (group3-channel7/6) 1 : a interrupt asserted when the counter matches the values in the PWM23LOAD register value 0 : no interrupt |
| 0 | Intis3cntzero | RO | Interrupt for counter=0 (group3-channel7/6) 1 : a interrupt asserted when the counter is zero 0 : no interrupt |

PWMCTL
This register configures the PWM counter mode.
Name: PWM control
Address Offset: 0x34
Table 6–14 PWMCTL Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:18 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 17:16 | Sync5mode | R/W | Synchronize mode (group5-channel11/10) 00 : update register value when counter reach to zero 01 : update register value when counter reach to load value 10 : update register value when counter reach to zero and load value 11 : not update |
| 15:14 | Sync4mode | R/W | Synchronize mode (group4-channel9/8) 00 : update register value when counter reach to zero 01 : update register value when counter reach to load value 10 : update register value when counter reach to zero and load value 11 : not update |
| 13:12 | Sync3mode | R/W | Synchronize mode (group3-channel7/6) 00 : update register value when counter reach to zero 01 : update register value when counter reach to load value 10 : update register value when counter reach to zero and load value 11 : not update |
| 11:10 | sync2mode | R/W | Synchronize mode (group2-channel5/4) 00 : update register value when counter reach to zero 01 : update register value when counter reach to load value 10 : update register value when counter reach to zero and load value 11 : not update |
| 9:8 | sync1mode | R/W | Synchronize mode (group1-channel3/2) 00 : update register value when counter reach to zero 01 : update register value when counter reach to load value 10 : update register value when counter reach to zero and load value 11 : not update |
| 7:6 | sync0mode | R/W | Synchronize mode (group0-channel1/0) 00 : update register value when counter reach to zero 01 : update register value when counter reach to load value 10 : update register value when counter reach to zero and load value 11 : not update |
| 5 | pwm5mode | R/W | Counter mode (group5-channel11/10) 1 : count-up/down mode 0 : count-up mode |
| 4 | pwm4mode | R/W | Counter mode (group4-channel9/8) 1 : count-up/down mode 0 : count-up mode |
| 3 | pwm3mode | R/W | Counter mode (group3-channel7/6) 1 : count-up/down mode 0 : count-up mode |
| 2 | pwm2mode | R/W | Counter mode (group2-channel5/4) 1 : count-up/down mode 0 : count-up mode |
| 1 | pwm1mode | R/W | Counter mode (group1-channel3/2) 1 : count-up/down mode 0 : count-up mode |
| 0 | pwm0mode | R/W | Counter mode (group0-channel1/0) 1 : count-up/down mode 0 : count-up mode |

PWM01LOAD
PWM23LOAD
PWM45LOAD
This register contains the load value for the PWM counter.
Name: PWM load value
Address Offset: 0x38, 0x3c, 0x40
Table 6–15 PWMnmLOAD Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | loadm | R/W | Counter load m value  (m=1 : group1-channel3/2 ; m=3 : group3-channel7/6 ; m=5 : group5-channel11/10) The counter load value |
| 15:0 | loadn | R/W | Counter load n value (n=0 : group0-channel1/0 ; n=2 : group2-channel5/4 ; n=4 : group4-channel9/8) The counter load value |

PWM01COUNT
PWM23COUNT
PWM45COUNT
This register contains the current value of the PWM counter.
Name: PWM current value
Address Offset: 0x44, 0x48, 0x4c
Table 6–16 PWMnmCOUNT Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | countm | RO | Counter m value (m=1 : group1-channel3/2 ; m=3 : group3-channel7/6 ; m=5 : group5-channel11/10) The current value of the counter |
| 15:0 | countn | RO | Counter n value (n=0 : group0-channel1/0 ; n=2 : group2-channel5/4 ; n=4 : group4-channel9/8) The current value of the counter |

PWM0CMP
PWM1CMP
PWM2CMP
PWM3CMP
PWM4CMP
PWM5CMP
This register contain a value to be compared against the counter.
Name: PWM compare value
Address Offset: 0x50, 0x54, 0x58, 0x5c, 0x60, 0x64
Table 6–17 PWMnCMP Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | compnb | R/W | Comparator B value (n=0 : group0-channel1; n=1 : group1-channel3 ; n=2 : group2-channel5 ; n=3 : group2-channel7 ; n=4 : group4-channel9 ; n=5 : group5-channel11) The value to be compared against the counter |
| 15:0 | compna | R/W | Comparator A value (n=0 : group0-channel0 ; n=1 : group1-channel2 ; n=2 : group2-channel4 ; n=3 : group2-channel6 ; n=4 : group4-channel8 ; n=5 : group5-channel10) The value to be compared against the counter |

PWM01DB
PWM23DB
PWM45DB
These registers contains the number of clock ticks to delay.
Name: PWM deadband value
Address Offset: 0x68, 0x6c, 0x70
Table 6–18 PWMnmDB Field Description

**Table:** (6 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:26 | Reserved and read as zero | Reserved and read as zero | Reserved and read as zero |
| 25 | dbmen | R/W | Dead-band m generator (group) enable (m=1 : group1-channel3/2 ; m=3 : group3-channel7/6 ; m=5 : group5-channel11/10) 1 : insert dead-band into the output signals 0 : pass the PWM signals through simply |
| 24 | dbnen | R/W | Dead-band n generator (group) enable (n=0 : group0-channel1/0 ; n=2 : group2-channel5/4 ; n=4 : group4-channel9/8) 1 : insert dead-band into the output signals 0 : pass the PWM signals through simply |
| 23:12 | delaym | R/W | Dead-band delay (m=1 : group1-channel3/2 ; m=3 : group3-channel7/6 ; m=5 : group5-channel11/10) The number of clock tick to delay |
| 11:0 | delayn | R/W | Dead-band delay (n=0 : group0-channel1/0 ; n=2 : group2-channel5/4 ; n=4 : group4-channel9/8) The number of clock tick to delay |

CAPCTL
This register configures input capture mode.
Name: Input capture mode control
Address Offset: 0x74
Table 6–19 CAPCTL Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:18 | Reserved and read as zero | Reserved and read as zero | Reserved and read as zero |
| 17:16 | cap5event | R/W | Capture5 edge event mode (group5-channel10) 00: posedge edge 01 : negedge edge 10 : Reserved and read as zero 11 : both edge |
| 15:14 | cap4event | R/W | Capture4 edge event mode (group4-channel8) 00: posedge edge 01 : negedge edge 10 : Reserved and read as zero 11 : both edge |
| 13:12 | cap3event | R/W | Capture3 edge event mode (group3-channel6) 00: posedge edge 01 : negedge edge 10 : Reserved and read as zero 11 : both edge |
| 11:10 | cap2event | R/W | Capture2 edge event mode (group2-channel4) 00: posedge edge 01 : negedge edge 10 : Reserved and read as zero 11 : both edge |
| 9:8 | cap1event | R/W | Capture1 edge event mode (group1-channel2) 00: posedge edge 01 : negedge edge 10 : Reserved and read as zero 11 : both edge |
| 7:6 | cap0event | R/W | Capture0 edge event mode (group0-channel0) 00: posedge edge 01 : negedge edge 10 : Reserved and read as zero 11 : both edge |
| 5 | cap5mode | R/W | Capture5 mode (group5-channel10) 1 : edge count mode 0 : edge time mode |
| 4 | cap4mode | R/W | Capture4 mode (group4-channel8) 1 : edge count mode 0 : edge time mode |
| 3 | cap3mode | R/W | Capture3 mode (group3-channel6) 1 : edge count mode 0 : edge time mode |
| 2 | cap2mode | R/W | Capture2 mode (group2-channel4) 1 : edge count mode 0 : edge time mode |
| 1 | cap1mode | R/W | Capture1 mode (group1-channel2) 1 : edge count mode 0 : edge time mode |
| 0 | cap0mode | R/W | Capture0 mode (group0-channel0) 1 : edge count mode 0 : edge time mode |

CAPINTEN
This register enables the interrupt for input capture function mode.
Name: interrupt enable for input capture mode
Address Offset: 0x78
Table 6–20 CAPINTEN Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:12 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 11 | cap5timie | R/W | Capture5 edge time interrupt enable (group5-channel10) 1 : interrupt enable 0 : interrupt disable |
| 10 | cap4timie | R/W | Capture4 edge time interrupt enable (group4-channel8) 1 : interrupt enable 0 : interrupt disable |
| 9 | cap3timie | R/W | Capture3 edge time interrupt enable (group3-channel6) 1 : interrupt enable 0 : interrupt disable |
| 8 | cap2timie | R/W | Capture2 edge time interrupt enable (group2-channel4) 1 : interrupt enable 0 : interrupt disable |
| 7 | cap1timie | R/W | Capture1 edge time interrupt enable (group1-channel2) 1 : interrupt enable 0 : interrupt disable |
| 6 | cap0timie | R/W | Capture0 edge time interrupt enable  (group0-channel0) 1 : interrupt enable 0 : interrupt disable |
| 5 | cap5cntie | R/W | Capture5 edge count interrupt enable (group5-channel10) 1 : interrupt enable 0 : interrupt disable |
| 4 | cap4cntie | R/W | Capture4 edge count interrupt enable (group4-channel8) 1 : interrupt enable 0 : interrupt disable |
| 3 | cap3cntie | R/W | Capture3 edge count interrupt enable (group3-channel6) 1 : interrupt enable 0 : interrupt disable |
| 2 | cap2cntie | R/W | Capture2 edge count interrupt enable (group2-channel4) 1 : interrupt enable 0 : interrupt disable |
| 1 | cap1cntie | R/W | Capture1 edge count interrupt enable (group1-channel2) 1 : interrupt enable 0 : interrupt disable |
| 0 | cap0cntie | R/W | Capture0 edge count interrupt enable (group0-channel0) 1 : interrupt enable 0 : interrupt disable |

CAPRIS
This register shows the state of raw interrupt for input capture function mode.
Name: raw interrupt status for input capture mode
Address Offset: 0x7c
Table 6–21 CAPRIS Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:12 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 11 | cap5timris | RO | Capture5 edge time raw interrupt occurs (group5-channel10) 1 : interrupt 0 : no interrupt |
| 10 | cap4timris | RO | Capture4 edge time raw interrupt occurs (group4-channel8) 1 : interrupt 0 : no interrupt |
| 9 | cap3timris | RO | Capture3 edge time raw interrupt occurs (group3-channel6) 1 : interrupt 0 : no interrupt |
| 8 | cap2timris | RO | Capture2 edge time raw interrupt occurs (group2-channel4) 1 : interrupt 0 : no interrupt |
| 7 | cap1timris | RO | Capture1 edge time raw interrupt occurs (group1-channel2) 1 : interrupt 0 : no interrupt |
| 6 | cap0timris | RO | Capture0 edge time raw interrupt occurs (group0-channel0) 1 : interrupt 0 : no interrupt |
| 5 | cap5cntris | RO | Capture5 edge count raw interrupt occurs (group5-channel10) 1 : interrupt 0 : no interrupt |
| 4 | cap4cntris | RO | Capture4 edge count raw interrupt occurs (group4-channel8) 1 : interrupt 0 : no interrupt |
| 3 | cap3cntris | RO | Capture3 edge count raw interrupt occurs (group3-channel6) 1 : interrupt 0 : no interrupt |
| 2 | cap2cntris | RO | Capture2 edge count raw interrupt occurs (group2-channel4) 1 : interrupt 0 : no interrupt |
| 1 | cap1cntris | RO | Capture1 edge count raw interrupt occurs (group1-channel2) 1 : interrupt 0 : no interrupt |
| 0 | cap0cntris | RO | Capture0 edge count raw interrupt occurs (group0-channel0) 1 : interrupt 0 : no interrupt |

CAPIC
This register clears internal interrupt.
Name: interrupt clear for input capture mode
Address Offset: 0x80
Table 6–22 CAPIC Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:12 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 11 | cap5timic | R/W | Clear Capture5 edge time interrupt (group5-channel10) 1 : clear interrupt 0 : not clear interrupt |
| 10 | cap4timic | R/W | Clear Capture4 edge time interrupt (group4-channel8) 1 : clear interrupt 0 : not clear interrupt |
| 9 | cap3timic | R/W | Clear Capture3 edge time interrupt (group3-channel6) 1 : clear interrupt 0 : not clear interrupt |
| 8 | cap2timic | R/W | Clear Capture2 edge time interrupt (group2-channel4) 1 : clear interrupt 0 : not clear interrupt |
| 7 | cap1timic | R/W | Clear Capture1 edge time interrupt (group1-channel2) 1 : clear interrupt 0 : not clear interrupt |
| 6 | cap0timic | R/W | Clear Capture0 edge time interrupt (group0-channel0) 1 : clear interrupt 0 : not clear interrupt |
| 5 | cap5cntic | R/W | Clear Capture5 edge count interrupt (group5-channel10) 1 : clear interrupt 0 : not clear interrupt |
| 4 | cap4cntic | R/W | Clear Capture4 edge count interrupt (group4-channel8) 1 : clear interrupt 0 : not clear interrupt |
| 3 | cap3cntic | R/W | Clear Capture3 edge count interrupt (group3-channel6) 1 : clear interrupt 0 : not clear interrupt |
| 2 | cap2cntic | R/W | Clear Capture2 edge count interrupt (group2-channel4) 1 : clear interrupt 0 : not clear interrupt |
| 1 | cap1cntic | R/W | Clear Capture1 edge count interrupt (group1-channel2) 1 : clear interrupt 0 : not clear interrupt |
| 0 | cap0cntic | R/W | Clear Capture0 edge count interrupt (group0-channel0) 1 : clear interrupt 0 : not clear interrupt |

CAPIS
This register shows the state of interrupt for input capture function mode.
Name: interrupt status for input capture mode
Address Offset: 0x84
Table 6–23 CAPIS Field Description

**Table:** (14 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:12 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 11 | cap5timis | RO | Capture5 edge time interrupt occurs (group5-channel10) 1 : interrupt 0 : no interrupt |
| 10 | cap4timis | RO | Capture4 edge time interrupt occurs (group4-channel8) 1 : interrupt 0 : no interrupt |
| 9 | cap3timis | RO | Capture3 edge time interrupt occurs (group3-channel6) 1 : interrupt 0 : no interrupt |
| 8 | cap2timis | RO | Capture2 edge time interrupt occurs (group2-channel4) 1 : interrupt 0 : no interrupt |
| 7 | cap1timis | RO | Capture1 edge time interrupt occurs (group1-channel2) 1 : interrupt 0 : no interrupt |
| 6 | cap0timis | RO | Capture0 edge time interrupt occurs (group0-channel0) 1 : interrupt 0 : no interrupt |
| 5 | cap5cntis | RO | Capture5 edge count interrupt occurs (group5-channel10) 1 : interrupt 0 : no interrupt |
| 4 | cap4cntis | RO | Capture4 edge count interrupt occurs (group4-channel8) 1 : interrupt 0 : no interrupt |
| 3 | cap3cntis | RO | Capture3 edge count interrupt occurs (group3-channel6) 1 : interrupt 0 : no interrupt |
| 2 | cap2cntis | RO | Capture2 edge count interrupt occurs (group2-channel4) 1 : interrupt 0 : no interrupt |
| 1 | cap1cntis | RO | Capture1 edge count interrupt occurs (group1-channel2) 1 : interrupt 0 : no interrupt |
| 0 | cap0cntis | RO | Capture0 edge count interrupt occurs (group0-channel0) 1 : interrupt 0 : no interrupt |

CAP01T
CAP23T
CAP45T
This register contains the counter value when edge match for input capture function mode.
Name: captured current value for input capture function mode
Address Offset: 0x88, 0x8c, 0x90
Table 6–24 CAPnmT Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | CAPnmTm | RO | Input capture m counter value (m=1 : group1-channel2 ; m=3 : group3-channel6 ; m=5 : group5-channel10) |
| 15:0 | CAPnmTn | RO | Input capture n counter value (n=0 : group1-channel0 ; n=2 : group3-channel4 ; n=4 : group5-channel8) |

CAP01MATCH
CAP23MATCH
CAP45MATCH
This register contains the match value for input capture function mode.
Name: capture match value for input capture function mode
Address Offset: 0x94, 0x98, 0x9c
Table 6–25 CAPnmMATCH Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | CAPnmMATCHm | R/W | Input capture m match value (m=1 : group1-channel2 ; m=3 : group3-channel6 ; m=5 : group5-channel10) |
| 15:0 | CAPnmMATCHn | R/W | Input capture n match value (n=0 : group1-channel0 ; n=2 : group3-channel4 ; n=4 : group5-channel8) |

TIMINTEN
This register configures the interrupt enable for timer function mode.
Name: interrupt enable for timer mode
Address Offset: 0xa0
Table 6–26 TIMINTEN Field Description

**Table:** (8 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | tim5ie | R/W | A interrupt enable when counter mathc the CMPnmMATCH value. (group5) 1 : interrupt enable 0 : interrupt disable |
| 4 | tim4ie | R/W | A interrupt enable when counter mathc the CMPnmMATCH value. (group4) 1 : interrupt enable 0 : interrupt disable |
| 3 | tim3ie | R/W | A interrupt enable when counter mathc the CMPnmMATCH value. (group3) 1 : interrupt enable 0 : interrupt disable |
| 2 | tim2ie | R/W | A interrupt enable when counter mathc the CMPnmMATCH value. (group2) 1 : interrupt enable 0 : interrupt disable |
| 1 | tim1ie | R/W | A interrupt enable when counter mathc the CMPnmMATCH value. (group1) 1 : interrupt enable 0 : interrupt disable |
| 0 | tim0ie | R/W | A interrupt enable when counter mathc the CMPnmMATCH value. (group0) 1 : interrupt enable 0 : interrupt disable |

TIMRIS
This register shows the state of raw internal interrupt for timer function mode.
Name: raw interrupt status for timer mode
Address Offset: 0xa4
Table 6–27 TIMRIS Field Description

**Table:** (8 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | tim5ris | RO | Timer5 raw interrupt occurs. (group5) 1 : interrupt 0 : no interrupt |
| 4 | tim4ris | RO | Timer4 raw interrupt occurs. (group4) 1 : interrupt 0 : no interrupt |
| 3 | tim3ris | RO | Timer3 raw interrupt occurs. (group3) 1 : interrupt 0 : no interrupt |
| 2 | tim2ris | RO | Timer2 raw interrupt occurs. (group2) 1 : interrupt 0 : no interrupt |
| 1 | tim1ris | RO | Timer1 raw interrupt occurs. (group1) 1 : interrupt 0 : no interrupt |
| 0 | tim0ris | RO | Timer0 raw interrupt occurs. (group0) 1 : interrupt 0 : no interrupt |

TIMIC
This register shows the interrupt clear for timer function mode.
Name: interrupt clear for timer mode
Address Offset: 0xa8
Table 6–28 TIMIC Field Description

**Table:** (8 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | tim5ic | R/W | Timer5 interrupt. (group5) 1 : clear interrupt 0 : not clear interrupt |
| 4 | tim4ic | R/W | Timer4 interrupt. (group4) 1 : clear interrupt 0 : not clear interrupt |
| 3 | tim3ic | R/W | Timer3 interrupt. (group3) 1 : clear interrupt 0 : not clear interrupt |
| 2 | tim2ic | R/W | Timer2 interrupt. (group2) 1 : clear interrupt 0 : not clear interrupt |
| 1 | tim1ic | R/W | Timer1 interrupt. (group1) 1 : clear interrupt 0 : not clear interrupt |
| 0 | tim0ic | R/W | Timer0 interrupt. (group0) 1 : clear interrupt 0 : not clear interrupt |

TIMIS
This register shows the status of interrupt for timer function mode.
Name: interrupt status for timer mode
Address Offset: 0xac
Table 6–29 TIMIS Field Description

**Table:** (8 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:6 | Reserved and read as zero. | Reserved and read as zero. | Reserved and read as zero. |
| 5 | tim5is | RO | Timer5 interrupt occurs. (group5) 1 : interrupt 0 : no interrupt |
| 4 | tim4is | RO | Timer4 interrupt occurs. (group4) 1 : interrupt 0 : no interrupt |
| 3 | tim3is | RO | Timer3 interrupt occurs. (group3) 1 : interrupt 0 : no interrupt |
| 2 | tim2is | RO | Timer2 interrupt occurs. (group2) 1 : interrupt 0 : no interrupt |
| 1 | tim1is | RO | Timer1 interrupt occurs. (group1) 1 : interrupt 0 : no interrupt |
| 0 | tim0is | RO | Timer0 interrupt occurs. (group0) 1 : interrupt 0 : no interrupt |

TIM01LOAD
TIM23LOAD
TIM45LOAD
This register contains the load value for timer function mode.
Name: load value for timer function mode
Address Offset: 0xb0, 0xb4, 0xb8
Table 6–30 TIMnmLOAD Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | timloadm | R/W | Timer m load value (m=1 : group1 ; m=3 : group3; m=5 : group5) |
| 15:0 | timloadn | R/W | Timer n load value (n=0 : group1 ; n=2 : group3 ; n=4 : group5) |

TIM01COUNT
TIM23COUNT
TIM45COUNT
This register contains the current value for timer function mode.
Name: current value for timer function mode
Address Offset: 0xbc, 0xc0, 0xc4
Table 6–31 TIMnmCOUNT Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | timcntm | RO | Timer m current count value (m=1 : group1 ; m=3 : group3; m=5 : group5) |
| 15:0 | timcntn | RO | Timer n current count value (n=0 : group1 ; n=2 : group3 ; n=4 : group5) |

CNT01VAL
CNT23VAL
CNT45VAL
This register contains the captured input pulse number value for input capture function mode.
Name: captured value for input capture function mode
Address Offset: 0xc8, 0xcc, 0xd0
Table 6–32 CNTnmVAL Field Description

**Table:** (3 rows, 4 cols)

| Bits | Name | R/W | Description |
|---|---|---|---|
| 31:16 | Cntmval | RO | Counter m captured input pulse number value (m=1 : group1 ; m=3 : group3; m=5 : group5) |
| 15:0 | Cntnval | RO | Counter n captured input pulse number value (n=0 : group1 ; n=2 : group3 ; n=4 : group5) |


### Work Flow
The following example shows how to initialize the PWM generator 0 with a 25-KHz frequency, and with a 75% duty cycles on the PWM0 pin and a 25% duty on the PWM1 pin. This example assumes the bus clock is 40-MHz.
Select the PWM count mode, for example, write 0x0 to PWMCTL (count up mode)
Set the PWM period. For a 25-KHz frequency, the period = 40us. The APB clock is 40-MHz and divided by 2, so that the PWM clock source is 20-MHz. This translates to 800 clock ticks per period. Use this value to set PWM01LOAD register. Write 0x320 to PWM01LOAD[31:16] and PWM01LOAD[15:0].
Set the pulse width of the PWM0 pin for a 75% duty cycle. Write 0xc8 to PWM0CMP[15:0].
Set the pulse width of the PWM1 pin for a 25% duty cycle. Write 0x258 to PWM0CMP[31:16].
Bypass Dead-Band insert, write 0x0 to PWM01DB.
Set the PWM clock frequency division and start the counter and enable output PWM signal, write 0x8000003 to PWMCFG.
