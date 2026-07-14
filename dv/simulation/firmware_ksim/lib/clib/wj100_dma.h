/*
 * wujian100_open DMA Controller Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * DMAC is AMBA2.0 AHB-lite compliant, supports up to 16 channels.
 * Each channel supports block trigger mode only.
 * Base address: 0x40000000 (16KB), referenced by existing dma.h.
 */
#ifndef __WJ100_DMA_H__
#define __WJ100_DMA_H__

#include "wj100_types.h"

/* DMA base + per-channel base address table.
 * 17 base addresses: 16 channel blocks (0x30 stride) + 1 global block.
 * Each channel has 9 32-bit registers (SAR/DAR/CTRL_A/CTRL_B/INT_MASK/
 * INT_STATUS/INT_CLEAR/SOFT_REQ/CH_EN), spanning 0x24 bytes (0x30 stride leaves 0xC unused).
 */
#define DMA_BASE_ADDR                 0x40000000U

#define DMA_CH0_BASE_ADDR             (DMA_BASE_ADDR + 0x0)
#define DMA_CH1_BASE_ADDR             (DMA_BASE_ADDR + 0x30)
#define DMA_CH2_BASE_ADDR             (DMA_BASE_ADDR + 0x60)
#define DMA_CH3_BASE_ADDR             (DMA_BASE_ADDR + 0x90)
#define DMA_CH4_BASE_ADDR             (DMA_BASE_ADDR + 0xC0)
#define DMA_CH5_BASE_ADDR             (DMA_BASE_ADDR + 0xF0)
#define DMA_CH6_BASE_ADDR             (DMA_BASE_ADDR + 0x120)
#define DMA_CH7_BASE_ADDR             (DMA_BASE_ADDR + 0x150)
#define DMA_CH8_BASE_ADDR             (DMA_BASE_ADDR + 0x180)
#define DMA_CH9_BASE_ADDR             (DMA_BASE_ADDR + 0x1B0)
#define DMA_CH10_BASE_ADDR            (DMA_BASE_ADDR + 0x1E0)
#define DMA_CH11_BASE_ADDR            (DMA_BASE_ADDR + 0x210)
#define DMA_CH12_BASE_ADDR            (DMA_BASE_ADDR + 0x240)
#define DMA_CH13_BASE_ADDR            (DMA_BASE_ADDR + 0x270)
#define DMA_CH14_BASE_ADDR            (DMA_BASE_ADDR + 0x2A0)
#define DMA_CH15_BASE_ADDR            (DMA_BASE_ADDR + 0x2D0)
#define DMA_GLOBAL_BASE_ADDR          (DMA_BASE_ADDR + 0x330)

/* Per-channel register offsets */
#define DMA_CH_SAR_OFFSET             0x0U
#define DMA_CH_DAR_OFFSET             0x4U
#define DMA_CH_CTRL_A_OFFSET           0x8U
#define DMA_CH_CTRL_B_OFFSET           0xCU
#define DMA_CH_INT_MASK_OFFSET         0x10U
#define DMA_CH_INT_STATUS_OFFSET       0x14U
#define DMA_CH_INT_CLEAR_OFFSET        0x18U
#define DMA_CH_SOFT_REQ_OFFSET         0x1CU
#define DMA_CH_EN_OFFSET               0x20U

/* Global register offsets (relative to DMA_GLOBAL_BASE_ADDR) */
#define DMA_CHSR_OFFSET                0x8U
#define DMA_DMACCFG_OFFSET              0xCU

/* DMA Channel n register field definitions */

/* CHn_CTRL_A (offset 0x8) — transfer size + address mode + width */
#define DMA_CH_CTRL_A_BLK_SIZE_MASK    0x00000FFFU   /* [11:0]   block size (max 4096 bytes) */
#define DMA_CH_CTRL_A_BLK_SIZE_POS     0U
#define DMA_CH_CTRL_A_SRC_WIDTH_MASK  0x00003000U   /* [13:12] src transfer width */
#define DMA_CH_CTRL_A_SRC_WIDTH_POS    12U
#define DMA_CH_CTRL_A_DST_WIDTH_MASK  0x0000C000U   /* [15:14] dst transfer width */
#define DMA_CH_CTRL_A_DST_WIDTH_POS    14U
#define DMA_CH_CTRL_A_SRC_INC_MASK    0x00030000U   /* [17:16] src addr mode */
#define DMA_CH_CTRL_A_SRC_INC_POS     16U
#define DMA_CH_CTRL_A_DST_INC_MASK    0x000C0000U   /* [19:18] dst addr mode */
#define DMA_CH_CTRL_A_DST_INC_POS     18U
#define DMA_CH_CTRL_A_SRC_LJ_MASK     0x00100000U   /* [20]    src little/big endian */
#define DMA_CH_CTRL_A_DST_LJ_MASK     0x00200000U   /* [21]    dst little/big endian */

/* Transfer width encoding */
#define DMA_WIDTH_8_BIT               0x0U
#define DMA_WIDTH_16_BIT              0x1U
#define DMA_WIDTH_32_BIT              0x2U

/* Address mode encoding */
#define DMA_ADDR_INCR                 0x0U
#define DMA_ADDR_DECR                 0x1U
#define DMA_ADDR_NO_CHANGE            0x2U

