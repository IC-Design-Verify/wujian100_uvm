/*
 * SoC F4 深化 + F5 基础：CPU 真实进 ISR（CLIC 中断端到端接管）
 *
 * 既有 vic_route 用例只观测到 pad_vic_int_vld 线断言为止，CPU 侧的
 * CLIC 接收→取向量表→跳 handler 路径从未被验证。本用例打通该路径。
 *
 * RTL 实证前提（E902_20191018.v）：
 *  - CLIC 寄存器窗口 0xE000E000（VIC_IN=20'hE000E，:20019/20062）；
 *    CLICINTIE 窗口 addr[11:10]=01（:2746-2756），entry word 偏移
 *    (0x40+i)*4（:2787-2788）→ 中断 k 的 intie 字节地址 =
 *    0xE000E000 + 0x400 + (0x40 + k/4)*4 + (k%4)
 *    TIM0 通道1 → int #17 → word idx 0x44 → 0xE000E510 byte1
 *  - mtvec[1:0]=11 为 CLIC 模式（hw_vector_clic_on，:6953）
 *  - CLIC 模式中断入口**恒硬件向量**（:6681 case 3'b111 → vec_adder_vbr
 *    = {mtvt,4'b0} + int_id，:6690-6692）：CPU 经 ibus 从
 *    mtvt + id*4 处**读出 handler 地址**再跳转（vector FSM 状态机
 *    BUF_VBR→WAIT_GRANT→WAIT_DATA，:17218-17225）
 *  - mtvt 是 CSR 0x307（:5859），低 6 位强制 0（64B 对齐，:6550）
 *  - 因此必须：mtvt 指向 ISRAM 中的向量表，表项[id] = handler 地址
 *
 * 步骤：
 *  S1: 在 ISRAM 建向量表（表项[17]=handler），mtvt 指向表基址（CSR 0x307）
 *  S2: 写 CLICINTIE[17]=1 + mstatus.mie=1
 *  S3: 启动 TIM0 通道1（load=0x400），等中断 → ISR 计数应 >0
 *      （不先清外设中断，观察反复进入 = 电平中断真实驱动 CLIC）
 *  S4: 停定时器（CTRL=0x2）→ 读 TIM1 EOI 清残留 pending → ISR 停止进入。
 *      必须先停：user 模式定时器到期自动重装 load 值，每 0x400 拍重新
 *      触发中断，仅读 EOI 挡不住下一次到期（实测 EOI 后 ISR 仍增长数百次）
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

/* CLIC: intie[17] 字地址（见文件头推导） */
#define CLIC_INTIE_17  0xE000E510     /* byte1 = int #17 */

volatile unsigned int isr_count  = 0;
volatile unsigned int isr_mcause = 0;
volatile unsigned int isr_mepc   = 0;

/* CLIC 中断 handler：记录现场，mret 原路返回（不推进 mepc） */
__attribute__((naked, aligned(64))) void clic_isr_handler(void)
{
    __asm__ volatile (
        "addi sp, sp, -16          \n"
        "sw   x10, 0(sp)           \n"
        "sw   x11, 4(sp)           \n"
        "sw   x12, 8(sp)           \n"
        "la   x11, isr_count       \n"
        "lw   x12, 0(x11)          \n"
        "addi x12, x12, 1          \n"
        "sw   x12, 0(x11)          \n"
        "csrr x10, mcause          \n"
        "la   x11, isr_mcause      \n"
        "sw   x10, 0(x11)          \n"
        "csrr x10, mepc            \n"
        "la   x11, isr_mepc        \n"
        "sw   x10, 0(x11)          \n"
        "lw   x10, 0(sp)           \n"
        "lw   x11, 4(sp)           \n"
        "lw   x12, 8(sp)           \n"
        "addi sp, sp, 16           \n"
        "mret                      \n"
    );
}

/* ISRAM 中的 CLIC 向量表（64B 对齐，表项 = handler 地址）。
   必须在 ISRAM（0x0 区）：vector FSM 经 ibus 取向量（:17218-17225
   WAIT_GRANT/WAIT_DATA），ibus 只映射 ISRAM；DSRAM 不在 ibus 上。
   地址选择（objdump 实测）：.text=0x8eb9，.data/.bss 到 0x9730，
   linker RAND 区在 0xeff0 —— 表放 0xA000（.bss 之上、RAND 之下）。
   （0x8000 曾砸在 .text 中间导致打印乱码，实证勿改回） */
