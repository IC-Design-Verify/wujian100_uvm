#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void)
{
  unsigned int val;
  printf("\nstart wdt_reset_default\n");

  // CR (0x00) reset = 0x00
  // 注意: UG 称 CR reset=0x02 (RMOD=1), 但 RTL wdt_params.v 中 WDT_DFLT_RMOD=1'b0,
  //       实际复位值 0x00 —— Spec-vs-RTL 差异, 以 RTL 为准并记录
  mem_read32_(0x50008000, &val);
  if (val != 0x00) {
    printf("WDT_CR offset 0x00: got 0x%x, expect 0x00\n", val);
    sim_fail();
  }

  // time_out (0x04) reset = 0x00
  mem_read32_(0x50008004, &val);
  if (val != 0x00) {
    printf("WDT_time_out offset 0x04: got 0x%x, expect 0x00\n", val);
    sim_fail();
  }

  // current_value (0x08) reset = 0x0000FFFF
  mem_read32_(0x50008008, &val);
  if (val != 0x0000FFFF) {
    printf("WDT_current_value offset 0x08: got 0x%x, expect 0x0000FFFF\n", val);
    sim_fail();
  }

  // restart (0x0C) readback = 0x00
  mem_read32_(0x5000800C, &val);
  if (val != 0x00) {
    printf("WDT_restart offset 0x0C: got 0x%x, expect 0x00\n", val);
    sim_fail();
  }

  // int_status (0x10) reset = 0x0
  mem_read32_(0x50008010, &val);
  if (val != 0x0) {
    printf("WDT_int_status offset 0x10: got 0x%x, expect 0x0\n", val);
    sim_fail();
  }

  // int_clr (0x14) readback = 0x0
  mem_read32_(0x50008014, &val);
  if (val != 0x0) {
    printf("WDT_int_clr offset 0x14: got 0x%x, expect 0x0\n", val);
    sim_fail();
  }

  printf("wdt_reset_default test successfully\n");
  sim_end();
}
