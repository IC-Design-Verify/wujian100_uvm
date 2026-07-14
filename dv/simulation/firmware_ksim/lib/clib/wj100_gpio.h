/*
 * wujian100_open General-Purpose I/O (GPIO) Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * 32-bit GPIO, each bit supports interrupt generation.
 * Base address: 0x60018000 (APB1, P5), 16KB window.
 */
#ifndef __WJ100_GPIO_H__
#define __WJ100_GPIO_H__

#include "wj100_types.h"

#define GPIO_BASE_ADDR          0x60018000U    /* APB1, P5 */

/* Register offsets */
#define GPIO_OUTPUT_DATA_OFFSET     0x00U   /* R/W, 32-bit, output data register */
#define GPIO_DIRECTION_OFFSET       0x04U   /* R/W, 32-bit, direction (0=in, 1=out) */
#define GPIO_CTL_OFFSET             0x08U   /* R/W, 32-bit, source (0=SW, 1=HW) */
/* 0x0C..0x2C reserved */
#define GPIO_INTEN_OFFSET           0x30U   /* R/W, 32-bit, interrupt enable */
#define GPIO_INTMASK_OFFSET         0x34U   /* R/W, 32-bit, interrupt mask (1=mask) */
#define GPIO_INTTYPE_LEVEL_OFFSET   0x38U   /* R/W, 32-bit, 0=level, 1=edge */
#define GPIO_INT_POLARITY_OFFSET    0x3CU   /* R/W, 32-bit, 0=active-low/falling, 1=active-high/rising */
#define GPIO_INTSTATUS_OFFSET       0x40U   /* RO , 32-bit, masked interrupt status */
#define GPIO_RAW_INTSTATUS_OFFSET   0x44U   /* RO , 32-bit, raw interrupt status (pre-masking) */
#define GPIO_INT_CLR_OFFSET         0x4CU   /* W  , 32-bit, clear edge interrupts (write-1-to-clear) */
#define GPIO_INPUT_DATA_OFFSET      0x50U   /* RO , 32-bit, input data */

/* Direction bit definitions */
#define GPIO_DIR_INPUT              0x0U
#define GPIO_DIR_OUTPUT             0x1U
#define GPIO_DIR_ALL_INPUT          0x00000000U
#define GPIO_DIR_ALL_OUTPUT         0xFFFFFFFFU

/* CTL bit definitions */
#define GPIO_CTL_SW_MODE            0x0U   /* software mode */
#define GPIO_CTL_HW_MODE            0x1U   /* hardware mode */

/* Interrupt type/level bit definitions */
#define GPIO_INTTYPE_LEVEL          0x0U   /* level-sensitive */
#define GPIO_INTTYPE_EDGE           0x1U   /* edge-sensitive */

/* Interrupt polarity definitions */
#define GPIO_INT_POL_ACTIVE_LOW     0x0U   /* active-low / falling-edge */
#define GPIO_INT_POL_ACTIVE_HIGH    0x1U   /* active-high / rising-edge */

/* INTEN bit definitions */
#define GPIO_INTEN_NORMAL           0x0U   /* normal GPIO signal */
#define GPIO_INTEN_INTERRUPT        0x1U   /* configured as interrupt */

/* INTMASK bit definitions */
#define GPIO_INT_UNMASKED            0x0U   /* (default) interrupt bits unmasked */
#define GPIO_INT_MASKED             0x1U   /* interrupt masked */

/* INT_CLR bit definitions */
#define GPIO_INT_CLR_NONE            0x0U
#define GPIO_INT_CLR_BIT(n)          (1U << (n))   /* write 1 to bit n to clear its interrupt */

/* Register accessors (suffixed _REG to avoid collision with field-value defines) */
#define GPIO_OUTPUT_DATA_REG(base)    (*(volatile uint32_t *)((base) + GPIO_OUTPUT_DATA_OFFSET))
#define GPIO_DIRECTION_REG(base)      (*(volatile uint32_t *)((base) + GPIO_DIRECTION_OFFSET))
#define GPIO_CTL_REG(base)            (*(volatile uint32_t *)((base) + GPIO_CTL_OFFSET))
#define GPIO_INTEN_REG(base)          (*(volatile uint32_t *)((base) + GPIO_INTEN_OFFSET))
#define GPIO_INTMASK_REG(base)        (*(volatile uint32_t *)((base) + GPIO_INTMASK_OFFSET))
#define GPIO_INTTYPE_LEVEL_REG(base)  (*(volatile uint32_t *)((base) + GPIO_INTTYPE_LEVEL_OFFSET))
#define GPIO_INT_POLARITY_REG(base)   (*(volatile uint32_t *)((base) + GPIO_INT_POLARITY_OFFSET))
#define GPIO_INTSTATUS_REG(base)      (*(volatile uint32_t *)((base) + GPIO_INTSTATUS_OFFSET))
#define GPIO_RAW_INTSTATUS_REG(base)  (*(volatile uint32_t *)((base) + GPIO_RAW_INTSTATUS_OFFSET))
#define GPIO_INT_CLR_REG(base)        (*(volatile uint32_t *)((base) + GPIO_INT_CLR_OFFSET))
#define GPIO_INPUT_DATA_REG(base)     (*(volatile uint32_t *)((base) + GPIO_INPUT_DATA_OFFSET))

#endif /* __WJ100_GPIO_H__ */
