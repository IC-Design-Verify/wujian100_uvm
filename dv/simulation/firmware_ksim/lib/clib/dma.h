// ***********************************************************************
// *****************                                                       
// ***** ***********                                                       
// *****   *********                                                       
// *****     *******      Copyright (c) 2025  Kang Yaopeng  
// *****       *****                                                       
// *****     *******             All rights reserved                       
// *****   *********                                                       
// ***** ***********                                                       
// *****************                                                       
// ***********************************************************************
// PROJECT        : 
// FILENAME       : dma.h
// Author         : IC_VERIFY
// LAST MODIFIED  : 2025-03-08 05:58
// ***********************************************************************
// DESCRIPTION    :
// ***********************************************************************
// $Revision: $
// $Id: $
// ***********************************************************************
#define DMA_BASE_ADDR 0x40000000

#define DMA_CH0_BASE_ADDR     DMA_BASE_ADDR+0x0
#define DMA_CH1_BASE_ADDR     DMA_BASE_ADDR+0x30
#define DMA_CH2_BASE_ADDR     DMA_BASE_ADDR+0x60
#define DMA_CH3_BASE_ADDR     DMA_BASE_ADDR+0x90
#define DMA_CH4_BASE_ADDR     DMA_BASE_ADDR+0xc0
#define DMA_CH5_BASE_ADDR     DMA_BASE_ADDR+0xf0
#define DMA_CH6_BASE_ADDR     DMA_BASE_ADDR+0x120
#define DMA_CH7_BASE_ADDR     DMA_BASE_ADDR+0x150
#define DMA_CH8_BASE_ADDR     DMA_BASE_ADDR+0x180
#define DMA_CH9_BASE_ADDR     DMA_BASE_ADDR+0x1b0
#define DMA_CH10_BASE_ADDR    DMA_BASE_ADDR+0x1e0
#define DMA_CH11_BASE_ADDR    DMA_BASE_ADDR+0x210
#define DMA_CH12_BASE_ADDR    DMA_BASE_ADDR+0x240
#define DMA_CH13_BASE_ADDR    DMA_BASE_ADDR+0x270
#define DMA_CH14_BASE_ADDR    DMA_BASE_ADDR+0x2a0
#define DMA_CH15_BASE_ADDR    DMA_BASE_ADDR+0x2d0
#define DMA_GLOBAL_BASE_ADDR  DMA_BASE_ADDR+0x330


#define DMA_CH_SAR_OFFSET         0x0
#define DMA_CH_DAR_OFFSET         0x4
#define DMA_CH_CTRL_A_OFFSET      0x8
#define DMA_CH_CTRL_B_OFFSET      0xc
#define DMA_CH_INT_MASK_OFFSET    0x10
#define DMA_CH_INT_STATUS_OFFSET  0x14
#define DMA_CH_INT_CLEAR_OFFSET   0x18
#define DMA_CH_SOFT_REQ_OFFSET    0x1c
#define DMA_CH_EN_OFFSET          0x20

#define DMA_CHSR_OFFSET           0x8
#define DMA_DMACCFG_OFFSET        0xc



// ***********************************************************************
// $Log: $
// $Revision $
