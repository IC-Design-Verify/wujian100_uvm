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

/* F3：match 边界验证
 *
 * 关键 RTL 依据（wujian100_open/soc/rtc.v）：
 *   - counter 节拍 = ext_clk/(DIV+1)；DIV=0 时每 5ns 计 1 次
 *   - match 触发：cnt==match 且 en 且 ien → int_flag 置位；
 *     raw_int_status@0x14 与 int_status@0x10 都来自 int_state；
 *     mask=1 时 int_status=0 但 raw 仍置 1
 *   - EOI：读 0x18 清中断（int_flag/int_state 均清）
 *   - 写寄存器后经 2-flop 跨域同步生效，写后读回前加 2~3 次 dummy 读
 *   - cnt_en=0 时 counter 冻结
 *   - match 触发后 counter 继续走（wen=0 时）或回 0（wen=1 时），
 *     raw 置位后读 current_value 已越过 match，不要断言"恰好等于 match"
 *   - ★ 重要：ien=0 时 int_intr=0 → int_flag 永不置位（raw 也保持 0）
 *
 * 断言清单：
 *   R1-M0x1:  load=0 match=0x1 CCR=0x5 → raw==1 → 读 0x00 v>=0x1 → EOI → raw==0
 *   R2-M0x40: load=0 match=0x40 CCR=0x5 → raw==1 → v>=0x40 → EOI → raw==0
 *   R3-M0x1000: load=0 match=0x1000 CCR=0x5 → raw==1 → v>=0x1000 → EOI → raw==0
 *   R4-M0x0:  特殊边界 load=0 match=0 → en+ien 后 raw 应立即（若干周期内）置 1
 *   ★ 跳过 match=0xFFFFFFFF 超大边界：仿真时长不可行，注释说明
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart rtc_match_boundary\n");

    /* R4: M=0 特殊边界 - load=0 match=0，起点即匹配 */
    mem_write32_(0x60004020, 0x0);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x5); /* en+ien, wen=0 */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR R4: timeout waiting raw==1 for M=0\n");sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004018, &v); /* EOI */

    /* R1: M=0x1 */
    mem_write32_(0x6000400c, 0x0); mem_write32_(0x60004020, 0x0);
    mem_write32_(0x60004008, 0x0); mem_write32_(0x60004004, 0x1);
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); }
    mem_write32_(0x6000400c, 0x5);
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR R1: timeout waiting raw==1 M=0x1\n");sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004000, &v);
    if(v < 0x1){printf("ERR R1: current_value=0x%08x expect >=0x1\n", v);sim_fail();}
    mem_read32_(0x60004018, &v); /* EOI */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR R1: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);

    /* R2: M=0x40（★ 必须重装 load=0：R1 停机时 counter 可能已越过 0x40，
     *    不重装则 cnt>match 永不匹配 → 轮询超时） */
    mem_write32_(0x6000400c, 0x0);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x40);
    for(i=0;i<3;i++){ mem_read32_(0x60004008, &v); }
    mem_write32_(0x6000400c, 0x5);
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR R2: timeout waiting raw==1 M=0x40\n");sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004000, &v);
    if(v < 0x40){printf("ERR R2: current_value=0x%08x expect >=0x40\n", v);sim_fail();}
    mem_read32_(0x60004018, &v);
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR R2: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);

    /* R3: M=0x1000（★ 同样重装 load=0） */
    mem_write32_(0x6000400c, 0x0);
    mem_write32_(0x60004008, 0x0);
    mem_write32_(0x60004004, 0x1000);
    for(i=0;i<3;i++){ mem_read32_(0x60004008, &v); }
    mem_write32_(0x6000400c, 0x5);
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); }

    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR R3: timeout waiting raw==1 M=0x1000\n");sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004000, &v);
    if(v < 0x1000){printf("ERR R3: current_value=0x%08x expect >=0x1000\n", v);sim_fail();}
    mem_read32_(0x60004018, &v);
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 10000){printf("ERR R3: timeout waiting raw==0\n");sim_fail();}
    }while(v != 0x0);

    /* ★ 跳过 M=0xFFFFFFFF 超大边界：仿真时长不可行，注释说明 */

    mem_write32_(0x6000400c, 0x0); /* 收尾 CCR=0 */
    printf("rtc_match_boundary test successfully\n");
    sim_end();
}
