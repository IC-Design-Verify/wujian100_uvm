# wujian100_open 模块分析文档索引

基于 `wujian100_open/doc/wujian100_open Userguide v1.0.docx` 提取，结合 RTL（`wujian100_open/soc/`）交叉核对生成。每份文档包含：模块概述、结构分析（RTL 子模块层次）、端口列表、寄存器配置（Memory Map + 字段描述）、工作流程、Spec-RTL 差异、信息来源与自检结果。

| 文档 | 模块 | 对应 User Guide 章节 |
|------|------|----------------------|
| [system_overview_analysis.md](system_overview_analysis.md) | SoC 系统概览（地址映射/PAD/中断） | Ch.1 |
| [tim_analysis.md](tim_analysis.md) | Timer（TIM ×8） | Ch.2 |
| [dma_analysis.md](dma_analysis.md) | DMA（16 通道 AHB-Lite DMAC） | Ch.3 |
| [usi_analysis.md](usi_analysis.md) | USI（UART/I2C/SPI 三合一 ×3） | Ch.4 |
| [wdt_analysis.md](wdt_analysis.md) | Watchdog（WDT） | Ch.5 |
| [pwm_analysis.md](pwm_analysis.md) | PWM（6 组发生器 + 捕获 + 定时器） | Ch.6 |
| [rtc_analysis.md](rtc_analysis.md) | RTC（AOU 常开域） | Ch.7 |
| [gpio_analysis.md](gpio_analysis.md) | GPIO（32-bit） | Ch.8 |

## `_src/` 目录

- `userguide.txt` — User Guide docx 提取的纯文本（含表格），章节行号边界：System Overview 156-398 / TIM 399-518 / DMA 519-695 / USI 696-1114 / WDT 1115-1190 / PWM 1191-1858 / RTC 1859-1955 / GPIO 1956-2094
- `rtl_structure.json` — `parse_rtl_structure.py` 对 `wujian100_open/soc/` 的结构提取结果（注意：多数模块 ports 字段为空，端口信息以 RTL 源码为准）

相关寄存器文档：`doc_summary/` 下的各 IP `*_registers.md`。
