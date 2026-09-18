#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F3: Output 模式读 input_data(0x50) 返回 last write value */
int test_start(void){
    uint32_t v=0;
    int i=0;
    static const uint32_t patterns[5] = {
        0x5A5A5A5A, 0xA5A5A5A5, 0x12345678, 0xFFFFFFFF, 0x00000000
    };
    printf("\nstart gpio_output_data_echo\n");

    mem_write32_(0x60018004, 0xFFFFFFFF);   /* direction: 全 Output */
    mem_write32_(0x60018008, 0x00000000);   /* ctl: 全 Software */

    for(i=0;i<5;i++){
        mem_write32_(0x60018000, patterns[i]);  /* output_data */
        mem_read32_(0x60018050, &v);            /* input_data 读回 last write */
        if(v != patterns[i]){
            printf("ERR: output echo got 0x%08x expect 0x%08x\n", v, patterns[i]);
            sim_fail();
        }
    }
    mem_write32_(0x60018004, 0x00000000);   /* 收尾回 Input */
    printf("gpio_output_data_echo test successfully\n");
    sim_end();
}
