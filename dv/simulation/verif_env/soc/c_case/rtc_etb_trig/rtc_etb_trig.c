#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F10: RTC ETB 触发输出——rtc_etb_trig 在 match 时产生 1 个 ext_clk 周期脉冲
 *
 * RTL 实证（rtc.v rtc_ig，:688-697）：rtc_etb_trig 是 cmp_res 在 i_rtc_ext_clk
 * 域的寄存（match 且 cnt_en → 下一拍拉高，再下一拍回落）。
 * SoC 集成：rtc_etb_trig 在 aou_top 为内部线网（aou_top.v:420/:586），
 * wujian100_open_top 未引出——无 ETB 消费者，差异记录。
 * etb_rtc_trig 输入在 aou_top.v:583 tie 1'b0（其功能是置位 cr_reg[2] 启动
 * counter，rtc.v:220）——SoC 级不可激励，仅注释记录。
 *
 * C 侧：制造两次 match（match 后 EOI + 重装 load 再来一次），
 * UVM 侧 soc_top_rtc_etb_trig_test 轮询 XMR tb_top.dut.x_aou_top.rtc_etb_trig
 * 确认脉冲出现 ≥2 次。
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart rtc_etb_trig\n");

    mem_write32_(0x6000400c, 0x0);      /* 停 */
    mem_write32_(0x60004020, 0x0);      /* DIV=0：每 ext_clk 计 1 次 */
    mem_write32_(0x60004008, 0x0);      /* load=0 */
    mem_write32_(0x60004004, 0x40);     /* match=0x40 */
    for(i=0;i<5;i++){ mem_read32_(0x60004020, &v); }

    /* 第 1 次 match */
    mem_write32_(0x6000400c, 0x5);      /* en + ien */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR M1: timeout waiting raw==1\n");sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004018, &v);        /* EOI */

    /* 第 2 次 match：重装 load=0 让 counter 重新经过 match（wen=0 不回绕，须重装） */
    mem_write32_(0x6000400c, 0x0);
    for(i=0;i<5;i++){ mem_read32_(0x6000400c, &v); }
    mem_write32_(0x60004008, 0x0);      /* load=0 → counter 重新加载 */
    for(i=0;i<5;i++){ mem_read32_(0x60004008, &v); }
    mem_write32_(0x6000400c, 0x5);
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR M2: timeout waiting raw==1\n");sim_fail();}
    }while(v != 0x1);
    mem_read32_(0x60004018, &v);        /* EOI */
    mem_write32_(0x6000400c, 0x0);

    printf("rtc_etb_trig test successfully\n");
    sim_end();
}
