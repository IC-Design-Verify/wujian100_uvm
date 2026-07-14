/*
 * wujian100_open Unified Serial Interface (USI) Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * USI supports three modes: UART, I2C, SPI. Mode is selected via MODE_SEL.
 * Base addresses:
 *   USI0 = 0x50028000 (APB0, P4)
 *   USI1 = 0x60028000 (APB1, P4)
 *   USI2 = 0x50029000 (APB0, P5)
 */
#ifndef __WJ100_USI_H__
#define __WJ100_USI_H__

#include "wj100_types.h"

#define USI0_BASE_ADDR      0x50028000U   /* APB0, P4 */
#define USI1_BASE_ADDR      0x60028000U   /* APB1, P4 */
#define USI2_BASE_ADDR      0x50029000U   /* APB0, P5 */

/* USI register offsets */
#define USI_CTRL_OFFSET           0x00U   /* R/W, USI control */
#define USI_MODE_SEL_OFFSET       0x04U   /* R/W, mode select (UART/I2C/SPI) */
#define USI_TX_FIFO_OFFSET        0x08U   /* W  , TX FIFO write port */
#define USI_RX_FIFO_OFFSET        0x0CU   /* R  , RX FIFO read port */
#define USI_FIFO_STA_OFFSET       0x10U   /* R  , FIFO status */
#define USI_CLK_DIV0_OFFSET       0x14U   /* R/W, clock divider 0 (UART baud / I2C/SPI clock) */
#define USI_CLK_DIV1_OFFSET       0x18U   /* R/W, clock divider 1 */

/* UART-specific registers */
#define USI_UART_CTRL_OFFSET      0x1CU   /* R/W, UART control */
#define USI_UART_STA_OFFSET       0x20U   /* R  , UART status */

/* I2C-specific registers */
#define USI_I2C_MODE_OFFSET       0x24U   /* R/W, I2C mode */
#define USI_I2C_ADDR_OFFSET       0x28U   /* R/W, I2C slave address */
#define USI_I2CM_CTRL_OFFSET      0x2CU   /* R/W, I2C master control */
#define USI_I2CM_CODE_OFFSET      0x30U   /* R/W, I2C master code/state */
#define USI_I2CS_CTRL_OFFSET      0x34U   /* R/W, I2C slave control */
#define USI_I2C_FM_DIV_OFFSET     0x38U   /* R/W, I2C fast mode divider */
#define USI_I2C_HOLD_OFFSET       0x3CU   /* R/W, I2C hold time */
#define USI_I2C_STA_OFFSET        0x40U   /* R  , I2C status */

/* SPI-specific registers */
#define USI_SPI_MODE_OFFSET       0x44U   /* R/W, SPI mode */
#define USI_SPI_CTRL_OFFSET       0x48U   /* R/W, SPI control */
#define USI_SPI_STA_OFFSET        0x4CU   /* R  , SPI status */

/* Shared interrupt / DMA registers */
#define USI_INTR_CTRL_OFFSET       0x50U   /* R/W, interrupt control */
#define USI_INTR_EN_OFFSET         0x54U   /* R/W, interrupt enable */
#define USI_INTR_STA_OFFSET        0x58U   /* R  , interrupt status (masked) */
#define USI_RAW_INTR_STA_OFFSET    0x5CU   /* R  , raw interrupt status */
#define USI_INTR_UNMASK_OFFSET     0x60U   /* R  , unmasked interrupt status */
#define USI_INTR_CLR_OFFSET        0x64U   /* W  , interrupt clear */
#define USI_DMA_CTRL_OFFSET        0x68U   /* R/W, DMA control */
#define USI_DMA_THRESHOLD_OFFSET   0x6CU   /* R/W, DMA threshold */
#define USI_SPI_NSS_DATA_OFFSET    0x70U   /* R/W, SPI NSS data */

/* USI_CTRL bit definitions */
#define USI_CTRL_EN                 (1U << 0)    /* USI enable */
#define USI_CTRL_FIFO_RESET         (1U << 1)   /* reset all FIFOs */

/* MODE_SEL encoding */
#define USI_MODE_UART               0x0U
#define USI_MODE_I2C                0x1U
#define USI_MODE_SPI                0x2U

/* UART_CTRL bit definitions */
#define USI_UART_CTRL_EN            (1U << 0)   /* UART enable */
#define USI_UART_DATA_BITS_POS      1U          /* [2:1] data bits 5/6/7/8 */
#define USI_UART_STOP_BITS_POS      3U          /* [4:3] stop bits 1/1.5/2 */
#define USI_UART_PARITY_EN          (1U << 5)   /* parity enable */
#define USI_UART_PARITY_EVEN        (1U << 6)   /* 0=odd, 1=even */

