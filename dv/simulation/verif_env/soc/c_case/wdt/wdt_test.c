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
int int_wdt_reset_flag=0;
   mem_read32_(0x20002000, &int_wdt_reset_flag);
    if ( int_wdt_reset_flag == 0x12345678){
  	printf("\nwdt reset test successfully\n");
  	sim_end();
	}
	// Timeout period of initialization: 0x3ffff, Timeout period: 0xffff
    mem_write32_(0x50008004, 0x10);;
	// Enable Watchdog, set Watchdog system reset pulse length to 256 pclk cycles
	// to ensure it can be sampled by oscclk. Actually, Watchdog system reset will
	// be only active for 2 oscclk cycles
    mem_write32_(0x50008000, 0x1d);;
	// Restart Watchdog counter, decreaseing from 0xffff
    mem_write32_(0x5000800c, 0x78);;
    mem_write32_(0x20002000, 0x12345678);;
while(1){}

} 



