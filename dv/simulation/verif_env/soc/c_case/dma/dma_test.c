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

int test_start (void)
{
int int_dma_flag=0;
int data_check=0;
long int rdata;

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
  mem_write32_(0x40000000, 0x00005000);
//set DMA ch0 dest addr DSRAM  0x20025000
  mem_write32_(0x40000004, 0x20025000);
//set DMA ch0 src incr, dest incr, block size:36 byte
  mem_write32_(0x40000008, 0x2300A);
//set DMA ch0 intr disable , block trigger , src little, dest little
  mem_write32_(0x4000000c, 0x5);

  mem_write32_(0x40000010, 0x1f);
// set DMA ch0 enable
  mem_write32_(0x40000020, 0x1);

// set DMA global enable
  mem_write32_(0x4000033c, 0x1);


// set DMA ch0 softer trigger
  mem_write32_(0x4000001c, 0x1);
  
 
  mem_read32_(0x40000014, &rdata);
//wait timer interrupt state 
    while (  rdata != 0xe){
      mem_read32_(0x40000014, &rdata);
	}


//clear timer interrut state
  mem_read32_(0x20025000, &rdata);
  //printf("111111111111111111111, data = %x\n", rdata);
  if(rdata != 0x11111111)
  {
  	sim_fail();
  }

  mem_read32_(0x20025004, &rdata);
  if(rdata != 0x22222222)
  {
  	sim_fail();
  }

  mem_read32_(0x20025008, &rdata);
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



