/*
 * wujian100_open Pulse Width Modulation (PWM) Register Definitions
 * Generated from wujian100_open Userguide v1.0
 *
 * PWM has 6 generators (groups 0..5), each with 1 32-bit counter,
 * 2 PWM comparators, 1 PWM signal generator, 1 interrupt generator.
 * 12 input/output channels total.
 * Base address: 0x5001C000 (APB0, P12), 16KB window.
 */
#ifndef __WJ100_PWM_H__
#define __WJ100_PWM_H__

#include "wj100_types.h"

#define PWM_BASE_ADDR          0x5001C000U    /* APB0, P12 */

/* Register offsets */
#define PWM_PWMCFG_OFFSET         0x00U   /* R/W, PWM configuration */
#define PWM_PWMINVERTTRIG_OFFSET  0x04U   /* R/W, output inversion + ADC trigger */
#define PWM_PWM01TRIG_OFFSET      0x08U   /* R/W, group 0/1 trigger compare value */
#define PWM_PWM23TRIG_OFFSET      0x0CU   /* R/W, group 2/3 trigger compare value */
#define PWM_PWM45TRIG_OFFSET      0x10U   /* R/W, group 4/5 trigger compare value */
#define PWM_PWMINTEN1_OFFSET      0x14U   /* R/W, interrupt enable  group 2/1/0 */
#define PWM_PWMINTEN2_OFFSET      0x18U   /* R/W, interrupt enable  group 5/4/3 */
#define PWM_PWMRIS1_OFFSET        0x1CU   /* RO , raw interrupt status group 2/1/0 */
#define PWM_PWMRIS2_OFFSET        0x20U   /* RO , raw interrupt status group 5/4/3 */
#define PWM_PWMIC1_OFFSET         0x24U   /* R/W, interrupt clear group 2/1/0 */
#define PWM_PWMIC2_OFFSET         0x28U   /* R/W, interrupt clear group 5/4/3 */
#define PWM_PWMIS1_OFFSET         0x2CU   /* RO , interrupt status group 2/1/0 */
#define PWM_PWMIS2_OFFSET         0x30U   /* RO , interrupt status group 5/4/3 */
#define PWM_PWMCTL_OFFSET         0x34U   /* R/W, PWM control (counter mode) */

/* PWM generator load/count/cmp registers */
#define PWM_PWM01LOAD_OFFSET      0x38U
#define PWM_PWM23LOAD_OFFSET      0x3CU
#define PWM_PWM45LOAD_OFFSET      0x40U
#define PWM_PWM01COUNT_OFFSET     0x44U
#define PWM_PWM23COUNT_OFFSET     0x48U
#define PWM_PWM45COUNT_OFFSET     0x4CU
#define PWM_PWM0CMP_OFFSET        0x50U
#define PWM_PWM1CMP_OFFSET        0x54U
#define PWM_PWM2CMP_OFFSET        0x58U
#define PWM_PWM3CMP_OFFSET        0x5CU
#define PWM_PWM4CMP_OFFSET        0x60U
#define PWM_PWM5CMP_OFFSET        0x64U
#define PWM_PWM01DB_OFFSET        0x68U
#define PWM_PWM23DB_OFFSET        0x6CU
#define PWM_PWM45DB_OFFSET        0x70U

/* Input capture registers */
#define PWM_CAPCTL_OFFSET         0x74U   /* R/W, capture control */
#define PWM_CAPINTEN_OFFSET       0x78U   /* R/W, capture interrupt enable */
#define PWM_CAPRIS_OFFSET         0x7CU   /* RO , capture raw interrupt */
#define PWM_CAPIC_OFFSET          0x80U   /* R/W, capture interrupt clear */
#define PWM_CAPIS_OFFSET          0x84U   /* RO , capture interrupt status */
#define PWM_CAP01T_OFFSET         0x88U
#define PWM_CAP23T_OFFSET         0x8CU
#define PWM_CAP45T_OFFSET         0x90U
#define PWM_CAP01MATCH_OFFSET     0x94U
#define PWM_CAP23MATCH_OFFSET     0x98U
#define PWM_CAP45MATCH_OFFSET     0x9CU

/* Timer-function-mode registers (PWM module also has timer mode) */
#define PWM_TIMINTEN_OFFSET       0xA0U
#define PWM_TIMRIS_OFFSET         0xA4U
#define PWM_TIMIC_OFFSET          0xA8U
#define PWM_TIMIS_OFFSET          0xACU
#define PWM_TIM01LOAD_OFFSET      0xB0U
#define PWM_TIM23LOAD_OFFSET      0xB4U
#define PWM_TIM45LOAD_OFFSET      0xB8U
#define PWM_TIM01COUNT_OFFSET     0xBCU
#define PWM_TIM23COUNT_OFFSET     0xC0U
#define PWM_TIM45COUNT_OFFSET     0xC4U
#define PWM_CNT01VAL_OFFSET       0xC8U
#define PWM_CNT23VAL_OFFSET       0xCCU
#define PWM_CNT45VAL_OFFSET       0xD0U

