/*
 * SoC F5: CLIC 中断嵌套（高优先级抢占低优先级 ISR）
 *
 * 前提（intr_isr_basic 已实证的 CLIC 端到端设施）+ 本用例新增 RTL 事实：
 *  - 嵌套判据（E902_20191018.v:15206-15210）：clic_pending_int_need_ack =
 *    pending && mie && (clic_cpu_int_il > cp0_iu_il) —— 新中断级别须严格
 *    大于当前 CPU 级别（mil/mintstatus）
 *  - mil 入/出栈（:6970-6982）：中断入口 mil <= 取中中断的 il；mret 时
 *    mil <= mpil（栈存的前级）—— 嵌套返回正确性由硬件保证
 *  - cliccfg（ctrl_reg_addr[11:2]==0x300 → 字节 0xE000EC00，:2827）：
 *    bit[4:1]=nlbits（:2221）。nlbits=3 时 il = {intcfg[7:5],5'h1f}
 *    （:2258-2262 case 4'd3）→ 级别 L 的 il = L*32+31
 *  - intcfg[7:5] = 3-bit 级别（CLICINTBITS_3，:2482）；窗口 addr[11:10]=10
 *    → intcfg[k] 字节地址 = 0xE000E800 + (0x40+k/4)*4 + k%4
 *  - TIM0 双通道寄存器（timer_dual_ch_parallel.c:27-32 实证）：
 *    ch1 LOAD=0x50000000/CTRL=0x08/EOI=0x0C；ch2 LOAD=0x50000014/CTRL=0x1C/EOI=0x20
 *
 * 编排（两个中断不同级别，高级在低级 ISR 窗口内到期）：
 *   #17 TIM0通道1 级别1（il=0x3F，load=0x100 先到期）
 *   #18 TIM0通道2 级别3（il=0x7F，load=0x400 → 1024 拍后到期）
 *  S1: CLIC 全配置（nlbits=3、intcfg 级别、intie×2、向量表、mtvec 模式）
 *  S2: 先启动两通道 → mie=1 → 低级 ISR（首次进入）：开 mie → 1.6 万拍窗口
 *  S3: ch2 在窗口内到期 → 0x7F > 0x3F → 嵌套进入高级 ISR（nest_flag）
 *  S4: 清理（停两通道 + 读两 EOI）→ 计数稳定；断言嵌套证据链
 *
 * ★ bug 检测器状态（2026-09-19 实证，配合 soc_top_intr_nesting_test）：
 *   本用例在本 RTL 上按设计 FAIL（嵌套路径缺陷）。取证链：
 *   - 嵌套发生 → GPR/内存倒计时的窗口循环均永不收敛（FSDB 实测
 *     29790 次嵌套、a2 装载仅一次却 7ms 不归零、内存版 1.9s 后
 *     窗口 sw 假 fault）
 *   - 直线 nop 窗口下：首个嵌套后系统劣化，最终在 low_handler 恢复
 *     序列 lw a1,4(sp) 处假异常 → crt0 __dummy(0x600) 非法指令风暴，
 *     main 轮询计数被破坏（既不成功也不超时，C 侧失去自报告能力）
 *   - 无嵌套对照（ch2 窗口后才到期）：全流程干净完成
 *   运行方式：make all C_TEST=intr_nesting/intr_nesting.c
 *            UTEST=soc_top_intr_nesting_test（UVM 看门狗 45ms 保证回归有界）
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define TIM1_LOAD    0x50000000
#define TIM1_CTRL    0x50000008
#define TIM1_EOI     0x5000000C
#define TIM1_INTSTAT 0x50000010

#define TIM2_LOAD    0x50000014
#define TIM2_CTRL    0x5000001C
#define TIM2_EOI     0x50000020
#define TIM2_INTSTAT 0x50000024

#define CLIC_CFG      0xE000EC00    /* [11:2]=0x300，bit[4:1]=nlbits */
#define CLIC_INTIE_17_18 0xE000E510 /* byte1 = #17, byte2 = #18（同一字，须一次写） */
#define CLIC_INTCFG_17 0xE000E911   /* intcfg[17]（0x800+(0x44)*4+1） */
#define CLIC_INTCFG_18 0xE000E912   /* intcfg[18]（0x800+(0x44)*4+2） */

#define CLIC_VEC_TABLE 0x0000A000   /* 同 intr_isr_basic：ISRAM 安全区 */

volatile unsigned int low_win    = 0;   /* 窗口倒计时：必须放内存 —— GPR 在每次中断入口被
                                   硬件回退（实测：8000 次窗口迭代在 ch2 每 1024 拍
                                   重触发的嵌套下永不完成，a2 装载仅一次却 7ms 未归零） */
volatile unsigned int low_count   = 0;
volatile unsigned int high_count  = 0;
volatile unsigned int low_active  = 0;
volatile unsigned int nest_flag   = 0;
volatile unsigned int low_mcause  = 0;
volatile unsigned int high_mcause = 0;

