# wujian100_open SoC 验证需求文档

> 基于 wujian100_open Userguide v1.0 提取
> 生成日期: 2026-07-16

---

## 修订历史

| 版本 | 日期 | 描述 |
|------|------|------|
| v1.0 | 2026-07-16 | 初版，从 User Guide 提取验证需求 |

---

## 目录

1. [系统级验证需求](#1-系统级验证需求)
2. [Timer (TIM)](#2-timer-tim)
3. [Direct Memory Access (DMA)](#3-direct-memory-access-dma)
4. [Unified Serial Interface (USI)](#4-unified-serial-interface-usi)
5. [Watchdog (WDT)](#5-watchdog-wdt)
6. [Pulse Width Modulation (PWM)](#6-pulse-width-modulation-pwm)
7. [Real-Time Clock (RTC)](#7-real-time-clock-rtc)
8. [General-Purpose I/O (GPIO)](#8-general-purpose-io-gpio)

---

## 1. 系统级验证需求

### 1.1 地址映射验证

| 地址范围 | 用途 | 大小 | 验证要点 |
|----------|------|------|----------|
| 0x0000_0000 ~ 0x0000_7FFF | Internal SRAM (指令) | 64KB | 读写正确性 |
| 0x2000_0000 ~ 0x2002_FFFF | Internal SRAM (数据) | 192KB | 读写正确性 |
| 0x4000_0000 ~ 0x401F_FFFF | MAIN BUS 外设 | 2MB | 外设寄存器访问 |
| 0x5000_0000 ~ 0x5004_FFFF | APB0 外设 | 320KB | APB0 子系统访问 |
| 0x6000_0000 ~ 0x6004_FFFF | APB1 外设 | 320KB | APB1 子系统访问 |
| 0xE000_E000 ~ 0xE000_EFFF | TCIP | 4KB | TCIP 访问 |

### 1.2 外设地址映射

#### APB0 外设

| 外设 | 基地址 | 大小 | 验证要点 |
|------|--------|------|----------|
| TIM0 | 0x5000_0000 | 1KB | Timer 寄存器访问 |
| TIM2 | 0x5000_0400 | 1KB | Timer 寄存器访问 |
| TIM4 | 0x5000_0800 | 1KB | Timer 寄存器访问 |
| TIM6 | 0x5000_0C00 | 1KB | Timer 寄存器访问 |
| USI0 | 0x5002_8000 | 16KB | UART/I2C/SPI 模式 |
| USI2 | 0x5002_9000 | 16KB | UART/I2C/SPI 模式 |
| WDT | 0x5000_8000 | 16KB | Watchdog 功能 |
| PWM | 0x5001_C000 | 16KB | PWM 波形生成 |

#### APB1 外设

| 外设 | 基地址 | 大小 | 验证要点 |
|------|--------|------|----------|
| TIM1 | 0x6000_0000 | 1KB | Timer 寄存器访问 |
| TIM3 | 0x6000_0400 | 1KB | Timer 寄存器访问 |
| TIM5 | 0x6000_0800 | 1KB | Timer 寄存器访问 |
| TIM7 | 0x6000_0C00 | 1KB | Timer 寄存器访问 |
| USI1 | 0x6002_8000 | 16KB | UART/I2C/SPI 模式 |
| GPIO | 0x6001_8000 | 16KB | GPIO 输入/输出/中断 |
| RTC | 0x6000_4000 | 16KB | RTC 计数/匹配中断 |
| PMU | 0x6003_0000 | 16KB | 电源管理 (Dummy) |

#### DMA 地址

| 外设 | 基地址 | 大小 | 验证要点 |
|------|--------|------|----------|
| DMA | 0x4000_0000 | 16KB | 16 通道 DMA 传输 |

### 1.3 中断源验证

| 中断号 | 中断源 | 验证要点 |
|--------|--------|----------|
| 1 | CoreTim | Core timer 中断 |
| 16 | GPIO0 | GPIO 中断（32-bit 每 bit 可配） |
| 18:17 | TIM0[1:0] | Timer0 的两个 timer 中断 |
| 20:19 | TIM1[1:0] | Timer1 的两个 timer 中断 |
| 22:21 | TIM2[1:0] | Timer2 的两个 timer 中断 |
| 24:23 | TIM3[1:0] | Timer3 的两个 timer 中断 |
| 25 | PWM | PWM 中断 |
| 26 | RTC | RTC 匹配中断 |
| 27 | WDT | Watchdog 中断 |
| 28 | USI0 | USI0 中断 |
| 29 | USI1 | USI1 中断 |
| 30 | USI2 | USI2 中断 |
| 31 | PMU | PMU 中断 |
| 32 | DMAC0 | DMA 控制器中断 |
| 34:33 | TIM4[1:0] | Timer4 的两个 timer 中断 |
| 36:35 | TIM5[1:0] | Timer5 的两个 timer 中断 |
| 38:37 | TIM6[1:0] | Timer6 的两个 timer 中断 |
| 40:39 | TIM7[1:0] | Timer7 的两个 timer 中断 |

### 1.4 复位验证

| 复位信号 | 极性 | 描述 |
|----------|------|------|
| PAD_MCURST | active-low | SoC 系统复位，需验证复位后寄存器状态 |

---

## 2. Timer (TIM)

### 2.1 概述

- 共 8 个 Timer 实例：TIM0/2/4/6 挂在 APB0，TIM1/3/5/7 挂在 APB1
- 每个 Timer IP 包含两个独立的 32-bit 递减计数器：Timer1 和 Timer2
- 支持两种工作模式：free-running 和 user-defined count

### 2.2 寄存器列表

| 偏移 | 寄存器名 | 宽度 | 访问 | 复位值 | 描述 |
|------|---------|------|------|--------|------|
| 0x00 | TimerNLoadCount | 32 | R/W | 0 | Timer N 加载值 |
| 0x04 | TimerNCurrentValue | 32 | R | 0 | Timer N 当前值 |
| 0x08 | TimerNControlReg | 4 | R/W | 0 | Timer N 控制寄存器 |
| 0x0C | TimerN_int_clr | 1 | R | 0 | 读清零中断 |
| 0x10 | TimerNIntStatus | 1 | R | 0 | 中断状态 |

> Timer1 偏移 0x00-0x10，Timer2 偏移 0x14-0x24

### 2.3 Timer Control Register 位域

| 位 | 名称 | 描述 |
|----|------|------|
| [4] | HW_TRIG_EN | 0=disable, 1=enable 硬件触发 |
| [2] | INT_MASK | 0=未屏蔽, 1=屏蔽中断 |
| [1] | MODE_SEL | 0=free-running, 1=user-defined count |
| [0] | EN | 0=disable, 1=enable |

### 2.4 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| TIM-001 | 寄存器读写 | P0 | 验证 TimerNLoadCount/TimerNControlReg R/W 正确性 |
| TIM-002 | TimerNCurrentValue 只读 | P1 | 验证只读寄存器写入无效 |
| TIM-003 | Free-running 模式 | P0 | 加载计数值后使能，验证计数递减到 0 后产生中断 |
| TIM-004 | User-defined 模式 | P0 | 加载计数值后使能，验证计数到 0 后自动重载 |
| TIM-005 | 中断生成与清除 | P0 | 验证中断状态寄存器置位，读 int_clr 清零 |
| TIM-006 | 中断屏蔽 | P1 | 设置 INT_MASK=1，验证中断被屏蔽 |
| TIM-007 | 复位后状态 | P1 | 验证复位后寄存器回到默认值，计数器停止 |
| TIM-008 | 硬件触发 | P2 | 使能 HW_TRIG_EN，验证硬件触发启动计数 |
| TIM-009 | 多个 Timer 并发 | P1 | 同时使能同 IP 的 Timer1 和 Timer2，验证各自独立计数 |

---

## 3. Direct Memory Access (DMA)

### 3.1 概述

- 符合 AMBA2.0 AHB-lite 规范
- 支持最多 16 个通道（Channel 0 优先级最高）
- 每个通道仅支持 block trigger mode
- 可编程块传输大小，最大 4096 字节
- 源/目的地址支持递增、递减、不变三种模式
- 支持 8/16/32-bit 传输宽度
- 支持 Little/Big-endian 数据格式
- 支持 16 级优先级

### 3.2 通道基址

| 通道 | 基地址 | 偏移 |
|------|--------|------|
| Channel 0 | 0x4000_0000 | 0x00 |
| Channel 1 | 0x4000_0030 | 0x30 |
| Channel 2 | 0x4000_0060 | 0x60 |
| ... | ... | ... |
| Channel n | 0x4000_0000 + 0x30*n | 0x30*n |
| Channel 15 | 0x4000_02D0 | 0x2D0 |
| Global | 0x4000_0330 | 0x330 |

### 3.3 通道寄存器列表

| 偏移 | 寄存器 | 宽度 | 访问 | 描述 |
|------|--------|------|------|------|
| 0x00 | SARn | 32 | R/W | Channel n 源地址 |
| 0x04 | DARn | 32 | R/W | Channel n 目的地址 |
| 0x08 | CHn_CTRL_A | 32 | R/W | Channel n 控制寄存器 A |
| 0x0C | CHn_CTRL_B | 32 | R/W | Channel n 控制寄存器 B |
| 0x10 | CHn_INT_MASK | 32 | R/W | Channel n 中断屏蔽 |
| 0x14 | CHn_INT_STATUS | 32 | R | Channel n 中断状态 |
| 0x18 | CHn_INT_CLEAR | 32 | W | Channel n 中断清除 |
| 0x1C | CHn_SOFT_REQ | 32 | R/W | Channel n 软触发请求 |
| 0x20 | CHn_EN | 32 | R/W | Channel n 使能控制 |

### 3.4 CHn_CTRL_A 位域

| 位 | 名称 | 描述 |
|----|------|------|
| [11:0] | block_size | 块传输大小，最大 4096 字节 |
| [13:12] | src_width | 00=8bit, 01=16bit, 10=32bit |
| [15:14] | dst_width | 00=8bit, 01=16bit, 10=32bit |
| [17:16] | src_inc | 00=递增, 01=递减, 10=不变 |
| [19:18] | dst_inc | 00=递增, 01=递减, 10=不变 |
| [20] | src_lj | 0=Little, 1=Big endian |
| [21] | dst_lj | 0=Little, 1=Big endian |

### 3.5 CHn_CTRL_B 位域

| 位 | 名称 | 描述 |
|----|------|------|
| [0] | blk_trig | Block trigger mode |
| [1] | tfr_int_en | 块传输完成中断使能 |
| [2] | half_int_en | 半块传输中断使能 |
| [3] | err_int_en | 错误中断使能 |

### 3.6 全局寄存器

| 寄存器 | 基地址偏移 | 描述 |
|--------|-----------|------|
| CHSR | 0x330 + 0x8 | Channel 忙状态寄存器 |
| DMACCFG | 0x330 + 0xC | DMAC 配置寄存器 |

### 3.7 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| DMA-001 | 寄存器读写 | P0 | 验证所有通道寄存器 R/W 正确性 |
| DMA-002 | 单通道 SRAM→SRAM 传输 | P0 | 配置 Channel 0 从 ISRAM 搬移到 DSRAM，验证数据正确 |
| DMA-003 | 多通道并发传输 | P1 | 配置多通道同时传输，验证优先级和独立完成 |
| DMA-004 | 传输完成中断 | P0 | 使能 tfr_int_en，验证传输完成后产生中断 |
| DMA-005 | 半传输中断 | P1 | 使能 half_int_en，验证半块完成时产生中断 |
| DMA-006 | 错误中断 | P1 | 配置非法地址，验证 err_int 产生 |
| DMA-007 | 中断屏蔽 | P1 | 设置 INT_MASK，验证中断被屏蔽 |
| DMA-008 | 软件触发 | P0 | 配置完成后写 SOFT_REQ 触发传输 |
| DMA-009 | 地址递增/递减/不变 | P1 | 验证三种地址模式的数据搬移正确性 |
| DMA-010 | 传输宽度组合 | P1 | 测试 src_width/dst_width 的 9 种组合 |
| DMA-011 | 块大小边界测试 | P1 | 测试 block_size = 1, 2, ... 4096 字节 |
| DMA-012 | Endian 模式 | P2 | 验证 Little/Big-endian 传输 |
| DMA-013 | 传输中止 | P1 | 传输中 disable CHn_EN，验证 DMA 停止 |
| DMA-014 | 使能保护 | P1 | CHn_EN 置 1 后修改 SAR/DAR/CTRL，验证写保护 |

---

## 4. Unified Serial Interface (USI)

### 4.1 概述

- 三个 USI 实例：USI0/2 在 APB0，USI1 在 APB1
- 支持三种串行协议模式：UART、I2C、SPI
- 通过 MODE_SEL 寄存器选择工作模式
- 支持 FIFO 发送/接收
- 支持中断和 DMA 控制

### 4.2 PAD 端口映射

| PAD 信号 | USI0 | USI1 | USI2 | 功能 |
|----------|------|------|------|------|
| NSS/CS | PAD_USI0_NSS | PAD_USI1_NSS | PAD_USI2_NSS | Slave select |
| SCLK | PAD_USI0_SCLK | PAD_USI1_SCLK | PAD_USI2_SCLK | Serial clock |
| SD0 | PAD_USI0_SD0 | PAD_USI1_SD0 | PAD_USI2_SD0 | Serial data 0 |
| SD1 | PAD_USI0_SD1 | PAD_USI1_SD1 | PAD_USI2_SD1 | Serial data 1 |

### 4.3 寄存器列表

| 偏移 | 寄存器名 | 描述 |
|------|---------|------|
| 0x00 | USI_CTRL | USI 控制 |
| 0x04 | MODE_SEL | 模式选择 (UART=0/I2C=1/SPI=2) |
| 0x08 | TX_FIFO | 发送 FIFO 写入端口 |
| 0x0C | RX_FIFO | 接收 FIFO 读取端口 |
| 0x10 | FIFO_STA | FIFO 状态 |
| 0x14 | CLK_DIV0 | 时钟分频 0 |
| 0x18 | CLK_DIV1 | 时钟分频 1 |
| **UART 专用** | | |
| 0x1C | UART_CTRL | UART 控制 |
| 0x20 | UART_STA | UART 状态 |
| **I2C 专用** | | |
| 0x24 | I2C_MODE | I2C 模式配置 |
| 0x28 | I2C_ADDR | I2C 从机地址 |
| 0x2C | I2CM_CTRL | I2C 主机控制 |
| 0x30 | I2CM_CODE | I2C 主机状态码 |
| 0x34 | I2CS_CTRL | I2C 从机控制 |
| 0x38 | I2C_FM_DIV | I2C 快速模式分频 |
| 0x3C | I2C_HOLD | I2C 保持时间 |
| 0x40 | I2C_STA | I2C 状态 |
| **SPI 专用** | | |
| 0x44 | SPI_MODE | SPI 模式 |
| 0x48 | SPI_CTRL | SPI 控制 |
| 0x4C | SPI_STA | SPI 状态 |
| **共享中断/DMA** | | |
| 0x50 | INTR_CTRL | 中断控制 |
| 0x54 | INTR_EN | 中断使能 |
| 0x58 | INTR_STA | 中断状态（屏蔽后） |
| 0x5C | RAW_INTR_STA | 原始中断状态 |
| 0x60 | INTR_UNMASK | 未屏蔽中断状态 |
| 0x64 | INTR_CLR | 中断清除 |
| 0x68 | DMA_CTRL | DMA 控制 |
| 0x6C | DMA_THRESHOLD | DMA 阈值 |
| 0x70 | SPI_NSS_DATA | SPI NSS 数据控制 |

### 4.4 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| USI-001 | 寄存器读写 | P0 | 验证所有寄存器 R/W 正确性 |
| USI-002 | 模式切换 | P0 | 验证 MODE_SEL 在 UART/I2C/SPI 间切换 |
| USI-003 | UART 发送 | P0 | 配置 UART 模式，写 TX_FIFO 发送数据 |
| USI-004 | UART 接收 | P0 | 配置 UART 模式，接收 RX_FIFO 数据 |
| USI-005 | UART 波特率 | P1 | 验证 CLK_DIV0 配置不同波特率 |
| USI-006 | UART 数据位/停止位/校验 | P1 | 验证 UART_CTRL 的 data_bits/stop_bits/parity 配置 |
| USI-007 | I2C 主机写 | P0 | 配置 I2C Master 模式，发送写操作 |
| USI-008 | I2C 主机读 | P0 | 配置 I2C Master 模式，执行读操作 |
| USI-009 | I2C 从机模式 | P1 | 配置 I2C Slave 模式，响应主机请求 |
| USI-010 | I2C 传输速率 | P1 | 验证标准/快速模式时钟频率 |
| USI-011 | SPI 主机模式 | P0 | 配置 SPI Master 模式，发送/接收数据 |
| USI-012 | SPI 从机模式 | P1 | 配置 SPI Slave 模式，响应主机 |
| USI-013 | SPI 时钟极性/相位 | P1 | 验证 SPI_MODE 的 CPOL/CPHA 配置 |
| USI-014 | FIFO 满/空状态 | P1 | 验证 FIFO_STA 的满空标志和 FIFO 深度 |
| USI-015 | FIFO 溢出 | P1 | FIFO 满时继续写入，验证溢出处理 |
| USI-016 | 中断生成 | P1 | 验证 RX FIFO 超过阈值、TX FIFO 空中断 |
| USI-017 | DMA 传输 | P2 | 配置 DMA 与 USI 配合传输 |

---

## 5. Watchdog (WDT)

### 5.1 概述

- 32-bit 递减计数器
- 计数从预设值递减到 0 时触发超时
- 支持两种模式：系统复位 / 先中断后复位
- 复位脉冲宽度可编程 (2~256 pclk 周期)
- 超时周期可编程
- 使能后只能由系统复位清除

### 5.2 寄存器列表

| 偏移 | 寄存器 | 宽度 | 访问 | 复位值 | 描述 |
|------|--------|------|------|--------|------|
| 0x00 | WDT_CR | 5 | R/W | 0x02 | WDT 控制寄存器 |
| 0x04 | WDT_time_out | 8 | R/W | 0x00 | 超时范围寄存器 |
| 0x08 | WDT_current_value | 32 | R | 0xFFFF_FFFF | 当前计数器值 |
| 0x0C | WDT_CRR | 8 | W | 0x00 | 计数器重启 (需写 0x76) |
| 0x10 | WDT_int_status | 1 | R | 0 | 中断状态 |
| 0x14 | WDT_int_clr | 1 | R | 0 | 中断清除 (读清零) |

### 5.3 WDT_CR 位域

| 位 | 名称 | 描述 |
|----|------|------|
| [4:2] | RPL | 复位脉冲长度：000=2 ~ 111=256 pclk 周期 |
| [1] | RMOD | 0=直接复位, 1=先中断后复位 |
| [0] | WDT_EN | WDT 使能，置位后只能系统复位清除 |

### 5.4 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| WDT-001 | 寄存器读写 | P0 | 验证 WDT_CR/WDT_time_out R/W 正确性 |
| WDT-002 | 计数器递减 | P0 | 使能 WDT，验证 current_value 从加载值递减 |
| WDT-003 | 超时复位 | P0 | 设置 RMOD=0，超时后验证系统复位产生 |
| WDT-004 | 中断+复位模式 | P0 | 设置 RMOD=1，首次超时产生中断，第二次超时复位 |
| WDT-005 | Watchdog 喂狗 (Kick) | P0 | 写 0x76 到 WDT_CRR，验证计数器重启 |
| WDT-006 | Kick 保护验证 | P1 | 写非 0x76 值到 WDT_CRR，验证计数器不重启 |
| WDT-007 | 复位脉冲宽度 | P2 | 验证不同 RPL 值对应的复位脉冲长度 |
| WDT-008 | 中断清除 | P1 | 超时中断后读 WDT_int_clr，验证中断清除 |
| WDT-009 | 使能锁定 | P1 | WDT_EN 置位后写 0 到该位，验证清除无效（只能系统复位） |
| WDT-010 | 超时周期编程 | P1 | 验证 TOP_INIT/TOP 选择不同超时周期 |

---

## 6. Pulse Width Modulation (PWM)

### 6.1 概述

- 6 个 PWM 生成器 (Group 0~5)，每个包含 1 个 32-bit 计数器 + 2 个比较器
- 12 个输入/输出通道
- 每个 PWM 信号生成器包含 2 个通道
- 支持 Up 和 Up/Down 计数模式
- 输出频率由 16-bit 加载值控制
- 支持 32-bit 输入捕获模式（边沿计数和边沿时间）
- 支持可编程死区插入
- 支持输出极性控制
- 支持 ADC 触发信号生成
- 内部集成 Timer 功能模式

### 6.2 寄存器列表

#### 全局配置寄存器

| 偏移 | 寄存器 | 描述 |
|------|--------|------|
| 0x00 | PWMCFG | PWM 配置（输出使能/捕获/定时器/时钟分频） |
| 0x04 | PWMINVERTTRIG | 输出反相和 ADC 触发控制 |
| 0x08 | PWM01TRIG | Group 0/1 触发比较值 |
| 0x0C | PWM23TRIG | Group 2/3 触发比较值 |
| 0x10 | PWM45TRIG | Group 4/5 触发比较值 |

#### 中断寄存器

| 偏移 | 寄存器 | 描述 |
|------|--------|------|
| 0x14 | PWMINTEN1 | Group 2/1/0 中断使能 |
| 0x18 | PWMINTEN2 | Group 5/4/3 中断使能 |
| 0x1C | PWMRIS1 | Group 2/1/0 原始中断状态 |
| 0x20 | PWMRIS2 | Group 5/4/3 原始中断状态 |
| 0x24 | PWMIC1 | Group 2/1/0 中断清除 |
| 0x28 | PWMIC2 | Group 5/4/3 中断清除 |
| 0x2C | PWMIS1 | Group 2/1/0 中断状态 |
| 0x30 | PWMIS2 | Group 5/4/3 中断状态 |

#### 计数/比较寄存器

| 偏移 | 寄存器 | 描述 |
|------|--------|------|
| 0x34 | PWMCTL | PWM 控制（计数模式选择） |
| 0x38 | PWM01LOAD | Group 0/1 加载值 |
| 0x3C | PWM23LOAD | Group 2/3 加载值 |
| 0x40 | PWM45LOAD | Group 4/5 加载值 |
| 0x44 | PWM01COUNT | Group 0/1 当前计数值 |
| 0x48 | PWM23COUNT | Group 2/3 当前计数值 |
| 0x4C | PWM45COUNT | Group 4/5 当前计数值 |
| 0x50~0x64 | PWM0CMP~PWM5CMP | 6 个比较值寄存器 |
| 0x68~0x70 | PWM01DB~PWM45DB | Group 0/1 死区值 |

#### 输入捕获寄存器

| 偏移 | 寄存器 | 描述 |
|------|--------|------|
| 0x74 | CAPCTL | 输入捕获控制 |
| 0x78 | CAPINTEN | 捕获中断使能 |
| 0x7C | CAPRIS | 捕获原始中断状态 |
| 0x80 | CAPIC | 捕获中断清除 |
| 0x84 | CAPIS | 捕获中断状态 |
| 0x88~0x90 | CAP01T~CAP45T | 捕获计数值 |
| 0x94~0x9C | CAP01MATCH~CAP45MATCH | 捕获匹配值 |
| 0xC8~0xD0 | CNT01VAL~CNT45VAL | 捕获脉冲计数值 |

#### Timer 功能寄存器

| 偏移 | 寄存器 | 描述 |
|------|--------|------|
| 0xA0 | TIMINTEN | Timer 中断使能 |
| 0xA4 | TIMRIS | Timer 原始中断状态 |
| 0xA8 | TIMIC | Timer 中断清除 |
| 0xAC | TIMIS | Timer 中断状态 |
| 0xB0~0xB8 | TIM01LOAD~TIM45LOAD | Timer 加载值 |
| 0xBC~0xC4 | TIM01COUNT~TIM45COUNT | Timer 当前值 |

### 6.3 PWMCFG 位域

| 位 | 名称 | 描述 |
|----|------|------|
| [27] | cntdiven | 时钟分频使能 |
| [26:24] | cntdiv | 时钟分频选择 (000=÷2 ~ 111=÷128) |
| [23:18] | tim5en~tim0en | Group 5~0 输出比较使能 |
| [17:12] | cap5en~cap0en | Group 5~0 输入捕获使能 |
| [11:0] | pwm11en~pwm0en | Channel 11~0 PWM 输出使能 |

### 6.4 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| PWM-001 | 寄存器读写 | P0 | 验证所有寄存器 R/W 正确性 |
| PWM-002 | 基本波形生成 | P0 | 配置 Group 0 Up-count，设置 LOAD 和 CMP，验证 PWM 输出频率/占空比 |
| PWM-003 | 多通道输出 | P0 | 使能多个通道，验证独立输出 |
| PWM-004 | 占空比精度 | P1 | 验证不同 CMP 值的占空比控制精度 |
| PWM-005 | Up/Down 计数模式 | P1 | 配置 Up/Down 模式，验证对称 PWM 波形 |
| PWM-006 | 时钟分频 | P1 | 验证 cntdiv 不同分频比下的 PWM 频率 |
| PWM-007 | 死区插入 | P1 | 设置 PWM01DB，验证死区生效 |
| PWM-008 | 输出极性控制 | P1 | 配置 PWMINVERTTRIG，验证输出反相 |
| PWM-009 | 输入捕获-边沿计数 | P1 | 配置 CAPCTL，验证输入边沿计数 |
| PWM-010 | 输入捕获-边沿时间 | P2 | 验证捕获模式下计数值读取 |
| PWM-011 | ADC 触发 | P2 | 配置 TRIG 比较值，验证触发信号生成 |
| PWM-012 | PWM 中断 | P1 | 使能中断，验证计数匹配时中断产生 |
| PWM-013 | Timer 功能模式 | P2 | 验证 TIMINTEN/TIMRIS/TIMIC/TIMIS 等 Timer 功能 |
| PWM-014 | PWM 输出 disable | P1 | disable pwnen 后验证输出为高阻 |
| PWM-015 | 多 Group 同步 | P2 | 配置多个 Group，验证同步启动/停止 |

---

## 7. Real-Time Clock (RTC)

### 7.1 概述

- 32-bit 递增计数器
- 可编程匹配值，匹配时产生中断
- 可编程加载值
- 支持 Wrap 模式（匹配时归零重置）
- 可编程时钟分频
- 中断可屏蔽

### 7.2 寄存器列表

| 偏移 | 寄存器 | 宽度 | 访问 | 复位值 | 描述 |
|------|--------|------|------|--------|------|
| 0x00 | RTC_current_value | 32 | R | 0x0 | 当前计数值 |
| 0x04 | RTC_match_value | 32 | RW | 0x0 | 匹配值 |
| 0x08 | RTC_load_value | 32 | RW | 0x0 | 加载值 |
| 0x0C | RTC_CCR | 4~2 | RW | 0x0 | 控制寄存器 |
| 0x10 | RTC_int_status | 32 | R | 0x0 | 中断状态 |
| 0x14 | RTC_raw_int_status | 32 | R | 0x0 | 原始中断状态 |
| 0x18 | RTC_int_clr | 32 | R | 0x0 | 中断清除 |
| 0x1C | RTC_COMP_VERSION | 32 | R | 0x0 | 版本寄存器 |
| 0x20 | RTC_DIV | 20 | RW | 0x4000 | 时钟分频值 |

### 7.3 RTC_CCR 位域

| 位 | 名称 | 描述 |
|----|------|------|
| [3] | rtc_wen | Wrap 使能：0=disable, 1=匹配时归零重置 |
| [2] | Rtc_en | 计数器使能：0=disable, 1=enable |
| [1] | rtc_mask | 中断屏蔽：0=unmask, 1=mask |
| [0] | rtc_ien | 中断使能：0=disable, 1=enable |

### 7.4 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| RTC-001 | 寄存器读写 | P0 | 验证 RTC_match/load/DIV/CCR R/W 正确性 |
| RTC-002 | 只读寄存器验证 | P1 | 验证 RTC_current_value/int_status 只读 |
| RTC-003 | 基本计数功能 | P0 | 使能计数器，验证 current_value 递增计数 |
| RTC-004 | 匹配中断 | P0 | 设置 match_value，计数匹配后验证中断产生 |
| RTC-005 | 加载值 | P0 | 写 load_value，验证计数器加载并从该值开始计数 |
| RTC-006 | Wrap 模式 | P1 | 设置 WEN=1，验证匹配时计数器归零 |
| RTC-007 | 中断屏蔽 | P1 | 设置 rtc_mask=1，验证中断被屏蔽 |
| RTC-008 | 中断使能/禁止 | P1 | 设置 rtc_ien=0，验证中断不产生 |
| RTC-009 | 中断清除 | P1 | 读 RTC_int_clr，验证中断状态清零 |
| RTC-010 | 时钟分频 | P1 | 配置 RTC_DIV 不同值，验证计数速率变化 |

---

## 8. General-Purpose I/O (GPIO)

### 8.1 概述

- 32-bit 宽度
- 每个 bit 独立控制方向和输出值
- 每个 bit 支持中断生成（电平/边沿触发，高/低有效可选）
- 支持软件模式和硬件模式选择
- 中断可屏蔽
- 连接 APB1 总线

### 8.2 寄存器列表

| 偏移 | 寄存器 | 宽度 | 访问 | 描述 |
|------|--------|------|------|------|
| 0x00 | gpio_output_data | 32 | R/W | GPIO 输出数据寄存器 |
| 0x04 | gpio_direction | 32 | R/W | GPIO 方向 (0=输入, 1=输出) |
| 0x08 | gpio_ctl | 32 | R/W | GPIO 数据源 (0=软件, 1=硬件) |
| 0x30 | gpio_inten | 32 | R/W | 中断使能 |
| 0x34 | gpio_intmask | 32 | R/W | 中断屏蔽 (1=mask) |
| 0x38 | gpio_inttype_level | 32 | R/W | 中断类型 (0=电平, 1=边沿) |
| 0x3C | gpio_int_polarity | 32 | R/W | 中断极性 (0=低/下降沿, 1=高/上升沿) |
| 0x40 | gpio_intstatus | 32 | R | 中断状态 (屏蔽后) |
| 0x44 | gpio_rawintstatus | 32 | R | 原始中断状态 (屏蔽前) |
| 0x4C | gpio_int_clr | 32 | W | 中断清除 (写 1 清零) |
| 0x50 | gpio_input_data | 32 | R | GPIO 输入数据 |

### 8.3 验证场景

| 编号 | 场景 | 优先级 | 描述 |
|------|------|--------|------|
| GPIO-001 | 寄存器读写 | P0 | 验证 output_data/direction/ctl/inten/intmask 等 R/W 正确性 |
| GPIO-002 | 输出模式 | P0 | 配置 direction=output，写 output_data，验证引脚输出 |
| GPIO-003 | 输入模式 | P0 | 配置 direction=input，从引脚施加电平，验证 input_data |
| GPIO-004 | 软件/硬件模式切换 | P1 | 配置 ctl 选择软件/硬件源，验证数据路径切换 |
| GPIO-005 | 电平中断 | P0 | 配置 inttype_level=0，施加有效电平，验证中断产生 |
| GPIO-006 | 边沿中断 | P0 | 配置 inttype_level=1，施加边沿跳变，验证中断产生 |
| GPIO-007 | 中断极性 | P1 | 验证 active-low / active-high / 上升沿 / 下降沿 |
| GPIO-008 | 中断屏蔽 | P1 | 设置 intmask=1，验证中断被屏蔽 |
| GPIO-009 | 中断清除 | P1 | 写 int_clr，验证边缘中断被清除 |
| GPIO-010 | 多 bit 独立控制 | P1 | 验证每个 bit 方向/输出/中断独立配置 |
| GPIO-011 | 原始中断 vs 屏蔽中断 | P2 | 验证 raw_intstatus 和 intstatus 的区别 |

---

## 附录：验证优先级定义

| 优先级 | 定义 | 预期覆盖率 |
|--------|------|-----------|
| **P0** | 基本功能，必须通过 | Line/Cond/Branch > 95% |
| **P1** | 扩展功能，建议覆盖 | Line/Cond > 80% |
| **P2** | 高级/边界场景，有条件覆盖 | 视项目进度决定 |

## 附录：全局验证清单

### 所有模块共享

- [ ] 寄存器默认值检查（复位后所有寄存器值是否符合规格）
- [ ] 寄存器读写检查（R/W、RO、WO 是否正确）
- [ ] 保留位读取为 0 检查
- [ ] 中断产生与清除流程
- [ ] 时钟使能/关闭时寄存器访问

### 系统级

- [ ] 地址映射正确性（每个外设的基地址是否正确）
- [ ] 中断号与中断源的对应关系
- [ ] 复位后所有外设处于禁用状态
- [ ] 跨模块交互（如 DMA+USI 联合传输）
