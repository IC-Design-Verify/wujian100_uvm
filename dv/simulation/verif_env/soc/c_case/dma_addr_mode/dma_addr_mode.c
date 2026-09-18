/*
 * DMA F2: SINC/DINC 地址更新模式
 * 编码（CTRL_A[7:6]=SINC, [5:4]=DINC）：00=increment 01=decrease 1x=no change
 *  S1: SINC=01 decrease —— SAR 指向源区高端，向下取数；
 *      dst[0]=src_hi ... dst[7]=src_lo（相对 incr 的顺序反转）
 *  S2: DINC=1x no-change —— 目的地址固定，最终值=最后一个源字；
 *      相邻单元不被改写
 *  S3: SINC=1x no-change —— 源地址固定，dst[0..7] 全部等于 src[0]
 * 数据校验均通过读取目的内存完成。
 *
 * 长度守护区（guard）：每轮把 dst[8..15] 清零，传输后必须仍为 0。
 * wujian100_open_for_debug dmac.v:15717 注入 bug（cntr_blk 重载 '-'→'*'，
 * R=(BLOCK_TL+1)*t 而非 (BLOCK_TL+1)-t，计数器按 t 递减、到 0 当拍才结束）
 * 使引擎把 block_tl（字节数）误当节拍数：实际节拍恒为 BLOCK_TL+2，
 * 32-bit 下 32B 配置实际传 33 拍 = 132B（≈4 倍越界），守护区必被踩脏。
 * 结构：地址模式断言（bug 无关，前 8 拍行为）必须 PASS；
 *       bug 证据（守护区/末拍值）累计后统一 FAIL —— 正确 RTL 下全 PASS。
 * S1 源向下越界区（0x50A0-0x50FC）、S2 源向上越界区预填哨兵保证检测确定性。
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define CH0      0x40000000
#define DMACCFG  0x4000033C

#define SRCA     0x00005100            /* S1 源区: 0x5100..0x511C */
#define DST1     0x20025000
#define SRCB     0x00005200            /* S2 源区 */
#define DST2     0x20026000
#define SRCC     0x00005300            /* S3 源（固定地址） */
#define DST3     0x20027000

/* CTRL_A: BLOCK_TL=31(32B) @ [23:12], SINC @ [7:6], DINC @ [5:4], W=32/32 */
#define CTRLA(sinc, dinc)  (0x1F000 | ((sinc) << 6) | ((dinc) << 4) | 0xA)

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
    mem_write32_(CH0 + 0x18, 0xf);     /* 清中断，供下一轮使用 */
}

