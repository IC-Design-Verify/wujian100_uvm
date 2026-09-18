/*
 * DMA F12 (C 侧): 中断聚合 + VIC 路由
 * 配置 ch0 正常传输（INT_EN=1, MASK=0x1f 全不屏蔽）→ 传输完成时
 * dmac_vic_if 聚合中断拉高（status&mask&int_en, dmac.v:4089），
 * 经 core_top ip_cpu_int_vld[32] 送 VIC（core_top.v:552）。
 * C 侧轮询 status 确认事件后保持不清除一段时间（让 UVM 侧可靠采样
 * pad_vic_int_vld[32] 电平），最后清中断并校验数据。
 * UVM 侧：soc_top_dma_vic_test
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

    printf("\nstart dma_vic_route\n");

    for (i = 0; i < 8; i++)
        mem_write32_(SRC + i * 4, 0x76767600 + i);
    for (i = 0; i < 8; i++)
        mem_write32_(DST + i * 4, 0);

    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x2300A);
    mem_write32_(CH0 + 0x0C, 0x5);            /* block trigger + INT_EN=1 */
    mem_write32_(CH0 + 0x10, 0x1f);           /* 全不屏蔽 */
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

    /* status 保持置位一段时间，使 vic_if 电平窗口足够 UVM 采样 */
    for (i = 0; i < 2000; i++)
        mem_read32_(CH0 + 0x14, &v);

    /* 清中断 → vic_if 应回落（UVM 侧记录电平序列） */
    mem_write32_(CH0 + 0x18, 0xf);

    for (i = 0; i < 8; i++) {
        mem_read32_(DST + i * 4, &v);
        if (v != 0x76767600 + i) {
            printf("ERR: dst[%d]=0x%x expect 0x%x\n", i, v, 0x76767600 + i);
            sim_fail();
        }
    }

    printf("dma_vic_route test successfully\n");
    sim_end();
}
