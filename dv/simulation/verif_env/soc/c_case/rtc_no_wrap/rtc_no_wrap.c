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

/* F2：wrap 使能行为 + wrap-to-0 实证（RTC）
 *
 * 关键 RTL 依据（wujian100_open/soc/rtc.v）：
 *   - CCR@0x0C: bit3=wen(wrap使能)，bit2=Rtc_en(计数使能)，bit1=rtc_mask，bit0=rtc_ien
 *   - wrap（wen=1 且 cnt==match）时 cnt<=0（rtc.v rtc_cnt），
 *     **不是**回绕到 load_value——Spec-vs-RTL 差异（UG 描述"wrap 回 load_value"）
 *   - int 触发：cnt==match 且 en 且 ien → raw/int_status=1；读 0x18 (EOI) 清除；
 *     mask=1 时 int_status=0 但 raw 仍置位
 *   - counter 计数时钟 = ext_clk 每 (DIV+1) 个周期 1 拍；DIV=0 → 每个 ext_clk 计 1 次
 *
 * 断言清单：
 *   P1: DIV=0, load=0, match=0x40, CCR=0x5（en=1,ien=1,wen=0），轮询 raw_int_status@0x14==1
 *       超时上限 100000 次，超时 sim_fail
 *   P2: 读 EOI@0x18 清除中断
 *   P3: 循环读 current_value（最多 200000 次），一旦读到 > match+0x40 即越过后未回绕
 *       （wen=0 不 wrap），break；未超到则 sim_fail
 *   P4（wrap-to-0 实证）：CCR=0 停；load=0x100; match=0x140; CCR=0xD（wen=1,en=1,ien=1）
 *       轮询 raw==1；读 EOI；密集读 current_value（2000 次），若读到 < 0x100 即证明 wrap 到 0
 *       而非 load_value；未读到则 sim_fail
 *   ★ Spec-vs-RTL 差异：UG 描述"wrap 回 load_value"，实际 RTL 为 cnt<=0
 */
int test_start(void){
    uint32_t v=0, v1=0, v2=0;
    int i=0, timeout=0;
    printf("\nstart rtc_no_wrap\n");

    /* P1: 基础配置 - DIV=0, load=0, match=0x40, CCR=0x5（en=1,ien=1,wen=0）*/
    mem_write32_(0x60004020, 0x0);    /* DIV=0 */
    mem_write32_(0x60004008, 0x0);    /* load=0 */
    mem_write32_(0x60004004, 0x40);    /* match=0x40 */
    for(i=0;i<3;i++){ mem_read32_(0x60004020, &v); } /* dummy 读×3 跨域同步 */
    mem_write32_(0x6000400c, 0x5);    /* CCR=0x5（en=1,ien=1,wen=0） */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); } /* dummy 读×3 跨域同步 */

    /* P1: 轮询 raw_int_status@0x14==1，超时上限 100000 */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR P1: timeout waiting raw_int_status==1 (raw=0x%08x)\n", v);sim_fail();}
    }while(v != 0x1);

    /* P2: 读 EOI@0x18 清除中断 */
    mem_read32_(0x60004018, &v);

    /* P3: wen=0 情况下验证计数器越过后不回绕（不会减到 0 以下） */
    /* 先记录当前值 */
    mem_read32_(0x60004000, &v1);
    for(i=0;i<200000;i++){
        mem_read32_(0x60004000, &v);
        if(v > 0x40 + 0x40){break;} /* 超过 match+0x40 即未回绕 */
    }
    if(v <= 0x40 + 0x40){printf("ERR P3: counter did not exceed match+0x40, v=0x%08x (可能已回绕)\n", v);sim_fail();}

    /* P4（wrap-to-0 实证）：wen=1，wrap 时 cnt<=0（不是回绕到 load_value） */
    mem_write32_(0x6000400c, 0x0);    /* CCR=0 停 */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); } /* dummy 读×3 同步 */
    mem_write32_(0x60004008, 0x100);  /* load=0x100 */
    mem_write32_(0x60004004, 0x140);  /* match=0x140 */
    for(i=0;i<3;i++){ mem_read32_(0x60004008, &v); } /* dummy 读×3 同步 */
    mem_write32_(0x6000400c, 0xD);    /* CCR=0xD（wen=1,en=1,ien=1） */
    for(i=0;i<3;i++){ mem_read32_(0x6000400c, &v); } /* dummy 读×3 同步 */

    /* 轮询 raw_int_status@0x14==1，超时上限 100000 */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR P4: timeout waiting raw_int_status==1 (raw=0x%08x)\n", v);sim_fail();}
    }while(v != 0x1);

    /* 读 EOI 清除 */
    mem_read32_(0x60004018, &v);

    /* 密集读 current_value 2000 次，若读到 < 0x100 即证明 wrap 到 0（不是 load_value 0x100） */
    /* 注意：不能用 v2==0 作未检出哨兵——wrap-to-0 的合法观测值本身就含 0，须用独立 found 标志 */
    { int found=0;
      for(i=0;i<2000;i++){
        mem_read32_(0x60004000, &v1);
        if(v1 < 0x100){found=1; break;} /* wrap-to-0 实证 */
      }
      if(!found){printf("ERR P4: counter never wrapped below load=0x100 (read 2000 samples); "
                        "可能 UG 描述 wrap 回 load_value 与 RTL cnt<=0 不符\n");sim_fail();}
    }

    printf("rtc_no_wrap test successfully\n");
    sim_end();
}