/* 低级 ISR（#17，级别1）：首次进入开 mie、开 1.6 万拍窗口。
   窗口内 ch2（级别3）到期 → 硬件嵌套进入 high_handler。
   再次进入（TIM 电平未清的重入）直接快速返回。 */
__attribute__((naked, aligned(64))) void low_handler(void)
{
    __asm__ volatile (
        "addi sp, sp, -16          \n"
        "sw   x10, 0(sp)           \n"
        "sw   x11, 4(sp)           \n"
        "sw   x12, 8(sp)           \n"
        /* 清 ch1 pending（TIM1 EOI=读 0x5000000C）：否则 mret 后硬件立即重取
           该电平中断、main 轮询循环永不推进 */
        "lui  x10, 0x50000         \n"
        "li   x11, 2               \n"
        "sw   x11, 0x8(x10)        \n"   /* TIM1_CTRL=stop: kill ch1 re-fire */
        "lw   x12, 0x0c(x10)       \n"   /* TIM1 EOI */
        "la   x11, low_count       \n"
        "lw   x12, 0(x11)          \n"
        "addi x12, x12, 1          \n"
        "sw   x12, 0(x11)          \n"
        "li   x10, 1               \n"
        "bne  x12, x10, 2f         \n"   /* 非首次进入 → 直接返回 */
        /* 首次：标记在窗 */
        "la   x11, low_active      \n"
        "li   x10, 1               \n"
        "sw   x10, 0(x11)          \n"
        /* 记录 mcause */
        "csrr x10, mcause          \n"
        "la   x11, low_mcause      \n"
        "sw   x10, 0(x11)          \n"
        /* 窗口内允许嵌套：mie=1 */
        "li   x10, 8               \n"
        "csrs mstatus, x10         \n"
        /* 窗口 = 400 条 nop 直线序列：任何循环状态（GPR 倒数/内存倒数）
           在 ch2 每 1024 拍重触发的嵌套风暴下都无法收敛（实测 GPR 版 29790
           次嵌套永不归零、内存版 1.9s 后 sw 还 fault）；直线代码仅靠 PC
           单调前进，mepc 恢复点即中断点，天然免疫任何状态回退。
           400 条 ≈ 1300+ 拍 > ch2 load=0x400 的 1024 拍 */
        ".rept 600                 \n"
        "nop                       \n"
        ".endr                     \n"
        /* 出窗：停双定时器并清源（防 mret 后 main 被重触发风暴回退），
           再清标记、关 mie */
        "lui  x10, 0x50000         \n"
        "li   x11, 2               \n"
        "sw   x11, 0x8(x10)        \n"   /* TIM1_CTRL=stop */
        "sw   x11, 0x1c(x10)       \n"   /* TIM2_CTRL=stop */
        "lw   x12, 0xc(x10)        \n"   /* TIM1_EOI */
        "lw   x12, 0x20(x10)       \n"   /* TIM2_EOI */
        "la   x11, low_active      \n"
        "sw   x0, 0(x11)           \n"
        "li   x10, 8               \n"
        "csrc mstatus, x10         \n"
        "2:                         \n"
        "lw   x10, 0(sp)           \n"
        "lw   x11, 4(sp)           \n"
        "lw   x12, 8(sp)           \n"
        "addi sp, sp, 16           \n"
        "mret                      \n"
    );
}

/* 高级 ISR（#18，级别3）：记录 mcause；若在低级窗口内进入则置 nest_flag */
__attribute__((naked, aligned(64))) void high_handler(void)
{
    __asm__ volatile (
        "addi sp, sp, -16          \n"
        "sw   x10, 0(sp)           \n"
        "sw   x11, 4(sp)           \n"
        "sw   x12, 8(sp)           \n"
        "la   x11, high_count      \n"
        "lw   x12, 0(x11)          \n"
        "addi x12, x12, 1          \n"
        "sw   x12, 0(x11)          \n"
        "csrr x10, mcause          \n"
        "la   x11, high_mcause     \n"
        "sw   x10, 0(x11)          \n"
        "la   x11, low_active      \n"
        "lw   x12, 0(x11)          \n"
        "beqz x12, 1f              \n"
        "la   x11, nest_flag       \n"
        "li   x10, 1               \n"
        "sw   x10, 0(x11)          \n"
        "1:                         \n"
        /* 清 ch2 pending（EOI=读 0x50000020）：否则 mret 后立即重入，
           低级窗口的循环无法推进 */
        "lui  x10, 0x50000         \n"
        "li   x11, 2               \n"
        "sw   x11, 0x1c(x10)       \n"   /* TIM2_CTRL=stop: kill re-fire */
        "lw   x12, 0x20(x10)       \n"
        "lw   x10, 0(sp)           \n"
        "lw   x11, 4(sp)           \n"
        "lw   x12, 8(sp)           \n"
        "addi sp, sp, 16           \n"
        "mret                      \n"
    );
}

