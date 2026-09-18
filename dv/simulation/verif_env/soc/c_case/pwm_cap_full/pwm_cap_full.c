#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F7：PWM 捕获功能（回环激励）
 *
 * 激励来源（TB，USE_APB0_PWM 编译入）：
 *   apb0/tb_top/apb0_pwm/pwm_test.v —— CH0 输出 20 个脉冲后，
 *   每个 CH0 posedge 翻转 input_data 并 force 到 PAD_PWM_CH2。
 *   即 CH0 输出 → CH2 输入回环，CH2 频率 = CH0 的一半。
 *
 * 关键 RTL 实证（wujian100_open/soc/pwm.v）：
 *   - 捕获通道映射：pwm_1_inst.i_capture = i_capture_2 = PAD_PWM_CH2
 *     （pwm.v:4422；wujian100_open_top.v:1413）→ 回环必须落在 cap1
 *   - capNmode 语义与直觉相反（pwm.v:5030-5038 pwm_gen 计数优先级）：
 *       cap_mode=1：pwm_cnt 每个 cap_edge +1、==cap_load 回 0 → 沿计数
 *                   （cnt_match = 沿计数到 cap_load 值，legacy 用法）
 *       cap_mode=0：pwm_cnt 每个 clk 自由累加 → 边沿时间戳
 *                   （cap_cnt 在沿时刻锁存 pwm_cnt = 时间戳；
 *                    cnt_match 要求自由计数器恰等于 cap_load，实际上不可达）
 *   - CAPCTL@0x74（18bit）：capNmode=bitN，capNeventa=[7+2N:6+2N]（00=rise,01=fall,11=both）
 *     → cap1: mode=bit1, eventa=[9:8]
 *   - CAPINTEN@0x78：bitN=capN cnt_match 使能，bit(6+N)=capN cnt_add 使能
 *   - CAPRIS@0x7C(RO)：bitN=capN cnt_match，bit(6+N)=capN cnt_add（原始位被 INTEN 门控）
 *   - CAPIC@0x80：写 bitN 清 cnt_match，写 bit(6+N) 清 cnt_add
 *   - CAP01MATCH@0x94：[31:16]=cap1 的 cap_load_value
 *   - CAP01T@0x88(RO)：[31:16]=cap1 捕获值（沿时刻的 pwm_cnt）
 *   - CNT01VAL@0xC8(RO)：[31:16]=cnt1（group1 的 pwm_cnt）
 *   - PWMCFG：bit0=pwm0en，bit13=cap1en
 *
 * 断言清单：
 *   S1：沿计数模式（mode=1）cnt_match（cap_load=0x20）→ CAPRIS bit1 置位 → CAPIC 清 0
 *   S2：沿计数 —— CNT01VAL[31:16] 两次读取不同（递增）
 *   S3：时间戳模式（mode=0）—— 两次捕获 CAP01T[31:16] 非 0 且不同
 *   S4：边沿选择 —— both 模式的沿计数增量 > rise-only 模式（同长度窗口，
 *       cap_load=0x8000 防回绕干扰）
 */