/* FIFO_STA bit definitions */
#define USI_TX_FIFO_EMPTY_MASK      (1U << 0)
#define USI_TX_FIFO_FULL_MASK       (1U << 1)
#define USI_RX_FIFO_EMPTY_MASK      (1U << 2)
#define USI_RX_FIFO_FULL_MASK       (1U << 3)
#define USI_TX_FIFO_LEVEL_POS        8U          /* TX FIFO fill level */
#define USI_RX_FIFO_LEVEL_POS        16U         /* RX FIFO fill level */

/* Interrupt status encoding (INTR_STA, RAW_INTR_STA) */
#define USI_INT_RX_FIFO_PEAK        (1U << 0)   /* RX FIFO over trigger */
#define USI_INT_TX_FIFO_EMPTY      (1U << 1)   /* TX FIFO empty */
#define USI_INT_RX_LINE_ERR         (1U << 2)   /* RX line error */
#define USI_INT_RX_TIMEOUT          (1U << 3)   /* RX timeout */

/* Convenience accessors */
#define USI_REG(base, off)  (*(volatile uint32_t *)((base) + (off)))
#define USI_CTRL(base)         USI_REG(base, USI_CTRL_OFFSET)
#define USI_MODE_SEL(base)     USI_REG(base, USI_MODE_SEL_OFFSET)
#define USI_TX_FIFO(base)      USI_REG(base, USI_TX_FIFO_OFFSET)
#define USI_RX_FIFO(base)      USI_REG(base, USI_RX_FIFO_OFFSET)
#define USI_FIFO_STA(base)     USI_REG(base, USI_FIFO_STA_OFFSET)
#define USI_CLK_DIV0(base)     USI_REG(base, USI_CLK_DIV0_OFFSET)
#define USI_CLK_DIV1(base)     USI_REG(base, USI_CLK_DIV1_OFFSET)
#define USI_UART_CTRL(base)    USI_REG(base, USI_UART_CTRL_OFFSET)
#define USI_UART_STA(base)     USI_REG(base, USI_UART_STA_OFFSET)
#define USI_I2C_MODE(base)     USI_REG(base, USI_I2C_MODE_OFFSET)
#define USI_I2C_ADDR(base)     USI_REG(base, USI_I2C_ADDR_OFFSET)
#define USI_I2CM_CTRL(base)    USI_REG(base, USI_I2CM_CTRL_OFFSET)
#define USI_I2CM_CODE(base)    USI_REG(base, USI_I2CM_CODE_OFFSET)
#define USI_I2CS_CTRL(base)    USI_REG(base, USI_I2CS_CTRL_OFFSET)
#define USI_I2C_FM_DIV(base)   USI_REG(base, USI_I2C_FM_DIV_OFFSET)
#define USI_I2C_HOLD(base)     USI_REG(base, USI_I2C_HOLD_OFFSET)
#define USI_I2C_STA(base)      USI_REG(base, USI_I2C_STA_OFFSET)
#define USI_SPI_MODE(base)     USI_REG(base, USI_SPI_MODE_OFFSET)
#define USI_SPI_CTRL(base)     USI_REG(base, USI_SPI_CTRL_OFFSET)
#define USI_SPI_STA(base)      USI_REG(base, USI_SPI_STA_OFFSET)
#define USI_INTR_CTRL(base)    USI_REG(base, USI_INTR_CTRL_OFFSET)
#define USI_INTR_EN(base)      USI_REG(base, USI_INTR_EN_OFFSET)
#define USI_INTR_STA(base)     USI_REG(base, USI_INTR_STA_OFFSET)
#define USI_RAW_INTR_STA(base) USI_REG(base, USI_RAW_INTR_STA_OFFSET)
#define USI_INTR_CLR(base)     USI_REG(base, USI_INTR_CLR_OFFSET)
#define USI_DMA_CTRL(base)     USI_REG(base, USI_DMA_CTRL_OFFSET)
#define USI_DMA_THRESHOLD(base) USI_REG(base, USI_DMA_THRESHOLD_OFFSET)
#define USI_SPI_NSS_DATA(base) USI_REG(base, USI_SPI_NSS_DATA_OFFSET)

#endif /* __WJ100_USI_H__ */
