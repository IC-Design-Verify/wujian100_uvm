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
int flag_data =0;
long int read_data =0;

//isram
   flag_data = 0x01010101;
   mem_write32_(0x0000a000, flag_data);
  mem_read32_(0x0000a000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}
//dsram0
   flag_data = 0x02020202;
   mem_write32_(0x20000000, flag_data);
  mem_read32_(0x20000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}
//dsram1
   flag_data = 0x03030303;
   mem_write32_(0x20010000, flag_data);
  mem_read32_(0x20010000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}
//dsram2
   flag_data = 0x04040404;
   mem_write32_(0x20020000, flag_data);
  mem_read32_(0x20020000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}
//dmac
   flag_data = 0x05050505;
   mem_write32_(0x40000000, flag_data);
  mem_read32_(0x40000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


//tim0
   flag_data = 0x06060606;
   mem_write32_(0x50000000, flag_data);
  mem_read32_(0x50000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//tim2
   flag_data = 0x07070707;
   mem_write32_(0x50000400, flag_data);
  mem_read32_(0x50000400, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//tim4
   flag_data = 0x08080808;
   mem_write32_(0x50000800, flag_data);
  mem_read32_(0x50000800, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//tim6
   flag_data = 0x09090909;
   mem_write32_(0x50000c00, flag_data);
  mem_read32_(0x50000c00, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//usi0
   flag_data = 0x0a0a0a;
   mem_write32_(0x50028010, flag_data);
  mem_read32_(0x50028010, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}
//usi2
   flag_data = 0x050505;
   mem_write32_(0x50029010, flag_data);
  mem_read32_(0x50029010, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


//wdt
   flag_data = 0xff;
   mem_write32_(0x50008004, flag_data);
  mem_read32_(0x50008004, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}



//pwm
   flag_data = 0xff00ff;
   mem_write32_(0x5001c004, flag_data);
  mem_read32_(0x5001c004, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}



//tim1
   flag_data = 0x0b0b0b0b;
   mem_write32_(0x60000000, flag_data);
  mem_read32_(0x60000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//tim3
   flag_data = 0x0c0c0c0c;
   mem_write32_(0x60000400, flag_data);
  mem_read32_(0x60000400, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//tim5
   flag_data = 0x0d0d0d0d;
   mem_write32_(0x60000800, flag_data);
  mem_read32_(0x60000800, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//tim7
   flag_data = 0x0e0e0e0e;
   mem_write32_(0x60000c00, flag_data);
  mem_read32_(0x60000c00, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//rtc
   flag_data = 0x12345678;
   mem_write32_(0x60004004, flag_data);
  mem_read32_(0x60004004, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


//usi1
   flag_data = 0x0f0f0f;
   mem_write32_(0x60028010, flag_data);
  mem_read32_(0x60028010, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//gpio
   flag_data = 0x12345678;
   mem_write32_(0x60018000, flag_data);
  mem_read32_(0x60018000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


printf("All IP write/read test Pass!\n");

//MAIN BUS DUMMY
   flag_data = 0x1;
  mem_read32_(0x10000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x2;
  mem_read32_(0x30000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}



   flag_data = 0x3;
  mem_read32_(0x40010000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}



   flag_data = 0x4;
  mem_read32_(0x40020000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x5;
  mem_read32_(0x40100000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x6;
  mem_read32_(0x80000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//LS BUS DUMMY
   flag_data = 0x11;
  mem_read32_(0x40200000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x12;
  mem_read32_(0x40300000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x13;
  mem_read32_(0x70000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x14;
  mem_read32_(0x78000000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


//APB0 BUS DUMMY
   flag_data = 0x22;
   mem_read32_(0x50004000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x23;
  mem_read32_(0x5000c000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x24;
  mem_read32_(0x50010000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x25;
  mem_read32_(0x50014000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x26;
  mem_read32_(0x50018000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x28;
  mem_read32_(0x50020000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

   flag_data = 0x29;
  mem_read32_(0x50024000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x2a;
  mem_read32_(0x50030000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}

//APB1 DUMMY
   flag_data = 0x32;
  mem_read32_(0x60008000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x33;
  mem_read32_(0x6000c000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x34;
  mem_read32_(0x60010000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x35;
  mem_read32_(0x60014000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x36;
  mem_read32_(0x6001c000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}


   flag_data = 0x37;
  mem_read32_(0x60020000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}



   flag_data = 0x38;
  mem_read32_(0x60024000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}



   flag_data = 0x39;
  mem_read32_(0x6002c000, &read_data);

	if(read_data != flag_data){
	sim_fail();
	}
  printf("\n Dummy IP read test Pass!\n");
 sim_end();

}