/* PWMCFG bit definitions */
#define PWM_CFG_PWM0EN            (1U << 0)    /* channel 0 PWM output enable */
#define PWM_CFG_PWM1EN            (1U << 1)
#define PWM_CFG_PWM2EN            (1U << 2)
#define PWM_CFG_PWM3EN            (1U << 3)
#define PWM_CFG_PWM4EN            (1U << 4)
#define PWM_CFG_PWM5EN            (1U << 5)
#define PWM_CFG_PWM6EN            (1U << 6)
#define PWM_CFG_PWM7EN            (1U << 7)
#define PWM_CFG_PWM8EN            (1U << 8)
#define PWM_CFG_PWM9EN            (1U << 9)
#define PWM_CFG_PWM10EN           (1U << 10)
#define PWM_CFG_PWM11EN           (1U << 11)
#define PWM_CFG_CAP0EN            (1U << 12)   /* group 0 ch0 capture enable */
#define PWM_CFG_CAP1EN            (1U << 13)
#define PWM_CFG_CAP2EN            (1U << 14)
#define PWM_CFG_CAP3EN            (1U << 15)
#define PWM_CFG_CAP4EN            (1U << 16)
#define PWM_CFG_CAP5EN            (1U << 17)
#define PWM_CFG_TIM0EN            (1U << 18)   /* group 0 output compare enable */
#define PWM_CFG_TIM1EN            (1U << 19)
#define PWM_CFG_TIM2EN            (1U << 20)
#define PWM_CFG_TIM3EN            (1U << 21)
#define PWM_CFG_TIM4EN            (1U << 22)
#define PWM_CFG_TIM5EN            (1U << 23)
#define PWM_CFG_CNTDIV_POS        24U          /* [26:24] counter freq div select */
#define PWM_CFG_CNTDIV_MASK       (0x7U << PWM_CFG_CNTDIV_POS)
#define PWM_CFG_CNTDIVEN          (1U << 27)   /* 1=div enable, 0=div disable (use sys clk) */

/* PWM_CFG_CNTDIV encoding */
#define PWM_CNT_DIV_BY_2          0x0U
#define PWM_CNT_DIV_BY_4          0x1U
#define PWM_CNT_DIV_BY_8          0x2U
#define PWM_CNT_DIV_BY_16         0x3U
#define PWM_CNT_DIV_BY_32         0x4U
#define PWM_CNT_DIV_BY_64         0x5U
#define PWM_CNT_DIV_BY_128        0x6U

/* PWMCTL — counter mode control */
#define PWM_CTL_MODE_POS          0U           /* per-group mode bits; see spec.
                                              * 0x0 = count-up mode */
#define PWM_CTL_COUNT_UP          0x0U
#define PWM_CTL_COUNT_UP_DOWN     0x1U

/* PWMINVERTTRIG — output inversion / ADC trigger polarity per group */
#define PWM_INV_TRIG_G0           (1U << 0)
#define PWM_INV_TRIG_G1           (1U << 1)
#define PWM_INV_TRIG_G2           (1U << 2)
#define PWM_INV_TRIG_G3           (1U << 3)
#define PWM_INV_TRIG_G4           (1U << 4)
#define PWM_INV_TRIG_G5           (1U << 5)

/* PWMINTEN1 / PWMRIS1 / PWMIC1 / PWMIS1 — group 0/1/2 (3 bits each) */
#define PWM_INT_G0_MASK           0x00000007U
#define PWM_INT_G1_MASK           0x00000070U
#define PWM_INT_G2_MASK           0x00000700U

/* PWMINTEN2 / PWMRIS2 / PWMIC2 / PWMIS2 — group 3/4/5 */
#define PWM_INT_G3_MASK           0x00000007U
#define PWM_INT_G4_MASK           0x00000070U
#define PWM_INT_G5_MASK           0x00000700U

/* PWM0CMP..PWM5CMP: lower 16 bits = ch N compare, upper 16 bits = ch N+1 compare */
#define PWM_CMP_LO_MASK           0x0000FFFFU
#define PWM_CMP_HI_POS            16U
#define PWM_CMP_HI_MASK           0xFFFF0000U

