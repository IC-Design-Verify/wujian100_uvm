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
    long int gpio_data_in = 0x0;
    int cnt=0;

    //*************************************
    //        Configure USI0 I2C
    //*************************************
    //sellect i2c
    mem_write32_((USI0_BADDR+MODE_SEL), 0x1);

    //F(sclk) : 100Khz
    //(CLK_DIV0+CLK_DIV1) = (20M/100K)-2 = 198
    mem_write32_((USI0_BADDR+CLK_DIV0), 0x63);
    mem_write32_((USI0_BADDR+CLK_DIV1), 0x63);

    //master mode
    mem_write32_((USI0_BADDR+I2C_MODE), 0x1);

    //slave address
    mem_write32_((USI0_BADDR+I2C_ADDR), 0x3c);

    //7bit-address
    //generate stop if tx fifo empty
    mem_write32_((USI0_BADDR+I2CM_CTRL), 0x2);

    //enable I2C
    //enable TX FIFO
    mem_write32_((USI0_BADDR+USI_CTRL), 0x7);
    
    //*************************************
    //       USI0 I2C send data
    //*************************************
    printf("==========================\n");
    printf("usi0 i2c transmit start\n");
    printf("send_data[0] : 0xa5\n");
    printf("send_data[1] : 0xa6\n");
    printf("send_data[2] : 0xa7\n");
    printf("send_data[3] : 0xa8\n");
    mem_write32_((USI0_BADDR+TX_FIFO), 0xa5);
    mem_write32_((USI0_BADDR+TX_FIFO), 0xa6);
    mem_write32_((USI0_BADDR+TX_FIFO), 0xa7);
    mem_write32_((USI0_BADDR+TX_FIFO), 0xa8);

    //wait untill 
    //  tx_fifo empty and
    //  transmit is not busy
    do{
        mem_read32_((USI0_BADDR+FIFO_STA), &reg);
        tx_empty = reg & 0x1;

        mem_read32_((USI0_BADDR+I2C_STA), &reg);
        i2c_busy = reg & 0x1;
    } while(tx_empty==0 || i2c_busy==1);
        
    printf("usi0 i2c transmit complete\n");
    printf("==========================\n");

    //GPIO SW CTL
    mem_write32_(0x60018008, 0x0);
    //GPIO input mode
    mem_write32_(0x60018004, 0x0);

    printf("Waiting I2C slave done!\n");
    do{
	    mem_read32_(0x60018050, &gpio_data_in);
      printf("Waiting...\n");
    } while(gpio_data_in != 0xdeadbeef);

    sim_end();
}
