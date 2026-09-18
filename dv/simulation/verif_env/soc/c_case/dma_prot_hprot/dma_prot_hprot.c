/*
 * DMA F9 (C 侧): PROTCTL → AHB HPROT
 * CTRLB.PROTCTL[18:15] = 4'b1010 (0xA)，发起 36 字节 32-bit 传输；
 * DMAC master 端每次有效传输的 m_hprot 应等于 0xA
 * （dmac.v fsmc: chi_m_prot = chregc_fsmc_protctl）。
 * UVM 侧 soc_top_dma_prot_test 在 m_htrans[1]==1 期间采样 m_hprot。
 * C 侧负责配置、触发、数据校验。
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define CH0      0x40000000
#define DMACFG   0x4000033C
#define SRC      0x00005000
#define DST      0x20025000

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）；触发后先延时再慢速轮询，
   避免 CPU 轮询 S6 饥饿 DMAC M3 端口（RTL 实证）。 */
static void dma_delay(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        __asm__ volatile ("nop");   /* 纯取指执行，零数据总线流量，-O3 不可删除 */
}

int test_start(void)
{
    unsigned int v;
    unsigned int timeout;
    int i;

    printf("\nstart dma_prot_hprot\n");

    for (i = 0; i < 8; i++)
        mem_write32_(SRC + i * 4, 0x39393900 + i);
    for (i = 0; i < 8; i++)
        mem_write32_(DST + i * 4, 0);

    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x2300A);
    mem_write32_(CH0 + 0x0C, (0xA << 15) | 0x5);   /* PROTCTL=0xA, block trig, INT_EN */
    mem_write32_(CH0 + 0x10, 0x1f);
    mem_write32_(CH0 + 0x20, 0x1);
    mem_write32_(DMACFG, 0x1);
    mem_write32_(CH0 + 0x1C, 0x1);

    /* 触发后先延时让 DMAC 完成传输，再慢速轮询 */
    dma_delay(5000);
    timeout = 0;
    do {
        mem_read32_(CH0 + 0x14, &v);
        if (++timeout >= 2000) { printf("ERR: transfer timeout, status=0x%x\n", v); sim_fail(); }
        dma_delay(200);
    } while (v != 0xe);
    mem_write32_(CH0 + 0x18, 0xf);

    for (i = 0; i < 8; i++) {
        mem_read32_(DST + i * 4, &v);
        if (v != 0x39393900 + i) {
            printf("ERR: dst[%d]=0x%x expect 0x%x\n", i, v, 0x39393900 + i);
            sim_fail();
        }
    }

    printf("dma_prot_hprot test successfully\n");
    sim_end();
}
