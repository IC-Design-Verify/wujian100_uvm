#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F12 + F13：USI TX FIFO 状态/阈值 + 中断寄存器链
 *
 * 关键 RTL 实证（wujian100_open/soc/usi0.v）：
 *   - FIFO_STA@0x0C（:399）：{rx_cnt[20:16], tx_cnt[12:8],
 *     rx_full[3], rx_empty[2], tx_full[1], tx_empty[0]}，复位 0x5
 *   - raw_intr_sta[17:0]（:426-444）位序（低位侧）：
 *     [0]tx_thold [1]tx_empty [2]tx_full [3]tx_read_err [4]tx_write_err
 *     [5]rx_thold [6]rx_empty [7]rx_full [8]rx_read_err [9]rx_write_err
 *     [10]uart_stop [11]uart_perr [12]i2c_stop [13]i2c_nack
 *     [14]i2cm_lose_arbi [15]i2cs_gcall [16]i2c_aerr [17]spi_stop
 *   - raw 位被 INTR_EN 门控（:3257/3306/3310：~fifo_intr_en 时强制清 0，
 *     与 PWM 同款门控语义）
 *   - INTR_CTRL@0x4C（:416）：{intr_edge[16], rx_fifo_th[9:8], tx_fifo_th[1:0]}
 *     th 编码（:3290-3297）：00=关断 01=4 10=8 11=12；intr_edge=0 时
 *     tx_thold 条件为 cnt<=th（电平型）
 *   - INTR_STA = raw & intr_mask（:445）；usi_intr = |INTR_STA（:594）
 *   - INTR_UNMASK@0x5C 即 intr_mask；INTR_CLR@0x60 写 1 清 raw
 *
 * 策略：USI0 UART、CLK_DIV0=0xFFFF（极慢波特，TX FIFO 基本不排空）
 * 断言清单：
 *   S1：写 4 字节 → FIFO_STA tx_cnt[12:8]==4、tx_empty=0
 *   S2：写满 16 → tx_full=1、tx_cnt==16
 *   S3：阈值中断：INTR_EN bit0=1 + th=01(≤4) → 满时 thold=0；禁用 USI 排空 FIFO
 *        后（cnt<=4）RAW bit0==1；INTR_CLR 后……电平型会重触发——先关 EN 再 CLR 验证清 0
 *   S4：mask 行为：EN=1 + UNMASK=0 → RAW=1 而 INTR_STA=0；UNMASK=1 → INTR_STA=1
 *   S5：EN=0 门控：清残留后 EN=0，事件条件下 RAW 保持 0
 * X 安全：只操作 TX 侧/USI0；不读任何 mode-status 寄存器（UART_STA 等）
 */
int test_start(void){
    uint32_t v=0;
    int i=0;
    printf("\nstart usi_fifo_threshold\n");

    /* ---- 配置：USI0 UART，极慢波特 ---- */
    mem_write32_(0x50028004, 0x0);          /* MODE_SEL=UART */
    mem_write32_(0x50028010, 0xffff);       /* CLK_DIV0 最大（几乎不排空） */
    mem_write32_(0x50028018, 0x3);          /* 8-N-1 */
    mem_write32_(0x50028000, 0x7);          /* usi_en+fm_en+tx_fifo_en */
    for(i=0;i<3;i++){ mem_read32_(0x50028000, &v); }

    /* ---- S1：4 字节 FIFO 计数 ---- */
    mem_write32_(0x50028008, 0x60);
    mem_write32_(0x50028008, 0x61);
    mem_write32_(0x50028008, 0x62);
    mem_write32_(0x50028008, 0x63);
    for(i=0;i<3;i++){ mem_read32_(0x5002800c, &v); }
    mem_read32_(0x5002800c, &v);
    /* 波特极慢，但 shift register 可能已取走 1 字节 → cnt 为 3 或 4 */
    v = (v >> 8) & 0x1f;
    if(v < 3 || v > 4){printf("ERR S1: tx_cnt=%0d expect 3~4\n", v);sim_fail();}

    /* ---- S2：写满至 16 → tx_full ---- */
    for(i=0;i<13;i++){ mem_write32_(0x50028008, 0xa0+i); }
    for(i=0;i<3;i++){ mem_read32_(0x5002800c, &v); }
    mem_read32_(0x5002800c, &v);
    if((v & 0x2) == 0){printf("ERR S2: tx_full not set, FIFO_STA=0x%08x\n", v);sim_fail();}
    if(((v >> 8) & 0x1f) != 16){printf("ERR S2: tx_cnt=%0d expect 16\n", (v>>8)&0x1f);sim_fail();}

    /* ---- S3：阈值中断（th=01 → cnt<=4 电平型） ---- */
    mem_write32_(0x50028050, 0x1);          /* INTR_EN bit0 = tx_thold */
    for(i=0;i<3;i++){ mem_read32_(0x50028050, &v); }
    mem_read32_(0x50028058, &v);            /* RAW：满时 cnt=16>4 → thold=0 */
    if((v & 0x1) != 0){printf("ERR S3: tx_thold set while full, RAW=0x%08x\n", v);sim_fail();}

    mem_write32_(0x50028000, 0x0);          /* 禁用 USI → FIFO 清空（fifo_en=0 → cnt=0） */
    for(i=0;i<5;i++){ mem_read32_(0x50028000, &v); }
    mem_read32_(0x50028058, &v);            /* cnt=0<=4 → thold=1 */
    if((v & 0x1) == 0){printf("ERR S3: tx_thold not set when empty, RAW=0x%08x\n", v);sim_fail();}

    /* ---- S4：mask 行为（此时 thold 条件仍成立） ---- */
    mem_read32_(0x50028054, &v);            /* INTR_STA：UNMASK=0 → 0 */
    if((v & 0x1) != 0){printf("ERR S4: INTR_STA bit0 set while masked, STA=0x%08x\n", v);sim_fail();}
    mem_write32_(0x5002805c, 0x1);          /* INTR_UNMASK bit0 */
    for(i=0;i<3;i++){ mem_read32_(0x50028054, &v); }
    mem_read32_(0x50028054, &v);
    if((v & 0x1) == 0){printf("ERR S4: INTR_STA bit0 not set after unmask, STA=0x%08x\n", v);sim_fail();}

    /* 电平型清除：先关 EN 再 CLR，验证清 0 */
    mem_write32_(0x50028050, 0x0);          /* INTR_EN=0（raw 被强制清） */
    mem_write32_(0x50028060, 0x1);          /* INTR_CLR bit0 */
    for(i=0;i<3;i++){ mem_read32_(0x50028058, &v); }
    mem_read32_(0x50028058, &v);
    if((v & 0x1) != 0){printf("ERR S4: RAW bit0 not cleared, RAW=0x%08x\n", v);sim_fail();}

    /* ---- S5：EN=0 门控（重开 USI+TX FIFO，EN 保持 0） ---- */
    mem_write32_(0x50028000, 0x7);
    for(i=0;i<5;i++){ mem_read32_(0x50028000, &v); }
    mem_read32_(0x50028058, &v);            /* 条件成立(cnt<=4)但 EN=0 → RAW=0 */
    if((v & 0x1) != 0){printf("ERR S5: RAW bit0 set with EN=0, RAW=0x%08x\n", v);sim_fail();}

    /* ---- 收尾 ---- */
    mem_write32_(0x50028000, 0x0);
    mem_write32_(0x5002805c, 0x0);

    printf("usi_fifo_threshold test successfully\n");
    sim_end();
}
