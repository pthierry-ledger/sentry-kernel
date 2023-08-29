// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

#ifndef __IRQ_H_
#define __IRQ_H_

/**
 * @brief IRQ num typedef that comply with cmsis header
 */
typedef enum IRQn
{
    /* core exceptions */
    RESET_IRQ      = -15,
    NMI_IRQ        = -14,
    HARDFAULT_IRQ  = -13,
    MEMMANAGE_IRQ  = -12,
    BUSFAULT_IRQ   = -11,
    USAGEFAULT_IRQ = -10,
    RES7_IRQ       = -9,
    RES8_IRQ       = -8,
    RES9_IRQ       = -7,
    RES10_IRQ      = -6,
    SVC_IRQ        = -5,
    RES12_IRQ      = -4,
    RES13_IRQ      = -3,
    PENDSV_IRQ     = -2,
    SYSTICK_IRQ    = -1,

    /* soc defined interrupts */
    WWDG_IRQ,
    PVD_IRQ,
    TAMP_STAMP_IRQ,
    RTC_WKUP_IRQ,
    FLASH_IRQ,
    RCC_IRQ,
    EXTI0_IRQ,
    EXTI1_IRQ,
    EXTI2_IRQ,
    EXTI3_IRQ,
    EXTI4_IRQ,
    DMA1_Stream0_IRQ,
    DMA1_Stream1_IRQ,
    DMA1_Stream2_IRQ,
    DMA1_Stream3_IRQ,
    DMA1_Stream4_IRQ,
    DMA1_Stream5_IRQ,
    DMA1_Stream6_IRQ,
    ADC_IRQ,
    CAN1_TX_IRQ,
    CAN1_RX0_IRQ,
    CAN1_RX1_IRQ,
    CAN1_SCE_IRQ,
    EXTI9_5_IRQ,
    TIM1_BRK_TIM9_IRQ,
    TIM1_UP_TIM10_IRQ,
    TIM1_TRG_COM_TIM11_IRQ,
    TIM1_CC_IRQ,
    TIM2_IRQ,
    TIM3_IRQ,
    TIM4_IRQ,
    I2C1_EV_IRQ,
    I2C1_ER_IRQ,
    I2C2_EV_IRQ,
    I2C2_ER_IRQ,
    SPI1_IRQ,
    SPI2_IRQ,
    USART1_IRQ,
    USART2_IRQ,
    USART3_IRQ,
    EXTI15_10_IRQ,
    RTC_Alarm_IRQ,
    OTG_FS_WKUP_IRQ,
    TIM8_BRK_TIM12_IRQ,
    TIM8_UP_TIM13_IRQ,
    TIM8_TRG_COM_TIM14_IRQ,
    TIM8_CC_IRQ,
    DMA1_Stream7_IRQ,
    FSMC_IRQ,
    SDIO_IRQ,
    TIM5_IRQ,
    SPI3_IRQ,
    UART4_IRQ,
    UART5_IRQ,
    TIM6_DAC_IRQ,
    TIM7_IRQ,
    DMA2_Stream0_IRQ,
    DMA2_Stream1_IRQ,
    DMA2_Stream2_IRQ,
    DMA2_Stream3_IRQ,
    DMA2_Stream4_IRQ,
    ETH_IRQ,
    ETH_WKUP_IRQ,
    CAN2_TX_IRQ,
    CAN2_RX0_IRQ,
    CAN2_RX1_IRQ,
    CAN2_SCE_IRQ,
    OTG_FS_IRQ,
    DMA2_Stream5_IRQ,
    DMA2_Stream6_IRQ,
    DMA2_Stream7_IRQ,
    USART6_IRQ,
    I2C3_EV_IRQ,
    I2C3_ER_IRQ,
    OTG_HS_EP1_OUT_IRQ,
    OTG_HS_EP1_IN_IRQ,
    OTG_HS_WKUP_IRQ,
    OTG_HS_IRQ,
    DCMI_IRQ,
    CRYP_IRQ,
    HASH_RNG_IRQ,
    FPU_IRQ,
} IRQn_Type;

#define __NVIC_VECTOR_LEN 82
#define __NVIC_PRIO_BITS 4

#endif/*__IRQ_H_*/
