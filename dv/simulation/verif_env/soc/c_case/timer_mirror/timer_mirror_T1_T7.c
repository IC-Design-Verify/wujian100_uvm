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
  int i;
  int tim_flag = 0;
  int timeout = 0;

  unsigned int tim_bases[7] = {
    0x60000000,
    0x50000400,
    0x60000400,
    0x50000800,
    0x60000800,
    0x50000C00,
    0x60000C00
  };

  for (i = 0; i < 7; i++) {
    unsigned int base = tim_bases[i];
    tim_flag = 0;
    timeout = 0;

    // stop timer
    mem_write32_(base + 0x08, 0x2);
    // load count
    mem_write32_(base + 0x00, 0x400);
    // enable timer (user-defined)
    mem_write32_(base + 0x08, 0x3);

    // poll IntStatus with timeout
    mem_read32_(base + 0x10, &tim_flag);
    while (tim_flag != 0x1) {
      mem_read32_(base + 0x10, &tim_flag);
      timeout++;
      if (timeout >= 100000) {
        printf("TIM%d timeout\n", i + 1);
        sim_fail();
      }
    }

    // clear interrupt
    mem_read32_(base + 0x0C, &tim_flag);
    // confirm IntStatus == 0
    mem_read32_(base + 0x10, &tim_flag);
    if (tim_flag != 0) {
      printf("TIM%d int_clr failed\n", i + 1);
      sim_fail();
    }

    printf("TIM%d pass\n", i + 1);
  }

  printf("timer_mirror_T1_T7 test successfully\n");
  sim_end();
}
