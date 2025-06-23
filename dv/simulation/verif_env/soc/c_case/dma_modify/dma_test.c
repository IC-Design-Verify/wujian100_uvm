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
#include "dma.h"

int test_start (void)
{
int int_dma_flag=0;
int data_check=0;
long int rdata;

int waddr, wdata;

//initial mem 
  mem_write32_(0x00005000, 0x11111111);
  mem_write32_(0x00005004, 0x22222222);
  mem_write32_(0x00005008, 0x33333333);
  mem_write32_(0x0000500c, 0x44444444);
  mem_write32_(0x00005010, 0x55555555);
  mem_write32_(0x00005014, 0x66666666);
  mem_write32_(0x00005018, 0x77777777);
  mem_write32_(0x0000501c, 0x88888888);

  mem_write32_(0x20025000, 0x00000000);
  mem_write32_(0x20025004, 0x00000000);
  mem_write32_(0x20025008, 0x00000000);
  mem_write32_(0x2002500c, 0x00000000);
  mem_write32_(0x20025010, 0x00000000);
  mem_write32_(0x20025014, 0x00000000);
  mem_write32_(0x20025018, 0x00000000);
  mem_write32_(0x2002501c, 0x00000000);


  //set DMA ch0 Source addr ISRAM 0x8005000
  waddr = DMA_CH0_BASE_ADDR + DMA_CH_SAR_OFFSET;
  wdata = 0x00005000 << 0;
  mem_write32_(waddr, wdata);
  //set DMA ch0 dest addr DSRAM  0x20025000
  waddr = DMA_CH0_BASE_ADDR + DMA_CH_DAR_OFFSET;
  wdata = 0x20025000 << 0;
  mem_write32_(waddr, wdata);
  //set DMA ch0 src incr, dest incr, block size:36 byte
  waddr = DMA_CH0_BASE_ADDR + DMA_CH_CTRL_A_OFFSET;
  wdata = 0x2<<16 | 0x3<<12 | 0<<6 | 0<<4 | 0x2<<2 | 0x2<<0;
  mem_write32_(waddr, wdata);
  //set DMA ch0 intr disable , block trigger , src little, dest little
  waddr = DMA_CH0_BASE_ADDR + DMA_CH_CTRL_B_OFFSET;
  wdata = 0x0<<15 | 0x0<<14 | 0x0<<13 | 0x2<<1 | 0x1<<0;
  mem_write32_(waddr, wdata);

  waddr = DMA_CH0_BASE_ADDR + DMA_CH_INT_MASK_OFFSET;
  wdata = 0x1<<4 | 0x1<<3 | 0x1<<2 | 0x1<<1 | 0x1<<0;
  mem_write32_(waddr, wdata);
  // set DMA ch0 enable
  waddr = DMA_CH0_BASE_ADDR + DMA_CH_EN_OFFSET;
  wdata = 0x1<<0;
  mem_write32_(waddr, wdata);

  // set DMA global enable
  waddr = DMA_GLOBAL_BASE_ADDR + DMA_DMACCFG_OFFSET;
  wdata = 0x1<<0;
  mem_write32_(waddr, wdata);


  // set DMA ch0 softer trigger
  waddr = DMA_CH0_BASE_ADDR + DMA_CH_SOFT_REQ_OFFSET;
  wdata = 0x1<<0;
  mem_write32_(waddr, wdata);
  
 
  waddr = 0x40000014;//DMA_CH0_BASE_ADDR + DMA_CH_INT_STATUS_OFFSET;
  mem_read32_(waddr, &rdata);
  //wait timer interrupt state 
    while (  rdata != 0xe){
      mem_read32_(waddr, &rdata);
	}


//clear timer interrut state
  mem_read32_(0x20025000, &rdata);
  printf("111111111111111111111, data = %x\n", rdata);
  if(rdata != 0x11111111)
  {
  	sim_fail();
  }

  mem_read32_(0x20025004, &rdata);
  printf("222222222222222222222, data = %x\n", rdata);
  if(rdata != 0x22222222)
  {
  	sim_fail();
  }

  mem_read32_(0x20025008, &rdata);
  printf("333333333333333333333, data = %x\n", rdata);
  if(rdata != 0x33333333)
  {
  	sim_fail();
  }

  mem_read32_(0x2002500c, &rdata);
  if(rdata != 0x44444444)
  {
  	sim_fail();
  }

  mem_read32_(0x20025010, &rdata);
  if(rdata != 0x55555555)
  {
  	sim_fail();
  }

  mem_read32_(0x20025014, &rdata);
  if(rdata != 0x66666666)
  {
  	sim_fail();
  }

  mem_read32_(0x20025018, &rdata);
  if(rdata != 0x77777777)
  {
  	sim_fail();
  }

  mem_read32_(0x2002501c, &rdata);
  if(rdata != 0x88888888)
  {
  	sim_fail();
  }

  printf("dma test successfully\n");
  sim_end();

}



