#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F2: PWM 输出极性反转（PWMINVERTTRIG）
 *
 * RTL 实证（pwm.v）：pwm_0_out = pwm0_inv ? ~pwm_0_out_pre : pwm_0_out_pre
 *   → 反转前后占空比互补（高电平占比 75% ↔ 25%）
 *
 * 阶段1：正常极性（LOAD=799, CMPA=200 → 高 75%）
 * 阶段2：PWMINVERTTRIG bit0=1 → 高 25%
 * UVM 侧 soc_top_pwm_polarity_invert_test 测首/末完整周期占空比互补。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_polarity_invert\n");

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c038, 799);
    mem_write32_(0x5001c050, 200);
    mem_write32_(0x5001c004, 0x0);          /* INVERTTRIG=0 正常 */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    mem_write32_(0x5001c000, 0x1);          /* pwm0en */
    for(i=0;i<800;i++){ mem_read32_(0x5001c044, &v); }

    mem_write32_(0x5001c004, 0x1);          /* pwm0inv=1 → 极性反转 */
    for(i=0;i<800;i++){ mem_read32_(0x5001c044, &v); }

    mem_write32_(0x5001c000, 0x0);
    printf("pwm_polarity_invert test successfully\n");
    sim_end();
}
