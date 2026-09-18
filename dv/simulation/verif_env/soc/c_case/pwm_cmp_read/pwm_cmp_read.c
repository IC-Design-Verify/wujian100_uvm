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

/* PWM 比较寄存器写读回环测试（pwm_cmp_read）
 *
 * 关键 RTL 依据（wujian100_open/soc/pwm.v）：
 *   - PWM0CMP@0x50/PWM1CMP@0x54 可读写，无设置位（全 32 位有效）
 *   - PWMCFG en=0 时计数器不递减，写比较值稳定读回
 *
 * 序列：
 *   1) PWMCFG=0（默认禁止 PWM）
 *   2) PWM0CMP=0x12345678, PWM1CMP=0x87654321
 *   3) 读回校验；然后写 0 再校验复位值 0
 */
int test_start(void){
    uint32_t v=0;
    printf("\nstart pwm_cmp_read\n");

    /* 先确保 PWM 禁止，比较寄存器独立可读写 */
    mem_write32_(0x5001C000 + 0x00, 0x00);
    mem_read32_(0x5001C000 + 0x00, &v);

    /* 写—读回环：PWM0CMP */
    mem_write32_(0x5001C000 + 0x50, 0x12345678);
    mem_read32_(0x5001C000 + 0x50, &v);
    if(v != 0x12345678){
        printf("ERR: PWM0CMP=0x%08x expect 0x12345678\n", v);
        sim_fail();
    }

    /* 写—读回环：PWM1CMP */
    mem_write32_(0x5001C000 + 0x54, 0x87654321);
    mem_read32_(0x5001C000 + 0x54, &v);
    if(v != 0x87654321){
        printf("ERR: PWM1CMP=0x%08x expect 0x87654321\n", v);
        sim_fail();
    }

    /* 清 0 验证 */
    mem_write32_(0x5001C000 + 0x50, 0x0);
    mem_write32_(0x5001C000 + 0x54, 0x0);
    mem_read32_(0x5001C000 + 0x50, &v);
    if(v != 0x0){
        printf("ERR: PWM0CMP clear=0x%08x expect 0\n", v);
        sim_fail();
    }
    mem_read32_(0x5001C000 + 0x54, &v);
    if(v != 0x0){
        printf("ERR: PWM1CMP clear=0x%08x expect 0\n", v);
        sim_fail();
    }

    printf("pwm_cmp_read test successfully\n");
    sim_end();
}
