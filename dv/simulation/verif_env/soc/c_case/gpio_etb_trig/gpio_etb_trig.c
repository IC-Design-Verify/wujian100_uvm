#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F11: GPIO ETB 触发输出——gpio0_etb_trig = int_level（极性调整后的 PAD 电平，
 * 2-flop pclk_int 同步；不经 en/mask/dir 门控，但 pclk_int 需 int_clk_en 才运行）
 *
 * C 侧配置 type=edge + inten 全使能以使能 pclk_int，再翻转 pol 使 int_level
 * 在 0x55555555 / 0xAAAAAAAA 间切换；UVM 侧 soc_top_gpio_etb_trig_test
 * 轮询 XMR tb_top.dut.x_aou_top.gpio0_etb_trig 确认两种值依序出现。
 *
 * 注：gpio0_etb_trig 在 wujian100_open_top 未连接到任何 ETB 消费者
 * （aou_top 内部线网，顶层端口未引出）——SoC 集成层面悬空，差异记录。
 *
 * PAD 激励：legacy 块 force PAD=0x55555555（全程 direction=Input 保持）。
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart gpio_etb_trig\n");

    mem_write32_(0x60018004, 0x0);          /* direction: 全 Input */
    mem_write32_(0x60018038, 0xFFFFFFFF);   /* type=edge（使 pclk_int 运行） */
    mem_write32_(0x60018030, 0xFFFFFFFF);   /* inten=全使能（int_clk_en=1） */

    mem_write32_(0x6001803C, 0xFFFFFFFF);   /* pol=high → int_level=0x55555555 */
    for(i=0;i<50;i++){ mem_read32_(0x60018044, &v); }  /* 保持供 UVM 观测 */

    mem_write32_(0x6001803C, 0x0);          /* pol=low → int_level=0xAAAAAAAA */
    for(i=0;i<50;i++){ mem_read32_(0x60018044, &v); }  /* 保持供 UVM 观测 */

    mem_write32_(0x60018030, 0x0);          /* 收尾 inten=0 */
    printf("gpio_etb_trig test successfully\n");
    sim_end();
}