#define CLIC_VEC_TABLE  0x0000A000

int test_start(void)
{
    unsigned int v;
    unsigned int timeout;
    int i;

    printf("\nstart intr_isr_basic\n");

    /* S1: 建 CLIC 向量表（全 64 项指向 handler，便于任何 id 都能进），
       mtvt = 表基址 */
    for (i = 0; i < 64; i++)
        mem_write32_(CLIC_VEC_TABLE + i * 4, (unsigned int)clic_isr_handler);
    __asm__ volatile ("csrw 0x307, %0" :: "r"(CLIC_VEC_TABLE));  /* mtvt */
    printf("S1: CLIC vector table at 0x%x, mtvt installed\n", CLIC_VEC_TABLE);

    /* S2a: mtvec 切到 CLIC 模式（crt0 默认 mode=00 直连，CLIC 不激活）。
       保留 crt0 的异常 handler 基址，只把 mode[1:0] 置 11。
       RTL 依据：E902_20191018.v:6953 hw_vector_clic_on 要求 mode==2'b11；
       :6513 mode = mtvec[1:0]。 */
    __asm__ volatile ("csrr %0, mtvec" : "=r"(v));
    __asm__ volatile ("csrw mtvec, %0" :: "r"((v & ~0x3u) | 0x3u));
    __asm__ volatile ("csrr %0, mtvec" : "=r"(v));
    printf("S2a diag: mtvec = 0x%x (mode bits expect 11)\n", v);

    /* S2: CLICINTIE[17]=1 + mstatus.mie=1（带读回诊断） */
    mem_write32_(CLIC_INTIE_17, 0x00000100);   /* byte1 = int17 enable */
    mem_read32_(CLIC_INTIE_17, &v);
    printf("S2 diag: CLICINTIE[16-19] word readback = 0x%x (expect 0x100)\n", v);
    __asm__ volatile ("csrs mstatus, %0" :: "r"(0x8u));
    __asm__ volatile ("csrr %0, mstatus" : "=r"(v));
    printf("S2 diag: mstatus = 0x%x (bit3 mie expect 1)\n", v);
    printf("S2: CLICINTIE[17]=1, mstatus.mie=1\n");

    /* S3: 启动 TIM0 通道1，等中断进 ISR */
    mem_write32_(TIM1_CTRL, 0x2);              /* 停 */
    mem_write32_(TIM1_LOAD, 0x400);
    mem_write32_(TIM1_CTRL, 0x3);              /* enable + unmask int */
    timeout = 0;
    while (isr_count == 0) {
        if (++timeout >= 500000) {
            /* 诊断：读 CLICINTIP[16-19]（0xE000E110 byte1）确认 pending 是否到达 CLIC */
            mem_read32_(0xE000E110, &v);
            printf("S3 diag: CLICINTIP[16-19] word = 0x%x (byte1=int17 pending)\n", v);
            mem_read32_(TIM1_INTSTAT, &v);
            printf("S3 diag: TIM1 IntStatus = 0x%x\n", v);
            printf("ERR S3: CPU never entered ISR (TIM1 int #17 not taken)\n");
            sim_fail();
        }
    }
    printf("S3 pass: CPU entered ISR %d times (level int re-entry expected)\n",
           isr_count);
    printf("S3 observation: mcause=0x%x mepc=0x%x\n", isr_mcause, isr_mepc);
    if ((isr_mcause & 0xFFF) != 17)
        printf("NOTE: mcause id field = %d (expect 17 if CLIC id in low 12 bits)\n",
               isr_mcause & 0xFFF);

    /* S4: 先停定时器（user 模式自动重装会周期性重触发），再读 EOI 清残留 */
    mem_write32_(TIM1_CTRL, 0x2);
    mem_read32_(TIM1_EOI, &v);
    {
        unsigned int settle = isr_count;
        unsigned int j;
        for (j = 0; j < 1000; j++)
            __asm__ volatile ("nop");
        if (isr_count > settle + 2) {
            printf("ERR S4: ISR still firing after TIM1 EOI (count %d -> %d)\n",
                   settle, isr_count);
            sim_fail();
        }
    }
    mem_read32_(TIM1_INTSTAT, &v);
    if (v & 1) { printf("ERR S4: TIM1 IntStatus=0x%x after EOI\n", v); sim_fail(); }
    printf("S4 pass: ISR stopped after TIM1 EOI (source clear gates CLIC input)\n");

    printf("intr_isr_basic test successfully\n");
    sim_end();
}
