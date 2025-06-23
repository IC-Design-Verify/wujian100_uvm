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
long int pwm_cap_intr_flag = 0x0;
//PWM ch0 count load 2 , cmp value 2
   mem_write32_(0x5001c038, 0x2);
   mem_write32_(0x5001c050, 0x2);
//PWM ch2 capture enable, int enble
   mem_write32_(0x5001c074, 0x302);
   mem_write32_(0x5001c078, 0x2);
//PWM ch2 cnt match 0x20 
   mem_write32_(0x5001c094, 0x200000);
//PWM ch0 output enable, ch2 capture enable ,clk div enable , div 4
   mem_write32_(0x5001c000, 0x9002001);

	mem_read32_(0x5001c07c, &pwm_cap_intr_flag);
      while (  pwm_cap_intr_flag != 0x2){
	mem_read32_(0x5001c07c, &pwm_cap_intr_flag);
	}

    printf("pwm io test pass! \n");

 sim_end();
}
