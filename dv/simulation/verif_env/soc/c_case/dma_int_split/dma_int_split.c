/*
 * DMA F6: 4 类中断 mask/status/clear 独立性
 * RTL 实证前提（dmac.v chregc）：
 *  - INT_STATUS 为 raw 状态：由 fsmc 事件直接置位，不经 mask/int_en 门控
 *    （status 置位见 :4030-4073，门控只在 vic 输出 chnregc_gbc_chiif :4089）
 *  - INT_CLEAR 每 bit 独立清对应 status
 *  - INT_MASK 读回 5 bit（{27'h0, maskpend, ...} :4016），bit4=maskpend
 *    为 UG 未文档化位
 * 步骤：
 *  S1: MASK=0x00（全屏蔽）传输 → status 仍 = 0xE（raw 不被 mask 门控）
 *  S2: 逐 bit 清：clear bit1 → 0xC；clear bit2 → 0x8；clear bit3 → 0x0
 *  S3: INT_EN=0（CTRLB bit0=0）传输 → status 仍 = 0xE（不被 int_en 门控）
 *  S4: MASK 写 0x1f 读回应为 0x1f（5 bit 存在，含 UG 未文档化的 maskpend）
 * 说明：error 中断（statusErr，仅 HRESP error 触发，:15755）依赖总线错误
 *      注入，本用例不覆盖。
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define CH0      0x40000000
#define DMACCFG  0x4000033C
#define SRC      0x00005000
#define DST      0x20025000

static void prep_mem(void)
{
    int i;
    for (i = 0; i < 8; i++)
        mem_write32_(SRC + i * 4, 0x5A5A5A00 + i);
    for (i = 0; i < 8; i++)
        mem_write32_(DST + i * 4, 0);
}

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）（空 for 循环会被
   整个删除）。触发后先延时让 DMAC 在无 S6 轮询压力下完成，再慢速轮询
   （RTL 实证：CPU 轮询 S6 会严重饥饿 DMAC M3 端口）。 */
static void dma_delay(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        __asm__ volatile ("nop");   /* 纯取指执行，零数据总线流量，-O3 不可删除 */
}

static void cfg_and_go(unsigned int ctrlb, unsigned int mask)
{
    unsigned int v;
    unsigned int timeout;
    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x2300A);
    mem_write32_(CH0 + 0x0C, ctrlb);
    mem_write32_(CH0 + 0x10, mask);
    mem_write32_(CH0 + 0x20, 0x1);
    mem_write32_(DMACCFG, 0x1);
    mem_write32_(CH0 + 0x1C, 0x1);
    dma_delay(5000);
    timeout = 0;
    do {
        mem_read32_(CH0 + 0x14, &v);
        if (++timeout >= 2000) { printf("ERR: transfer timeout, status=0x%x\n", v); sim_fail(); }
        dma_delay(200);
    } while ((v & 0x2) == 0);   /* 等 statustfr */
}

int test_start(void)
{
    unsigned int v;

    printf("\nstart dma_int_split\n");
    mem_write32_(DMACCFG, 0x1);

    /* S1: 全屏蔽传输，status 仍应置位（raw） */
    prep_mem();
    cfg_and_go(0x5, 0x00);
    mem_read32_(CH0 + 0x14, &v);
    if (v != 0xe) { printf("ERR S1: status=0x%x expect 0xE (raw, unmasked)\n", v); sim_fail(); }
    printf("S1 pass: INT_STATUS=0xE with INT_MASK=0 (raw status not gated by mask)\n");

    /* S2: 逐 bit 独立清除
       注意（实测）：INT_CLEAR 写 → status 生效有 3 拍寄存器延迟
       （we→clearbit→status），AHB 流水线使紧随其后的读在数据相位采到
       清除前的旧值。每次清除后插 asm-nop 延时再读回。 */
    mem_write32_(CH0 + 0x18, 0x2);           /* cleartfr */
    dma_delay(50);
    mem_read32_(CH0 + 0x14, &v);
    if (v != 0xc) { printf("ERR S2a: status=0x%x expect 0xC\n", v); sim_fail(); }
    mem_write32_(CH0 + 0x18, 0x4);           /* clearhtfr */
    dma_delay(50);
    mem_read32_(CH0 + 0x14, &v);
    if (v != 0x8) { printf("ERR S2b: status=0x%x expect 0x8\n", v); sim_fail(); }
    mem_write32_(CH0 + 0x18, 0x8);           /* cleartrgetcmpfr */
    dma_delay(50);
    mem_read32_(CH0 + 0x14, &v);
    if (v != 0x0) { printf("ERR S2c: status=0x%x expect 0x0\n", v); sim_fail(); }
    printf("S2 pass: per-bit INT_CLEAR clears tfr/htfr/trgetcmpfr independently\n");

    /* S3: INT_EN=0 传输，status 仍应置位 */
    prep_mem();
    cfg_and_go(0x4, 0x1f);                   /* CTRLB bit0=0 → INT_EN=0 */
    mem_read32_(CH0 + 0x14, &v);
    if (v != 0xe) { printf("ERR S3: status=0x%x expect 0xE with INT_EN=0\n", v); sim_fail(); }
    mem_write32_(CH0 + 0x18, 0xf);           /* 清干净 */
    printf("S3 pass: INT_STATUS=0xE with INT_EN=0 (status not gated by int_en)\n");

    /* S4: INT_MASK 读回 5 bit（含 UG 未文档化的 bit4 maskpend）
       同样防 write→readback 竞态 */
    mem_write32_(CH0 + 0x10, 0x1f);
    dma_delay(50);
    mem_read32_(CH0 + 0x10, &v);
    if (v != 0x1f) { printf("ERR S4: INT_MASK readback=0x%x expect 0x1F\n", v); sim_fail(); }
    mem_write32_(CH0 + 0x10, 0x00);
    dma_delay(50);
    mem_read32_(CH0 + 0x10, &v);
    if (v != 0x00) { printf("ERR S4b: INT_MASK readback=0x%x expect 0\n", v); sim_fail(); }
    printf("S4 pass: INT_MASK has 5 writable bits (bit4=maskpend, undocumented in UG)\n");

    printf("dma_int_split test successfully\n");
    sim_end();
}
