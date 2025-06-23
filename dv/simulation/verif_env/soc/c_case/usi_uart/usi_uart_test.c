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
#define UART_CTRL   0x18
#define UART_STA    0x1c

int test_start(void)
{
    long int reg;
    long int tx_empty;
    long int rx_empty;
    long int uart_busy;
    int cnt=0;

    int int_dma_flag=0;
    int data_check=0;
    long int rdata;

    //*******************************
    //          USI0 UART
    //*******************************
    //sellect uart
    mem_write32_((USI0_BADDR+MODE_SEL), 0x0);

    //baud rate : 9600
    //CLK_DIV0 = (20M/(16*9600))-1 = 129
    mem_write32_((USI0_BADDR+CLK_DIV0), 0x81);

    //data_bits=8
    //stop_bits=1
    //parity disable
    mem_write32_((USI0_BADDR+UART_CTRL), 0x3);

    //enable uart
    //enable tx fifo
    mem_write32_((USI0_BADDR+USI_CTRL), 0x7);

    //*******************************
    //          USI1 UART
    //*******************************
    //sellect uart
    mem_write32_((USI1_BADDR+MODE_SEL), 0x0);

    //baud rate : 9600
    //CLK_DIV0 = (20M/(16*9600))-1 = 129
    mem_write32_((USI1_BADDR+CLK_DIV0), 0x81);

    //data_bits=8
    //stop_bits=1
    //parity disable
    mem_write32_((USI1_BADDR+UART_CTRL), 0x3);

    //enable uart
    //enable rx fifo
    mem_write32_((USI1_BADDR+USI_CTRL), 0xb);

    //*******************************
    //      USI0 UART send data
    //*******************************
    //send data
    printf("===========================\n");
    printf("usi0 uart transmit start\n");
    printf("send_data[0] : 0x60\n");
    printf("send_data[1] : 0x61\n");
    printf("send_data[2] : 0x62\n");
    printf("send_data[3] : 0x63\n");
    mem_write32_((USI0_BADDR+TX_FIFO), 0x60);
    mem_write32_((USI0_BADDR+TX_FIFO), 0x61);
    mem_write32_((USI0_BADDR+TX_FIFO), 0x62);
    mem_write32_((USI0_BADDR+TX_FIFO), 0x63);

  //sim_save();
    //wait untill 
    //  tx_fifo empty and
    //  transmit is not busy
    do{
        mem_read32_((USI0_BADDR+FIFO_STA), &reg);
        tx_empty = reg & 0x1;

        mem_read32_((USI0_BADDR+UART_STA), &reg);
        uart_busy = reg & 0x1;
    } while(tx_empty==0 || uart_busy==1);
        
    printf("usi0 uart transmit complete\n");
    printf("===========================\n");

    //*******************************
    //     USI1 UART receive data
    //*******************************
    printf("===========================\n");
    printf("usi1 uart receive start\n");

    do{
        mem_read32_((USI1_BADDR+FIFO_STA), &reg);
        rx_empty = reg & 0x4;

        if(rx_empty==0){
            mem_read32_((USI1_BADDR+RX_FIFO), &reg);
            printf("receive_data[%i] : 0x%x\n",cnt,reg);
            cnt++;
        }
    } while(cnt<4);

    printf("usi1 uart receive complete\n");
    printf("==========================\n");

//initial mem 
  mem_write32_(0x20025020, 0x11111111);
  mem_write32_(0x20025024, 0x22222222);
  mem_write32_(0x20025028, 0x33333333);
  mem_write32_(0x2002502c, 0x44444444);
  mem_write32_(0x20025030, 0x55555555);
  mem_write32_(0x20025034, 0x66666666);
  mem_write32_(0x20025038, 0x77777777);
  mem_write32_(0x2002503c, 0x88888888);

  mem_write32_(0x20025000, 0x00000000);
  mem_write32_(0x20025004, 0x00000000);
  mem_write32_(0x20025008, 0x00000000);
  mem_write32_(0x2002500c, 0x00000000);
  mem_write32_(0x20025010, 0x00000000);
  mem_write32_(0x20025014, 0x00000000);
  mem_write32_(0x20025018, 0x00000000);
  mem_write32_(0x2002501c, 0x00000000);

  sim_save();

    printf("11111111111111111111111111\n");

  sim_end();
}
