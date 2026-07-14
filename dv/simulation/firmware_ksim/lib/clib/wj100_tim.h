/*
 * wujian100_open Timer (TIM) Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * Each TIMER IP provides two independent timers (Timer1/Timer2).
 * Base addresses (from Peripheral Address Map):
 *   APB0: TIM0=0x50000000, TIM2=0x50000400, TIM4=0x50000800, TIM6=0x50000C00
 *   APB1: TIM1=0x60000000, TIM3=0x60000400, TIM5=0x60000800, TIM7=0x60000C00
 */
#ifndef __WJ100_TIM_H__
#define __WJ100_TIM_H__

#include "wj100_types.h"

/* Base addresses for all 8 timer instances */
#define TIM0_BASE_ADDR      0x50000000U   /* APB0, P0 */
#define TIM1_BASE_ADDR      0x60000000U   /* APB1, P0 */
#define TIM2_BASE_ADDR      0x50000400U   /* APB0, P1 */
#define TIM3_BASE_ADDR      0x60000400U   /* APB1, P1 */
#define TIM4_BASE_ADDR      0x50000800U   /* APB0, P2 */
#define TIM5_BASE_ADDR      0x60000800U   /* APB1, P2 */
#define TIM6_BASE_ADDR      0x50000C00U   /* APB0, P3 */
#define TIM7_BASE_ADDR      0x60000C00U   /* APB1, P3 */

/* Register offset within a timer IP (each IP contains Timer1 + Timer2) */
#define TIM_LOAD_COUNT_OFFSET        0x00U   /* R/W, 32-bit, Timer1 load value */
#define TIM_CURRENT_VALUE_OFFSET     0x04U   /* R  , 32-bit, Timer1 current value */
#define TIM_CONTROL_REG_OFFSET       0x08U   /* R/W, 4-bit , Timer1 control */
#define TIM_INT_CLR_OFFSET           0x0CU   /* R  , 1-bit , Timer1 interrupt clear (read-to-clear) */
#define TIM_INT_STATUS_OFFSET        0x10U   /* R  , 1-bit , Timer1 interrupt status */

/* Timer2 register offsets (mirror of Timer1, +0x14) */
#define TIM2_LOAD_COUNT_OFFSET       0x14U
#define TIM2_CURRENT_VALUE_OFFSET    0x18U
#define TIM2_CONTROL_REG_OFFSET      0x1CU
#define TIM2_INT_CLR_OFFSET          0x20U
#define TIM2_INT_STATUS_OFFSET       0x24U

/* Timer Control Register (TIM_CONTROL_REG) bit definitions */
#define TIM_CTRL_ENABLE              (1U << 0)   /* 0=disabled, 1=enabled */
#define TIM_CTRL_MODE_USER          (1U << 1)   /* 0=free-running, 1=user-defined count */
#define TIM_CTRL_INT_MASK           (1U << 2)   /* 0=not masked, 1=masked */
#define TIM_CTRL_HW_TRIG_EN         (1U << 4)   /* 0=disable, 1=enable hardware trigger */

#define TIM_CTRL_DISABLE_VAL        0x0U
#define TIM_CTRL_ENABLE_FREE_RUN     (TIM_CTRL_ENABLE)
#define TIM_CTRL_ENABLE_USER_DEF     (TIM_CTRL_ENABLE | TIM_CTRL_MODE_USER)

/* Convenience register accessors */
#define TIM_LOAD_COUNT(base)         (*(volatile uint32_t *)((base) + TIM_LOAD_COUNT_OFFSET))
#define TIM_CURRENT_VALUE(base)       (*(volatile uint32_t *)((base) + TIM_CURRENT_VALUE_OFFSET))
#define TIM_CONTROL_REG(base)         (*(volatile uint32_t *)((base) + TIM_CONTROL_REG_OFFSET))
#define TIM_INT_CLR(base)             (*(volatile uint32_t *)((base) + TIM_INT_CLR_OFFSET))
#define TIM_INT_STATUS(base)          (*(volatile uint32_t *)((base) + TIM_INT_STATUS_OFFSET))

#define TIM2_LOAD_COUNT(base)         (*(volatile uint32_t *)((base) + TIM2_LOAD_COUNT_OFFSET))
#define TIM2_CURRENT_VALUE(base)       (*(volatile uint32_t *)((base) + TIM2_CURRENT_VALUE_OFFSET))
#define TIM2_CONTROL_REG(base)         (*(volatile uint32_t *)((base) + TIM2_CONTROL_REG_OFFSET))
#define TIM2_INT_CLR(base)             (*(volatile uint32_t *)((base) + TIM2_INT_CLR_OFFSET))
#define TIM2_INT_STATUS(base)          (*(volatile uint32_t *)((base) + TIM2_INT_STATUS_OFFSET))

#endif /* __WJ100_TIM_H__ */
