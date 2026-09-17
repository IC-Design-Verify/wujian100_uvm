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
  unsigned int t1_curr, t2_curr;
  unsigned int t1_stat, t2_stat;
  unsigned int timeout;
  unsigned int t1_prev, t2_prev;

  printf("\nstart timer_dual_ch_parallel\n");

  // stop both channels
  mem_write32_(0x50000008, 0x2);  // T1 ControlReg
  mem_write32_(0x5000001c, 0x2);  // T2 ControlReg

  // load counts: T1=0x400, T2=0x200
  mem_write32_(0x50000000, 0x400);
  mem_write32_(0x50000014, 0x200);

  // enable both, user-defined mode
  mem_write32_(0x50000008, 0x3);
  mem_write32_(0x5000001c, 0x3);

  // observe countdown: sample both CurrentValues for a fixed number of
  // iterations, well before T2 (period 0x200) wraps and reloads.
  // Both channels must be strictly decreasing and independent.
  {
    int k;
    mem_read32_(0x50000004, &t1_prev);
    mem_read32_(0x50000018, &t2_prev);
    for (k = 0; k < 8; k++) {
      mem_read32_(0x50000004, &t1_curr);
      mem_read32_(0x50000018, &t2_curr);

      if (t1_curr >= t1_prev) {
        printf("Timer1 current not decreasing\n");
        sim_fail();
      }
      if (t2_curr >= t2_prev) {
        printf("Timer2 current not decreasing\n");
        sim_fail();
      }
      t1_prev = t1_curr;
      t2_prev = t2_curr;
    }
  }

  // poll Timer1IntStatus == 1 with timeout
  timeout = 0;
  mem_read32_(0x50000010, &t1_stat);
  while (t1_stat != 0x1) {
    mem_read32_(0x50000010, &t1_stat);
    timeout++;
    if (timeout >= 100000) {
      printf("TIM1 int timeout\n");
      sim_fail();
    }
  }

  // poll Timer2IntStatus == 1 with timeout
  timeout = 0;
  mem_read32_(0x50000024, &t2_stat);
  while (t2_stat != 0x1) {
    mem_read32_(0x50000024, &t2_stat);
    timeout++;
    if (timeout >= 100000) {
      printf("TIM2 int timeout\n");
      sim_fail();
    }
  }

  // clear Timer1 only, then verify isolation
  mem_read32_(0x5000000c, &t1_stat);  // int_clr

  mem_read32_(0x50000010, &t1_stat);
  if (t1_stat != 0) {
    printf("Timer1IntStatus not cleared after int_clr\n");
    sim_fail();
  }

  mem_read32_(0x50000024, &t2_stat);
  if (t2_stat != 1) {
    printf("Timer2IntStatus wrongly cleared by Timer1 int_clr\n");
    sim_fail();
  }

  // clear Timer2
  mem_read32_(0x50000020, &t2_stat);  // int_clr

  mem_read32_(0x50000024, &t2_stat);
  if (t2_stat != 0) {
    printf("Timer2IntStatus not cleared after int_clr\n");
    sim_fail();
  }

  printf("timer_dual_ch_parallel test successfully\n");
  sim_end();
}
