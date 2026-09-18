/*
 * DMA F3: 传输宽度 SRC_TR_WIDTH / DST_TR_WIDTH
 * 编码：00=8-bit 01=16-bit 10=32-bit 11=reserved
 *  S1: 8-bit  —— 32 字节逐字节搬运，dst 与 src 字节一致
 *      （fsmc 读侧按 s_data_sel 选字节拼装 buffrdata，写侧逐字节写出）
 *  S2: 16-bit —— 32 字节按半字搬运，dst 与 src 一致
 *  S3: 32-bit —— 基线宽度回归
 *  S4: reserved 11 —— UG 标保留；仅验证寄存器可写可读回（容忍方式），
 *      不发起传输（hsize 会输出 3'b011，属带外行为，不在本用例断言）
 * 数据模式：src[k] 字节 = k（0..127），8/16/32-bit 搬运后字读回应一致。
 * 注入 bug 提醒：dmac.v:15717 cntr_blk 重载 '-'→'*'，R=(BLOCK_TL+1)*t 而非
 * (BLOCK_TL+1)-t（t=宽度字节数，计数器按 t 递减、到 0 当拍才结束），
 * 使引擎把 block_tl（字节数）误当节拍数：实际节拍恒为 BLOCK_TL+2，
 * 正确节拍=(BLOCK_TL+1)/t。32B 配置下 8/16/32-bit 实际搬运 33/66/132 字节
 * （越界 +1/+34/+100B）。S1/S2/S3 守护区确定性 FAIL，本用例是该 bug 的
 * 检测器兼定量刻画（正确 RTL 下全部 PASS）。
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

static void prep(void)
{
    int i;
    /* src 字节流 = 00 01 02 ... 9F（小端字：0x03020100, 0x07060504, ...）
       预填 160B：bug 下 32-bit 实际读 132B，预填富余保证越界读取内容确定非零 */
    for (i = 0; i < 40; i++)
        mem_write32_(SRC + i * 4,
            (unsigned int)(i * 4) | ((i * 4 + 1) << 8) | ((i * 4 + 2) << 16) | ((i * 4 + 3) << 24));
    for (i = 0; i < 32; i++)
        mem_write32_(DST + i * 4, 0);
}

static int check(void)
{
    unsigned int v, exp;
    int i;
    for (i = 0; i < 8; i++) {
        exp = (unsigned int)(i * 4) | ((i * 4 + 1) << 8) | ((i * 4 + 2) << 16) | ((i * 4 + 3) << 24);
        mem_read32_(DST + i * 4, &v);
        if (v != exp) {
            printf("ERR: dst[%d]=0x%x expect 0x%x\n", i, v, exp);
            sim_fail();
        }
    }
    /* 守护区测量：配置 32B，正确 RTL 下 dst[8..31] 必须全 0。
       注入 bug（dmac.v:15717 reload '-'→'*'）特征：实际节拍=BLOCK_TL+2，
       越界字节 = (BLOCK_TL+2)*t - 32 → 8/16/32-bit 越界 +1/+34/+100B，
       对应脏守护字 1/9/25 个（dst[8] 起）。先打印全部脏字，
       用脏字个数定量刻画 bug，再由 test_start 汇总判 FAIL。 */
    {
        int dirty = 0;
        for (i = 8; i < 32; i++) {
            mem_read32_(DST + i * 4, &v);
            if (v != 0) { printf("GUARD: dst[%d]=0x%x expect 0 (overrun)\n", i, v); dirty++; }
        }
        if (dirty)
            printf("GUARD-SUMMARY: %d guard words dirty "
                   "(injected dmac.v:15717: beats=BLOCK_TL+2 regardless of width)\n", dirty);
        return dirty;
    }
    return 0;
}

int test_start(void)
{
    unsigned int v;
    int bug_hits = 0;

    printf("\nstart dma_tr_width\n");

    /* S1: 8-bit */
    prep();
    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x1F000);          /* 32B, incr/incr, 8/8 */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    run_and_wait();
    bug_hits += check();
    printf("S1: 8-bit  transfer ok; guard overrun +1B   expected w/ bug\n");

    /* S2: 16-bit */
    prep();
    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x1F005);          /* 32B, incr/incr, 16/16 */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    run_and_wait();
    bug_hits += check();
    printf("S2: 16-bit transfer ok; guard overrun +34B  expected w/ bug\n");

    /* S3: 32-bit */
    prep();
    mem_write32_(CH0 + 0x00, SRC);
    mem_write32_(CH0 + 0x04, DST);
    mem_write32_(CH0 + 0x08, 0x1F00A);          /* 32B, incr/incr, 32/32 */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    run_and_wait();
    bug_hits += check();
    printf("S3: 32-bit transfer ok; guard overrun +100B expected w/ bug\n");

    /* S4: reserved TR_WIDTH=11，仅验证寄存器容忍（写/读回），不发起传输 */
    mem_write32_(CH0 + 0x08, 0x1F00F);          /* SRCW=11, DSTW=11 */
    mem_read32_(CH0 + 0x08, &v);
    if (v != 0x1F00F) {
        printf("ERR S4: CTRLA readback=0x%x expect 0x1F00F\n", v);
        sim_fail();
    }
    printf("S4 pass: reserved TR_WIDTH=11 accepted by register (no transfer issued)\n");

    if (bug_hits) {
        printf("ERR: dma_tr_width detected injected block-length bug "
               "(%d dirty guard words across widths)\n", bug_hits);
        sim_fail();
    }
    printf("dma_tr_width test successfully\n");
    sim_end();
}
