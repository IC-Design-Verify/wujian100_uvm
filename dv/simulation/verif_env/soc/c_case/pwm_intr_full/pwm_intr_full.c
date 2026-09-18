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

/* F9：PWM 中断完整验证
 *
 * 关键 RTL 依据（wujian100_open/soc/pwm.v）：
 *   - PWMRIS1@0x1C(RO): [8]=cnt_zero, [9]=cnt_load, [10]=compa_up,
 *     [11]=compb_up, [12]=compa_down, [13]=compb_down（group0）
 *     （pwm.v:4752 位序实证：compb_down/compa_down/compb_up/compa_up/cnt_load/cnt_zero）
 *   - PWMIS1==PWMRIS1（RTL 直接 assign，pwm.v:4758）
 *   - PWMINTEN1@0x14: 位使能对应 group 中断
 *   - PWMIC1@0x24: 写 1 清除
 *   - PWMCFG@0x00: [11:0]=pwm0~11en
 *   - PWM01LOAD@0x38, PWM0CMP@0x50
 *   - PWMRIS2@0x20(RO): group3=[0..5]
 *   - 写配置后加 2~3 次 dummy 读再读状态
 *   - int_fault：PAD_PWM_FAULT 悬空 X，任何用例不得写 INTEN1[0]=1
 *
 * 断言清单：
 *   S1: group0 up 模式 → zero/load/compa_up/compb_up 事件最终置位
 *   S2: 清除 PWMIC1=0x1700 → 位清 0
 *   S3: INTEN=0 门控验证
 *   S4: group3 在 PWMRIS2 中验证
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart pwm_intr_full\n");

    /* 全程 INTEN1[0]（int_fault）保持 0，PAD_PWM_FAULT 悬空 X */

    /* S1: group0 up 模式 */
    mem_write32_(0x5001C000 + 0x00, 0x0);        /* PWMCFG=0 全停 */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    mem_write32_(0x5001C000 + 0x38, 0x00000200); /* PWM01LOAD=0x200 */
    mem_write32_(0x5001C000 + 0x50, 0x01800080); /* PWM0CMP=[31:16]compb=0x180, [15:0]compa=0x80 */
    mem_write32_(0x5001C000 + 0x14, 0x0F00);     /* PWMINTEN1: bit8/9/10/11 = zero/load/compa_up/compb_up */
    mem_write32_(0x5001C000 + 0x00, 0x1);        /* PWMCFG: pwm0en */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    /* 轮询 cnt_zero (bit8) 置位 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0x1C, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S1: timeout waiting PWMRIS1 bit8\n");sim_fail();}
    }while((v & 0x100) == 0);

    /* 轮询 load/compa_up/compb_up 最终置位 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0x1C, &v);
        if((v & 0x0F00) == 0x0F00) break;
        timeout++;
        if(timeout >= 100000){printf("ERR S1: timeout waiting all group0 bits\n");sim_fail();}
    }while(1);

    /* 断言 PWMIS1==PWMRIS1 */
    mem_read32_(0x5001C000 + 0x2C, &v);
    if(v != 0x0F00){printf("ERR S1: PWMIS1=0x%08x expect 0x0F00 (PWMIS1==PWMRIS1)\n", v);sim_fail();}

    /* S2: 清除 */
    mem_write32_(0x5001C000 + 0x24, 0x0F00);     /* PWMIC1: 清 group0 位 */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0x1C, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S2: timeout waiting PWMRIS1 bits cleared\n");sim_fail();}
    }while((v & 0x0F00) != 0);

    /* S3: INTEN=0 门控 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0x38, 0x00000200); /* PWM01LOAD=0x200 */
    mem_write32_(0x5001C000 + 0x14, 0x00);       /* PWMINTEN1=0 */
    mem_write32_(0x5001C000 + 0x00, 0x1);        /* PWMCFG: pwm0en */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    for(i=0;i<200;i++){ mem_read32_(0x5001C000 + 0x1C, &v); } /* dummy 读 200 次 */
    mem_read32_(0x5001C000 + 0x1C, &v);
    if(v != 0x0){printf("ERR S3: PWMRIS1=0x%08x expect 0 (INTEN=0 门控)\n", v);sim_fail();}

    /* S4: group3 在 PWMRIS2 中 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }
    
    mem_write32_(0x5001C000 + 0x3C, 0x01000000); /* PWM23LOAD: [31:16]=group3 load=0x100 */
    mem_write32_(0x5001C000 + 0x18, 0x01);       /* PWMINTEN2: bit0 (group3 zero) */
    mem_write32_(0x5001C000 + 0x00, 0x40);       /* PWMCFG: pwm6en (bit6) */
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0x20, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S4: timeout waiting PWMRIS2 bit0\n");sim_fail();}
    }while((v & 0x1) == 0);

    mem_write32_(0x5001C000 + 0x28, 0x01);       /* PWMIC2: 清 group3 zero */
    timeout=0;
    do{
        mem_read32_(0x5001C000 + 0x20, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S4: timeout waiting PWMRIS2 bit0==0\n");sim_fail();}
    }while((v & 0x1) != 0);

    /* 收尾 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    mem_write32_(0x5001C000 + 0x14, 0x0);
    mem_write32_(0x5001C000 + 0x18, 0x0);

    printf("pwm_intr_full test successfully\n");
    sim_end();
}
