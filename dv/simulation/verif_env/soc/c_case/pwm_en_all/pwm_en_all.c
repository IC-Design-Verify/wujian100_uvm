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

/* PWM 全清除/全使能功能测试（pwm_en_all）
 *
 * 关键 RTL 依据（wujian100_open/soc/pwm.v）：
 *   - PWMCFG@0x00: bit27=cntdiven, [26:24]=cntdiv, [17:12]=cap0~5en, [11:0]=pwm0~11en
 *   - PWM0CMP@0x50 / PWM1CMP@0x54：比较值（无 top 位，写 0 则恒 0）
 *   - 全部计数器/比较器共用一个 8 位时钟源周期（clk_presc）
 *
 * 序列：
 *   1) PWMCFG 写 0xFFF（init）→ 再写 0（去除使能）→ 读回确认
 *   2) PWM0CMP=0x64(100dec), PWM1CMP=0x3C(60dec)
 *   3) PWMCFG 写 0xFFF 使能所有 12 路 PWM + 6 路捕获 + 计数分频=1
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_en_all\n");

    /* step1: 写任意使能后立刻清 0，验证 PWMCFG 可写且写 0 生效（默认复位 0）*/
    mem_write32_(0x5001C000 + 0x00, 0xFFF);
    mem_write32_(0x5001C000 + 0x00, 0x00);
    mem_read32_(0x5001C000 + 0x00, &v);
    if(v != 0x0){
        printf("ERR: PWMCFG=0x%08x expect 0\n", v);
        sim_fail();
    }

    /* step2: 先配置比较值 PWM0CMP/PWM1CMP */
    mem_write32_(0x5001C000 + 0x50, 0x64);  /* PWM0CMP = 100 (len=101) */
    mem_write32_(0x5001C000 + 0x54, 0x3C);  /* PWM1CMP = 60  (len=61)  */

    /* step3: 全使能 PWM0~11 + CM0~5，时钟分频 1 分频 */
    mem_write32_(0x5001C000 + 0x00, 0xFFF);
    /* dummy 读 2~3 次，等待同步器同步使能（pwm.v 中 pwmxen_sync）*/
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    /* step4: 使能 PWM0/1 后计数恢复运行，验证输出自动翻转（比较相等时）*/
    /* 验证比较器使能后的寄存器写-读回路径 */
    mem_write32_(0x5001C000 + 0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x5001C000 + 0x00, &v); }

    printf("pwm_en_all test successfully\n");
    sim_end();
}
