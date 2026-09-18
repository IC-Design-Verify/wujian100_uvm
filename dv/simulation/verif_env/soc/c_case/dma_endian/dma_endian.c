/*
 * DMA F7: 大小端转换 SRCDTLGC(CTRLB[13]) / DSTDTLGC(CTRLB[14])
 * RTL 实证（dmac.v fsmc，32-bit 对齐路径）：
 *  - 读侧 SRCDTLGC=1：rdata_word = 字节反转（{:8} {m_hrdata[7:0],[15:8],[23:16],[31:24]}）
 *  - 写侧 DSTDTLGC=1：m_hwdata = buffrdata 字节反转
 * 32-bit 宽度 + 对齐地址下 4 组合期望：
 *  (0,0) 直通；(1,0) 反转；(0,1) 反转；(1,1) 双重反转=直通
 * 数据模式：src[i] = 0x11223344 + i*0x01010101（字节互异，反转可检出）
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

static unsigned int bswap(unsigned int x)
{
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
}

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）；触发后先延时再慢速轮询，
   避免 CPU 轮询 S6 饥饿 DMAC M3 端口（RTL 实证）。 */
static void dma_delay(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        __asm__ volatile ("nop");   /* 纯取指执行，零数据总线流量，-O3 不可删除 */
}

static void run_and_wait(void)
{
    unsigned int v;
    unsigned int timeout = 0;
    mem_write32_(CH0 + 0x20, 0x1);
    mem_write32_(DMACCFG, 0x1);
    mem_write32_(CH0 + 0x1C, 0x1);
    dma_delay(5000);
    do {
        mem_read32_(CH0 + 0x14, &v);
        if (++timeout >= 2000) { printf("ERR: transfer timeout, status=0x%x\n", v); sim_fail(); }
        dma_delay(200);
    } while ((v & 0x2) == 0);
    mem_write32_(CH0 + 0x18, 0xf);
}

int test_start(void)
{
    unsigned int v, exp;
    int i, round;
    /* (srcdtlgc, dstdtlgc) 4 组合；expected: 0=直通 1=反转 */
    static const unsigned int scfg[4] = {0, 1, 0, 1};
    static const unsigned int dcfg[4] = {0, 0, 1, 1};
    static const unsigned int rev [4] = {0, 1, 1, 0};

    printf("\nstart dma_endian\n");

    for (i = 0; i < 8; i++)
        mem_write32_(SRC + i * 4, 0x11223344 + i * 0x01010101);

    for (round = 0; round < 4; round++) {
        for (i = 0; i < 8; i++)
            mem_write32_(DST + i * 4, 0);

        mem_write32_(CH0 + 0x00, SRC);
        mem_write32_(CH0 + 0x04, DST);
        mem_write32_(CH0 + 0x08, 0x1F00A);          /* 32B, incr/incr, 32/32 */
        mem_write32_(CH0 + 0x0C, 0x5 | (scfg[round] << 13) | (dcfg[round] << 14));
        mem_write32_(CH0 + 0x10, 0x1f);
        run_and_wait();

        for (i = 0; i < 8; i++) {
            exp = 0x11223344 + i * 0x01010101;
            if (rev[round]) exp = bswap(exp);
            mem_read32_(DST + i * 4, &v);
            if (v != exp) {
                printf("ERR R%d: dst[%d]=0x%x expect 0x%x (srcdtlgc=%d dstdtlgc=%d)\n",
                       round, i, v, exp, scfg[round], dcfg[round]);
                sim_fail();
            }
        }
        printf("round %d (srcdtlgc=%d dstdtlgc=%d) pass\n", round, scfg[round], dcfg[round]);
    }

    printf("dma_endian test successfully\n");
    sim_end();
}
