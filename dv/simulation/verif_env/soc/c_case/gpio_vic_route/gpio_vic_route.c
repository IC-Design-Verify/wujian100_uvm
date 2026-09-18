#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F12: GPIO 中断路由——gpio_intr_flag → core_top ip_cpu_int_vld[16]
 *
 * C 侧制造"断言→撤销"电平；UVM 侧 soc_top_gpio_vic_route_test
 * 轮询 XMR tb_top.dut.x_cpu_top.pad_vic_int_vld[16] 确认断言与撤销。
 *
 * PAD 激励：legacy 块 force PAD=0x55555555（全程 direction=Input 保持）。
 * level-high + 全使能 + 不屏蔽 → intstatus=0x55555555 → flag 断言；
 * inten=0 → intstatus 归 0 → flag 撤销。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart gpio_vic_route\n");

    mem_write32_(0x60018004, 0x0);          /* direction: 全 Input（保持 legacy force） */
    mem_write32_(0x60018038, 0x0);          /* type=level */
    mem_write32_(0x6001803C, 0xFFFFFFFF);   /* pol=high（全 bit） */
    mem_write32_(0x60018034, 0x0);          /* mask=0 */
    mem_write32_(0x60018030, 0xFFFFFFFF);   /* inten=全使能 → 中断断言 */

    mem_read32_(0x60018040, &v);
    if(v != 0x55555555){printf("ERR: intstatus=0x%08x expect 0x55555555\n", v);sim_fail();}

    for(i=0;i<50;i++){ mem_read32_(0x60018040, &v); }  /* 保持断言一段时间供 UVM 观测 */

    mem_write32_(0x60018030, 0x0);          /* inten=0 → 中断撤销 */
    mem_read32_(0x60018040, &v);
    if(v != 0){printf("ERR: intstatus after inten=0 =0x%08x expect 0\n", v);sim_fail();}

    for(i=0;i<50;i++){ mem_read32_(0x60018040, &v); }  /* 保持撤销一段时间供 UVM 观测 */

    printf("gpio_vic_route test successfully\n");
    sim_end();
}
