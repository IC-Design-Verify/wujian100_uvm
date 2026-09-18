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

/* F5：中断状态/原始状态/EOI 清除语义验证
 *
 * 关键 RTL 依据（wujian100_open/soc/rtc.v）：
 *   - raw_int_status@0x14 与 int_status@0x10 都来自 int_state
 *   - mask=1 时 int_status=0 但 raw 仍置 1
 *   - EOI：读 0x18 清中断（int_flag/int_state 均清）
 *   - ien=0 时 int_intr=0 → int_flag 永不置位（raw 也保持 0）
 *
 * 断言清单：
 *   S1: CCR=0x7(mask=1) → raw==1 → int_status==0
 *   S2: 读 EOI → raw==0, int_status==0
 *   S3: mask=0(0x5) — counter 重装后 → raw==1 且 int_status==1 → EOI → 两者均==0
 *   S4: 0x18 读值 == 0（EOI 读返回 0）
 *   收尾 CCR=0
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart rtc_int_mask_raw\n");

    /* S1: CCR=0x7 (en+ien+mask=1) */
    mem_write32_(0x6000400c, 0x0);
    mem_write32_(0x60004020, 0x0);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x40);
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x7);   /* en+ien+mask */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR S1: timeout waiting raw==1\n");sim_fail();}
    }while(v != 0x1);

    mem_read32_(0x60004010, &v);
    if(v != 0){printf("ERR S1: int_status=0x%08x expect 0 (mask 屏蔽)\n", v);sim_fail();}

    /* S2: 读 EOI → raw 可清 */
    mem_read32_(0x60004018, &v);

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S2: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);

    mem_read32_(0x60004010, &v);
    if(v != 0){printf("ERR S2: int_status=0x%08x expect 0\n", v);sim_fail();}

    /* S3: mask=0 对照 — 先重装 counter */
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
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S3: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);
    timeout=0;
    do{
        mem_read32_(0x60004010, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR S3: timeout waiting int_status==0\n");sim_fail();}
    }while(v != 0x0);

    /* S4: EOI 寄存器读值本身 == 0（读 0x18 恒返回 0，无需等待中断；
     *  注意：不能在此重新触发中断——counter 自 S3 起一直运行已远超 match，
     *  wen=0 不回绕，必须重装 load 才能再触发，简化为直接读值检查） */
    mem_read32_(0x60004018, &v);
    if(v != 0){printf("ERR S4: int_clr readback=0x%08x expect 0\n", v);sim_fail();}

    mem_write32_(0x6000400c, 0x0); /* 收尾 CCR=0 */
    printf("rtc_int_mask_raw test successfully\n");
    sim_end();
}
