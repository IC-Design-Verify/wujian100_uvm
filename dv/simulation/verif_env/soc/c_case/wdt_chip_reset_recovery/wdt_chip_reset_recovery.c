#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void){
    uint32_t flag=0, v=0;
    mem_read32_(0x20002000,&flag);

    if(flag != 0x87654321){
        /* 首次启动：置 flag，配置 WDT RMOD=0 快速超时触发整芯片复位 */
        mem_write32_(0x20002000,0x87654321);
        mem_write32_(0x50008004,0x00);   /* time_out=0x00 → TOP=0 → 65535 pclk */
        mem_write32_(0x50008000,0x1);    /* CR: EN=1, RMOD=0（超时立即复位） */
        while(1){}
    }

    /* 复位后启动：验证各模块寄存器回到复位值 */
    mem_read32_(0x50008000,&v);          /* WDT CR 复位值 0x00（EN 已清，RTL 实测） */
    if(v!=0x00){printf("ERR: WDT_CR=0x%x expect 0x00\n",v);sim_fail();}

    mem_read32_(0x50008010,&v);          /* WDT int_status 复位 0 */
    if(v!=0x0){printf("ERR: WDT_int_status=0x%x expect 0\n",v);sim_fail();}

    mem_read32_(0x50008008,&v);          /* WDT current_value 复位 0x0000FFFF */
    if(v!=0x0000FFFF){printf("ERR: WDT_current_value=0x%x expect 0xFFFF\n",v);sim_fail();}

    mem_read32_(0x50000008,&v);          /* TIM0 ControlReg 复位 0（timer 禁用） */
    if(v!=0x0){printf("ERR: TIM0_ControlReg=0x%x expect 0\n",v);sim_fail();}

    mem_read32_(0x60018000,&v);          /* GPIO SWPORTA_DR 复位 0 */
    if(v!=0x0){printf("ERR: GPIO_SWPORTA_DR=0x%x expect 0\n",v);sim_fail();}

    printf("wdt_chip_reset_recovery test successfully\n");
    sim_end();
}
