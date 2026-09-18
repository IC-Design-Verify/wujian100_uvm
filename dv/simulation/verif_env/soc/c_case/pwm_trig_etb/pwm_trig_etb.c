#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F3 + F11：PWM 触发输出 / ETB 触发输出
 *
 * RTL 实证（pwm.v）：
 *   - trigger = (cnt==trigval 上升沿 & trenu) | (下降沿 & trend)
 *     trenu/trend 来自 PWMINVERTTRIG 高位：tr0enu=bit13, tr0end=bit14
 *   - trigval = {pwm01trig_h[15:0], pwm01trig[15:0]}（PWM01TRIG@0x08）
 *   - pwm_xx_trig = OR(trigger0..5)（triginv 默认 0）
 *     → 输出到 apb0_sub_top.pwm_xx_trig（SoC 顶层未接 ETB，只内部可见）
 *   - pwm_tim_etb_trig = tim_cnt_match_flag_divedge（每个 group 的 tim 匹配）
 *     → apb0_sub_top.pwm_tim0_etb_trig
 *   - F11（ETB 触发输入）在 aou/apb0 顶层 tie 0，无法激励，仅记录。
 *
 * C 侧：PWM01TRIG=0x80，INVERTTRIG bit13=1（tr0enu），pwm0en 持续运行；
 *   TIM01LOAD tim0=0x100 + tim0en + TIM_INT_EN bit0（tim 匹配产生 etb 脉冲）。
 * UVM 侧 soc_top_pwm_trig_etb_test 用 wait(===) 在 XMR 上数脉冲：
 *   tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_xx_trig ≥ 1
 *   tb_top.dut.x_pdu_top.x_sub_apb0_top.pwm_tim0_etb_trig ≥ 1
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart pwm_trig_etb\n");

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c038, 0x100);        /* PWM01LOAD group0=0x100 */
    mem_write32_(0x5001c008, 0x80);         /* PWM01TRIG: trigval0=0x80 */
    mem_write32_(0x5001c004, 0x2000);       /* PWMINVERTTRIG bit13 = tr0enu */
    mem_write32_(0x5001c0b0, 0x100);        /* TIM01LOAD: tim0=0x100 */
    mem_write32_(0x5001c0a0, 0x1);          /* TIM_INT_EN: tim0 */
    mem_write32_(0x5001c000, 0x40001);      /* pwm0en(bit0) + tim0en(bit18) */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    /* 确认 tim0 中断确实发生（间接确认 etb 脉冲源活动） */
    timeout=0;
    do{
        mem_read32_(0x5001c0a4, &v);        /* TIMRIS bit0 */
        timeout++;
        if(timeout>=100000){printf("ERR: timeout waiting TIMRIS bit0\n");sim_fail();}
    }while((v & 0x1) == 0);
    mem_write32_(0x5001c0a8, 0x1);          /* TIM_INT_CLR tim0 */

    /* 保持运行窗口，给 UVM 数脉冲 */
    for(i=0;i<1000;i++){ mem_read32_(0x5001c044, &v); }

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c004, 0x0);
    mem_write32_(0x5001c0a0, 0x0);
    printf("pwm_trig_etb test successfully\n");
    sim_end();
}
