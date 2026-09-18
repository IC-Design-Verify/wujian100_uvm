#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F6: RTC 时钟分频矩阵
 *
 * RTL 实证（rtc.v rtc_clk_div）：counter 节拍 = ext_clk/(DIV+1)
 *   cnt 计到 ==rtcclkdivcfg 时出一拍分频时钟 → 每 DIV+1 个 ext_clk 计 1 次数
 *   TB ext_clk ≈ 5ns（els_clk 200MHz）
 *
 * 方法：固定计数距离 D=0x100（load=0, match=D），用轮询 raw_int_status 的
 * 迭代次数作为"时间"代理（每次迭代 = 1 次 APB 读，时长恒定），
 * 断言迭代次数随 DIV 单调增长且比例大致符合 (DIV+1) 之比。
 *   DIV=0 → 期望 ~N0 次
 *   DIV=3 → 期望 ~4×N0（容忍窗口 [2.5×N0, ∞)，下限放宽吸收 APB/同步开销）
 *   DIV=15 → 期望 ~16×N0（容忍窗口 [10×N0, ∞)）
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    long c0=0, c3=0, c15=0;
    printf("\nstart rtc_div_matrix\n");

    /* 轮次宏展开不便，手工三轮 */

    /* 轮 1: DIV=0 */
    mem_write32_(0x6000400c, 0x0);
    mem_write32_(0x60004020, 0x0);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x100);
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x5);      /* en + ien */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }
    do{
        mem_read32_(0x60004014, &v);
        c0++;
        if(c0 >= 2000000){printf("ERR DIV0: timeout (raw=0x%08x)\n", v);sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004018, &v);        /* EOI */
    mem_write32_(0x6000400c, 0x0);

    /* 轮 2: DIV=3（期望 4 倍） */
    mem_write32_(0x60004020, 0x3);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x100);
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x5);
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }
    do{
        mem_read32_(0x60004014, &v);
        c3++;
        if(c3 >= 2000000){printf("ERR DIV3: timeout (raw=0x%08x)\n", v);sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004018, &v);
    mem_write32_(0x6000400c, 0x0);

    /* 轮 3: DIV=15（期望 16 倍） */
    mem_write32_(0x60004020, 0xF);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x100);
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x5);
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }
    do{
        mem_read32_(0x60004014, &v);
        c15++;
        if(c15 >= 2000000){printf("ERR DIV15: timeout (raw=0x%08x)\n", v);sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004018, &v);
    mem_write32_(0x6000400c, 0x0);

    printf("div poll counts: DIV0=%d DIV3=%d DIV15=%d\n", c0, c3, c15);

    /* 单调性 + 比例窗口（下限放宽，上限不设——APB 轮询开销只会放大比值） */
    if(!(c3 > c0)){printf("ERR: DIV3(%d) 未大于 DIV0(%d)\n", c3, c0);sim_fail();}
    if(!(c15 > c3)){printf("ERR: DIV15(%d) 未大于 DIV3(%d)\n", c15, c3);sim_fail();}
    if(!(c3*10 > c0*25)){printf("ERR: DIV3/DIV0 比例过低 (%d vs %d, 期望约4x, 窗口>=2.5x)\n", c3, c0);sim_fail();}
    if(!(c15*10 > c3*25)){printf("ERR: DIV15/DIV3 比例过低 (%d vs %d, 期望约4x, 窗口>=2.5x)\n", c15, c3);sim_fail();}

    printf("rtc_div_matrix test successfully\n");
    sim_end();
}
