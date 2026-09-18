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

/* F9 + F11：RTC 复位默认值验证
 *
 * 关键 RTL 依据（wujian100_open/soc/rtc.v）：
 *   - RTC 基址 0x60004000；寄存器见 dsn.rst(2026-09-18 整理)
 *   - DIV@0x20: rtc_pdu_apbif 是 AOU 域寄存器组合镜像（rtc.v:216），
 *     DIV 复位读值 = 0x1（aou div_reg reset 20'b1），不是 UG 标称的 0x4000。
 *     该差异记录如下（Spec-vs-RTL）。
 *   - COMP_VERSION@0x1C: 读值 = 0x3230312a（rtc.v:20 RTC_VERSION_ID）
 *   - 其余寄存器复位均为 0（en=0 时计数器不动，current_value=0）
 *
 * 断言清单：
 *   S1: current_value@0x00 == 0            （en=0，计数器不动）
 *   S2: match_value@0x04  == 0
 *   S3: load_value@0x08   == 0
 *   S4: CCR@0x0C          == 0
 *   S5: int_status@0x10   == 0
 *   S6: raw_int_status@0x14 == 0
 *   S7: int_clr@0x18      == 0
 *   S8: COMP_VERSION@0x1C == 0x3230312a
 *   S9: DIV@0x20          == 0x1  （★ 不是 0x4000；UG 标称 0x4000，Spec-vs-RTL 差异）
 *  S10: 写 0x1C = 0xFFFFFFFF 后读回仍为 0x3230312a（RO 写忽略）
 */
int test_start(void){
    uint32_t v=0;
    printf("\nstart rtc_reset_default\n");

    /* S1: current_value RO */
    mem_read32_(0x60004000, &v);
    if(v != 0){printf("ERR S1: current_value=0x%08x expect 0\n", v); sim_fail();}

    /* S2: match_value */
    mem_read32_(0x60004004, &v);
    if(v != 0){printf("ERR S2: match_value=0x%08x expect 0\n", v); sim_fail();}

    /* S3: load_value */
    mem_read32_(0x60004008, &v);
    if(v != 0){printf("ERR S3: load_value=0x%08x expect 0\n", v); sim_fail();}

    /* S4: CCR */
    mem_read32_(0x6000400c, &v);
    if(v != 0){printf("ERR S4: CCR=0x%08x expect 0\n", v); sim_fail();}

    /* S5: int_status */
    mem_read32_(0x60004010, &v);
    if(v != 0){printf("ERR S5: int_status=0x%08x expect 0\n", v); sim_fail();}

    /* S6: raw_int_status */
    mem_read32_(0x60004014, &v);
    if(v != 0){printf("ERR S6: raw_int_status=0x%08x expect 0\n", v); sim_fail();}

    /* S7: int_clr */
    mem_read32_(0x60004018, &v);
    if(v != 0){printf("ERR S7: int_clr=0x%08x expect 0\n", v); sim_fail();}

    /* S8: COMP_VERSION 读值 = 0x3230312a (rtc.v:20 RTC_VERSION_ID) */
    mem_read32_(0x6000401c, &v);
    if(v != 0x3230312a){printf("ERR S8: COMP_VERSION=0x%08x expect 0x3230312a\n", v); sim_fail();}

    /* S9: DIV 复位读值 = 0x1（aou div_reg reset 20'b1, rtc.v:216）
     * ★ Spec-vs-RTL 差异：UG 标称 0x4000，此处 RTL 实测为 1 */
    mem_read32_(0x60004020, &v);
    if(v != 0x1){printf("ERR S9: DIV=0x%08x expect 0x1 (UG 标称 0x4000；Spec-vs-RTL 差异，见 rtc.v:216)\n", v); sim_fail();}

    /* S10: 写 COMP_VERSION（RO）→ 写忽略，读回仍 0x3230312a */
    mem_write32_(0x6000401c, 0xFFFFFFFF);
    mem_read32_(0x6000401c, &v);
    if(v != 0x3230312a){printf("ERR S10: COMP_VERSION write ignored test failed, readback=0x%08x expect 0x3230312a\n", v); sim_fail();}

    printf("rtc_reset_default test successfully\n");
    sim_end();
}
