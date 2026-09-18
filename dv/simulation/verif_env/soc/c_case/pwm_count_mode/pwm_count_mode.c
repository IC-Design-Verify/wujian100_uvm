#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F4: 计数模式 up vs up-down
 *
 * RTL 实证（pwm.v pwm_gen）：
 *   - up 模式（pwm_mode=0）：cnt 0→load 回 0，周期 = LOAD+1 tick
 *   - up-down（pwm_mode=1）：inc_flag 在 load-1 与 1 处翻转，
 *     周期 = 2×(LOAD-1) tick ≈ 2×up
 *   - PWMCTL[5:0] = pwm0~5mode
 *
 * 阶段1：up 模式；阶段2：up-down 模式（PWMCTL bit0=1）
 * UVM 侧 soc_top_pwm_count_mode_test 测首/末周期比 ≈ 2。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart pwm_count_mode\n");

    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c038, 799);
    mem_write32_(0x5001c050, 200);
    mem_write32_(0x5001c034, 0x0);          /* PWMCTL: up 模式 */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    mem_write32_(0x5001c000, 0x1);          /* pwm0en */
    for(i=0;i<800;i++){ mem_read32_(0x5001c044, &v); }

    mem_write32_(0x5001c034, 0x1);          /* pwm0mode=1 up-down */
    for(i=0;i<1600;i++){ mem_read32_(0x5001c044, &v); }  /* 周期加倍，等待加倍 */

    mem_write32_(0x5001c000, 0x0);
    printf("pwm_count_mode test successfully\n");
    sim_end();
}
