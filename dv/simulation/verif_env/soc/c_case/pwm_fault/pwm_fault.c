#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F10：PWM fault 中断
 *
 * RTL 实证（pwm.v:4718）：
 *   int_fault <= 1 当 (fault & intenfault)，PWMIC1 bit0 写 1 清除
 *   fault = PAD_PWM_FAULT 输入 —— TB 悬空为 X，必须由 UVM 侧 force
 *
 * 协同协议：
 *   UVM（soc_top_pwm_fault_test）：t=0 起 force tb_top.PAD_PWM_FAULT=0（消 X），
 *     延迟后 force 1（注入 fault），保持到测试结束。
 *   C 侧：INTEN1[0]=1，轮询 PWMRIS1 bit0 置位 → PWMIC1 bit0 清除 → 确认清 0。
 *   轮询带超时上界：若 UVM 未注入则失败而非挂死。
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart pwm_fault\n");

    mem_write32_(0x5001c014, 0x1);          /* PWMINTEN1 bit0 = int_fault 使能 */
    for(i=0;i<3;i++){ mem_read32_(0x5001c014, &v); }

    /* 等 UVM force PAD_PWM_FAULT=1 */
    timeout=0;
    do{
        mem_read32_(0x5001c01c, &v);        /* PWMRIS1 */
        timeout++;
        if(timeout>=200000){printf("ERR: timeout waiting PWMRIS1 bit0 (fault)\n");sim_fail();}
    }while((v & 0x1) == 0);

    mem_write32_(0x5001c014, 0x0);          /* 先关 INTEN1[0]：fault 仍为高，防止清除后立刻重触发 */
    mem_write32_(0x5001c024, 0x1);          /* PWMIC1 bit0 清除 */
    for(i=0;i<3;i++){ mem_read32_(0x5001c01c, &v); }
    mem_read32_(0x5001c01c, &v);
    if((v & 0x1) != 0){printf("ERR: PWMRIS1 bit0 not cleared, RIS1=0x%08x\n", v);sim_fail();}
    printf("pwm_fault test successfully\n");
    sim_end();
}
