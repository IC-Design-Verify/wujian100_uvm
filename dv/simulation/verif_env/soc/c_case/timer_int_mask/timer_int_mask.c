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

int test_start(void)
{
  unsigned int istat;
  unsigned int timeout;
  unsigned int i;
  unsigned int dummy;

  printf("\nstart timer_int_mask\n");

  // NOTE: Timer1IntStatus is POST-mask in RTL (tim.v: IntStatus = raw & ~mask).
  // So with mask=1 the status register reads 0 while the raw interrupt is pending;
  // clearing the mask bit then exposes the pending interrupt as IntStatus==1.

  // stop Timer1
  mem_write32_(0x50000008, 0x2);

  // case A: mask = 1 (enable + user-defined + interrupt_mask)
  mem_write32_(0x50000000, 0x400);
  mem_write32_(0x50000008, 0x7);

  // wait long enough for the timer to expire (0x400 pclk cycles);
  // APB read loop below takes far longer than that
  for (i = 0; i < 20000; i++) {
    mem_read32_(0x50000004, &dummy);  // read CurrentValue as delay
  }

  // key assertion: masked => IntStatus stays 0 even though timer expired
  mem_read32_(0x50000010, &istat);
  if (istat != 0) {
    printf("Timer1IntStatus should be 0 with mask=1, actual=0x%08x\n", istat);
    sim_fail();
  }
  printf("\nTimer1 mask=1: IntStatus kept 0 while expired (masked) - pass\n");

  // unmask (keep enabled, user-defined): pending raw interrupt must now show
  mem_write32_(0x50000008, 0x3);
  timeout = 0;
  mem_read32_(0x50000010, &istat);
  while (istat != 0x1) {
    mem_read32_(0x50000010, &istat);
    timeout++;
    if (timeout >= 100000) {
      printf("TIM1 IntStatus not set after unmask\n");
      sim_fail();
    }
  }
  printf("Timer1 unmask: pending interrupt visible as IntStatus==1 - pass\n");

  // clear interrupt
  mem_read32_(0x5000000c, &istat);  // int_clr

  mem_read32_(0x50000010, &istat);
  if (istat != 0) {
    printf("Timer1IntStatus not cleared after int_clr\n");
    sim_fail();
  }

  // case B: mask = 0 (enable + user-defined), normal interrupt flow
  mem_write32_(0x50000008, 0x2);  // stop first
  mem_write32_(0x50000000, 0x400);
  mem_write32_(0x50000008, 0x3);

  timeout = 0;
  mem_read32_(0x50000010, &istat);
  while (istat != 0x1) {
    mem_read32_(0x50000010, &istat);
    timeout++;
    if (timeout >= 100000) {
      printf("TIM1 IntStatus timeout with mask=0\n");
      sim_fail();
    }
  }

  mem_read32_(0x5000000c, &istat);  // int_clr

  mem_read32_(0x50000010, &istat);
  if (istat != 0) {
    printf("Timer1IntStatus not cleared after int_clr with mask=0\n");
    sim_fail();
  }

  printf("\nTimer1 mask=0 case passed\n");

  printf("timer_int_mask test successfully\n");
  sim_end();
}
