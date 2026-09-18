#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F16：多实例地址独立性（USI2 镜像 + 交叉干扰）
 *
 * RTL/集成实证：
 *   - USI0=0x50028000(APB0 P4), USI1=0x60028000(APB1 P4), USI2=0x50029000(APB0 P5)
 *   - 三实例同一 RTL（usi0.v/usi1.v/usi2.v 同构），寄存器映射一致
 *   - USI2 TXD pad 在 TB 未接线，仅验证内部发送流程完成（FIFO 排空+busy 落）
 *
 * 断言清单：
 *   S1：USI2 配 UART(8-N-1, 9600) 发 4 字节，FIFO_STA tx_empty=1 且 UART_STA busy=0
 *   S2：USI2 配置后 USI0/USI1 寄存器保持复位值（交叉干扰检查）
 *   S3：USI0 独立可配（CLK_DIV0 写读回环），且不影响 USI2 已配置值
 */
int test_start(void){
    uint32_t v=0;
    int i=0, timeout=0;
    printf("\nstart usi_mirror_inst\n");

    /* ---- S1：USI2 UART 发送 ---- */
    mem_write32_(0x50029004, 0x0);          /* USI2 MODE_SEL=UART */
    mem_write32_(0x50029010, 0x81);         /* CLK_DIV0=0x81 (9600@20M) */
    mem_write32_(0x50029018, 0x3);          /* UART_CTRL 8-N-1 */
    mem_write32_(0x50029000, 0x7);          /* USI_CTRL: usi_en+fm_en+tx_fifo_en */
    for(i=0;i<3;i++){ mem_read32_(0x50029000, &v); }

    mem_write32_(0x50029008, 0x70);
    mem_write32_(0x50029008, 0x71);
    mem_write32_(0x50029008, 0x72);
    mem_write32_(0x50029008, 0x73);

    timeout=0;
    do{
        mem_read32_(0x5002900c, &v);        /* FIFO_STA bit0 tx_empty */
        timeout++;
        if(timeout>=2000000){printf("ERR S1: USI2 tx_empty timeout, FIFO_STA=0x%08x\n", v);sim_fail();}
    }while((v & 0x1) == 0);

    timeout=0;
    do{
        mem_read32_(0x5002901c, &v);        /* UART_STA bit0 busy */
        timeout++;
        if(timeout>=2000000){printf("ERR S1: USI2 busy timeout, UART_STA=0x%08x\n", v);sim_fail();}
    }while((v & 0x1) != 0);

    /* ---- S2：USI0/USI1 未被 USI2 干扰（保持复位值） ---- */
    mem_read32_(0x50028004, &v);            /* USI0 MODE_SEL */
    if(v != 0x0){printf("ERR S2: USI0 MODE_SEL=0x%08x expect 0\n", v);sim_fail();}
    mem_read32_(0x50028010, &v);            /* USI0 CLK_DIV0 */
    if(v != 0x20){printf("ERR S2: USI0 CLK_DIV0=0x%08x expect 0x20\n", v);sim_fail();}
    mem_read32_(0x60028004, &v);            /* USI1 MODE_SEL */
    if(v != 0x0){printf("ERR S2: USI1 MODE_SEL=0x%08x expect 0\n", v);sim_fail();}
    mem_read32_(0x60028010, &v);            /* USI1 CLK_DIV0 */
    if(v != 0x20){printf("ERR S2: USI1 CLK_DIV0=0x%08x expect 0x20\n", v);sim_fail();}

    /* ---- S3：USI0 独立可配，回读验证不影响 USI2 ---- */
    mem_write32_(0x50028010, 0x55);         /* USI0 CLK_DIV0=0x55 */
    for(i=0;i<3;i++){ mem_read32_(0x50028010, &v); }
    mem_read32_(0x50028010, &v);
    if(v != 0x55){printf("ERR S3: USI0 CLK_DIV0=0x%08x expect 0x55\n", v);sim_fail();}
    mem_read32_(0x50029010, &v);            /* USI2 CLK_DIV0 保持 0x81 */
    if(v != 0x81){printf("ERR S3: USI2 CLK_DIV0=0x%08x expect 0x81\n", v);sim_fail();}

    printf("usi_mirror_inst test successfully\n");
    sim_end();
}
