#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F17：USI 复位默认值（3 实例 × 全寄存器）
 *
 * 关键 RTL 实证（wujian100_open/soc/usi0.v:448-473 复位块 + 读译码 396-424）：
 *   多数寄存器复位值非 0（UG/plan 表格称全 0，属文档偏差，以 RTL 为准）：
 *     USI_CTRL=0x0, MODE_SEL=0x0, FIFO_STA=0x5(tx_empty=1+rx_empty=1),
 *     CLK_DIV0=0x20, CLK_DIV1=0x30, UART_CTRL=0x3(8-N-1), I2C_MODE=0x1,
 *     I2C_ADDR=0x133, I2CM_CTRL=0x0, I2CM_CODE=0x1, I2CS_CTRL=0x0,
 *     I2C_FM_DIV=0x5, I2C_HOLD=0x5, SPI_MODE=0x1, SPI_CTRL=0x7,
 *     INTR_CTRL=0x101(rx_th[9:8]=01+tx_th[1:0]=01), INTR_EN=0x0,
 *     INTR_UNMASK=0x0, DMA_CTRL=0x0, DMA_TH=0x808(rx=8,tx=8)
 *   SPI_NSS_DATA@0x6C 在 RTL 无地址译码（usi0.v ADDR 定义止于 0x68），
 *     读走 default 分支返回 0 —— 验证读 0 即确认未实现
 *   FIFO_STA 位序（usi0.v:399）：{rx_cnt[20:16], tx_cnt[12:8],
 *     rx_full[3], rx_empty[2], tx_full[1], tx_empty[0]}
 *
 * X 安全：不读 UART_STA/I2C_STA/SPI_STA/RAW_INTR_STA/INTR_STA
 *   （反映未驱动 pad 的 X 输入，会污染 CPU 寄存器堆）；
 *   不读写 TX/RX_FIFO@0x08（会 pop/push 数据口）；
 *   INTR_CLR@0x60 为 WO 不读。
 *
 * 断言清单：3 实例 × 上表 21 项逐项比对，不符即 ERR + sim_fail
 */
static unsigned int usi_base[3] = {0x50028000, 0x60028000, 0x50029000};

/* {offset, 期望复位值} —— 全部来自 usi0.v 复位块实证 */
static unsigned int exp_tbl[][2] = {
    {0x00, 0x000},   /* USI_CTRL    */
    {0x04, 0x000},   /* MODE_SEL    */
    {0x0C, 0x005},   /* FIFO_STA    */
    {0x10, 0x020},   /* CLK_DIV0    */
    {0x14, 0x030},   /* CLK_DIV1    */
    {0x18, 0x003},   /* UART_CTRL   */
    {0x20, 0x001},   /* I2C_MODE    */
    {0x24, 0x133},   /* I2C_ADDR    */
    {0x28, 0x000},   /* I2CM_CTRL   */
    {0x2C, 0x001},   /* I2CM_CODE   */
    {0x30, 0x000},   /* I2CS_CTRL   */
    {0x34, 0x005},   /* I2C_FM_DIV  */
    {0x38, 0x005},   /* I2C_HOLD    */
    {0x40, 0x001},   /* SPI_MODE    */
    {0x44, 0x007},   /* SPI_CTRL    */
    {0x4C, 0x101},   /* INTR_CTRL   */
    {0x50, 0x000},   /* INTR_EN     */
    {0x5C, 0x000},   /* INTR_UNMASK */
    {0x64, 0x000},   /* DMA_CTRL    */
    {0x68, 0x808},   /* DMA_TH      */
    {0x6C, 0x000},   /* SPI_NSS_DATA（未实现，default 读 0） */
};

int test_start(void){
    uint32_t v=0;
    int i=0, j=0;
    int err=0;
    printf("\nstart usi_reset_default\n");

    for(i=0; i<3; i++){
        for(j=0; j<21; j++){
            mem_read32_(usi_base[i] + exp_tbl[j][0], &v);
            if(v != exp_tbl[j][1]){
                printf("ERR: USI%0d offset 0x%02x = 0x%08x expect 0x%03x\n",
                       i, exp_tbl[j][0], v, exp_tbl[j][1]);
                err++;
            }
        }
    }

    if(err){ printf("usi_reset_default FAILED: %0d mismatches\n", err); sim_fail(); }
    printf("usi_reset_default test successfully\n");
    sim_end();
}
