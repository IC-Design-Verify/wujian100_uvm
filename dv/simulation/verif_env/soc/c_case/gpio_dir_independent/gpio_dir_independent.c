#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F2: 方向寄存器单 bit 独立（寄存器级读写一致性；Input 位 PAD 悬空值不断言） */
int test_start(void){
    uint32_t v=0;
    int i=0;
    static const uint32_t dir_patterns[6] = {
        0xAAAAAAAA, 0x55555555, 0x00000001, 0x80000000, 0xFFFFFFFF, 0x00000000
    };
    printf("\nstart gpio_dir_independent\n");

    for(i=0;i<6;i++){
        mem_write32_(0x60018004, dir_patterns[i]);
        mem_read32_(0x60018004, &v);
        if(v != dir_patterns[i]){
            printf("ERR: direction got 0x%08x expect 0x%08x\n", v, dir_patterns[i]);
            sim_fail();
        }
    }

    /* 混合方向下 Output 半字读回：direction=0x0000FFFF（低 16 Output），
     * output_data=0xFFFFFFFF，input_data 低 16 bit 应读回 0xFFFF；
     * 高 16 bit 为 Input（PAD 悬空），不断言其值 */
    mem_write32_(0x60018004, 0x0000FFFF);
    mem_write32_(0x60018000, 0xFFFFFFFF);
    mem_read32_(0x60018050, &v);
    if((v & 0x0000FFFF) != 0x0000FFFF){
        printf("ERR: mixed-dir low half got 0x%08x expect xxxxFFFF\n", v);
        sim_fail();
    }
    mem_read32_(0x60018004, &v);
    if(v != 0x0000FFFF){
        printf("ERR: direction reread got 0x%08x expect 0x0000FFFF\n", v);
        sim_fail();
    }

    mem_write32_(0x60018004, 0x00000000);   /* 收尾回 Input */
    printf("gpio_dir_independent test successfully\n");
    sim_end();
}
