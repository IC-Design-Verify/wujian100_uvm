/*
 * wujian100_open Watchdog (WDT) Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * WDT counts down from a预设 value to zero, triggers timeout.
 * Base address: 0x50008000 (APB0, P7), 16KB window.
 */
#ifndef __WJ100_WDT_H__
#define __WJ100_WDT_H__

#include "wj100_types.h"

#define WDT_BASE_ADDR          0x50008000U    /* APB0, P7 */

/* Register offsets */
#define WDT_CR_OFFSET          0x00U   /* R/W, 5-bit , WDT control register, reset 5'h2 */
#define WDT_TIME_OUT_OFFSET    0x04U   /* R/W, 8-bit , timeout range register */
#define WDT_CURRENT_VAL_OFFSET 0x08U   /* R  , 32-bit, current counter value */
#define WDT_CRR_OFFSET         0x0CU   /* W  , 8-bit , counter restart (kick) */
#define WDT_INT_STATUS_OFFSET  0x10U   /* R  , 1-bit , interrupt status */
#define WDT_INT_CLR_OFFSET     0x14U   /* R  , 1-bit , interrupt clear (read-to-clear) */

/* WDT_CR (Control Register) bit definitions */
#define WDT_CR_WDT_EN         (1U << 0)   /* [0]  WDT enable. Once set, only system reset clears. reset=0 */
#define WDT_CR_RMOD           (1U << 1)   /* [1]  0=reset, 1=interrupt first then reset on second timeout. reset=1 */
#define WDT_CR_RPL_POS        2U          /* [4:2] reset pulse length: 000..111 = 2..256 pclk cycles. reset=0 */
#define WDT_CR_RPL_MASK       (0x7U << WDT_CR_RPL_POS)
#define WDT_CR_RESET_VAL      0x2U

/* WDT_CR_RPL encoding */
#define WDT_RPL_2_CYCLES      0x0U
#define WDT_RPL_4_CYCLES      0x1U
#define WDT_RPL_8_CYCLES      0x2U
#define WDT_RPL_16_CYCLES     0x3U
#define WDT_RPL_32_CYCLES     0x4U
#define WDT_RPL_64_CYCLES     0x5U
#define WDT_RPL_128_CYCLES    0x6U
#define WDT_RPL_256_CYCLES    0x7U

/* WDT_TIME_OUT (Timeout Range Register) */
#define WDT_TOP_INIT_POS      4U          /* [7:4] timeout period for first kick */
#define WDT_TOP_INIT_MASK     (0xFU << WDT_TOP_INIT_POS)
#define WDT_TOP_POS           0U          /* [3:0] timeout period */
#define WDT_TOP_MASK          (0xFU << WDT_TOP_POS)
/* For i=0..15 -> t=0xFFFFFFFF (timeout period). See spec table. */

/* WDT_CURRENT_VAL */
#define WDT_CURRENT_VAL_MASK  0xFFFFFFFFU   /* [31:0] current counter value, reset 0xffff */

/* WDT_CRR — Counter Restart Register.
 * Restart requires writing 0x76. Reading returns 0.  */
#define WDT_KICK_KEY          0x76U
#define WDT_CRR_KEY_VAL       0x76U

/* WDT_INT_STATUS / WDT_INT_CLR bit definitions */
#define WDT_INT_ACTIVE        (1U << 0)   /* [0] =1 interrupt active */

/* Register accessors */
#define WDT_CR(base)              (*(volatile uint32_t *)((base) + WDT_CR_OFFSET))
#define WDT_TIME_OUT(base)        (*(volatile uint32_t *)((base) + WDT_TIME_OUT_OFFSET))
#define WDT_CURRENT_VAL(base)     (*(volatile uint32_t *)((base) + WDT_CURRENT_VAL_OFFSET))
#define WDT_CRR(base)             (*(volatile uint32_t *)((base) + WDT_CRR_OFFSET))
#define WDT_INT_STATUS(base)      (*(volatile uint32_t *)((base) + WDT_INT_STATUS_OFFSET))
#define WDT_INT_CLR(base)         (*(volatile uint32_t *)((base) + WDT_INT_CLR_OFFSET))

#endif /* __WJ100_WDT_H__ */
