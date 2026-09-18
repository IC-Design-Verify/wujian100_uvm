#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F8: GPIO 复位值——可编程寄存器复位后应全 0
 *
 * 注：input_data @0x50 不在本清单——其值来自 PAD（TB legacy 激励块
 * apb1/tb_top/apb1_gpio/gpio_test.v 在 oe==0 时 force PAD=0x55555555），
 * 属激励相关而非 RTL 复位值，由 gpio_intr_combo 用例覆盖其读值。 */
int test_start(void){
    uint32_t v=0;
    static const uint32_t ro_regs[9] = {
        0x60018000, /* output_data */
        0x60018004, /* direction */
        0x60018008, /* ctl */
        0x60018030, /* inten */
        0x60018034, /* intmask */
        0x60018038, /* inttype_level */
        0x6001803C, /* int_polarity */
        0x60018040, /* intstatus */
        0x60018044  /* rawintstatus */
    };
    int i=0;
    printf("\nstart gpio_reset_default\n");

    for(i=0;i<9;i++){
        mem_read32_(ro_regs[i], &v);
        if(v != 0x0){
            printf("ERR: GPIO reg 0x%08x reset got 0x%x expect 0\n", ro_regs[i], v);
            sim_fail();
        }
    }
    printf("gpio_reset_default test successfully\n");
    sim_end();
}
