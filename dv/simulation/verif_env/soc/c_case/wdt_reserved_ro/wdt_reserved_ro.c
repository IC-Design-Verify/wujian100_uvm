#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void)
{
  unsigned int val;
  printf("\nstart wdt_reserved_ro\n");

  // ① CR (0x00): 写 0xFFFFFFFE (bit0=0, WDT 保持关闭)
  // 注意: UG 称 CR 仅低 5 bit 有效, 但 RTL wdt.v 中 wdt_cr_ir 为 6 bit
  //       (bit5 可读写的未文档化存储位), 读回应为 0x3E —— 以 RTL 为准并记录
  mem_write32_(0x50008000, 0xFFFFFFFE);
  mem_read32_(0x50008000, &val);
  if (val != 0x3E) {
    printf("WDT_CR offset 0x00: got 0x%x, expect 0x3E\n", val);
    sim_fail();
  }

  // ② time_out (0x04): 写 0xFFFFFFFF，读回==0x000000FF（高 24 位 reserved 读 0）
  mem_write32_(0x50008004, 0xFFFFFFFF);
  mem_read32_(0x50008004, &val);
  if (val != 0x000000FF) {
    printf("WDT_time_out offset 0x04: got 0x%x, expect 0x000000FF\n", val);
    sim_fail();
  }

  // ③ current_value (0x08): RO，写 0xFFFFFFFF 忽略，读回仍==0x0000FFFF
  mem_write32_(0x50008008, 0xFFFFFFFF);
  mem_read32_(0x50008008, &val);
  if (val != 0x0000FFFF) {
    printf("WDT_current_value offset 0x08: got 0x%x, expect 0x0000FFFF\n", val);
    sim_fail();
  }

  // ④ int_status (0x10): RO，写 0xFFFFFFFF 忽略，读回应==0x0
  mem_write32_(0x50008010, 0xFFFFFFFF);
  mem_read32_(0x50008010, &val);
  if (val != 0x0) {
    printf("WDT_int_status offset 0x10: got 0x%x, expect 0x0\n", val);
    sim_fail();
  }

  printf("wdt_reserved_ro test successfully\n");
  sim_end();
}
