#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F5/F6: GPIO 中断 4 件套（inten/intmask/inttype/intpol）+ 状态/原始状态/清除
 *
 * 激励来源（关键依赖）：legacy 激励块 apb1/tb_top/apb1_gpio/gpio_test.v 在
 * oe==全输入 时 force 全部 PAD = 0x55555555 并保持（等 oe==0xFFFFFFFF 永不发生）。
 * 本用例全程保持 direction=0（全 Input），故 PAD 恒定 0x55555555：
 *   奇数 bit (0,2,4..) = 1，偶数 bit (1,3,5..) = 0
 *
 * RTL 语义（gpio0.v gpio_ctrl，已查证）：
 *   - pol=1: 高电平/上升沿有效；pol=0: 输入取反（低电平/下降沿有效）
 *   - raw = en 门控后的状态（en=0 → raw==0）；intstatus = raw & ~mask
 *   - edge 状态位：int_edge_out 置位（不经 type 门控），写 0x60（实际清除地址，
 *     RTL 将 gpio_int_clr_wen 解码在 INT_LEVEL_SYNC_OFFSET；UG 标称的 0x4C 未解码、
 *     写无效——差异记录）写 1 清除；
 *     level 状态为组合逻辑，激励仍在时无法清除
 *   - 中断仅在 direction=Input 时产生（gpio_sw_dir==0 门控）
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart gpio_intr_combo\n");

    mem_write32_(0x60018004, 0x0);   /* direction: 全 Input（保持！） */

    /* S1: inten=0 基线——即使 PAD0=1，en 门控使 raw/intstatus 全 0 */
    mem_write32_(0x60018030, 0x0);   /* inten=0 */
    mem_read32_(0x60018044, &v);     /* rawintstatus */
    if(v != 0){printf("ERR S1: raw=0x%08x expect 0 (inten=0 gate)\n", v);sim_fail();}
    mem_read32_(0x60018040, &v);     /* intstatus */
    if(v != 0){printf("ERR S1: intstatus=0x%08x expect 0\n", v);sim_fail();}

    /* S2: level-high：inten=1, type=0(level), pol=1(high), mask=0
     *     PAD=0x55555555 → 奇数 bit 高电平 → raw=intstatus=0x55555555 */
    mem_write32_(0x60018038, 0x0);   /* type=level */
    mem_write32_(0x6001803C, 0xFFFFFFFF);   /* pol=high（全 bit 高电平有效） */
    mem_write32_(0x60018034, 0x0);   /* mask=0（全不屏蔽） */
    mem_write32_(0x60018030, 0xFFFFFFFF);   /* inten=全使能 */
    mem_read32_(0x60018044, &v);
    if(v != 0x55555555){printf("ERR S2: raw=0x%08x expect 0x55555555\n", v);sim_fail();}
    mem_read32_(0x60018040, &v);
    if(v != 0x55555555){printf("ERR S2: intstatus=0x%08x expect 0x55555555\n", v);sim_fail();}

    /* S3: mask bit0 → intstatus bit0 清 0，raw 不变 */
    mem_write32_(0x60018034, 0x1);   /* mask bit0 */
    mem_read32_(0x60018040, &v);
    if(v != 0x55555554){printf("ERR S3: intstatus=0x%08x expect 0x55555554\n", v);sim_fail();}
    mem_read32_(0x60018044, &v);
    if(v != 0x55555555){printf("ERR S3: raw=0x%08x expect 0x55555555 (mask 不影响 raw)\n", v);sim_fail();}
    mem_write32_(0x60018034, 0x0);   /* 解除 mask */

    /* S4: level-low（pol=0，输入取反）→ 偶数 bit（PAD=0）有效 */
    mem_write32_(0x6001803C, 0x0);   /* pol=low */
    mem_read32_(0x60018044, &v);
    if(v != 0xAAAAAAAA){printf("ERR S4: raw=0x%08x expect 0xAAAAAAAA (pol=low)\n", v);sim_fail();}

    /* S5: edge 模式 + 清中断
     * ★ RTL 实证（gpio0.v）：int_clr 的实际写解码是 GPIO_INT_LEVEL_SYNC_OFFSET(0x60)，
     *   写 0x60 时 gpio_int_clr_wen 置位、pwdata 组合驱动 gpio_int_clr 清 edge 状态位；
     *   UG 标称的 int_clr@0x4C（GPIO_INT_CLR_OFFSET）在 RTL 中无任何解码——写 0x4C 无效。
     * ★ edge 状态位不经 type 门控：S2~S4 的 pol 翻转已在 status_edge 留下残留，
     *   且 int_level 经 pclk_int 2-flop 同步，切 edge 模式后需等待再清。
     *   本阶段验证：a) 0x60 清 edge 有效；b) 0x4C 清 edge 无效（Spec-vs-RTL 差异记录）。 */
    mem_write32_(0x60018038, 0xFFFFFFFF);   /* type=edge（全 bit） */
    for(i=0;i<20;i++){ mem_read32_(0x60018040, &v); } /* 等 int_level 同步/残留沿落位 */
    mem_write32_(0x60018060, 0xFFFFFFFF); /* 清 edge 状态位（实际清除地址 0x60；副作用 level_sync=1，edge 路径不受影响） */
    mem_read32_(0x60018044, &v);
    if(v != 0){printf("ERR S5: raw after clr@0x60=0x%08x expect 0\n", v);sim_fail();}
    mem_write32_(0x6001803C, 0xFFFFFFFF);   /* pol 全 0→全 1：pad=1 的 bit 产生上升沿 */
    for(i=0;i<20;i++){ mem_read32_(0x60018040, &v); } /* 等 int_level 2-flop 同步 + 沿捕获 */
    mem_read32_(0x60018044, &v);
    if(v != 0x55555555){printf("ERR S5: raw(edge)=0x%08x expect 0x55555555\n", v);sim_fail();}
    mem_read32_(0x60018044, &v);     /* 再读：edge 粘性保持 */
    if(v != 0x55555555){printf("ERR S5: edge not sticky, raw=0x%08x\n", v);sim_fail();}
    mem_write32_(0x6001804C, 0xFFFFFFFF); /* UG 标称 int_clr@0x4C —— RTL 未解码，应清不掉 */
    mem_read32_(0x60018044, &v);
    if(v != 0x55555555){printf("ERR S5: clr@0x4C unexpectedly cleared, raw=0x%08x (expect sticky 0x55555555 per RTL)\n", v);sim_fail();}
    mem_write32_(0x60018060, 0xFFFFFFFF); /* 0x60 再清 */
    mem_read32_(0x60018044, &v);
    if(v != 0){printf("ERR S5: raw after 2nd clr@0x60=0x%08x expect 0 (edge 可清)\n", v);sim_fail();}

    mem_write32_(0x60018030, 0x0);   /* 收尾 inten=0 */
    printf("gpio_intr_combo test successfully\n");
    sim_end();
}
