#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F7: RTC PDU→AOU 跨域同步（C 侧可观测部分）
 *
 * RTL 结构（rtc.v）：CPU 在 PDU 域（pclk）写寄存器，经 pdu_aou_wen_* 握手 +
 * 2-flop 同步写入 AOU 域寄存器；CPU 读回的 mr_reg/cr_reg/div_reg 是 AOU 域
 * 寄存器的组合镜像（rtc_pdu_apbif）→ 写后读回相等即证明 PDU→AOU→镜像 全链路。
 *
 * 本用例：对 4 个可写寄存器分别写入特征值，轮询读回直到等于写入值
 * （有界迭代），证明跨域同步在有限拍内收敛。
 * 注：跨域信号（pdu_aou_wen_*）的波形级时序由代码检视覆盖（rtc.v:196
 * gate_en / rtc_clr_sync 等），本用例验证其功能结果。
 */
int test_start(void){
    uint32_t v=0;
    int i=0, n=0;
    printf("\nstart rtc_cross_domain\n");

    /* 确保 counter 停止，避免 EOI 外干扰 */
    mem_write32_(0x6000400c, 0x0);
    for(i=0;i<5;i++){ mem_read32_(0x6000400c, &v); }

    /* R1: match_value 跨域 */
    mem_write32_(0x60004004, 0xA5A50001);
    for(n=0;n<1000;n++){ mem_read32_(0x60004004, &v); if(v == 0xA5A50001) break; }
    if(v != 0xA5A50001){printf("ERR R1: match sync not converged, readback=0x%08x (n=%d)\n", v, n);sim_fail();}

    /* R2: load_value 跨域 */
    mem_write32_(0x60004008, 0x5A5A0002);
    for(n=0;n<1000;n++){ mem_read32_(0x60004008, &v); if(v == 0x5A5A0002) break; }
    if(v != 0x5A5A0002){printf("ERR R2: load sync not converged, readback=0x%08x (n=%d)\n", v, n);sim_fail();}

    /* R3: CCR 跨域（写 wen 位=1，不影响 en） */
    mem_write32_(0x6000400c, 0x8);
    for(n=0;n<1000;n++){ mem_read32_(0x6000400c, &v); if(v == 0x8) break; }
    if(v != 0x8){printf("ERR R3: CCR sync not converged, readback=0x%08x (n=%d)\n", v, n);sim_fail();}

    /* R4: DIV 跨域 */
    mem_write32_(0x60004020, 0x12345);
    for(n=0;n<1000;n++){ mem_read32_(0x60004020, &v); if(v == 0x12345) break; }
    if(v != 0x12345){printf("ERR R4: DIV sync not converged, readback=0x%08x (n=%d)\n", v, n);sim_fail();}

    /* 收尾：CCR=0，load/match/div 留给后续测试自行配置 */
    mem_write32_(0x6000400c, 0x0);
    printf("rtc_cross_domain test successfully\n");
    sim_end();
}
