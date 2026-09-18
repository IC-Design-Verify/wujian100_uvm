/*
 * SoC F1(缺口): 错位访问 + 保留 gap 读行为
 *
 * RTL 实证前提：
 *  - E902 LSU 对非对齐 word/half 访问触发同步异常（E902_20191018.v:18513-18518：
 *    WORD 时 |addr[1:0]、HALF 时 addr[0] → dp_ctrl_misalign），
 *    异常向量 MISL_VEC=4（load 错位）/ MISS_VEC=6（store 错位）（:18035）
 *  - MAIN matrix 有 default error 逻辑（matrix.v:33729-33742：未译码访问
 *    hresp=ERROR(01) 两拍应答 + hready 脉冲）；E902 收到 load bus error 后
 *    是陷阱还是吞错由 S7 实测刻画
 *  - dummy slave（dummy.v:68-70）固定 hrdata=0 / hready=1 / hresp=OKAY
 *
 * 步骤：
 *  S1: 安装自定义 mtvec 处理函数（asm，记录 mcause/mepc 并跳过故障指令）
 *  S2: DSRAM 错位 word 读  → 应陷阱，mcause=4
 *  S3: DSRAM 错位 word 写  → 应陷阱，mcause=6
 *  S4: DSRAM 错位 half 读  → 应陷阱，mcause=4
 *  S5: 对齐 word 读对照     → 不陷阱，数据正确
 *  S6: MAIN dummy S7 (0x40010000) 读 → 返回 0，无陷阱（dummy 固定 OKAY/0）
 *  S7: 未译码 gap (0x40004000，S6 与 S7 之间) → 实测刻画（HRESP=ERROR 后
 *      CPU 侧行为：陷阱 mcause=? 或返回全 1/全 0）
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define DUMMY_MAIN_S7   0x40010000
#define UNDECODED_GAP   0x40004000    /* S6(DMA,≤0x40003FFF) 与 S7(≥0x40010000) 之间 */

volatile unsigned int trap_count = 0;
volatile unsigned int last_mcause = 0;
volatile unsigned int last_mepc   = 0;

/* 自定义陷阱处理：纯汇编（naked），记录 mcause/mepc，按指令长度跳过故障指令。
   rv32emc：压缩指令低 2 bit != 11 为 16 位指令，否则 32 位。 */
__attribute__((naked, aligned(64))) void misalign_handler(void)
{
    __asm__ volatile (
        "addi sp, sp, -16          \n"
        "sw   x10, 0(sp)           \n"
        "sw   x11, 4(sp)           \n"
        "sw   x12, 8(sp)           \n"
        /* 计数 */
        "la   x11, trap_count      \n"
        "lw   x12, 0(x11)          \n"
        "addi x12, x12, 1          \n"
        "sw   x12, 0(x11)          \n"
        /* 记录 mcause */
        "csrr x10, mcause          \n"
        "la   x11, last_mcause     \n"
        "sw   x10, 0(x11)          \n"
        /* 记录并推进 mepc */
        "csrr x10, mepc            \n"
        "la   x11, last_mepc       \n"
        "sw   x10, 0(x11)          \n"
        "lhu  x11, 0(x10)          \n"   /* 取故障指令低半字判长度 */
        "andi x11, x11, 3          \n"
        "li   x12, 3               \n"
        "beq  x11, x12, 1f         \n"
        "addi x10, x10, 2          \n"   /* 16 位压缩指令 */
        "j    2f                   \n"
        "1:   addi x10, x10, 4     \n"   /* 32 位指令 */
        "2:   csrw mepc, x10       \n"
        "lw   x10, 0(sp)           \n"
        "lw   x11, 4(sp)           \n"
        "lw   x12, 8(sp)           \n"
        "addi sp, sp, 16           \n"
        "mret                      \n"
    );
}

static void install_handler(void)
{
    unsigned int h = (unsigned int)misalign_handler;
    __asm__ volatile ("csrw mtvec, %0" :: "r"(h & ~0x3u));  /* 基址模式，向量偏移为 0 */
}

