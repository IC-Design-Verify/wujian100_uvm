#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void){
    uint32_t flag=0,stage=0,cr=0;
    mem_read32_(0x20002000,&flag);
    if(flag==0xFF){printf("wdt_rpl_pulse test successfully\n");sim_end();}
    stage=flag+1;
    /* 第 8 档配置完后写完成哨兵 0xFF（否则第 9 次启动会继续发脉冲，UVM 期望恰好 8 个） */
    mem_write32_(0x20002000,(stage==8)?0xFF:stage);
    cr=((stage-1)<<2)|0x1;  /* RPL=stage-1, RMOD=0(立即复位), EN=1 */
    mem_write32_(0x50008004,0x00);
    mem_write32_(0x50008000,cr);
    while(1){}
}
