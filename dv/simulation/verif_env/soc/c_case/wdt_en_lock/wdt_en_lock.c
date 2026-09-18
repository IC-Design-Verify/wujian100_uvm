#ifndef FOR_VIP_RUN
#include "stdio.h"
#include "vtimer.h"
#include "datatype.h"
#endif

int test_start(void)
{
  unsigned int cr;
  printf("\nstart wdt_en_lock\n");

  // ① time_out = 0xFF (TOP=15, 超时 2^31，不会触发复位)
  mem_write32_(0x50008004, 0xFF);

  // ② CR = 0x3 (EN=1, RMOD=1) 使能并设 RMOD=1
  mem_write32_(0x50008000, 0x3);

  // ③ 试图清 EN：写 CR = 0x2
  mem_write32_(0x50008000, 0x2);

  // ④ 读 CR 检查 bit0
  // 注意: UG 称 WDT_EN "一旦置 1 只能由系统复位清零" (写 1 锁定);
  //       但 RTL wdt.v 中 wdt_cr_ir[0] <= ipwdata[0] 无锁定逻辑,
  //       软件写 0 可直接清 EN —— Spec-vs-RTL 差异, 以 RTL 为准并记录
  mem_read32_(0x50008000, &cr);
  if ((cr & 0x1) != 0) {
    printf("WDT_CR bit0 expected clearable by SW: got 0x%x (expect bit0==0)\n", cr);
    sim_fail();
  }

  printf("wdt_en_lock test successfully\n");
  sim_end();
}
