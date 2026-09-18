/*
 * SoC F13: 错误响应（HRESP / 访问未实现地址）
 *
 * RTL 实证前提：
 *  - MAIN matrix default error（matrix.v:33729-33742）：未译码访问
 *    hresp=ERROR(01) 两拍应答 → E902 抛 access fault（load mcause=5 /
 *    store mcause=7），由 addr_misalign S7 实证 load 侧
 *  - dummy slave（dummy.v:68-70）：hrdata=0 / hready=1 / hresp=OKAY，
 *    即"已译码到 dummy"与"完全未译码"行为不同
 *  - APB dummy：paddr 译码到 apb0/apb1 dummy 端口（行为待实测）
 *
 * 步骤：
 *  S1: MAIN 未译码 gap 读（0x40004000）→ mcause=5（load access fault）
 *  S2: MAIN 未译码 gap 写（0x40004000）→ mcause=7（store access fault）
 *  S3: MAIN dummy S7 读（0x40010000）→ 不陷阱，返回 0
 *  S4: MAIN dummy S9 读（0x40100000）→ 不陷阱，返回 0
 *  S5: LS dummy（0x40200000, S10 子桥内 dummy S0）读 → 实测刻画
 *  S6: APB0 dummy P9（0x50010000）读 → 实测刻画
 *  S7: 未映射高位地址（0xA0000000，超出 S11 上界 0x9FFFFFFF）→ mcause=5
 */
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

volatile unsigned int trap_count = 0;
volatile unsigned int last_mcause = 0;
volatile unsigned int last_mepc   = 0;

/* 同 addr_misalign：naked asm 处理函数，记录 mcause/mepc 并跳过故障指令 */
__attribute__((naked, aligned(64))) void slverr_handler(void)
{
    __asm__ volatile (
        "addi sp, sp, -16          \n"
        "sw   x10, 0(sp)           \n"
        "sw   x11, 4(sp)           \n"
        "sw   x12, 8(sp)           \n"
        "la   x11, trap_count      \n"
        "lw   x12, 0(x11)          \n"
        "addi x12, x12, 1          \n"
        "sw   x12, 0(x11)          \n"
        "csrr x10, mcause          \n"
        "la   x11, last_mcause     \n"
        "sw   x10, 0(x11)          \n"
        "csrr x10, mepc            \n"
        "la   x11, last_mepc       \n"
        "sw   x10, 0(x11)          \n"
        "lhu  x11, 0(x10)          \n"
        "andi x11, x11, 3          \n"
        "li   x12, 3               \n"
        "beq  x11, x12, 1f         \n"
        "addi x10, x10, 2          \n"
        "j    2f                   \n"
        "1:   addi x10, x10, 4     \n"
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
    unsigned int h = (unsigned int)slverr_handler;
    __asm__ volatile ("csrw mtvec, %0" :: "r"(h & ~0x3u));
}

/* 读 addr，返回 (data, 是否陷阱, mcause)。用全局简单传递 */
static unsigned int probe_data;
static unsigned int probe_traps;
static unsigned int probe_cause;
static void probe_read(unsigned int addr)
{
    unsigned int base = trap_count;
    __asm__ volatile ("lw %0, 0(%1)" : "=r"(probe_data) : "r"(addr));
    probe_traps = trap_count - base;
    probe_cause = last_mcause;
}
static void probe_write(unsigned int addr)
{
    unsigned int base = trap_count;
    __asm__ volatile ("sw %0, 0(%1)" :: "r"(0xDEADBEEF), "r"(addr) : "memory");
    probe_traps = trap_count - base;
    probe_cause = last_mcause;
}

int test_start(void)
{
    printf("\nstart hresp_slverr\n");
    install_handler();

    /* S1: MAIN 未译码 gap 读 → load access fault (mcause=5) */
    probe_data = 0;
    probe_read(0x40004000);
    if (probe_traps != 1 || probe_cause != 5) {
        printf("ERR S1: MAIN undecoded read: traps=%d mcause=%d expect 1/5\n",
               probe_traps, probe_cause);
        sim_fail();
    }
    printf("S1 pass: MAIN undecoded read -> load access fault (mcause=5)\n");

    /* S2: MAIN 未译码 gap 写 → store access fault (mcause=7) */
    probe_write(0x40004000);
    if (probe_traps != 1 || probe_cause != 7) {
        printf("ERR S2: MAIN undecoded write: traps=%d mcause=%d expect 1/7\n",
               probe_traps, probe_cause);
        sim_fail();
    }
    printf("S2 pass: MAIN undecoded write -> store access fault (mcause=7)\n");

    /* S3: MAIN dummy S7 读 → 无陷阱，返回 0 */
    probe_data = 0x5A5A5A5A;
    probe_read(0x40010000);
    if (probe_traps != 0 || probe_data != 0) {
        printf("ERR S3: MAIN dummy S7 read: data=0x%x traps=%d expect 0/0\n",
               probe_data, probe_traps);
        sim_fail();
    }
    printf("S3 pass: MAIN dummy S7 (0x40010000) read returns 0, no trap\n");

    /* S4: MAIN dummy S9 读 → 无陷阱，返回 0 */
    probe_data = 0x5A5A5A5A;
    probe_read(0x40100000);
    if (probe_traps != 0 || probe_data != 0) {
        printf("ERR S4: MAIN dummy S9 read: data=0x%x traps=%d expect 0/0\n",
               probe_data, probe_traps);
        sim_fail();
    }
    printf("S4 pass: MAIN dummy S9 (0x40100000) read returns 0, no trap\n");

    /* S5: LS dummy S0（0x40200000）读 —— 实测刻画（LS 子桥行为） */
    probe_data = 0x5A5A5A5A;
    probe_read(0x40200000);
    printf("S5 observation: LS dummy 0x40200000 read: data=0x%x traps=%d mcause=%d\n",
           probe_data, probe_traps, probe_cause);
    if (probe_traps == 0 && probe_data == 0)
        printf("S5 pass: LS dummy returns 0, no trap\n");
    else if (probe_traps == 1 && probe_cause == 5)
        printf("S5 pass: LS dummy access faulted (mcause=5)\n");
    else { printf("ERR S5: unexpected LS dummy behavior\n"); sim_fail(); }

    /* S6: APB0 dummy P9（0x50010000）读 —— 实测刻画（APB 桥 + dummy 端口） */
    probe_data = 0x5A5A5A5A;
    probe_read(0x50010000);
    printf("S6 observation: APB0 dummy 0x50010000 read: data=0x%x traps=%d mcause=%d\n",
           probe_data, probe_traps, probe_cause);
    if (probe_traps == 0)
        printf("S6 pass: APB0 dummy read completed (data=0x%x)\n", probe_data);
    else if (probe_traps == 1 && (probe_cause == 5 || probe_cause == 4))
        printf("S6 pass: APB0 dummy access faulted (mcause=%d)\n", probe_cause);
    else { printf("ERR S6: unexpected APB0 dummy behavior\n"); sim_fail(); }

    /* S7: 超出 S11 上界的完全未映射地址（0xA0000000）读 → mcause=5 */
    probe_data = 0;
    probe_read(0xA0000000);
    if (probe_traps != 1 || probe_cause != 5) {
        printf("ERR S7: unmapped high read: traps=%d mcause=%d expect 1/5\n",
               probe_traps, probe_cause);
        sim_fail();
    }
    printf("S7 pass: unmapped high address (0xA0000000) read -> load access fault\n");

    printf("hresp_slverr test successfully\n");
    sim_end();
}
