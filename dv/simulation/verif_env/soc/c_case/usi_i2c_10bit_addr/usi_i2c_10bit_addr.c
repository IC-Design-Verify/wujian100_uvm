#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F5：I2C 地址模式（7-bit 正向 + 10-bit 阴性）
 *
 * 通路：USI0(master) ←→ USI1(slave)（TB bridge：SCLK 直连 + SDA 按 oe_n 换向，
 *   apb0/tb_top/apb0_usi/usi_i2c_test.v，USI_I2C define 编译入）
 *
 * 关键 RTL 实证（usi0.v）：
 *   - master 10-bit：i2cm_amode = I2CM_CTRL[0]（:579）；地址两字节由
 *     I2C_ADDR[9:0] 直接发出（:1360-1392 状态机）
 *   - slave 10-bit 不支持：i2cs_amode = 1'b0 硬连线（:583）→ slave 只匹配 7-bit
 *   - raw_intr_sta[13] = i2c_nack_intr（:426-444 位序）
 *   - I2CM_CTRL[1] = tx fifo 空时产生 stop（基线用法 0x2）
 *
 * 断言清单：
 *   S1（7-bit 正向对照）：双方 0x3c 7-bit，USI0 发 2 字节，USI1 收到并比对一致
 *   S2（10-bit 阴性）：USI0 改 10-bit（amode=1, addr=0x133），USI1 保持 7-bit 0x3c
 *       → 地址阶段 NACK（RAW_INTR bit13 置位），USI1 RX 无数据
 *       → 证明 10-bit 地址相位真实发出且与 7-bit slave 不匹配（协议正确行为）
 */
int test_start(void){
    uint32_t v=0;
    int i=0, cnt=0, timeout=0;
    printf("\nstart usi_i2c_10bit_addr\n");

    /* ============ S1：7-bit 正向对照 ============ */
    /* USI0 master 7-bit（同基线） */
    mem_write32_(0x50028004, 0x1);          /* MODE_SEL=I2C */
    mem_write32_(0x50028010, 0x63);         /* CLK_DIV0 */
    mem_write32_(0x50028014, 0x63);         /* CLK_DIV1 */
    mem_write32_(0x50028020, 0x1);          /* I2C_MODE=master */
    mem_write32_(0x50028024, 0x3c);         /* I2C_ADDR=0x3c */
    mem_write32_(0x50028028, 0x2);          /* I2CM_CTRL: stop if tx empty */
    mem_write32_(0x50028000, 0x7);          /* usi_en+fm_en+tx_fifo_en */
    /* USI1 slave 7-bit */
    mem_write32_(0x60028004, 0x1);
    mem_write32_(0x60028020, 0x0);          /* slave */
    mem_write32_(0x60028024, 0x3c);
    mem_write32_(0x60028000, 0xb);          /* usi_en+fm_en+rx_fifo_en */
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }

    mem_write32_(0x50028008, 0xa5);
    mem_write32_(0x50028008, 0xa6);

    timeout=0;
    do{
        mem_read32_(0x5002800c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR S1: tx_empty timeout\n");sim_fail();}
    }while((v & 0x1) == 0);
    timeout=0;
    do{
        mem_read32_(0x5002803c, &v);        /* I2C_STA bit0 busy */
        timeout++;
        if(timeout>=2000000){printf("ERR S1: i2c busy timeout\n");sim_fail();}
    }while((v & 0x1) != 0);

    cnt=0; timeout=0;
    while(cnt<2){
        mem_read32_(0x6002800c, &v);
        timeout++;
        if(timeout>=4000000){printf("ERR S1: rx timeout, got %0d\n", cnt);sim_fail();}
        if((v & 0x4) == 0){
            uint32_t exp = (cnt==0) ? 0xa5 : 0xa6;
            mem_read32_(0x60028008, &v);
            if(v != exp){printf("ERR S1: rx[%0d]=0x%02x expect 0x%02x\n", cnt, v, exp);sim_fail();}
            cnt++;
        }
    }
    printf("S1 7-bit loopback pass\n");

    /* ============ S2：10-bit 阴性 ============ */
    mem_write32_(0x50028000, 0x0);          /* 双方停 */
    mem_write32_(0x60028000, 0x0);
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }

    /* USI0 master 10-bit：amode=1（I2CM_CTRL bit0），addr=0x133 */
    mem_write32_(0x50028024, 0x133);
    mem_write32_(0x50028028, 0x3);          /* amode=1 + stop-if-empty */
    mem_write32_(0x50028050, 0x2000);       /* INTR_EN bit13 = i2c_nack */
    /* USI1 保持 7-bit slave 0x3c */
    mem_write32_(0x50028000, 0x7);
    mem_write32_(0x60028000, 0xb);
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }

    mem_write32_(0x50028008, 0x5a);         /* 触发传输 */

    timeout=0;
    do{
        mem_read32_(0x50028058, &v);        /* RAW_INTR_STA bit13 */
        timeout++;
        if(timeout>=2000000){printf("ERR S2: timeout waiting i2c_nack, RAW=0x%08x\n", v);sim_fail();}
    }while((v & 0x2000) == 0);

    /* USI1 不应收到任何数据 */
    mem_read32_(0x6002800c, &v);
    if((v & 0x4) == 0){printf("ERR S2: USI1 unexpectedly received data, FIFO_STA=0x%08x\n", v);sim_fail();}

    /* 清除 + 收尾 */
    mem_write32_(0x50028050, 0x0);          /* 先关 EN（raw 门控清零） */
    mem_write32_(0x50028060, 0x2000);       /* INTR_CLR bit13 */
    for(i=0;i<3;i++){ mem_read32_(0x50028058, &v); }
    mem_read32_(0x50028058, &v);
    if((v & 0x2000) != 0){printf("ERR S2: nack not cleared, RAW=0x%08x\n", v);sim_fail();}

    mem_write32_(0x50028000, 0x0);
    mem_write32_(0x60028000, 0x0);
    printf("S2 10-bit negative pass\n");
    printf("usi_i2c_10bit_addr test successfully\n");
    sim_end();
}