int test_start(void)
{
    unsigned int v;
    unsigned int base_count;

    printf("\nstart addr_misalign\n");

    /* 准备一个对齐数据源 */
    mem_write32_(0x20008000, 0xA5A5A5A5);

    install_handler();

    /* S2: 错位 word 读 → mcause=4 */
    base_count = trap_count;
    __asm__ volatile ("lw %0, 0(%1)" : "=r"(v) : "r"(0x20008001));
    if (trap_count != base_count + 1 || last_mcause != 4) {
        printf("ERR S2: misaligned word read: trap_count=%d mcause=%d expect 4\n",
               trap_count, last_mcause);
        sim_fail();
    }
    printf("S2 pass: misaligned word read trapped, mcause=4 (load misaligned)\n");

    /* S3: 错位 word 写 → mcause=6 */
    base_count = trap_count;
    __asm__ volatile ("sw %0, 0(%1)" :: "r"(0x12345678), "r"(0x20008002) : "memory");
    if (trap_count != base_count + 1 || last_mcause != 6) {
        printf("ERR S3: misaligned word write: trap_count=%d mcause=%d expect 6\n",
               trap_count, last_mcause);
        sim_fail();
    }
    printf("S3 pass: misaligned word write trapped, mcause=6 (store misaligned)\n");

    /* S4: 错位 half 读 → mcause=4 */
    base_count = trap_count;
    __asm__ volatile ("lh %0, 0(%1)" : "=r"(v) : "r"(0x20008001));
    if (trap_count != base_count + 1 || last_mcause != 4) {
        printf("ERR S4: misaligned half read: trap_count=%d mcause=%d expect 4\n",
               trap_count, last_mcause);
        sim_fail();
    }
    printf("S4 pass: misaligned halfword read trapped, mcause=4\n");

    /* S5: 对齐 word 读对照 —— 不陷阱且数据正确 */
    base_count = trap_count;
    mem_read32_(0x20008000, &v);
    if (v != 0xA5A5A5A5) { printf("ERR S5: aligned read 0x%x expect 0xA5A5A5A5\n", v); sim_fail(); }
    if (trap_count != base_count) { printf("ERR S5: aligned read trapped unexpectedly\n"); sim_fail(); }
    /* S3 的错位写应被异常拦下，不得污染存储 */
    mem_read32_(0x20008000, &v);
    if (v != 0xA5A5A5A5) { printf("ERR S5b: misaligned store leaked data: 0x%x\n", v); sim_fail(); }
    printf("S5 pass: aligned access untrapped; aborted misaligned store left no trace\n");

    /* S6: MAIN dummy S7 读 → 0，无陷阱（dummy.v: hrdata=0, hresp=OKAY） */
    base_count = trap_count;
    mem_read32_(DUMMY_MAIN_S7, &v);
    if (v != 0) { printf("ERR S6: dummy read 0x%x expect 0\n", v); sim_fail(); }
    if (trap_count != base_count) { printf("ERR S6: dummy read trapped\n"); sim_fail(); }
    printf("S6 pass: MAIN dummy S7 read returns 0, no trap (no SLVERR in this SoC)\n");

    /* S7: 未译码 gap —— matrix.v:33729-33742 default error：未译码访问
       hresp=ERROR(01) 两拍应答 → E902 抛 load access fault（mcause=5）。
       实测：trap +1、mcause=5、目的寄存器保持读出前值（此处 0xFFFFFFFF）。
       严格断言：必须陷阱且 mcause=5。 */
    base_count = trap_count;
    v = 0xFFFFFFFF;
    mem_read32_(UNDECODED_GAP, &v);
    if (trap_count != base_count + 1 || last_mcause != 5) {
        printf("ERR S7: undecoded read: traps=+%d mcause=%d, expect trap mcause=5\n",
               trap_count - base_count, last_mcause);
        sim_fail();
    }
    printf("S7 pass: undecoded access trapped with mcause=5 (load access fault, "
           "matrix default-error HRESP=ERROR confirmed)\n");

    printf("addr_misalign test successfully\n");
    sim_end();
}
