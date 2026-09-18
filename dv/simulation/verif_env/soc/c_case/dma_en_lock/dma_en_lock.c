/*
 * DMA F8: CH_EN 锁定与硬件自清
 *  1. EN=1 后写 SAR/DAR/CTRLA/CTRLB 应被忽略（dmac.v:3796/3821/3846/3925
 *     写使能带 ~chn_en），读回保持原值
 *  2. soft_req 触发传输完成（INT_STATUS==0xE）后 EN 由硬件自动清 0
 *     （dmac.v:15746 fsmc_regc_chnen_clr = blk_evtend）
 *  3. EN 清 0 后 SAR 恢复可写
 *  4. 数据搬运正确性顺带校验
 *
 * 注意（RTL 实证）：DMAC master 端口与 CPU 总线活动存在严重仲裁竞争，
 * CPU 在传输期间轮询 S6 寄存器可使 M3 长时间饥饿（实测 36B 传输 stall
 * 达 1.5ms）。因此触发后先 volatile 延时让 DMAC 在无竞争下完成，再慢速轮询。
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

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）（空 for 循环会被
   整个删除 —— 0918 实证：NOP 延时失效导致传输期轮询饥饿 DMAC M3）。
   触发后先延时让 DMAC 在无 S6 轮询压力下完成，再慢速轮询。 */
static void dma_delay(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        __asm__ volatile ("nop");   /* 纯取指执行，零数据总线流量，-O3 不可删除 */
}

static void wait_done(unsigned int chbase)
{
    unsigned int v;
    unsigned int timeout = 0;
    dma_delay(5000);
    mem_read32_(chbase + 0x14, &v);
    while (v != 0xe) {
        dma_delay(200);
        mem_read32_(chbase + 0x14, &v);
        if (++timeout >= 2000) { printf("ERR: dma timeout, status=0x%x\n", v); sim_fail(); }
    }
}

int test_start(void)
{
    unsigned int v;
    int i;

    printf("\nstart dma_en_lock\n");

    for (i = 0; i < 8; i++)
        mem_write32_(SRC + i * 4, 0x11111111 * (i + 1));
    for (i = 0; i < 8; i++)
        mem_write32_(DST + i * 4, 0);

    /* 配置 ch0: 36 字节, src/dst incr, 32-bit, block trigger, INT_EN */
    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x2300A);
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    mem_write32_(CH0 + 0x20, 0x1);      /* EN=1 → 锁定生效 */
    mem_write32_(DMACCFG, 0x1);

    /* S1: 锁定期间写 SAR/DAR/CTRLA/CTRLB 应被忽略 */
    mem_write32_(CH0 + 0x00, 0xDEADBEEF);
    mem_write32_(CH0 + 0x04, 0xDEADBEEF);
    mem_write32_(CH0 + 0x08, 0xDEADBEEF);
    mem_write32_(CH0 + 0x0C, 0xDEADBEEF);
    mem_read32_(CH0 + 0x00, &v);
    if (v != SRC)  { printf("ERR: SAR not locked, readback 0x%x\n", v);  sim_fail(); }
    mem_read32_(CH0 + 0x04, &v);
    if (v != DST)  { printf("ERR: DAR not locked, readback 0x%x\n", v);  sim_fail(); }
    mem_read32_(CH0 + 0x08, &v);
    if (v != 0x2300A) { printf("ERR: CTRLA not locked, readback 0x%x\n", v); sim_fail(); }
    mem_read32_(CH0 + 0x0C, &v);
    if (v != 0x5)  { printf("ERR: CTRLB not locked, readback 0x%x\n", v);  sim_fail(); }
    printf("S1 pass: SAR/DAR/CTRLA/CTRLB locked while EN=1\n");

    /* S2: 触发并完成传输 */
    mem_write32_(CH0 + 0x1C, 0x1);
    wait_done(CH0);

    /* S3: EN 硬件自清 */
    mem_read32_(CH0 + 0x20, &v);
    if (v != 0) { printf("ERR: EN not auto-cleared, readback 0x%x\n", v); sim_fail(); }
    printf("S2/S3 pass: transfer done, EN auto-cleared to 0\n");

    /* S4: EN=0 后 SAR 恢复可写 */
    mem_write32_(CH0 + 0x00, 0x00006000);
    mem_read32_(CH0 + 0x00, &v);
    if (v != 0x6000) { printf("ERR: SAR not writable after EN auto-clear, 0x%x\n", v); sim_fail(); }
    printf("S4 pass: SAR writable again after EN auto-clear\n");

    /* 数据校验 */
    for (i = 0; i < 8; i++) {
        mem_read32_(DST + i * 4, &v);
        if (v != 0x11111111 * (i + 1)) {
            printf("ERR: dst[%d] = 0x%x, expect 0x%x\n", i, v, 0x11111111 * (i + 1));
            sim_fail();
        }
    }

    printf("dma_en_lock test successfully\n");
    sim_end();
}
