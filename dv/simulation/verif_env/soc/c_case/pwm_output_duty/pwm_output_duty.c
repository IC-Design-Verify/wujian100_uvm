#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F1(cntdiv) + F5(LOAD/CMP 周期占空比)：PWM 输出波形
 *
 * RTL 实证（pwm.v pwm_gen）：
 *   - pwm_0 = 1 当 pwm_cnt >= compa（:5123）→ 高电平 tick 数 = LOAD-CMP+1
 *   - up 模式周期 = LOAD+1 个计数 tick（cnt 到 load 回 0）
 *   - PWMCFG bit27=cntdiven, [26:24]=cntdiv 分频
 *
 * 阶段1：LOAD=799, CMPA=200（compa 在 PWM0CMP[15:0]）→ 高电平 600/800=75%
 * 阶段2：cntdiven=1, cntdiv=0（分频生效，周期变长）
 * UVM 侧 soc_top_pwm_output_duty_test 测 PAD_PWM_CH0 的首/末完整周期与占空比。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_output_duty\n");

    mem_write32_(0x5001c000, 0x0);          /* PWMCFG=0 全停 */
    mem_write32_(0x5001c038, 799);          /* PWM01LOAD group0=799（周期 800 tick） */
    mem_write32_(0x5001c050, 200);          /* PWM0CMP compa=200 → 高 600/800=75% */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    /* 阶段1：不分频 */
    mem_write32_(0x5001c000, 0x1);          /* pwm0en=1 */
    for(i=0;i<800;i++){ mem_read32_(0x5001c044, &v); }  /* ~10+ 周期 */

    /* 阶段2：cntdiven=1, cntdiv=0 → 周期变长 */
    mem_write32_(0x5001c000, 0x08000001);   /* cntdiven + pwm0en */
    for(i=0;i<800;i++){ mem_read32_(0x5001c044, &v); }

    mem_write32_(0x5001c000, 0x0);
    printf("pwm_output_duty test successfully\n");
    sim_end();
}
