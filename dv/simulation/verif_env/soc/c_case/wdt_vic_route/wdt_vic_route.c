#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void){
    uint32_t v=0;
    int t=0;
    
    mem_write32_(0x50008004,0x00);
    mem_write32_(0x50008000,0x3);
    
    t=0;
    mem_read32_(0x50008010,&v);
    while(v!=0x1&&t<500000){mem_read32_(0x50008010,&v);t++;}
    if(t>=500000){printf("ERR:timeout\n");sim_fail();}
    
    mem_read32_(0x50008014,&v);
    mem_read32_(0x50008010,&v);
    if(v!=0){printf("ERR:int not clear\n");sim_fail();}
    
    printf("wdt_vic_route test successfully\n");
    sim_end();
}
