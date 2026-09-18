/*
 * SoC F4: 多中断源路由独立性 —— TIM1(17) / PWM(25) / WDT(27) 并发断言
 *
 * 路由表（core_top.v:537-556 实证）：
 *   ip_cpu_int_vld[17] = tim0_wic_intr[0]（TIM0 宏通道 1）
 *   ip_cpu_int_vld[25] = pwm_wic_intr
 *   ip_cpu_int_vld[27] = wdt_wic_intr
 *
 * 步骤（C 侧制造三源同时 pending 的窗口）：
 *  S1: 配置 WDT(int mode) + TIM1(load=0x400) + PWM(group0 cnt_zero inten)
 *  S2: 轮询三个外设的 raw/pending status 直到三者同时挂起
 *  S3: 保持窗口（UVM 侧 soc_top_intr_multi_test 采样三线重叠断言）
 *  S4: 逐个清：PWM(PWMIC1) → TIM(读 EOI) → WDT(读 int_clr)，
 *      每清一个验证对应 status 归零、其余两源不受影响（独立性）
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* TIM0 宏 (APB0 0x50000000) 通道 1 —— DesignWare 定时器寄存器布局 */
#define TIM1_LOAD    0x50000000
#define TIM1_CTRL    0x50000008
#define TIM1_EOI     0x5000000C
#define TIM1_INTSTAT 0x50000010
/* PWM (APB0 0x5001C000) */
#define PWM_EN       0x5001C000
#define PWM_INTEN1   0x5001C014
#define PWM_RIS1     0x5001C01C
#define PWM_IC1      0x5001C024
#define PWM_01LOAD   0x5001C038
/* WDT (APB0 0x50008000) */
#define WDT_CR       0x50008000
#define WDT_TORR     0x50008004
#define WDT_INTSTAT  0x50008010
#define WDT_EOI      0x50008014

static void soc_delay(unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; i++)
        __asm__ volatile ("nop");
}

int test_start(void)
{
    unsigned int v1, v2, v3;
    unsigned int timeout;

    printf("\nstart intr_multi_route\n");

    /* S1: 三源配置（WDT 最快超时，TIM/PWM 其次） */
    mem_write32_(WDT_TORR, 0x00);          /* TOP=0 快速超时 */
    mem_write32_(WDT_CR, 0x3);             /* EN=1, RMOD=1 → 首次超时产生中断 */
    mem_write32_(TIM1_CTRL, 0x2);          /* 先停 */
    mem_write32_(TIM1_LOAD, 0x400);
    mem_write32_(TIM1_CTRL, 0x3);          /* enable + user-defined, int unmasked */
    mem_write32_(PWM_EN, 0x0);
    mem_write32_(PWM_01LOAD, 0x100);       /* group0 周期 0x100 */
    mem_write32_(PWM_INTEN1, 0x100);       /* bit8 = group0 cnt_zero 中断使能 */
    mem_write32_(PWM_EN, 0x1);             /* pwm0en */

    /* S2: 等三者同时 pending */
    timeout = 0;
    while (1) {
        mem_read32_(WDT_INTSTAT, &v1);
        mem_read32_(TIM1_INTSTAT, &v2);
        mem_read32_(PWM_RIS1, &v3);
        if ((v1 & 1) && (v2 & 1) && (v3 & 0x100)) break;
        if (++timeout >= 500000) {
            printf("ERR S2: not all pending: wdt=0x%x tim=0x%x pwm=0x%x\n", v1, v2, v3);
            sim_fail();
        }
    }
    printf("S2 pass: WDT/TIM1/PWM all pending simultaneously\n");

    /* S3: 保持窗口供 UVM 采样三线同时断言 */
    soc_delay(3000);

    /* S4a: 清 PWM，验证 PWM 归零且 WDT/TIM 仍 pending */
    mem_write32_(PWM_IC1, 0x100);
    mem_read32_(PWM_RIS1, &v3);
    mem_read32_(WDT_INTSTAT, &v1);
    mem_read32_(TIM1_INTSTAT, &v2);
    if ((v3 & 0x100) || !(v1 & 1) || !(v2 & 1)) {
        printf("ERR S4a: after PWM clear: pwm=0x%x wdt=0x%x tim=0x%x\n", v3, v1, v2);
        sim_fail();
    }
    printf("S4a pass: PWM cleared, WDT/TIM unaffected\n");

    /* S4b: 清 TIM1（读 EOI），验证 TIM 归零且 WDT 仍 pending */
    mem_read32_(TIM1_EOI, &v1);            /* 读 EOI 清中断 */
    soc_delay(50);                          /* 状态寄存器同步需要几拍 */
    mem_read32_(TIM1_INTSTAT, &v2);
    mem_read32_(WDT_INTSTAT, &v1);
    if ((v2 & 1) || !(v1 & 1)) {
        printf("ERR S4b: after TIM EOI: tim=0x%x wdt=0x%x\n", v2, v1);
        sim_fail();
    }
    printf("S4b pass: TIM1 cleared via EOI, WDT unaffected\n");

    /* S4c: 清 WDT（读 int_clr/EOI），验证全清 */
    mem_read32_(WDT_EOI, &v1);
    mem_read32_(WDT_INTSTAT, &v1);
    if (v1 & 1) {
        printf("ERR S4c: WDT int_status=0x%x after EOI\n", v1);
        sim_fail();
    }
    printf("S4c pass: WDT cleared via EOI read\n");

    printf("intr_multi_route test successfully\n");
    sim_end();
}