int test_start(void)
{
    unsigned int v;
    unsigned int timeout;

    printf("\nstart intr_nesting\n");

    /* S1: 向量表 + mtvt + mtvec CLIC 模式 */
    for (v = 0; v < 64; v++)
        mem_write32_(CLIC_VEC_TABLE + v * 4, (unsigned int)low_handler);
    mem_write32_(CLIC_VEC_TABLE + 18 * 4, (unsigned int)high_handler);
    __asm__ volatile ("csrw 0x307, %0" :: "r"(CLIC_VEC_TABLE));
    __asm__ volatile ("csrr %0, mtvec" : "=r"(v));
    __asm__ volatile ("csrw mtvec, %0" :: "r"((v & ~0x3u) | 0x3u));

    /* S1: CLIC 配置——nlbits=3（级别字段使能），两级中断不同级别 */
    mem_write32_(CLIC_CFG, 0x06);                 /* bit[4:1]=3 */
    mem_write32_(CLIC_INTCFG_17, 0x20);           /* 级别 1（intcfg[7:5]=001） */
    mem_write32_(CLIC_INTCFG_18, 0x60);           /* 级别 3（intcfg[7:5]=011） */
    mem_write32_(CLIC_INTIE_17_18, 0x00010100);   /* byte1=#17, byte2=#18（各自 bit0=1） */

    /* S2: 启动两通道（均在 mie 之前；ch1 先到期，ch2 在低级 ISR 窗口内到期） */
    mem_write32_(TIM1_CTRL, 0x2);
    mem_write32_(TIM1_LOAD, 0x100);               /* ch1：256 拍后到期 */
    mem_write32_(TIM1_CTRL, 0x3);
    mem_write32_(TIM2_CTRL, 0x2);
    mem_write32_(TIM2_LOAD, 0x200);               /* ch2：512 拍后到期（低级窗口内） */
    mem_write32_(TIM2_CTRL, 0x3);
    __asm__ volatile ("csrs mstatus, %0" :: "r"(0x8u));

    /* 等嵌套证据（窗口 ~1.6 万拍 + 调度余量） */
    timeout = 0;
    while (!(nest_flag && high_count && low_count)) {
        if (++timeout >= 200000) {
            printf("ERR S2: no nesting evidence: low=%d high=%d nest=%d\n",
                   low_count, high_count, nest_flag);
            printf("     low_mcause=0x%x high_mcause=0x%x\n", low_mcause, high_mcause);
            mem_read32_(TIM1_INTSTAT, &v);
            printf("DIAG tim1_intstat=0x%x\n", v);
            mem_read32_(TIM2_INTSTAT, &v);
            printf("DIAG tim2_intstat=0x%x\n", v);
            mem_read32_(0xE000E110, &v);
            printf("DIAG clicintip[16-19]=0x%x\n", v);
            mem_read32_(CLIC_INTIE_17_18, &v);
            printf("DIAG clicintie[16-19]=0x%x\n", v);
            mem_read32_(CLIC_INTCFG_17, &v);
            printf("DIAG clicintcfg17=0x%x cfg18next\n", v);
            mem_read32_(0x50000018, &v);
            printf("DIAG tim2_curval=0x%x\n", v);
            mem_read32_(0x5000001C, &v);
            printf("DIAG tim2_ctrl_rb=0x%x\n", v);
            mem_read32_(0x50000004, &v);
            printf("DIAG tim1_curval=0x%x\n", v);
            sim_fail();
        }
    }
    printf("S2/S3 pass: low entered %d, high entered %d, nested-in-window=%d\n",
           low_count, high_count, nest_flag);
    printf("     low_mcause=0x%x (expect 0xb8000011), high_mcause=0x%x (expect 0xb8000012)\n",
           low_mcause, high_mcause);
    if ((low_mcause & 0xFFFFFFF) != 0x8000011 ||
        (high_mcause & 0xFFFFFFF) != 0x8000012) {
        printf("ERR S3: mcause ID mismatch\n");
        sim_fail();
    }

    /* S4: 清理并验证停止（先停源再 EOI） */
    mem_write32_(TIM1_CTRL, 0x2);
    mem_write32_(TIM2_CTRL, 0x2);
    {
        unsigned int l0 = low_count, h0 = high_count;
        unsigned int j;
        mem_read32_(TIM1_EOI, &v);
        mem_read32_(TIM2_EOI, &v);
        for (j = 0; j < 2000; j++)
            __asm__ volatile ("nop");
        if (low_count > l0 + 2 || high_count > h0 + 2) {
            printf("ERR S4: ISR still firing after stop: low %d->%d high %d->%d\n",
                   l0, low_count, h0, high_count);
            sim_fail();
        }
    }
    mem_read32_(TIM1_INTSTAT, &v);
    if (v & 1) { printf("ERR S4: TIM1 IntStatus=0x%x\n", v); sim_fail(); }
    mem_read32_(TIM2_INTSTAT, &v);
    if (v & 1) { printf("ERR S4: TIM2 IntStatus=0x%x\n", v); sim_fail(); }
    printf("S4 pass: both sources stopped, counts settled\n");

    printf("intr_nesting test successfully\n");
    sim_end();
}
