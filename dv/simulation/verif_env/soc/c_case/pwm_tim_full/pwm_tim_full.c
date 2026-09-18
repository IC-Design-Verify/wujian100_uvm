/*
Copyright (c) 2019 Alibaba Group Holding Limited

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR OTHER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F8：PWM Timer 中断完整验证
 *
 * 关键 RTL 依据（wujian100_open/soc/pwm.v）：
 *   - PWMCFG@0x00: [23:18]=tim0~5en（bit18=tim0en, bit19=tim1en, ...）
 *   - TIM01LOAD@0xB0: [15:0]=tim0_load, [31:16]=tim1_load
 *   - TIM23LOAD@0xB4: [15:0]=tim2_load, [31:16]=tim3_load
 *   - TIM45LOAD@0xB8: [15:0]=tim4_load, [31:16]=tim5_load
 *   - TIM_INT_EN@0xA0: [5:0]=tim0~5 中断使能
 *   - TIMRIS@0xA4(RO): [5:0]=tim0~5 原始中断状态
 *   - TIMIS@0xAC(RO): 与 TIMRIS 直接 assign 相等（pwm.v:4767）
 *   - TIM_INT_CLR@0xA8: 写 1 清除对应 timer 中断
 *   - cnt==tim_load 时产生 tim_cnt_match → TIMRIS bit N 置位（pwm.v:5193）
 *   - 写配置后加 2~3 次 dummy 读再读状态
 *
 * 断言清单：
 *   S1: tim0 使能 → TIMRIS bit0==1 → TIMIS==TIMRIS → EOI → bit0==0
 *   S2: tim1 使能 → TIMRIS bit1==1 → EOI → bit1==0
 *   S3: tim2/tim5 使能验证
 *   S4: INTEN=0 门控：counter 过 match 后 TIMRIS bit0 仍为 0
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart pwm_tim_full\n");

    /* S1: tim0 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x0);        /* PWMCFG=0 全停 */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0xB0, 0x00000040); /* TIM01LOAD: tim0=0x40 */
    mem_write32_(0x5001C000 + 0xA0, 0x01);       /* TIM_INT_EN: tim0 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x40000);    /* PWMCFG: tim0en (bit18) */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S1: timeout waiting TIMRIS bit0\n");sim_fail();}
    }while((v & 0x1) == 0);

    mem_read32_(0x5001C000 + 0xAC, &v);
    if(v != 0x1){printf("ERR S1: TIMIS=0x%08x expect 0x1 (TIMIS==TIMRIS)\n", v);sim_fail();}

    mem_write32_(0x5001C000 + 0xA8, 0x01);       /* TIM_INT_CLR: 清 tim0 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S1: timeout waiting TIMRIS bit0==0\n");sim_fail();}
    }while((v & 0x1) != 0);

    /* S2: tim1 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0xB0, 0x40000040); /* TIM01LOAD: tim1=0x40 */
    mem_write32_(0x5001C000 + 0xA0, 0x02);       /* TIM_INT_EN: tim1 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x80000);    /* PWMCFG: tim1en (bit19) */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S2: timeout waiting TIMRIS bit1\n");sim_fail();}
    }while((v & 0x2) == 0);

    mem_write32_(0x5001C000 + 0xA8, 0x02);       /* TIM_INT_CLR: 清 tim1 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S2: timeout waiting TIMRIS bit1==0\n");sim_fail();}
    }while((v & 0x2) != 0);

    /* S3: tim2 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0xB4, 0x00000040); /* TIM23LOAD: tim2=0x40 */
    mem_write32_(0x5001C000 + 0xA0, 0x04);       /* TIM_INT_EN: tim2 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x100000);   /* PWMCFG: tim2en (bit20) */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S3: timeout waiting TIMRIS bit2\n");sim_fail();}
    }while((v & 0x4) == 0);

    mem_write32_(0x5001C000 + 0xA8, 0x04);       /* TIM_INT_CLR: 清 tim2 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S3: timeout waiting TIMRIS bit2==0\n");sim_fail();}
    }while((v & 0x4) != 0);

    /* S3b: tim5 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0xB8, 0x40000040); /* TIM45LOAD: tim5=0x40 */
    mem_write32_(0x5001C000 + 0xA0, 0x20);       /* TIM_INT_EN: tim5 使能 */
    mem_write32_(0x5001C000 + 0x00, 0x800000);   /* PWMCFG: tim5en (bit23) */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S3b: timeout waiting TIMRIS bit5\n");sim_fail();}
    }while((v & 0x20) == 0);

    mem_write32_(0x5001C000 + 0xA8, 0x20);       /* TIM_INT_CLR: 清 tim5 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0xA4, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S3b: timeout waiting TIMRIS bit5==0\n");sim_fail();}
    }while((v & 0x20) != 0);

    /* S4: INTEN=0 门控验证 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0xB0, 0x00000040); /* TIM01LOAD: tim0=0x40 */
    mem_write32_(0x5001C000 + 0xA0, 0x00);       /* TIM_INT_EN: 无使能 */
    mem_write32_(0x5001C000 + 0x00, 0x40000);    /* PWMCFG: tim0en */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    for(i=0;i<200;i++){ mem_read32_(0x5001C000 + 0xA4, &v); } /* dummy 读 200 次 */
    mem_read32_(0x5001C000 + 0xA4, &v);
    if(v != 0x0){printf("ERR S4: TIMRIS=0x%08x expect 0 (INTEN=0 门控)\n", v);sim_fail();}

    /* 收尾 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    mem_write32_(0x5001C000 + 0xA0, 0x0);

    printf("pwm_tim_full test successfully\n");
    sim_end();
}
