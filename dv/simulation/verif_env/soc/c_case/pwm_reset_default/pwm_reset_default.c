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

/* F12：PWM 复位默认值验证
 *
 * 关键 RTL 依据（wujian100_open/soc/pwm.v）：
 *   - 所有寄存器复位值均为 0x0（含 RO 寄存器）
 *   - PWMCFG@0x00: bit27=cntdiven, [26:24]=cntdiv, [23:18]=tim0~5en,
 *     [17:12]=cap0~5en, [11:0]=pwm0~11en
 *   - 全部 53 个寄存器偏移 0x00~0xD0，步长 0x04
 *   - 写配置后加 2~3 次 dummy 读再读状态（pwm.v）
 *
 * 断言清单：全部 53 个寄存器（0x00~0xD0 步长 0x04）复位读值 == 0
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_reset_default\n");

    /* 全部 53 个寄存器偏移：0x00, 0x04, ..., 0xD0（(0xD0-0x00)/0x04+1 = 53）*/
    for(i=0; i<=0xD0; i+=4){
        mem_read32_(0x5001C000 + i, &v);
        if(v != 0){printf("ERR: offset 0x%02x = 0x%08x expect 0\n", i, v); sim_fail();}
    }

    printf("pwm_reset_default test successfully\n");
    sim_end();
}
