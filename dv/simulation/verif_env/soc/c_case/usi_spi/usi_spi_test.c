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
#define SPI_MODE    0x40
#define SPI_CTRL    0x44
#define SPI_STA     0x48

int test_start(void)
{
    long int reg;
    long int tx_empty;
    long int rx_empty;
    long int spi_busy;
    int cnt=0;

    //*******************************
    //          USI0 SPI
    //*******************************
    //sellect spi
    mem_write32_((USI0_BADDR+MODE_SEL), 0x2);

    //F(sclk) : 100Khz
    //CLK_DIV0 = 20M/100K = 200
    mem_write32_((USI0_BADDR+CLK_DIV0), 0xc8);

    //master mode
    mem_write32_((USI0_BADDR+SPI_MODE), 0x1);

    //data_size=16
    //transmit only
    //CPHA=0
    //CPOL=0
    mem_write32_((USI0_BADDR+SPI_CTRL), 0x1f);

    //enable SPI
    //enable TX FIFO
    mem_write32_((USI0_BADDR+USI_CTRL), 0x7);
    
    //*******************************
    //          USI1 SPI
    //*******************************
    //sellect spi
    mem_write32_((USI1_BADDR+MODE_SEL), 0x2);

    //slave mode
    mem_write32_((USI1_BADDR+SPI_MODE), 0x0);

    //data_size=16
    //receive only
    //CPHA=0
    //CPOL=0
    mem_write32_((USI1_BADDR+SPI_CTRL), 0x2f);

    //enable SPI
    //enable RX FIFO
    mem_write32_((USI1_BADDR+USI_CTRL), 0xb);
    
    //*******************************
    //       USI0 SPI send data
    //*******************************
    //send data
    printf("==========================\n");
    printf("usi0 spi transmit start\n");
    printf("send_data[0] : 0x1234\n");
    printf("send_data[1] : 0x1235\n");
    printf("send_data[2] : 0x1236\n");
    printf("send_data[3] : 0x1237\n");
    mem_write32_((USI0_BADDR+TX_FIFO), 0x1234);
    mem_write32_((USI0_BADDR+TX_FIFO), 0x1235);
    mem_write32_((USI0_BADDR+TX_FIFO), 0x1236);
    mem_write32_((USI0_BADDR+TX_FIFO), 0x1237);

    //wait untill 
    //  tx_fifo empty and
    //  transmit is not busy
    do{
        mem_read32_((USI0_BADDR+FIFO_STA), &reg);
        tx_empty = reg & 0x1;

        mem_read32_((USI0_BADDR+SPI_STA), &reg);
        spi_busy = reg & 0x1;
    } while(tx_empty==0 || spi_busy==1);
        
    printf("usi0 spi transmit complete\n");
    printf("==========================\n");

    //*******************************
    //      USI1 SPI receive data
    //*******************************
    printf("==========================\n");
    printf("usi1 spi receive start\n");

    do{
        mem_read32_((USI1_BADDR+FIFO_STA), &reg);
        rx_empty = reg & 0x4;

        if(rx_empty==0){
            mem_read32_((USI1_BADDR+RX_FIFO), &reg);
            printf("receive_data[%i] : 0x%x\n",cnt,reg);
            cnt++;
        }
    } while(cnt<4);

    printf("usi1 spi receive complete\n");
    printf("==========================\n");

 sim_end();

}
