#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void){
    uint32_t v=0,p=0,i=0;
    uint32_t timeout=0;

    mem_write32_(0x50008004,0xF0);
    mem_write32_(0x50008000,0x3);

    /* 等待 initial load: cnt 从复位值 0xFFFF 加载到 TOP_INIT(0x7FFFFFFF)
     * (enable 上升沿后 ~3 pclk 完成, 首次直接读可能采到加载前的 0xFFFF) */
    do {
        mem_read32_(0x50008008,&v);
        timeout++;
    } while (v <= 0x10000 && timeout < 1000);
    if (timeout >= 1000) {printf("ERR:F5 no initial load, cnt=0x%08x\n",v);sim_fail();}

    /* 严格递减窗口 ×8 */
    for(i=0;i<8;i++){
        mem_read32_(0x50008008,&p);
        if(p>=v){printf("ERR:F5 decrement\n");sim_fail();}
        v=p;
    }

    mem_write32_(0x5000800C,0x76);
    mem_read32_(0x50008008,&p);
    if(p<0xFF00){printf("ERR:kick 0x76 got 0x%08x\n",p);sim_fail();}

    /* 错误 magic 不应触发重载; 100 次空转读 (~1.5k pclk) 后 cnt 仍应远低于重载值 0xFFFF,
     * 且累计递减量 << 0xFFFF 不会回绕 (回绕会误判为重载) */
    mem_write32_(0x5000800C,0x78);
    for(i=0;i<100;i++){mem_read32_(0x50008008,&v);}
    mem_read32_(0x50008008,&p);
    if(p>0xFFF0){printf("ERR:0x78 invalid got high 0x%08x\n",p);sim_fail();}

    mem_write32_(0x5000800C,0xFF);
    for(i=0;i<100;i++){mem_read32_(0x50008008,&v);}
    mem_read32_(0x50008008,&p);
    if(p>0xFFF0){printf("ERR:0xFF invalid got high 0x%08x\n",p);sim_fail();}

    mem_write32_(0x5000800C,0x00);
    for(i=0;i<100;i++){mem_read32_(0x50008008,&v);}
    mem_read32_(0x50008008,&p);
    if(p>0xFFF0){printf("ERR:0x00 invalid got high 0x%08x\n",p);sim_fail();}

    mem_write32_(0x5000800C,0x76);
    printf("wdt_magic_kick test successfully\n");
    sim_end();
}
