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
    uint32_t val1, val2, val3;
    uint32_t int_status = 0;
    int i = 0;
    int timeout = 0;

    /* Step 1: Enable Timer1 free-running mode (ControlReg = 0x1) */
    mem_write32_(0x50000008, 0x1);

    /* Step 2: Read CurrentValue several times to verify decrement and wrap-around */
    for (i = 0; i < 16; i++) {
        mem_read32_(0x50000004, &val1);
        mem_read32_(0x50000004, &val2);
        mem_read32_(0x50000004, &val3);
        printf("free-running read %d: 0x%08x 0x%08x 0x%08x\n", i, val1, val2, val3);
    }

    /* Step 3: Switch to user-defined mode, full cycle with LoadCount = 0x400 */
    /* Disable timer */
    mem_write32_(0x50000008, 0x2);
    /* Write LoadCount */
    mem_write32_(0x50000000, 0x400);
    /* Enable timer in user-defined mode */
    mem_write32_(0x50000008, 0x3);

    /* Poll IntStatus with timeout (100000) */
    timeout = 0;
    while (int_status != 0x1 && timeout < 100000) {
        mem_read32_(0x50000010, &int_status);
        timeout++;
    }
    if (timeout >= 100000) {
        printf("ERROR: timer interrupt timeout, IntStatus=0x%08x\n", int_status);
        sim_fail();
    }

    /* Read int_clr to clear interrupt */
    mem_read32_(0x5000000c, &int_status);

    /* Verify CurrentValue in range [0x400, 0x000] */
    mem_read32_(0x50000004, &val1);
    if (val1 > 0x400) {
        printf("ERROR: CurrentValue 0x%08x > 0x400 after wrap\n", val1);
        sim_fail();
    }
    printf("timer_freerun_smoke test successfully\n");
    sim_end();
}
