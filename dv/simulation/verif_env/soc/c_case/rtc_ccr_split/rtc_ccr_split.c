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

/* F4：CCR 控制位独立验证（ien / mask）
 *
 * 关键 RTL 依据（wujian100_open/soc/rtc.v）：
 *   - CCR@0x0C: bit3=wen, bit2=Rtc_en, bit1=rtc_mask, bit0=rtc_ien
 *   - match 触发：cnt==match 且 en 且 ien → raw/int_status=1；
 *     mask=1 时 int_status=0 但 raw 仍置 1
 *   - EOI：读 0x18 清中断
 *   - ien=0 时 int_intr=0 → int_flag 永不置位（raw 也保持 0）
 *   - en 位已由 rtc_counter_inc 覆盖，本用例不再重复验证
 *
 * 断言清单：
 *   S1 ien=0 屏蔽：CCR=0x4(en=1,ien=0) → counter 过 match → raw==0（ien=0 时 raw 也不置位）
 *   S2 mask=1：CCR=0x7 → raw==1 → int_status==0（mask 屏蔽）→ EOI → raw==0
 *   S3 mask=0 对照：CCR=0x5 → raw==1 → int_status==1 → EOI → 两者均==0
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart rtc_ccr_split\n");

    /* S1: ien=0 屏蔽 - CCR=0x4 (en=1, ien=0) */
    mem_write32_(0x6000400c, 0x0);   /* CCR=0 停 */
    mem_write32_(0x60004020, 0x0);   /* DIV=0 */
    mem_write32_(0x60004008, 0x0);   /* load=0 */
    mem_write32_(0x60004004, 0x40);  /* match=0x40 */
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x4);   /* en=1, ien=0 */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    /* 等 counter 早已过 0x40（约 200 次 dummy 读，每 ext_clk 5ns = 1us 足够）*/
    for(i=0;i<200;i++){ mem_read32_(0x60004000, &v); }

    mem_read32_(0x60004014, &v);
    if(v != 0){printf("ERR S1: raw_int_status=0x%08x expect 0 (ien=0 屏蔽)\n", v);sim_fail();}
    mem_write32_(0x6000400c, 0x0); /* CCR=0 停 */

    /* S2: mask=1 - CCR=0x7 (en=1, ien=1, mask=1) */
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x40);
    for(i=0;i<3;i++){ mem_read32_(0x60004008, &v); }
    mem_write32_(0x6000400c, 0x7);   /* en+ien+mask */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S2: timeout waiting raw==1\n");sim_fail();}
    }while(v != 0x1);

    mem_read32_(0x60004010, &v);
    if(v != 0){printf("ERR S2: int_status=0x%08x expect 0 (mask 屏蔽)\n", v);sim_fail();}

    mem_read32_(0x60004018, &v); /* EOI */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S2: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);

    /* S3: mask=0 对照 - CCR=0x5 (en=1, ien=1, mask=0) */
    mem_write32_(0x6000400c, 0x0);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x40);
    for(i=0;i<3;i++){ mem_read32_(0x60004008, &v); }
    mem_write32_(0x6000400c, 0x5);   /* en+ien, mask=0 */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S3: timeout waiting raw==1\n");sim_fail();}
    }while(v != 0x1);

    mem_read32_(0x60004010, &v);
    if(v != 0x1){printf("ERR S3: int_status=0x%08x expect 1 (mask=0 未屏蔽)\n", v);sim_fail();}

    mem_read32_(0x60004018, &v); /* EOI */
    timeout=0;
    do{
        mem_read32_(0x60004010, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S3: timeout waiting int_status==0\n");sim_fail();}
    }while(v != 0x0);
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S3: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);

    mem_write32_(0x6000400c, 0x0); /* 收尾 CCR=0 */
    printf("rtc_ccr_split test successfully\n");
    sim_end();
}
