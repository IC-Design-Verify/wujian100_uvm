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

/* F1 + F4 en 位验证：RTC 计数器递增与停止
 *
 * 关键 RTL 依据（wujian100_open/soc/rtc.v）：
 *   - CCR@0x0C: bit3=wen(wrap使能)，bit2=Rtc_en(计数使能)，bit1=rtc_mask，bit0=rtc_ien
 *   - counter 计数时钟 = ext_clk 每 (DIV+1) 个周期 1 拍（rtc_clk_div: cnt==cfg 时出一拍）
 *     DIV=0 → 每个 ext_clk 计 1 次；TB ext_clk≈5ns
 *   - load_value 写入后经跨域同步加载为计数起点；CCR/match/load/div 写入后
 *     经 2-flop 同步到 AOU 域生效（写入后读回前加 2~3 次 dummy 读）
 *   - wrap（wen=1 且 cnt==match）时 cnt<=0（rtc.v rtc_cnt），
 *     **不是**回绕到 load_value——Spec-vs-RTL 差异
 *
 * 断言清单：
 *   S1: CCR=0（停），写 CCR=0、DIV=0、load=0、match=0xFFFFFFFF，dummy 读×3
 *   S2: 写 CCR=0x4（en=1,wen=0,ien=0），dummy 读×10；读 current_value 两次，断言 v2>v1（递增）
 *   S3: 写 CCR=0（en=0），dummy 读×10；读 current_value 两次，断言 v2==v1（停止）
 *   S4: 重新 en=1，读 current_value 两次，断言 v2>v1（再次递增）
 */
int test_start(void){
    uint32_t v1=0, v2=0;
    int i=0;
    printf("\nstart rtc_counter_inc\n");

    /* S1: 初始化 - 停用计数器，清零配置 */
    mem_write32_(0x6000400c, 0x0);    /* CCR=0（停） */
    mem_write32_(0x60004020, 0x0);    /* DIV=0 */
    mem_write32_(0x60004008, 0x0);    /* load=0 */
    mem_write32_(0x60004004, 0xFFFFFFFF); /* match=0xFFFFFFFF */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v1); } /* dummy 读×3 等待跨域同步 */

    /* S2: en=1，验证计数器递增 */
    mem_write32_(0x6000400c, 0x4);   /* CCR=0x4（en=1,wen=0,ien=0） */
    for(i=0;i<10;i++){ mem_read32_(0x6000400c, &v1); } /* dummy 读×10 等待同步 */
    mem_read32_(0x60004000, &v1);    /* 读 current_value */
    for(i=0;i<10;i++){ mem_read32_(0x60004000, &v1); } /* dummy 读×10 */
    mem_read32_(0x60004000, &v2);    /* 再读 current_value */
    if(v2 <= v1){printf("ERR S2: current_value not increasing v1=0x%08x v2=0x%08x\n", v1, v2);sim_fail();}

    /* S3: en=0，验证计数器停止 */
    mem_write32_(0x6000400c, 0x0);   /* CCR=0（en=0） */
    for(i=0;i<10;i++){ mem_read32_(0x6000400c, &v1); } /* dummy 读×10 */
    mem_read32_(0x60004000, &v1);    /* 读 current_value */
    for(i=0;i<10;i++){ mem_read32_(0x60004000, &v1); } /* dummy 读×10 */
    mem_read32_(0x60004000, &v2);    /* 再读 current_value */
    if(v2 != v1){printf("ERR S3: current_value not stable v1=0x%08x v2=0x%08x\n", v1, v2);sim_fail();}

    /* S4: 重新 en=1，验证再次递增 */
    mem_write32_(0x6000400c, 0x4);   /* CCR=0x4（en=1） */
    for(i=0;i<10;i++){ mem_read32_(0x6000400c, &v1); } /* dummy 读×10 */
    mem_read32_(0x60004000, &v1);
    for(i=0;i<10;i++){ mem_read32_(0x60004000, &v1); } /* dummy 读×10 */
    mem_read32_(0x60004000, &v2);
    if(v2 <= v1){printf("ERR S4: current_value not increasing after re-enable v1=0x%08x v2=0x%08x\n", v1, v2);sim_fail();}

    printf("rtc_counter_inc test successfully\n");
    sim_end();
}
