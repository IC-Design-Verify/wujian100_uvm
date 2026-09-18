# SoC 级验证代码审查报告 (2026-09-18)

> 审查人：main（doc_review 降级接管——两次委派均未落盘交付物，
> 回复分别为推理片段与无交付物的简短 PASS；按 CCB 规则降级本地审查）
> 审查范围：Task #12 第一批 4 用例 + 1 UVM 类 + 1 include

## 结论：可合入（仿真实证 + 本地走查）

## 1. addr_misalign.c — misalign_handler naked asm — PASS

- 寄存器保存/恢复配对：x10/x11/x12 先入栈（sp-16），mret 前恢复、sp 还原 ✓
- mepc 推进：`lhu` 取故障指令低半字，`&3==3` 判 32 位指令 +4，否则 +2 ✓
  （rv32emc 含 C 扩展，压缩指令低 2bit ≠ 11；E902 实证有效）
- 仿真实证：S2 错位 word 读 mcause=4、S3 错位写 mcause=6、S4 错位 half 读
  mcause=4、S5 对齐对照无陷阱、S7 未译码读 mcause=5（load access fault）
  ——handler 记录的 mcause 全部命中预期，证明 mepc 推进正确（否则后续指令
  会执行错乱导致 FAIL 或挂死）
- `aligned(64)`：mtvec 基址对齐要求（E902 mtvec[1:0]=0 基址模式，且
  硬件要求基址 64B 对齐）✓

## 2. soc_top_intr_multi_test.svh — overlap/seen/clr 采样 — PASS

- seen/clr 模式与既有 vic_route 测试（wdt/rtc/gpio dfx）一致：
  先见过 1、后见 0 才算完整电平窗口 ✓
- overlap 仅在 [17]&&[25]&&[27] 同高时置位；C 侧 S3 保持窗口 3000 nop
  保证可采；50ns 采样周期远小于窗口 ✓
- 无误报路径：任一线未断言/未撤销/无重叠都会 uvm_error ✓
- uvm_hdl_read 路径 `tb_top.dut.x_cpu_top.pad_vic_int_vld` 与既有
  wdt/rtc/gpio_vic_route 测试逐字一致 ✓
- 仿真实证：INTR_MULTI 报告三线并发断言+独立清除，C 侧 S2/S4a/S4b/S4c 全过

## 3. hresp_slverr.c — PASS

- probe_read/probe_write 复用同款 handler（trap_count 差分法）✓
- S1/S2/S7 严格断言 mcause=5/7/5；S5/S6 为实测刻画步（观察 LS/APB dummy
  行为后按观察值收敛为"返回 0 无陷阱"）✓
- 仿真实证全 S 通过

## 4. bus_cpu_dma_concurrent.c — PASS

- DMA 寄存器序列与 dma_* 系列用例一致（SAR/DAR/CTRLA/CTRLB/INT_MASK/EN/
  DMACCFG/soft_req 顺序相同）✓
- 已含注入 bug 兼容说明（64B 配置实传 65 拍，源/目的 300B 富余预填，
  只校验前 64B）✓
- CPU 并发区 0x20028000 与 DMA 两区（0x20010000/0x20020000）不重叠 ✓
- 仿真实证全 S 通过

## 5. soc_top_testcase_pkg.svh — PASS

- 仅追加一行 `include "soc_top_intr_multi_test.svh"`，位置在 dma 之后、
  无重复、编译零错误 ✓

---

# 增补审查：intr_isr_basic.c（2026-09-19）

> 审查人：main（降级接管——doc_review 连续两次失败：上游 API error、
> 回复为推理片段且无交付物。按 batch-1 同款降级模式处理）

## 结论：可合入（PASS × 3 / 问题 × 0）

## 1. naked asm handler — PASS

- 仅使用 x10/x11/x12/sp 四个寄存器；x10/x11/x12 保存（sp-16 三槽）与
  恢复（mret 前）严格配对，sp 增减配对 ✓
- rv32emc 下 `la` 展开 lui+addi（目标寄存器 x11，无隐藏临时寄存器），
  异步中断语义下所触寄存器全在保存集内 ✓
- mret 不推进 mepc = 中断返回语义（区别于 addr_misalign 的异常跳过语义）；
  实证：ISR 进入 4 次后主程序 poll 循环正常退出、S4 正常执行 ✓

## 2. 向量表选址 0xA000 — PASS

- objdump 实测：.text=0x8eb9、.data 至 0x96f8、.bss 至 0x9730、
  linker RAND 区始于 0xeff0；表（256B）放 0xA000-0xA0FF，上下均留余量 ✓
- 实证：0x8000 时代 S1 打印乱码（砸中 .text），挪 0xA000 后打印干净、
  全用例 PASS；注释已固化"勿改回"警告 ✓

## 3. S4 停表→EOI 顺序 — PASS

- TIM user 模式到期自动重装 load（0x400 拍周期性重触发），仅读 EOI
  无法阻止下一次到期（实测 EOI-only 后 ISR 仍 +454 次）；先 CTRL=0x2
  停表、再 EOI 清残留 pending 后计数稳定 ✓
- S4 最终断言含 IntStatus==0 复核 ✓

## 4. 仿真实证（run6，UVM_CASE_PASS）

```
S1: CLIC vector table at 0xa000, mtvt installed
S2a diag: mtvec = 0x83 (mode bits expect 11)
S2 diag: CLICINTIE[16-19] word readback = 0x100
S2 diag: mstatus = 0x1808 (bit3 mie expect 1)
S3 pass: CPU entered ISR 4 times
S3 observation: mcause=0xb8000011 mepc=0x28b6   ← bit31=1 中断 + ID=17
S4 pass: ISR stopped after TIM1 EOI
```
