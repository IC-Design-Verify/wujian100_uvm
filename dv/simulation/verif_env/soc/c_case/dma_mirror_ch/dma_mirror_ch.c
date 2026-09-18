/*
 * DMA 多通道镜像（ch1 / ch2 / ch15）：与 ch0 基线相同的最小传输序列，
 * 验证通道寄存器组译码（stride 0x30）与各通道独立搬运能力。
 * 同时校验非活跃通道 INT_STATUS 保持 0（无串扰）。
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define DMA_BASE 0x40000000
#define DMACFG   0x4000033C

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）；触发后先延时再慢速轮询，
   避免 CPU 轮询 S6 饥饿 DMAC M3 端口（RTL 实证）。 */
static void dma_delay(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        __asm__ volatile ("nop");   /* 纯取指执行，零数据总线流量，-O3 不可删除 */
}

static void run_ch(unsigned int chbase, unsigned int src, unsigned int dst,
                   unsigned int pattern, int chno)
{
    unsigned int v;
    unsigned int timeout;
    int i;

    for (i = 0; i < 8; i++)
        mem_write32_(src + i * 4, pattern + i);
    for (i = 0; i < 8; i++)
        mem_write32_(dst + i * 4, 0);

    mem_write32_(chbase + 0x00, src);
    mem_write32_(chbase + 0x04, dst);
    mem_write32_(chbase + 0x08, 0x2300A);     /* 36B, incr/incr, 32/32 */
    mem_write32_(chbase + 0x0C, 0x5);
    mem_write32_(chbase + 0x10, 0x1f);
    mem_write32_(chbase + 0x20, 0x1);
    mem_write32_(DMACFG, 0x1);
    mem_write32_(chbase + 0x1C, 0x1);

    dma_delay(5000);
    timeout = 0;
    do {
        mem_read32_(chbase + 0x14, &v);
        if (++timeout >= 2000) {
            printf("ERR: ch%d transfer timeout, status=0x%x\n", chno, v);
            sim_fail();
        }
        dma_delay(200);
    } while (v != 0xe);

    for (i = 0; i < 8; i++) {
        mem_read32_(dst + i * 4, &v);
        if (v != pattern + i) {
            printf("ERR: ch%d dst[%d]=0x%x expect 0x%x\n", chno, i, v, pattern + i);
            sim_fail();
        }
    }
    mem_write32_(chbase + 0x18, 0xf);         /* 清中断 */
    printf("ch%d transfer pass\n", chno);
}

int test_start(void)
{
    unsigned int v;

    printf("\nstart dma_mirror_ch\n");

    run_ch(DMA_BASE + 0x030, 0x00005200, 0x20026000, 0x1C1C1C00, 1);
    run_ch(DMA_BASE + 0x060, 0x00005300, 0x20027000, 0x2D2D2D00, 2);
    run_ch(DMA_BASE + 0x2D0, 0x00005400, 0x20028000, 0x3E3E3E00, 15);

    /* 无串扰检查：ch0 从未配置，INT_STATUS 应为 0 */
    mem_read32_(DMA_BASE + 0x14, &v);
    if (v != 0) {
        printf("ERR: ch0 INT_STATUS=0x%x, expect 0 (cross-channel interference)\n", v);
        sim_fail();
    }

    printf("dma_mirror_ch test successfully\n");
    sim_end();
}
