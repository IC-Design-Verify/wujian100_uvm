/*
 * DMA F11: 寄存器复位值检查
 * 复位后（未做任何配置）读 ch0 全部 10 个寄存器（含 UG 标 reserved 的 0x24
 * GRP_LEN_EXT）、ch7/ch15 抽查、CHSR(0x338)、DMACCFG(0x33C)，全部应为 0。
 * RTL 依据：dmac.v chregc 各寄存器 hrst_n 分支均清 0；gbregc 同。
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define DMA_BASE   0x40000000
#define CH_STRIDE  0x30
#define CH0        (DMA_BASE + 0x000)
#define CH7        (DMA_BASE + 0x150)
#define CH15       (DMA_BASE + 0x2D0)
#define CHSR       (DMA_BASE + 0x338)
#define DMACCFG    (DMA_BASE + 0x33C)

static int check_zero(unsigned int addr)
{
    unsigned int v;
    mem_read32_(addr, &v);
    if (v != 0) {
        printf("ERR: addr 0x%x reset value = 0x%x, expect 0\n", addr, v);
        return 1;
    }
    return 0;
}

int test_start(void)
{
    int err = 0;
    int i;
    unsigned int ch;

    printf("\nstart dma_reset_default\n");

    /* 每通道 10 个寄存器: SAR/DAR/CTRLA/CTRLB/INT_MASK/INT_STATUS/INT_CLEAR/
     * SOFT_REQ/EN/GRP_LEN_EXT(0x24, UG 标 reserved 但 RTL 有译码) */
    for (i = 0; i < 3; i++) {
        ch = (i == 0) ? CH0 : (i == 1) ? CH7 : CH15;
        err += check_zero(ch + 0x00);  /* SAR */
        err += check_zero(ch + 0x04);  /* DAR */
        err += check_zero(ch + 0x08);  /* CTRL_A */
        err += check_zero(ch + 0x0C);  /* CTRL_B */
        err += check_zero(ch + 0x10);  /* INT_MASK */
        err += check_zero(ch + 0x14);  /* INT_STATUS */
        err += check_zero(ch + 0x18);  /* INT_CLEAR (WO, 读 mux 无此项应回 0) */
        err += check_zero(ch + 0x1C);  /* SOFT_REQ  (WO, 同上) */
        err += check_zero(ch + 0x20);  /* CH_EN */
        err += check_zero(ch + 0x24);  /* GRP_LEN_EXT */
    }

    err += check_zero(CHSR);
    err += check_zero(DMACCFG);

    if (err) {
        printf("dma_reset_default FAILED, %d mismatches\n", err);
        sim_fail();
    }
    printf("dma_reset_default test successfully\n");
    sim_end();
}
