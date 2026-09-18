#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F12: RTC 中断路由——rtc0_vic_intr → core_top ip_cpu_int_vld[26]
 *
 * C 侧制造"断言→撤销"：match 触发中断（ien=1, mask=0），保持若干拍后 EOI 清除。
 * UVM 侧 soc_top_rtc_vic_route_test 轮询 XMR
 * tb_top.dut.x_cpu_top.pad_vic_int_vld[26] 确认断言与撤销。
 * 路由依据：core_top.v:546 assign ip_cpu_int_vld[26] = rtc_wic_intr。
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart rtc_vic_route\n");

    mem_write32_(0x6000400c, 0x0);      /* 停 */
    mem_write32_(0x60004020, 0x0);      /* DIV=0 */
    mem_write32_(0x60004008, 0x0);      /* load=0 */
    mem_write32_(0x60004004, 0x40);     /* match=0x40 */
    for(i=0;i<5;i++){ mem_read32_(0x60004020, &v); }

    mem_write32_(0x6000400c, 0x5);      /* en + ien，mask=0 → 中断可上路 */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR: timeout waiting raw==1\n");sim_fail();}
    }while(v != 0x1);

    for(i=0;i<100;i++){ mem_read32_(0x60004010, &v); }  /* 保持断言供 UVM 观测 */

    mem_read32_(0x60004018, &v);        /* EOI → 撤销 */
    timeout=0;
    do{
        mem_read32_(0x60004014, &v);
        timeout++;
        if(timeout >= 100000){printf("ERR: timeout waiting raw==0 after EOI\n");sim_fail();}
    }while(v != 0x0);

    for(i=0;i<100;i++){ mem_read32_(0x60004010, &v); }  /* 保持撤销供 UVM 观测 */

    mem_write32_(0x6000400c, 0x0);
    printf("rtc_vic_route test successfully\n");
    sim_end();
}
