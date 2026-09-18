#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

/* F9: Reserved gap 行为——0x0C~0x2C 与 0x48 读 0、写忽略；高位 gap 0x54~0x7C 读 0 */
int test_start(void){
    uint32_t v=0;
    uint32_t addr=0;
    printf("\nstart gpio_reserved_gap\n");

    /* 低位 gap 0x0C~0x2C 与 0x48：读 0 */
    for(addr=0x6001800C; addr<=0x6001802C; addr+=4){
        mem_read32_(addr, &v);
        if(v != 0x0){printf("ERR: gap 0x%08x got 0x%x expect 0\n", addr, v);sim_fail();}
    }
    mem_read32_(0x60018048, &v);
    if(v != 0x0){printf("ERR: gap 0x60018048 got 0x%x expect 0\n", v);sim_fail();}

    /* 写忽略：写全 1 后读回仍 0 */
    mem_write32_(0x6001800C, 0xFFFFFFFF);
    mem_read32_(0x6001800C, &v);
    if(v != 0x0){printf("ERR: gap 0x6001800C write not ignored, got 0x%x\n", v);sim_fail();}
    mem_write32_(0x60018048, 0xFFFFFFFF);
    mem_read32_(0x60018048, &v);
    if(v != 0x0){printf("ERR: gap 0x60018048 write not ignored, got 0x%x\n", v);sim_fail();}

    /* 高位 gap 0x54~0x7C：读 0 */
    for(addr=0x60018054; addr<=0x6001807C; addr+=4){
        mem_read32_(addr, &v);
        if(v != 0x0){printf("ERR: gap 0x%08x got 0x%x expect 0\n", addr, v);sim_fail();}
    }
    printf("gpio_reserved_gap test successfully\n");
    sim_end();
}
