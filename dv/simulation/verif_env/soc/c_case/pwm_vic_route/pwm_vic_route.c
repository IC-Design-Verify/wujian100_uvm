#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F14：PWM 中断 VIC 路由（pwm_int → CPU pad_vic_int_vld[25]）
 *
 * RTL 实证：core_top.v:545 pad_vic_int_vld[25] = pwm_int
 *   pwmint = OR(各组中断) | int_fault；组中断原始位被各自 INTEN 门控。
 *
 * C 侧：group0 cnt_zero 中断（INTEN1 bit8），LOAD=0x100 持续运行；
 *   轮询 PWMRIS1 bit8 确认置位，保持一段时间（UVM 测 VIC 断言），
 *   再 PWMIC1 清除 + 确认（UVM 测 VIC 解除）。
 * UVM 侧 soc_top_pwm_vic_route_test：轮询 tb_top.dut.x_cpu_top.pad_vic_int_vld[25]
 *   先见 1 后见 0。
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart pwm_vic_route\n");

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c038, 0x100);        /* PWM01LOAD group0=0x100 */
    mem_write32_(0x5001c014, 0x100);        /* PWMINTEN1 bit8 = group0 cnt_zero */
    mem_write32_(0x5001c000, 0x1);          /* pwm0en */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001c01c, &v);        /* PWMRIS1 bit8 */
        timeout++;
        if(timeout>=100000){printf("ERR: timeout waiting PWMRIS1 bit8\n");sim_fail();}
    }while((v & 0x100) == 0);

    /* 保持中断断言窗口，给 UVM 轮询时间 */
    for(i=0;i<200;i++){ mem_read32_(0x5001c01c, &v); }

    mem_write32_(0x5001c024, 0x100);        /* PWMIC1 清 bit8 */
    for(i=0;i<3;i++){ mem_read32_(0x5001c01c, &v); }
    mem_read32_(0x5001c01c, &v);
    if((v & 0x100) != 0){printf("ERR: PWMRIS1 bit8 not cleared\n");sim_fail();}

    /* 保持解除窗口 */
    for(i=0;i<200;i++){ mem_read32_(0x5001c01c, &v); }

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c014, 0x0);
    printf("pwm_vic_route test successfully\n");
    sim_end();
}