int test_start(void){
    uint32_t v=0, v1=0, v2=0;
    uint32_t d_rise=0, d_both=0;
    int i=0, timeout=0;
    printf("\nstart pwm_cap_full\n");

    /* ---- 公共配置：CH0 输出（legacy 回环源） ---- */
    mem_write32_(0x5001c000, 0x0);          /* PWMCFG=0 全停 */
    mem_write32_(0x5001c038, 0x100);        /* PWM01LOAD group0=0x100 */
    mem_write32_(0x5001c050, 0x80);         /* PWM0CMP compa=0x80（50% 占空） */

    /* ---- S1：沿计数模式 cnt_match（同 legacy 配置） ---- */
    mem_write32_(0x5001c094, 0x00200000);   /* CAP01MATCH[31:16]=cap1 match=0x20 */
    mem_write32_(0x5001c074, 0x302);        /* CAPCTL: cap1mode=1(沿计数), cap1eventa=[9:8]=11(both) */
    mem_write32_(0x5001c078, 0x2);          /* CAPINTEN: bit1=cap1 cnt_match 使能 */
    mem_write32_(0x5001c000, 0x2001);       /* PWMCFG: pwm0en(bit0)+cap1en(bit13) */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001c07c, &v);        /* CAPRIS */
        timeout++;
        if(timeout>=200000){printf("ERR S1: timeout waiting CAPRIS bit1, CAPRIS=0x%08x\n", v);sim_fail();}
    }while((v & 0x2) == 0);

    mem_write32_(0x5001c080, 0x2);          /* CAPIC: 清 cap1 cnt_match */
    for(i=0;i<3;i++){ mem_read32_(0x5001c07c, &v); }
    mem_read32_(0x5001c07c, &v);
    if((v & 0x2) != 0){printf("ERR S1: CAPRIS bit1 not cleared, CAPRIS=0x%08x\n", v);sim_fail();}

    /* ---- S2：沿计数 —— cnt1 变化（同 S1 配置继续跑） ---- */
    mem_read32_(0x5001c0c8, &v1);           /* CNT01VAL */
    v1 = (v1 >> 16) & 0xffff;
    for(i=0;i<200;i++){ mem_read32_(0x5001c07c, &v); }  /* 等待若干捕获沿 */
    mem_read32_(0x5001c0c8, &v2);
    v2 = (v2 >> 16) & 0xffff;
    if(v2 == v1){printf("ERR S2: cnt1 not changing, v1=0x%x v2=0x%x\n", v1, v2);sim_fail();}

    /* ---- S3：时间戳模式 —— 两次捕获值非 0 且不同 ---- */
    mem_write32_(0x5001c000, 0x0);          /* 停 */
    mem_write32_(0x5001c074, 0x0);          /* cap1mode=0(时间戳), eventa=00(rise) */
    mem_write32_(0x5001c078, 0x80);         /* CAPINTEN: bit7=cap1 cnt_add 使能 */
    mem_write32_(0x5001c080, 0x82);         /* 清 cap1 match+add 残留 */
    mem_write32_(0x5001c000, 0x2001);       /* 重启 pwm0en+cap1en */
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }

    timeout=0;
    do{
        mem_read32_(0x5001c07c, &v);
        timeout++;
        if(timeout>=200000){printf("ERR S3: timeout waiting 1st cnt_add\n");sim_fail();}
    }while((v & 0x80) == 0);
    mem_write32_(0x5001c080, 0x80);         /* 清 cnt_add */
    mem_read32_(0x5001c088, &v1);           /* CAP01T */
    v1 = (v1 >> 16) & 0xffff;

    timeout=0;
    do{
        mem_read32_(0x5001c07c, &v);
        timeout++;
        if(timeout>=200000){printf("ERR S3: timeout waiting 2nd cnt_add\n");sim_fail();}
    }while((v & 0x80) == 0);
    mem_write32_(0x5001c080, 0x80);
    mem_read32_(0x5001c088, &v2);
    v2 = (v2 >> 16) & 0xffff;
    if((v1 == 0) || (v2 == 0) || (v1 == v2)){
        printf("ERR S3: edge time capture bad, v1=0x%x v2=0x%x\n", v1, v2);sim_fail();
    }

    /* ---- S4：边沿选择 —— both 增量 > rise 增量（沿计数模式，cap_load 调高防回绕） ---- */
    mem_write32_(0x5001c094, 0x80000000);   /* CAP01MATCH[31:16]=0x8000（窗口内不回绕） */

    /* rise-only 窗口 */
    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c074, 0x2);          /* mode=1(沿计数) + eventa=00(rise) */
    mem_write32_(0x5001c078, 0x0);
    mem_write32_(0x5001c000, 0x2001);
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }
    mem_read32_(0x5001c0c8, &v1); v1 = (v1 >> 16) & 0xffff;
    for(i=0;i<400;i++){ mem_read32_(0x5001c07c, &v); }
    mem_read32_(0x5001c0c8, &v2); v2 = (v2 >> 16) & 0xffff;
    d_rise = (v2 - v1) & 0xffff;

    /* both 窗口（同长度） */
    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c074, 0x302);        /* mode=1 + eventa=[9:8]=11(both) */
    mem_write32_(0x5001c000, 0x2001);
    for(i=0;i<3;i++){ mem_read32_(0x5001c000, &v); }
    mem_read32_(0x5001c0c8, &v1); v1 = (v1 >> 16) & 0xffff;
    for(i=0;i<400;i++){ mem_read32_(0x5001c07c, &v); }
    mem_read32_(0x5001c0c8, &v2); v2 = (v2 >> 16) & 0xffff;
    d_both = (v2 - v1) & 0xffff;

    if((d_rise == 0) || (d_both <= d_rise)){
        printf("ERR S4: both not > rise, d_rise=0x%x d_both=0x%x\n", d_rise, d_both);sim_fail();
    }

    /* ---- 收尾 ---- */
    mem_write32_(0x5001c000, 0x0);
    mem_write32_(0x5001c074, 0x0);
    mem_write32_(0x5001c078, 0x0);

    printf("pwm_cap_full test successfully\n");
    sim_end();
}
