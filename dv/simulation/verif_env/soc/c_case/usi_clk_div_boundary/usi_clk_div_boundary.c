#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F15：CLK_DIV0 波特率边界（C+UVM 配对）
 *
 * RTL 实证（usi0.v）：UART bit 周期 = 16 × (CLK_DIV0+1) pclk
 *   → CLK_DIV0=0x81(129) → 2080 pclk/bit；CLK_DIV0=0x40(64) → 1040 pclk/bit
 *   比值恰好 2.0
 *
 * C 侧：USI0 UART，阶段1 DIV=0x81 连发 16×0x55（每 bit 都翻转，UVM 测沿间隔），
 *   停 → 阶段2 DIV=0x40 连发 16×0x55。
 * UVM 侧 soc_top_usi_uart_baud_test：记录 PAD_USI0_SD0 所有沿时刻，
 *   以最大沿间隔（阶段间重配置空隙）切分两簇，各取最小间隔（=1 bit 宽度），
 *   断言 簇1/簇2 ≈ 2.0（窗口 1.85~2.15）。
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart usi_clk_div_boundary\n");

    /* ---- 阶段1：DIV=0x81 ---- */
    mem_write32_(0x50028004, 0x0);          /* UART */
    mem_write32_(0x50028010, 0x81);
    mem_write32_(0x50028018, 0x3);          /* 8-N-1 */
    mem_write32_(0x50028000, 0x7);
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }
    for(i=0;i<16;i++){ mem_write32_(0x50028008, 0x55); }

    timeout=0;
    do{
        mem_read32_(0x5002800c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR P1: tx_empty timeout\n");sim_fail();}
    }while((v & 0x1) == 0);
    timeout=0;
    do{
        mem_read32_(0x5002801c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR P1: busy timeout\n");sim_fail();}
    }while((v & 0x1) != 0);

    /* ---- 阶段2：DIV=0x40（2 倍速） ---- */
    mem_write32_(0x50028000, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }
    mem_write32_(0x50028010, 0x40);
    mem_write32_(0x50028000, 0x7);
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }
    for(i=0;i<16;i++){ mem_write32_(0x50028008, 0x55); }

    timeout=0;
    do{
        mem_read32_(0x5002800c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR P2: tx_empty timeout\n");sim_fail();}
    }while((v & 0x1) == 0);
    timeout=0;
    do{
        mem_read32_(0x5002801c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR P2: busy timeout\n");sim_fail();}
    }while((v & 0x1) != 0);

    mem_write32_(0x50028000, 0x0);
    printf("usi_clk_div_boundary test successfully\n");
    sim_end();
}