int test_start(void)
{
    unsigned int v;
    int i;
    int bug_hits = 0;

    printf("\nstart dma_addr_mode\n");

    /* ---- S1: SINC=01 decrease ---- */
    for (i = 0; i < 8; i++)
        mem_write32_(SRCA + i * 4, 0xB0B0B000 + i);   /* src_lo=...B000, src_hi=...B007 */
    for (i = 0; i < 24; i++)                          /* 源向下越界区预填哨兵（bug 下会被读到） */
        mem_write32_(0x50A0 + i * 4, 0x5E970000 + i);
    for (i = 0; i < 16; i++)                          /* dst[0..7] 数据区 + dst[8..15] 守护区清零 */
        mem_write32_(DST1 + i * 4, 0);
    mem_write32_(CH0 + 0x00, SRCA + 7 * 4);           /* 从高端 0x511C 开始 */
    mem_write32_(CH0 + 0x04, DST1);
    mem_write32_(CH0 + 0x08, CTRLA(1, 0));            /* SINC=01 decr, DINC=00 incr */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    run_and_wait();
    for (i = 0; i < 8; i++) {
        mem_read32_(DST1 + i * 4, &v);
        if (v != 0xB0B0B007 - i) {   /* dst[0]=src_hi=...B007, 依次递减 */
            printf("ERR S1: dst[%d]=0x%x expect 0x%x\n", i, v, 0xB0B0B007 - i);
            sim_fail();
        }
    }
    for (i = 8; i < 16; i++) {       /* 守护区：正确 RTL 下不被触碰 */
        mem_read32_(DST1 + i * 4, &v);
        if (v != 0) {
            printf("GUARD S1: dst[%d]=0x%x expect 0 (block length overrun)\n", i, v);
            bug_hits++;
        }
    }
    printf("S1: SINC=decrease verified (dst[0..7]=src_hi..src_lo); guard dirty=%d\n",
           bug_hits);

    /* ---- S2: DINC=1x no-change ---- */
    for (i = 0; i < 8; i++)
        mem_write32_(SRCB + i * 4, 0xC0C0C000 + i);
    for (i = 0; i < 26; i++)            /* 源向上越界区预填哨兵（bug 下 33 拍会读到 SRCB+0x80） */
        mem_write32_(SRCB + 0x20 + i * 4, 0xBAD00000 + i);
    for (i = 0; i < 4; i++)
        mem_write32_(DST2 + i * 4, 0);
    mem_write32_(CH0 + 0x00, SRCB);
    mem_write32_(CH0 + 0x04, DST2);
    mem_write32_(CH0 + 0x08, CTRLA(0, 2));            /* SINC=00 incr, DINC=10 no-change */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    run_and_wait();
    /* DINC=no-change 的 bug 无关判据：相邻单元不被改写（地址从不动）。
       末值判据：正确 RTL 留下 src[7]=0xC0C0C007；bug 下 33 拍留下
       第 33 拍读到的哨兵 0xBAD00018 —— 两种值分别对应两种 RTL，其余值报错。 */
    mem_read32_(DST2 + 4, &v);
    if (v != 0) { printf("ERR S2: dst+4 touched: 0x%x (DINC no-change violated)\n", v); sim_fail(); }
    mem_read32_(DST2, &v);
    if (v == 0xC0C0C007) {
        printf("S2: DINC=no-change verified, fixed dst=0x%x (last src word, correct RTL)\n", v);
    } else if (v == 0xBAD00018) {
        printf("GUARD S2: fixed dst=0x%x = sentinel of beat 33 "
               "(block length overrun, neighbor clean)\n", v);
        bug_hits++;
    } else {
        printf("ERR S2: fixed dst=0x%x matches neither correct nor buggy RTL\n", v);
        sim_fail();
    }

    /* ---- S3: SINC=1x no-change ---- */
    mem_write32_(SRCC, 0xD0D0D0D0);
    mem_write32_(SRCC + 4, 0xE0E0E0E0);               /* 不应被读到 */
    for (i = 0; i < 16; i++)                          /* 数据区 + 守护区清零 */
        mem_write32_(DST3 + i * 4, 0);
    mem_write32_(CH0 + 0x00, SRCC);
    mem_write32_(CH0 + 0x04, DST3);
    mem_write32_(CH0 + 0x08, CTRLA(2, 0));            /* SINC=10 no-change, DINC=00 incr */
    mem_write32_(CH0 + 0x0C, 0x5);
    mem_write32_(CH0 + 0x10, 0x1f);
    run_and_wait();
    for (i = 0; i < 8; i++) {
        mem_read32_(DST3 + i * 4, &v);
        if (v != 0xD0D0D0D0) {
            printf("ERR S3: dst[%d]=0x%x expect 0xD0D0D0D0\n", i, v);
            sim_fail();
        }
    }
    {
        int d3 = 0;
        for (i = 8; i < 16; i++) {     /* 守护区：正确 RTL 下不被触碰 */
            mem_read32_(DST3 + i * 4, &v);
            if (v != 0) {
                printf("GUARD S3: dst[%d]=0x%x expect 0 (block length overrun)\n", i, v);
                d3++;
            }
        }
        bug_hits += d3;
        printf("S3: SINC=no-change verified (dst[0..7]=src[0]); guard dirty=%d\n", d3);
    }

    if (bug_hits) {
        printf("ERR: dma_addr_mode detected injected block-length bug "
               "(%d guard violations; dmac.v:15717 beats=BLOCK_TL+2)\n", bug_hits);
        sim_fail();
    }
    printf("dma_addr_mode test successfully\n");
    sim_end();
}
