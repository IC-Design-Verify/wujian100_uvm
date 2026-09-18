#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F7: 中断约束——direction=Output 时禁止中断；附带验证 ctl(Hardware 模式)的 RTL 实际行为
 *
 * RTL 实证（gpio0.v gpio_apbif）：
 *   1) 中断门控只看 gpio_sw_dir（方向寄存器）——edge 置位含 gpio_sw_dir==0（:697），
 *      level 状态 dir==1 → 0（:711）；
 *   2) ctl@0x08 无任何写存储（write decode 无 GPIO_SW_HW_CTRL_OFFSET 分支），
 *      读恒返回 32'b0（:509）——即 Hardware 模式在本 RTL 配置中未实现，
 *      写 ctl 是完全无操作（这也解释了"ctl 不参与中断门控"）。
 *
 * 执行顺序注意：legacy 激励块 apb1/tb_top/apb1_gpio/gpio_test.v 在 oe==0 期间
 * force PAD=0x55555555，一旦 oe==0xFFFFFFFF 即 release 并进入后续 wait 状态
 * （不再回 force 0x55555555）→ 依赖 PAD 激励的 B 段必须先做，
 * Output 段（DUT 自驱 PAD，不依赖 legacy force）后做。
 */
int test_start(void){
    uint32_t v=0;
    printf("\nstart gpio_intr_constraint\n");

    mem_write32_(0x60018030, 0xFFFFFFFF);   /* inten=全使能 */
    mem_write32_(0x60018034, 0x0);   /* mask=0 */
    mem_write32_(0x6001803C, 0xFFFFFFFF);   /* pol=high（全 bit） */

    /* B（先做，依赖 legacy PAD 激励）: 写 ctl=1(Hardware) + direction=Input + level
     * RTL 中 ctl 无存储、无门控作用，中断仍按 Software 模式产生 */
    mem_write32_(0x60018004, 0x0);        /* direction: 全 Input（PAD=legacy 0x55555555） */
    mem_write32_(0x60018038, 0x0);        /* type=level */
    mem_write32_(0x60018008, 0x1);        /* ctl=Hardware —— 写无效（RTL 未实现） */
    mem_read32_(0x60018008, &v);
    if(v != 0){printf("ERR B: ctl readback=0x%08x expect 0 (ctl 无存储, RTL 实测)\n", v);sim_fail();}
    mem_read32_(0x60018044, &v);
    if(v != 0x55555555){printf("ERR B: raw=0x%08x expect 0x55555555 (ctl 无门控, RTL 实测)\n", v);sim_fail();}

    /* A1: Output + level 模式（DUT 自驱 PAD=0xFFFFFFFF，dir 门控应无中断） */
    mem_write32_(0x60018004, 0xFFFFFFFF); /* direction: 全 Output */
    mem_write32_(0x60018000, 0xFFFFFFFF);
    mem_read32_(0x60018040, &v);
    if(v != 0){printf("ERR A1: intstatus=0x%08x expect 0 (Output+level)\n", v);sim_fail();}
    mem_read32_(0x60018044, &v);
    if(v != 0){printf("ERR A1: raw=0x%08x expect 0 (Output+level)\n", v);sim_fail();}

    /* A2: Output + edge 模式，制造真实边沿（0→全1 上升沿）
     * 注意：edge 状态位不经 type 门控——B 阶段 pol 翻转已留下残留，
     * 须先经 0x60（RTL 实际清除地址）清掉，再验证 dir 门控 */
    mem_write32_(0x60018038, 0xFFFFFFFF); /* type=edge（全 bit） */
    mem_write32_(0x60018060, 0xFFFFFFFF); /* 清 edge 残留（0x4C 在 RTL 未解码，无效） */
    mem_read32_(0x60018044, &v);
    if(v != 0){printf("ERR A2: raw after clr@0x60=0x%08x expect 0\n", v);sim_fail();}
    mem_write32_(0x60018000, 0x0);
    mem_write32_(0x60018000, 0xFFFFFFFF);
    mem_read32_(0x60018040, &v);
    if(v != 0){printf("ERR A2: intstatus=0x%08x expect 0 (Output+edge)\n", v);sim_fail();}
    mem_read32_(0x60018044, &v);
    if(v != 0){printf("ERR A2: raw=0x%08x expect 0 (Output+edge)\n", v);sim_fail();}

    mem_write32_(0x60018004, 0x0);        /* 收尾回 Input */
    mem_write32_(0x60018030, 0x0);        /* inten=0 */
    printf("gpio_intr_constraint test successfully\n");
    sim_end();
}
