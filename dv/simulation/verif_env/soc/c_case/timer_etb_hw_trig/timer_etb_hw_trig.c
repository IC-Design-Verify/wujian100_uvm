/*
Copyright (c) 2019 Alibaba Group Holding Limited

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start (void)
{
    uint32_t val = 0;
    int timeout = 0;

    printf("\nETB hw trigger test\n");

    /* Step 3: Stop -> LoadCount=0x400 -> ControlReg=0x12 (hw_trig_en + user-defined, enable=0) */
    mem_write32_(0x50000008, 0x2);
    mem_write32_(0x50000000, 0x400);
    mem_write32_(0x50000008, 0x12);

    /* Step 4: Poll IntStatus==1 (timeout 500000) */
    mem_read32_(0x50000010, &val);
    while (val != 0x1 && timeout < 500000) {
        mem_read32_(0x50000010, &val);
        timeout++;
    }
    if (timeout >= 500000) {
        printf("ERROR: ETB hw trigger timeout, IntStatus=0x%08x\n", val);
        sim_fail();
    }
    printf("ETB hw trigger start OK\n");

    /* Step 5: Clear interrupt */
    mem_read32_(0x5000000c, &val);

    /* Step 6: Poll ControlReg bit0==0 (timeout 500000) */
    timeout = 0;
    mem_read32_(0x50000008, &val);
    while ((val & 0x1) != 0x0 && timeout < 500000) {
        mem_read32_(0x50000008, &val);
        timeout++;
    }
    if (timeout >= 500000) {
        printf("ERROR: ETB off timeout, ControlReg=0x%08x\n", val);
        sim_fail();
    }

    /* Step 7: Read CurrentValue twice; must both be 0 and equal */
    mem_read32_(0x50000004, &val);
    uint32_t val1 = val;
    /* Short delay loop */
    for (int i = 0; i < 100; i++) { __asm__ volatile ("nop"); }
    mem_read32_(0x50000004, &val);
    uint32_t val2 = val;
    if (val1 != 0 || val2 != 0 || val1 != val2) {
        printf("ERROR: CurrentValue not zero in disable state: 0x%08x, 0x%08x\n", val1, val2);
        sim_fail();
    }

    printf("ETB hw trigger off OK\n");
    printf("timer_etb_hw_trig test successfully\n");
    sim_end();
}
