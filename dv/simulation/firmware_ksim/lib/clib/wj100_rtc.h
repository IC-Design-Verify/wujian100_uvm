/*
 * wujian100_open Real-Time Clock (RTC) Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * RTC has 32-bit incrementing counter + comparator for interrupt generation.
 * Base address: 0x60004000 (APB1, P6), 16KB window.
 */
#ifndef __WJ100_RTC_H__
#define __WJ100_RTC_H__

#include "wj100_types.h"

#define RTC_BASE_ADDR          0x60004000U    /* APB1, P6 */

/* Register offsets */
#define RTC_CURRENT_VALUE_OFFSET   0x00U   /* RO , 32-bit, current counter value, reset 0x0 */
#define RTC_MATCH_VALUE_OFFSET     0x04U   /* R/W, 32-bit, counter match register */
#define RTC_LOAD_VALUE_OFFSET      0x08U   /* R/W, 32-bit, counter load register */
#define RTC_CCR_OFFSET             0x0CU   /* R/W, 4-bit , counter control register */
#define RTC_INT_STATUS_OFFSET      0x10U   /* RO , 32-bit, interrupt status (masked) */
#define RTC_RAW_INT_STATUS_OFFSET  0x14U   /* RO , 32-bit, raw interrupt status */
#define RTC_INT_CLR_OFFSET         0x18U   /* RO , 32-bit, end-of-interrupt (read-to-clear) */
#define RTC_COMP_VERSION_OFFSET    0x1CU   /* RO , 32-bit, component version */
#define RTC_DIV_OFFSET             0x20U   /* R/W, 20-bit, RTC clock divider, reset 0x4000 */

/* RTC_CCR bit definitions */
#define RTC_CCR_IEN                (1U << 0)   /* [0] 0=int disabled, 1=int enabled. reset=0 */
#define RTC_CCR_MASK               (1U << 1)   /* [1] 0=unmasked, 1=masked. reset=0 */
#define RTC_CCR_EN                 (1U << 2)   /* [2] 0=counter disabled, 1=enabled. reset=0 */
#define RTC_CCR_WEN                (1U << 3)   /* [3] 0=wrap disabled, 1=wrap on match. reset=0 */
#define RTC_CCR_RESET_VAL          0x0U

/* Convenience combinations */
#define RTC_CCR_DISABLE            0x0U
#define RTC_CCR_ENABLE_NO_INT      RTC_CCR_EN
#define RTC_CCR_ENABLE_WITH_INT    (RTC_CCR_EN | RTC_CCR_IEN)
#define RTC_CCR_ENABLE_WRAP_INT    (RTC_CCR_EN | RTC_CCR_IEN | RTC_CCR_WEN)

/* RTC_INT_STATUS / RTC_RAW_INT_STATUS bit definitions */
#define RTC_INT_STAT_ACTIVE        (1U << 0)   /* [0] 1=interrupt active */

/* RTC_DIV */
#define RTC_DIV_MASK               0x000FFFFFU  /* [19:0] divider value, reset 0x4000 */
#define RTC_DIV_RESET_VAL          0x4000U

/* Register accessors */
#define RTC_CURRENT_VALUE(base)   (*(volatile uint32_t *)((base) + RTC_CURRENT_VALUE_OFFSET))
#define RTC_MATCH_VALUE(base)     (*(volatile uint32_t *)((base) + RTC_MATCH_VALUE_OFFSET))
#define RTC_LOAD_VALUE(base)      (*(volatile uint32_t *)((base) + RTC_LOAD_VALUE_OFFSET))
#define RTC_CCR(base)             (*(volatile uint32_t *)((base) + RTC_CCR_OFFSET))
#define RTC_INT_STATUS(base)      (*(volatile uint32_t *)((base) + RTC_INT_STATUS_OFFSET))
#define RTC_RAW_INT_STATUS(base)  (*(volatile uint32_t *)((base) + RTC_RAW_INT_STATUS_OFFSET))
#define RTC_INT_CLR(base)         (*(volatile uint32_t *)((base) + RTC_INT_CLR_OFFSET))
#define RTC_COMP_VERSION(base)    (*(volatile uint32_t *)((base) + RTC_COMP_VERSION_OFFSET))
#define RTC_DIV(base)             (*(volatile uint32_t *)((base) + RTC_DIV_OFFSET))

#endif /* __WJ100_RTC_H__ */
