/*
 * SoC F2/F11: CPU 与 DMAC 总线并发 —— 仲裁下双方数据完整性
 *
 * 原理：DMAC M3 与 CPU M0/M1/M2 并发访问 MAIN matrix。CPU 在 DMA 传输进行
 * 中持续对另一片 DSRAM 做密集读写（验证 CPU 侧不被饿死且数据正确），
 * DMA 侧做 256 字节搬运（验证 DMA 侧数据正确）。
 *
 * 注意（DMA 注入 bug 兼容）：dmac.v:15717 使 beats=BLOCK_TL+2，
 * 64B 配置(block_tl=63)在 32-bit 宽度下实传 65 拍 = 260B，src/dst 预填
 * 300B 富余。本用例只校验前 64B（配置长度），越界区由 dma_tr_width/
 * dma_addr_mode 专项刻画。
 *
 * 检查：
 *  S1: DMA 进行中 CPU 对 DSRAM-B 做 512 次读写测试，全部正确
 *      （CPU 不被 M3 饿死的证据：若仲裁出错 CPU 会读到脏数据或挂死）
 *  S2: DMA 完成后前 64B 与源一致（DMA 不被 CPU 干扰的证据）
 *  S3: 全过程无总线错误（statusErr bit0 不置位）
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define CH0      0x40000000
#define DMACCFG  0x4000033C
#define DMA_SRC  0x20010000
#define DMA_DST  0x20020000
#define CPU_AREA 0x20028000        /* CPU 并发工作区，与 DMA 两区均不重叠 */

int test_start(void)
{
    unsigned int v, status;
    unsigned int timeout;
    int i;

    printf("\nstart bus_cpu_dma_concurrent\n");

    /* 源 300B 富余预填（含 bug 越界区），目的 300B 清零 */
    for (i = 0; i < 75; i++)
        mem_write32_(DMA_SRC + i * 4, 0xC0DE0000 + i);
    for (i = 0; i < 75; i++)
        mem_write32_(DMA_DST + i * 4, 0);

    /* 配置 DMA：64B, incr/incr, 32/32, INT_MASK 全屏蔽（轮询 raw status） */
    mem_write32_(CH0 + 0x00, DMA_SRC);
    mem_write32_(CH0 + 0x04, DMA_DST);
    mem_write32_(CH0 + 0x08, 0x3F00A);     /* BLOCK_TL=63(64B), 32/32-bit */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    mem_write32_(CH0 + 0x20, 0x1);
    mem_write32_(DMACCFG, 0x1);
    mem_write32_(CH0 + 0x1C, 0x1);         /* soft_req 触发 */

    /* S1: DMA 传输进行中，CPU 立即开始密集访问另一片 DSRAM。
       不做任何延时 —— 刻意让 CPU 流量与 DMA 并发竞争 matrix */
    for (i = 0; i < 512; i++) {
        mem_write32_(CPU_AREA + (i % 64) * 4, 0x5A000000 + i);
        mem_read32_(CPU_AREA + (i % 64) * 4, &v);
        if (v != 0x5A000000 + i) {
            printf("ERR S1: CPU concurrent access corrupted at iter %d: 0x%x\n", i, v);
            sim_fail();
        }
    }
    printf("S1 pass: CPU 512 concurrent R/W ops all correct during DMA transfer\n");

    /* 等 DMA 完成（慢速轮询） */
    timeout = 0;
    do {
        mem_read32_(CH0 + 0x14, &status);
        if (++timeout >= 500000) {
            printf("ERR: DMA timeout under CPU concurrency, status=0x%x\n", status);
            sim_fail();
        }
    } while ((status & 0x2) == 0);

    /* S3: 无总线错误 */
    if (status & 0x1) {
        printf("ERR S3: statusErr set under concurrency, status=0x%x\n", status);
        sim_fail();
    }
    printf("S3 pass: no bus error (statusErr=0) under concurrency\n");
    mem_write32_(CH0 + 0x18, 0xf);

    /* S2: DMA 数据完整性（前 64B = 配置长度） */
    for (i = 0; i < 16; i++) {
        mem_read32_(DMA_DST + i * 4, &v);
        if (v != 0xC0DE0000 + i) {
            printf("ERR S2: DMA dst[%d]=0x%x expect 0x%x\n", i, v, 0xC0DE0000 + i);
            sim_fail();
        }
    }
    printf("S2 pass: DMA 64B payload intact under CPU concurrency\n");

    printf("bus_cpu_dma_concurrent test successfully\n");
    sim_end();
}