/* Register accessors */
#define PWM_REG(base, off)  (*(volatile uint32_t *)((base) + (off)))
#define PWM_PWMCFG(base)         PWM_REG(base, PWM_PWMCFG_OFFSET)
#define PWM_PWMINVERTTRIG(base)  PWM_REG(base, PWM_PWMINVERTTRIG_OFFSET)
#define PWM_PWM01TRIG(base)      PWM_REG(base, PWM_PWM01TRIG_OFFSET)
#define PWM_PWM23TRIG(base)      PWM_REG(base, PWM_PWM23TRIG_OFFSET)
#define PWM_PWM45TRIG(base)      PWM_REG(base, PWM_PWM45TRIG_OFFSET)
#define PWM_PWMINTEN1(base)      PWM_REG(base, PWM_PWMINTEN1_OFFSET)
#define PWM_PWMINTEN2(base)      PWM_REG(base, PWM_PWMINTEN2_OFFSET)
#define PWM_PWMRIS1(base)        PWM_REG(base, PWM_PWMRIS1_OFFSET)
#define PWM_PWMRIS2(base)        PWM_REG(base, PWM_PWMRIS2_OFFSET)
#define PWM_PWMIC1(base)         PWM_REG(base, PWM_PWMIC1_OFFSET)
#define PWM_PWMIC2(base)         PWM_REG(base, PWM_PWMIC2_OFFSET)
#define PWM_PWMIS1(base)         PWM_REG(base, PWM_PWMIS1_OFFSET)
#define PWM_PWMIS2(base)         PWM_REG(base, PWM_PWMIS2_OFFSET)
#define PWM_PWMCTL(base)         PWM_REG(base, PWM_PWMCTL_OFFSET)
#define PWM_PWM01LOAD(base)      PWM_REG(base, PWM_PWM01LOAD_OFFSET)
#define PWM_PWM23LOAD(base)      PWM_REG(base, PWM_PWM23LOAD_OFFSET)
#define PWM_PWM45LOAD(base)      PWM_REG(base, PWM_PWM45LOAD_OFFSET)
#define PWM_PWM01COUNT(base)     PWM_REG(base, PWM_PWM01COUNT_OFFSET)
#define PWM_PWM23COUNT(base)     PWM_REG(base, PWM_PWM23COUNT_OFFSET)
#define PWM_PWM45COUNT(base)     PWM_REG(base, PWM_PWM45COUNT_OFFSET)
#define PWM_PWM0CMP(base)        PWM_REG(base, PWM_PWM0CMP_OFFSET)
#define PWM_PWM1CMP(base)        PWM_REG(base, PWM_PWM1CMP_OFFSET)
#define PWM_PWM2CMP(base)        PWM_REG(base, PWM_PWM2CMP_OFFSET)
#define PWM_PWM3CMP(base)        PWM_REG(base, PWM_PWM3CMP_OFFSET)
#define PWM_PWM4CMP(base)        PWM_REG(base, PWM_PWM4CMP_OFFSET)
#define PWM_PWM5CMP(base)        PWM_REG(base, PWM_PWM5CMP_OFFSET)
#define PWM_PWM01DB(base)        PWM_REG(base, PWM_PWM01DB_OFFSET)
#define PWM_PWM23DB(base)        PWM_REG(base, PWM_PWM23DB_OFFSET)
#define PWM_PWM45DB(base)        PWM_REG(base, PWM_PWM45DB_OFFSET)
#define PWM_CAPCTL(base)         PWM_REG(base, PWM_CAPCTL_OFFSET)
#define PWM_CAPINTEN(base)       PWM_REG(base, PWM_CAPINTEN_OFFSET)
#define PWM_CAPRIS(base)         PWM_REG(base, PWM_CAPRIS_OFFSET)
#define PWM_CAPIC(base)          PWM_REG(base, PWM_CAPIC_OFFSET)
#define PWM_CAPIS(base)          PWM_REG(base, PWM_CAPIS_OFFSET)
#define PWM_CAP01T(base)         PWM_REG(base, PWM_CAP01T_OFFSET)
#define PWM_CAP23T(base)         PWM_REG(base, PWM_CAP23T_OFFSET)
#define PWM_CAP45T(base)         PWM_REG(base, PWM_CAP45T_OFFSET)
#define PWM_CAP01MATCH(base)     PWM_REG(base, PWM_CAP01MATCH_OFFSET)
#define PWM_CAP23MATCH(base)     PWM_REG(base, PWM_CAP23MATCH_OFFSET)
#define PWM_CAP45MATCH(base)     PWM_REG(base, PWM_CAP45MATCH_OFFSET)
#define PWM_TIMINTEN(base)       PWM_REG(base, PWM_TIMINTEN_OFFSET)
#define PWM_TIMRIS(base)         PWM_REG(base, PWM_TIMRIS_OFFSET)
#define PWM_TIMIC(base)          PWM_REG(base, PWM_TIMIC_OFFSET)
#define PWM_TIMIS(base)          PWM_REG(base, PWM_TIMIS_OFFSET)
#define PWM_TIM01LOAD(base)      PWM_REG(base, PWM_TIM01LOAD_OFFSET)
#define PWM_TIM23LOAD(base)      PWM_REG(base, PWM_TIM23LOAD_OFFSET)
#define PWM_TIM45LOAD(base)      PWM_REG(base, PWM_TIM45LOAD_OFFSET)
#define PWM_TIM01COUNT(base)     PWM_REG(base, PWM_TIM01COUNT_OFFSET)
#define PWM_TIM23COUNT(base)     PWM_REG(base, PWM_TIM23COUNT_OFFSET)
#define PWM_TIM45COUNT(base)     PWM_REG(base, PWM_TIM45COUNT_OFFSET)
#define PWM_CNT01VAL(base)       PWM_REG(base, PWM_CNT01VAL_OFFSET)
#define PWM_CNT23VAL(base)       PWM_REG(base, PWM_CNT23VAL_OFFSET)
#define PWM_CNT45VAL(base)       PWM_REG(base, PWM_CNT45VAL_OFFSET)

#endif /* __WJ100_PWM_H__ */
