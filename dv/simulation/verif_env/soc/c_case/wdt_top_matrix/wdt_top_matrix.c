#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* WDT_USER_TOP_0..15（wdt_params.v）：TOP 编码表 */
static const uint32_t top_table[16] = {
  0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
  0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
  0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
  0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF
};

int test_start(void){
    uint32_t v=0, exp=0;
    int j=0, d=0;

    /* ① TOP_INIT 抽查：time_out=0xF0 → TOP_INIT=15 → 首次使能加载 0x7FFFFFFF
     * （been_started=0 时 top_sel 选 TOP_INIT；仅首次使能可观测） */
    mem_write32_(0x50008004,0xF0);
    mem_write32_(0x50008000,0x1);        /* CR: EN=1, RMOD=0 */
    for(d=0;d<20;d++){mem_read32_(0x50008008,&v);}  /* 等 initial load（哑读延时，防空循环被 -O3 优化） */
    mem_read32_(0x50008008,&v);
    if(v>0x7FFFFFFF || v<0x7FFF0000){
        printf("ERR: TOP_INIT[15] load got 0x%08x expect ~0x7FFFFFFF\n",v);sim_fail();
    }

    /* ② TOP 全 16 档：重新使能（0→1 上升沿触发 initial_load，
     *   been_started=1 后 top_sel 选 TOP[j]），读 current_value 比对编码表 */
    for(j=0;j<16;j++){
        mem_write32_(0x50008000,0x0);    /* 先禁用（RTL 实测 EN 可软件清） */
        mem_write32_(0x50008004,(uint32_t)j);  /* TOP=j（TOP_INIT 此时无关） */
        mem_write32_(0x50008000,0x1);    /* 重新使能 → 加载 TOP[j] */
        for(d=0;d<20;d++){mem_read32_(0x50008008,&v);}  /* 等 initial load 完成 */
        mem_read32_(0x50008008,&v);
        exp = top_table[j];
        /* 允许读回前少量递减（APB 读延迟 ~数十 pclk），窗口 0x1000 */
        if(v>exp || v<exp-0x1000){
            printf("ERR: TOP[%d] load got 0x%08x expect ~0x%08x\n",j,v,exp);sim_fail();
        }
    }

    mem_write32_(0x50008000,0x0);        /* 收尾禁用 WDT，防超时复位 */
    printf("wdt_top_matrix test successfully\n");
    sim_end();
}
