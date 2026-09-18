#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start (void)
{
    uint32_t magic = 0, int_status = 0, cr = 0;
    int timeout = 0;

    printf("\nwdt_rmod_interrupt test\n");

    /* Check magic for reset-restart distinction */
    mem_read32_(0x20002000, &magic);

    if (magic != 0x12345678) {
        /* First boot (after power-on or global reset) */
        printf("First boot after reset\n");
        /* Mark as configured */
        mem_write32_(0x20002000, 0x12345678);
        /* time_out = 0x00 (TOP=0 fast expire) */
        mem_write32_(0x50008004, 0x00);
        /* CR = 0x3 (EN=1, RMOD=1) */
        mem_write32_(0x50008000, 0x3);

        /* Poll int_status==1 (first timeout -> interrupt not reset); timeout 500000 */
        mem_read32_(0x50008010, &int_status);
        while (int_status != 0x1 && timeout < 500000) {
            mem_read32_(0x50008010, &int_status);
            timeout++;
        }
        if (timeout >= 500000) {
            printf("ERROR: wdt timeout interrupt overflow, int_status=0x%08x\n", int_status);
            sim_fail();
        }
        printf("wdt rmod first timeout interrupt OK\n");

        /* Read int_clr */
        mem_read32_(0x50008014, &int_status);
        mem_read32_(0x50008010, &int_status);
        if (int_status != 0) {
            printf("ERROR: int_status not cleared after int_clr: 0x%08x\n", int_status);
            sim_fail();
        }

        /* Do NOT feed watchdog - wait for second timeout to trigger system reset */
        while (1) {}
    } else {
        /* After reset: CR bit0 must be 0 (reset clears EN; F1 "only system reset can clear") */
        mem_read32_(0x50008000, &cr);
        if ((cr & 0x1) != 0) {
            printf("ERROR: WDT_CR bit0 not cleared after reset: 0x%08x\n", cr);
            sim_fail();
        }
        printf("wdt_rmod_interrupt test successfully\n");
        sim_end();
    }
    return 0;
}
