/*
 * DMA F4 (C 侧): 双通道并发与优先级抢占
 *  ch1（低优先级）：1024 字节块，源区 B(0x5600) → 目的 B(0x2002A000)
 *  ch0（高优先级）：64 字节小块，源区 A(0x5500) → 目的 A(0x2002C000)
 * 序列：EN 两通道 → 触发 ch1 → 短延时后触发 ch0。
 * 期望：两通道均完成且数据正确；UVM 侧 soc_top_dma_dual_arb_test
 * 采样 m_haddr 验证 ch0 抢占（B 区传输中出现 A 区传输，A 完成后 B 继续）。
 * 注：wujian100_open_for_debug dmac.v:15717 注入 bug（cntr_blk 重载 '-'→'*'）
 * 使 32-bit 传输实际长度 = 4×配置值。ch1 取 1024B（bug 下 4KB）恰好仍落在
 * B 区分类范围（dst 0x2002A000-0x2002B000 / src 0x5600-0x6600）内，
 * 不触碰 ch0 的 A 区，保证仲裁观测不受长度 bug 干扰。
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define CH0      0x40000000
#define CH1      0x40000030
#define DMACFG   0x4000033C
#define SRCA     0x00005500
#define DSTA     0x2002C000
#define SRCB     0x00005600
#define DSTB     0x2002A000

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）（空 for 循环会被
   整个删除 —— 0918 实证）。触发后先延时再慢速轮询，避免 CPU 轮询 S6
   饥饿 DMAC M3 端口（RTL 实证）。 */
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

    printf("\nstart dma_dual_ch_arb\n");

    for (i = 0; i < 16; i++)                 /* ch0: 64 字节 */
        mem_write32_(SRCA + i * 4, 0xAAAA0000 + i);
    for (i = 0; i < 256; i++)                /* ch1: 1024 字节 */
        mem_write32_(SRCB + i * 4, 0xBBBB0000 + i);
    for (i = 0; i < 16; i++)
        mem_write32_(DSTA + i * 4, 0);
    for (i = 0; i < 256; i++)
        mem_write32_(DSTB + i * 4, 0);

    /* ch1: BLOCK_TL=1023(1024B)；bug 下实际写 4KB，恰在 B 区内 */
    mem_write32_(CH1 + 0x00, SRCB);
    mem_write32_(CH1 + 0x04, DSTB);
    mem_write32_(CH1 + 0x08, (1023 << 12) | 0xA);
    mem_write32_(CH1 + 0x0C, 0x5);
    mem_write32_(CH1 + 0x10, 0x1f);
    /* ch0: BLOCK_TL=63(64B) */
    mem_write32_(CH0 + 0x00, SRCA);
    mem_write32_(CH0 + 0x04, DSTA);
    mem_write32_(CH0 + 0x08, (63 << 12) | 0xA);
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);

    mem_write32_(CH1 + 0x20, 0x1);
    mem_write32_(CH0 + 0x20, 0x1);
    mem_write32_(DMACFG, 0x1);

    /* 先触发 ch1（低优先级），短延时后触发 ch0（高优先级，应抢占）。
       注意（RTL 实证）：CPU 在传输期间轮询 S6 会严重饥饿 DMAC M3 端口，
       因此全部用 volatile 延时制造 ch0 的触发窗口，不做中途读。 */
    mem_write32_(CH1 + 0x1C, 0x1);
    dma_delay(2000);                         /* 让 ch1 先跑若干拍 */
    mem_write32_(CH0 + 0x1C, 0x1);

    /* 长延时等待（4096B+64B 双通道传输），再慢速轮询 */
    dma_delay(40000);
    timeout = 0;
    do {
        mem_read32_(CH0 + 0x14, &v);
        if (++timeout >= 2000) { printf("ERR: ch0 timeout, status=0x%x\n", v); sim_fail(); }
        dma_delay(500);
    } while ((v & 0x2) == 0);
    timeout = 0;
    do {
        mem_read32_(CH1 + 0x14, &v);
        if (++timeout >= 2000) { printf("ERR: ch1 timeout, status=0x%x\n", v); sim_fail(); }
        dma_delay(500);
    } while ((v & 0x2) == 0);

    /* 数据校验 */
    for (i = 0; i < 16; i++) {
        mem_read32_(DSTA + i * 4, &v);
        if (v != 0xAAAA0000 + i) {
            printf("ERR: ch0 dst[%d]=0x%x expect 0x%x\n", i, v, 0xAAAA0000 + i);
            sim_fail();
        }
    }
    for (i = 0; i < 256; i++) {
        mem_read32_(DSTB + i * 4, &v);
        if (v != 0xBBBB0000 + i) {
            printf("ERR: ch1 dst[%d]=0x%x expect 0x%x\n", i, v, 0xBBBB0000 + i);
            sim_fail();
        }
    }

    printf("both channels completed with correct data\n");
    printf("dma_dual_ch_arb test successfully\n");
    sim_end();
}
