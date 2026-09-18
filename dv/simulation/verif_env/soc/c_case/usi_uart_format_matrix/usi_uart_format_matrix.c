#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F3：UART 数据位/停止位/校验格式矩阵
 *
 * 通路：USI0(TX) → PAD_USI0_SD0 ──(TB force)── PAD_USI1_SCLK(RXD) → USI1(RX)
 *   （apb0/tb_top/apb0_usi/usi_uart_test.v，USI_UART define 编译入）
 *
 * UART_CTRL[5:0] 实证（usi0.v:570-573 + 3581-3603 case）：
 *   [1:0] dbit：00=5bit 01=6bit 10=7bit 11=8bit
 *   [3:2] pbit（停止位）：00=1, 01=1.5, 10=2
 *   [4] pen=校验使能；[5] eps=偶校验选择
 *
 * 矩阵（每轮：双方同格式重配 → USI0 发 2 字节 → USI1 收并比对 → 双方停）：
 *   R1: 8-N-1 (0x03)  数据 0x60/0x61   （基线格式复现+数据比对）
 *   R2: 7-E-1 (0x32)  数据 0x25/0x5A   （7bit+偶校验）
 *   R3: 8-O-2 (0x1B)  数据 0xC3/0x3C   （8bit+奇校验+2停止位）
 *   R4: 5-N-1 (0x00)  数据 0x0A/0x15   （5bit）
 *   R5: 6-N-1 (0x01)  数据 0x2B/0x3F   （6bit）
 * 数据值均按数据位宽掩码；格式不一致则 USI1 收到错误数据 → 比对失败。
 * ★RTL 实证发现：rx_shift 仅在上电复位清零（usi0.v:3723），帧间不清——
 *   sub-8bit 模式下 RX FIFO 高位残留上一帧数据（如 R3 的 0xC3 bit5 会在
 *   R4 的 5bit 接收中漏出）。比对前必须按数据位宽掩码（软件使用惯例）。
 */
#define USI0 0x50028000
#define USI1 0x60028000

static unsigned int fmt_tbl[5]  = {0x03, 0x32, 0x1B, 0x00, 0x01};
static unsigned int wmask[5]    = {0xFF, 0x7F, 0xFF, 0x1F, 0x3F};
static unsigned int data0[5]    = {0x60, 0x25, 0xC3, 0x0A, 0x2B};
static unsigned int data1[5]    = {0x61, 0x5A, 0x3C, 0x15, 0x3F};

static void uart_cfg_pair(unsigned int fmt){
    uint32_t v=0;
    int i;
    /* 双方全停 */
    mem_write32_(USI0+0x00, 0x0);
    mem_write32_(USI1+0x00, 0x0);
    for(i=0;i<3;i++){ mem_read32_(USI0+0x00, &v); }
    /* USI0 TX 侧 */
    mem_write32_(USI0+0x04, 0x0);           /* MODE_SEL=UART */
    mem_write32_(USI0+0x10, 0x81);          /* 9600 */
    mem_write32_(USI0+0x18, fmt);
    mem_write32_(USI0+0x00, 0x7);           /* usi_en+fm_en+tx_fifo_en */
    /* USI1 RX 侧 */
    mem_write32_(USI1+0x04, 0x0);
    mem_write32_(USI1+0x10, 0x81);
    mem_write32_(USI1+0x18, fmt);
    mem_write32_(USI1+0x00, 0xb);           /* usi_en+fm_en+rx_fifo_en */
    for(i=0;i<3;i++){ mem_read32_(USI0+0x00, &v); }
}

static void uart_round(int idx){
    uint32_t v=0;
    int cnt=0, timeout=0;
    uart_cfg_pair(fmt_tbl[idx]);

    mem_write32_(USI0+0x08, data0[idx]);
    mem_write32_(USI0+0x08, data1[idx]);

    /* 等 TX 完成 */
    timeout=0;
    do{
        mem_read32_(USI0+0x0c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR R%0d: tx_empty timeout\n", idx+1);sim_fail();}
    }while((v & 0x1) == 0);
    timeout=0;
    do{
        mem_read32_(USI0+0x1c, &v);
        timeout++;
        if(timeout>=2000000){printf("ERR R%0d: tx busy timeout\n", idx+1);sim_fail();}
    }while((v & 0x1) != 0);

    /* USI1 收 2 字节并比对 */
    cnt=0;
    timeout=0;
    while(cnt<2){
        mem_read32_(USI1+0x0c, &v);
        timeout++;
        if(timeout>=4000000){printf("ERR R%0d: rx timeout, got %0d bytes\n", idx+1, cnt);sim_fail();}
        if((v & 0x4) == 0){                 /* rx_empty==0 → 有数据 */
            uint32_t exp = (cnt==0) ? data0[idx] : data1[idx];
            mem_read32_(USI1+0x08, &v);
            v = v & wmask[idx];             /* sub-8bit：掩掉 rx_shift 残留高位 */
            if(v != exp){
                printf("ERR R%0d: rx[%0d]=0x%02x expect 0x%02x\n", idx+1, cnt, v, exp);
                sim_fail();
            }
            cnt++;
        }
    }

    /* 双方停 */
    mem_write32_(USI0+0x00, 0x0);
    mem_write32_(USI1+0x00, 0x0);
}

int test_start(void){
    int i;
    printf("\nstart usi_uart_format_matrix\n");
    for(i=0;i<5;i++){
        uart_round(i);
        printf("round %0d (UART_CTRL=0x%02x) pass\n", i+1, fmt_tbl[i]);
    }
    printf("usi_uart_format_matrix test successfully\n");
    sim_end();
}