/* Convenience helpers for CHn_CTRL_A */
#define DMA_CTRL_A_SIZE(n)            (((n) << DMA_CH_CTRL_A_BLK_SIZE_POS) & DMA_CH_CTRL_A_BLK_SIZE_MASK)
#define DMA_CTRL_A_SRC_WIDTH(w)      (((w) << DMA_CH_CTRL_A_SRC_WIDTH_POS) & DMA_CH_CTRL_A_SRC_WIDTH_MASK)
#define DMA_CTRL_A_DST_WIDTH(w)      (((w) << DMA_CH_CTRL_A_DST_WIDTH_POS) & DMA_CH_CTRL_A_DST_WIDTH_MASK)
#define DMA_CTRL_A_SRC_INC(m)        (((m) << DMA_CH_CTRL_A_SRC_INC_POS)  & DMA_CH_CTRL_A_SRC_INC_MASK)
#define DMA_CTRL_A_DST_INC(m)        (((m) << DMA_CH_CTRL_A_DST_INC_POS)  & DMA_CH_CTRL_A_DST_INC_MASK)

/* CHn_CTRL_B (offset 0xC) */
#define DMA_CH_CTRL_B_BLK_TRIG_MASK  0x00000001U   /* [0]    block trigger mode */
#define DMA_CH_CTRL_B_TFR_INT_EN     0x00000002U   /* [1]    block transfer completion interrupt */
#define DMA_CH_CTRL_B_HALF_INT_EN    0x00000004U   /* [2]    half block transfer interrupt */
#define DMA_CH_CTRL_B_ERR_INT_EN     0x00000008U   /* [3]    error interrupt */

/* CHn_INT_MASK (offset 0x10) */
#define DMA_INT_TFR_MASK             0x00000001U   /* [0]    block transfer completion */
#define DMA_INT_HALF_TFR_MASK        0x00000002U   /* [1]    half block transfer */
#define DMA_INT_ERR_MASK              0x00000004U   /* [2]    error */
#define DMA_INT_ALL_MASK              0x00000007U   /* mask all interrupts */

/* CHn_INT_STATUS (offset 0x14) */
#define DMA_INT_STATUS_TFR            0x00000001U   /* [0]    block transfer completed */
#define DMA_INT_STATUS_HALF_TFR       0x00000002U   /* [1]    half block transfer completed */
#define DMA_INT_STATUS_ERR            0x00000004U   /* [2]    error occurred */
#define DMA_STATUS_TRANSFER_DONE      (DMA_INT_STATUS_TFR)   /* legacy */

/* CHn_INT_CLEAR (offset 0x18) — write-1-to-clear interrupt flags */
#define DMA_INT_CLR_TFR               0x00000001U
#define DMA_INT_CLR_HALF_TFR          0x00000002U
#define DMA_INT_CLR_ERR               0x00000004U

/* CHn_SOFT_REQ (offset 0x1C) — software handshaking trigger */
#define DMA_SOFT_REQ_MASK             0x00000001U

/* CHn_EN (offset 0x20) — channel enable */
#define DMA_CH_EN_MASK                0x00000001U
#define DMA_CH_ENABLE                 0x1U
#define DMA_CH_DISABLE                0x0U

/* Global registers (offset within DMA_GLOBAL_BASE_ADDR) */
#define DMA_CHSR_BUSY_CH0_MASK        0x00000001U   /* Channel busy status, bit n = channel n busy */
#define DMA_DMACCFG_DMACEN_MASK       0x00000001U   /* DMAC enable */
#define DMA_DMACCFG_ENABLE            0x1U
#define DMA_DMACCFG_DISABLE           0x0U

/* Register accessors for a channel base */
#define DMA_CH_SAR(base)         (*(volatile uint32_t *)((base) + DMA_CH_SAR_OFFSET))
#define DMA_CH_DAR(base)         (*(volatile uint32_t *)((base) + DMA_CH_DAR_OFFSET))
#define DMA_CH_CTRL_A(base)     (*(volatile uint32_t *)((base) + DMA_CH_CTRL_A_OFFSET))
#define DMA_CH_CTRL_B(base)     (*(volatile uint32_t *)((base) + DMA_CH_CTRL_B_OFFSET))
#define DMA_CH_INT_MASK(base)   (*(volatile uint32_t *)((base) + DMA_CH_INT_MASK_OFFSET))
#define DMA_CH_INT_STATUS(base) (*(volatile uint32_t *)((base) + DMA_CH_INT_STATUS_OFFSET))
#define DMA_CH_INT_CLEAR(base)  (*(volatile uint32_t *)((base) + DMA_CH_INT_CLEAR_OFFSET))
#define DMA_CH_SOFT_REQ(base)   (*(volatile uint32_t *)((base) + DMA_CH_SOFT_REQ_OFFSET))
#define DMA_CH_EN(base)         (*(volatile uint32_t *)((base) + DMA_CH_EN_OFFSET))

#define DMA_CHSR                (*(volatile uint32_t *)(DMA_GLOBAL_BASE_ADDR + DMA_CHSR_OFFSET))
#define DMA_DMACCFG             (*(volatile uint32_t *)(DMA_GLOBAL_BASE_ADDR + DMA_DMACCFG_OFFSET))

#endif /* __WJ100_DMA_H__ */
