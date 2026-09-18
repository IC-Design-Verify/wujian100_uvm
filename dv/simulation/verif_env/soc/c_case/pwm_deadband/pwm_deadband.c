#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F6：PWM 死区（deadband）—— group0 互补对 CH0/CH1
 *
 * RTL 实证（pwm.v）：
 *   - PWM01DB@0x68：db0en=bit24, db1en=bit25, delay0=[11:0], delay1=[23:12]
 *   - deadband_en 时同组两路输出互补并插入 delay 个 tick 的死区
 *   - group0 = pwm0/pwm1（PWM01LOAD 低半 = group0 load）
 *
 * 配置：LOAD=399（周期 400 tick），CMPA=200，db0en=1，delay0=0x10
 * UVM 侧 soc_top_pwm_deadband_test 在 PAD_PWM_CH0/CH1 上检查：
 *   两路均翻转、从不同时为高（死区无重叠）。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_deadband\n");

    mem_write32_(0x5001c000, 0x0);          /* PWMCFG=0 全停 */
    mem_write32_(0x5001c038, 399);          /* PWM01LOAD group0=399 */
    mem_write32_(0x5001c050, 200);          /* PWM0CMP compa=200 */
    mem_write32_(0x5001c068, 0x01000010);   /* PWM01DB: db0en(bit24)=1, delay0=0x10 */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    mem_write32_(0x5001c000, 0x3);          /* pwm0en(bit0)+pwm1en(bit1) */
    for(i=0;i<1500;i++){ mem_read32_(0x5001c044, &v); }  /* 跑 20+ 周期 */

    mem_write32_(0x5001c000, 0x0);
    printf("pwm_deadband test successfully\n");
    sim_end();
}
