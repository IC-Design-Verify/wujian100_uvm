#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F9 + F10：SPI 格式矩阵（CPOL/CPHA/DATA_SIZE）+ master→slave 数据通路
 *
 * 通路：USI0(master) → USI1(slave)，TB force PAD_USI1_{SCLK,NSS,SD0}=USI0 对应
 *   （apb0/tb_top/apb0_usi/usi_spi_test.v，USI_SPI define 编译入）
 *
 * SPI_CTRL[8:0] 实证（usi0.v:586-591）：
 *   [3:0] data_size：实际位宽 = N+1（N<3 强制按 7 即 8bit；0xF→16bit）
 *   [4] rx_disable（1=不收）  [5] tx_disable（1=不发）
 *   [6] cpha  [7] cpol  [8] nss_toggle
 *   基线：master=0x1F(tx only,16bit,cpol0,cpha0)，slave=0x2F(rx only,16bit)
 * SPI_MODE@0x40：1=master，0=slave；SPI_STA@0x48 bit0=busy
 *
 * 矩阵（每轮：双方停 → 重配 → USI0 发 2 帧 → USI1 收并比对 → 停）：
 *   R1: 16bit CPOL0 CPHA0（基线复现 + 数据比对）
 *   R2: 8bit  CPOL0 CPHA1
 *   R3: 8bit  CPOL1 CPHA0
 *   R4: 4bit  CPOL1 CPHA1
 */
#define USI0 0x50028000
#define USI1 0x60028000

static unsigned int mcfg[4] = {0x1F,   0x57,   0x97,   0xD3};
static unsigned int scfg[4] = {0x2F,   0x67,   0xA7,   0xE3};
static unsigned int sdat0[4] = {0x5A5A, 0xA5,  0x3C,   0x5};
static unsigned int sdat1[4] = {0x1234, 0x96,  0xC3,   0xA};

static void spi_round(int idx){
    uint32_t v=0;
    int cnt=0, timeout=0, i=0;

    /* 双方全停 */
    mem_write32_(USI0+0x00, 0x0);
    mem_write32_(USI1+0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(USI0+0x00, &v); }

    /* USI0 master */
    mem_write32_(USI0+0x04, 0x2);           /* MODE_SEL=SPI */
    mem_write32_(USI0+0x10, 0xc8);          /* SCK 100KHz */
    mem_write32_(USI0+0x40, 0x1);           /* master */
    mem_write32_(USI0+0x44, mcfg[idx]);
    mem_write32_(USI0+0x00, 0x7);           /* usi_en+fm_en+tx_fifo_en */
    /* USI1 slave */
    mem_write32_(USI1+0x04, 0x2);
    mem_write32_(USI1+0x40, 0x0);           /* slave */
    mem_write32_(USI1+0x44, scfg[idx]);
    mem_write32_(USI1+0x00, 0xb);           /* usi_en+fm_en+rx_fifo_en */
    for(i=0;i<3;i++){ mem_read32_(USI0+0x00, &v); }

    mem_write32_(USI0+0x08, sdat0[idx]);
    mem_write32_(USI0+0x08, sdat1[idx]);

    /* 等 master 发完 */
    timeout=0;
    do{
        mem_read32_(USI0+0x0c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR R%0d: tx_empty timeout\n", idx+1);sim_fail();}
    }while((v & 0x1) == 0);
    timeout=0;
    do{
        mem_read32_(USI0+0x48, &v);         /* SPI_STA bit0 busy */
        timeout++;
        if(timeout>=2000000){printf("ERR R%0d: spi busy timeout\n", idx+1);sim_fail();}
    }while((v & 0x1) != 0);

    /* USI1 收 2 帧并比对 */
    cnt=0; timeout=0;
    while(cnt<2){
        mem_read32_(USI1+0x0c, &v);
        timeout++;
        if(timeout>=4000000){printf("ERR R%0d: rx timeout, got %0d frames\n", idx+1, cnt);sim_fail();}
        if((v & 0x4) == 0){
            uint32_t exp = (cnt==0) ? sdat0[idx] : sdat1[idx];
            mem_read32_(USI1+0x08, &v);
            if(v != exp){
                printf("ERR R%0d: rx[%0d]=0x%04x expect 0x%04x\n", idx+1, cnt, v, exp);
                sim_fail();
            }
            cnt++;
        }
    }

    mem_write32_(USI0+0x00, 0x0);
    mem_write32_(USI1+0x00, 0x0);
    printf("round %0d (m SPI_CTRL=0x%02x) pass\n", idx+1, mcfg[idx]);
}

int test_start(void){
    int i;
    printf("\nstart usi_spi_format_matrix\n");
    for(i=0;i<4;i++){
        spi_round(i);
    }
    printf("usi_spi_format_matrix test successfully\n");
    sim_end();
}
