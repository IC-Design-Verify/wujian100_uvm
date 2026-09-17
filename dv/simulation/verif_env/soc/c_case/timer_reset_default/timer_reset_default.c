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

    /* Timer1 registers: 0x00, 0x04, 0x08, 0x0C, 0x10 */
    /* Timer2 registers: 0x14, 0x18, 0x1C, 0x20, 0x24 */
    /* All reset values are 0 */

    /* Check Timer1LoadCount (0x00) */
    mem_read32_(0x50000000, &val);
    if (val != 0) {
        printf("ERROR: Timer1LoadCount offset 0x00 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer1CurrentValue (0x04) */
    mem_read32_(0x50000004, &val);
    if (val != 0) {
        printf("ERROR: Timer1CurrentValue offset 0x04 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer1ControlReg (0x08) */
    mem_read32_(0x50000008, &val);
    if (val != 0) {
        printf("ERROR: Timer1ControlReg offset 0x08 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer1_int_clr (0x0C) */
    mem_read32_(0x5000000C, &val);
    if (val != 0) {
        printf("ERROR: Timer1_int_clr offset 0x0C actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer1IntStatus (0x10) */
    mem_read32_(0x50000010, &val);
    if (val != 0) {
        printf("ERROR: Timer1IntStatus offset 0x10 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer2LoadCount (0x14) */
    mem_read32_(0x50000014, &val);
    if (val != 0) {
        printf("ERROR: Timer2LoadCount offset 0x14 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer2CurrentValue (0x18) */
    mem_read32_(0x50000018, &val);
    if (val != 0) {
        printf("ERROR: Timer2CurrentValue offset 0x18 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer2ControlReg (0x1C) */
    mem_read32_(0x5000001C, &val);
    if (val != 0) {
        printf("ERROR: Timer2ControlReg offset 0x1C actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer2_int_clr (0x20) */
    mem_read32_(0x50000020, &val);
    if (val != 0) {
        printf("ERROR: Timer2_int_clr offset 0x20 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* Check Timer2IntStatus (0x24) */
    mem_read32_(0x50000024, &val);
    if (val != 0) {
        printf("ERROR: Timer2IntStatus offset 0x24 actual=0x%08x expected=0x00000000\n", val);
        sim_fail();
    }

    /* All registers passed */
    printf("timer_reset_default test successfully\n");
    sim_end();
}
