#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F13：多组 PWM 独立配置
 *
 * RTL 实证：6 组独立 LOAD/计数器（pwm_cnt_0..5），groupN 用 PWMxxLOAD 对应半字。
 *   group0 = pwm0/pwm1（PWM01LOAD 低半），group3 = pwm6/pwm7（PWM23LOAD 高半）
 *
 * 配置：group0 LOAD=0x100（CH0），group3 LOAD=0x400（CH6），同 CMP 比例 1/2。
 * UVM 侧 soc_top_pwm_multi_group_test 测 CH0/CH6 周期比 ≈ 4。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_multi_group\n");

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c038, 0x100);        /* PWM01LOAD: group0=0x100 */
    mem_write32_(0x5001c03c, 0x04000100);   /* PWM23LOAD: group3=0x400（高半），group2=0x100 */
    mem_write32_(0x5001c050, 0x80);         /* PWM0CMP compa=0x80（50%） */
    mem_write32_(0x5001c05c, 0x200);        /* PWM3CMP compa=0x200（50%） */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    mem_write32_(0x5001c000, 0x41);         /* pwm0en(bit0) + pwm6en(bit6) */
    for(i=0;i<2000;i++){ mem_read32_(0x5001c044, &v); }

    mem_write32_(0x5001c000, 0x0);
    printf("pwm_multi_group test successfully\n");
    sim_end();
}
