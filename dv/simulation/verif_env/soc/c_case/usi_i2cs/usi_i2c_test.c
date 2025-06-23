/*
Copyright (c) 2019 Alibaba Group Holding Limited

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

#define USI0_BADDR  0x50028000
#define USI1_BADDR  0x60028000
#define REG32(addr) *((volatile unsigned int *)(addr))

#define USI_CTRL    0x00
#define MODE_SEL    0x04
#define TX_FIFO     0x08
#define RX_FIFO     0x08
#define FIFO_STA    0x0c
#define CLK_DIV0    0x10
#define CLK_DIV1    0x14
#define I2C_MODE    0x20
#define I2C_ADDR    0x24
#define I2CM_CTRL   0x28
#define I2CS_CTRL   0x30
#define I2C_STA     0x3c

int test_start(void)
{
    long int reg;
    long int tx_empty;
    long int rx_empty;
    long int i2c_busy;
    int cnt=0;

    //GPIO output mode
    mem_write32_(0x60018004, 0xffffffff);
    //GPIO output data 
    mem_write32_(0x60018000, 0x00000000);

    //*************************************
    //        Configure USI1 I2C
    //*************************************
    //sellect i2c
    mem_write32_((USI1_BADDR+MODE_SEL), 0x1);

    //slave mode
    mem_write32_((USI1_BADDR+I2C_MODE), 0x0);

    //slave address
    mem_write32_((USI1_BADDR+I2C_ADDR), 0x3c);

    //enable I2C
    //enable RX FIFO
    mem_write32_((USI1_BADDR+USI_CTRL), 0xb);
    
    //*************************************
    //      USI1 I2C receive data
    //*************************************
    printf("==========================\n");
    printf("usi1 i2c receive start\n");

    do{
        mem_read32_((USI1_BADDR+FIFO_STA), &reg);
        rx_empty = reg & 0x4;

        if(rx_empty==0){
            mem_read32_((USI1_BADDR+RX_FIFO), &reg);
            printf("receive_data[%i] : 0x%x\n",cnt,reg);
            cnt++;
        }
    } while(cnt<4);

    printf("usi1 i2c receive complete\n");
    printf("==========================\n");

    //GPIO output mode
    mem_write32_(0x60018004, 0xffffffff);
    //GPIO output data 
    mem_write32_(0x60018000, 0xdeadbeef);


    while(1) {}
}
