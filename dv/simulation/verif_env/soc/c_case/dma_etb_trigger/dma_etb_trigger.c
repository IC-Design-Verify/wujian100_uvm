/*
 * DMA F5 ETB (C 侧): ETB 硬件触发
 * SoC 中 etb_dmacch0_trg 在 ahb_matrix_top.v:1022 被 tie-off 为 1'b0，
 * 本用例由 UVM 侧 soc_top_dma_etb_test 通过 force 驱动该触发输入。
 * C 侧：配置 ch0 + EN=1 + DMACEN=1，但不写 soft_req；长超时轮询
 * INT_STATUS，等待 UVM force 的 ETB 触发使传输完成，校验数据。
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

/* 延时：asm nop 不会被 -O3 -funroll-all-loops 优化掉（空 for 循环会被整个删除，0918 实证）。轮询间插延时：force 生效后传输与
   轮询并发，延时间隙保证 DMAC M3 端口能拿到总线（RTL 实证：连续 S6
   轮询会使 M3 饥饿）。 */
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

    printf("\nstart dma_etb_trigger\n");

    for (i = 0; i < 8; i++)
        mem_write32_(SRC + i * 4, 0xEB7B0000 + i);
    for (i = 0; i < 8; i++)
        mem_write32_(DST + i * 4, 0);

    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x2300A);
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    mem_write32_(CH0 + 0x20, 0x1);
    mem_write32_(DMACFG, 0x1);
    /* 不写 SOFT_REQ —— 等待 UVM force etb_dmacch0_trg */

    /* 不写 SOFT_REQ —— 等待 UVM force etb_dmacch0_trg。
       轮询间插 volatile 延时：force 生效后传输与轮询并发，延时间隙保证
       DMAC M3 端口能拿到总线（RTL 实证：连续 S6 轮询会使 M3 饥饿） */
    timeout = 0;
    do {
        mem_read32_(CH0 + 0x14, &v);
        if (++timeout >= 200000) {
            printf("ERR: ETB trigger timeout (no transfer without soft_req)\n");
            sim_fail();
        }
        dma_delay(200);
    } while ((v & 0x2) == 0);

    for (i = 0; i < 8; i++) {
        mem_read32_(DST + i * 4, &v);
        if (v != 0xEB7B0000 + i) {
            printf("ERR: dst[%d]=0x%x expect 0x%x\n", i, v, 0xEB7B0000 + i);
            sim_fail();
        }
    }

    printf("ETB-triggered transfer completed\n");
    printf("dma_etb_trigger test successfully\n");
    sim_end();
}
