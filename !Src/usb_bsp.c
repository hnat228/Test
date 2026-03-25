
#include "usb_bsp.h"
#include "delay.h"

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
    #if defined ( __ICCARM__ )      /* !< IAR Compiler */
      #pragma data_alignment=4
    #endif
  #endif     /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
  __ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END;

uint8_t Send_Buffer[2];
uint8_t PrevXferDone = 1;
uint8_t isReceived = 0;

extern uint32_t ADC_ConvertedValueX;
extern uint32_t ADC_ConvertedValueX_1;
/* Private function prototypes ----------------------------------------------- */
//extern USB_OTG_CORE_HANDLE USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE * pdev);
#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
extern uint32_t USBD_OTG_EP1IN_ISR_Handler(USB_OTG_CORE_HANDLE * pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler(USB_OTG_CORE_HANDLE * pdev);
#endif

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
#if defined ( __ICCARM__ )      /* !< IAR Compiler */
#pragma data_alignment=4
#endif
#endif                          /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN uint8_t Send_Buffer[2] __ALIGN_END;


void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE * pdev)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

  // PA8 - SOF
  //GPIOA->MODER &= ~GPIO_MODER_MODER8;           //clear MODER
  //GPIOA->MODER |= GPIO_MODER_MODER8_1;          //set mode AF
  //GPIOA->OTYPER &= ~GPIO_OTYPER_OT_8;           //clear OTYPER, set type PP
  //GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR8;    //clear OSPEEDR
  //GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8;     //set very high speed 100MHz
  
  //PA9 - VBUS
  //GPIOA->MODER &= ~GPIO_MODER_MODER9;          //set mode IN
  //GPIOA->OTYPER &= ~GPIO_OTYPER_OT_9;          //clear OTYPER, PP
  //GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR9;   //clear OSPEEDR
  //GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9;    //set very high speed 100MHz
  
  //PA10 - ID
  //GPIOA->MODER &= ~GPIO_MODER_MODER10;          //clear MODER
  //GPIOA->MODER |= GPIO_MODER_MODER10_1;         //set mode AF
  //GPIOA->OTYPER |= GPIO_OTYPER_OT_10;           //set type, OD
  //GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR10;   //clear OSPEEDR
  //GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10;    //set very high speed 100MHz
  //GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR10;          //clear PUPDR
  //GPIOA->PUPDR = GPIO_PUPDR_PUPDR10_0;          //set PU
  
  // PA11 - DM
  GPIOA->MODER &= ~GPIO_MODER_MODER11;          //clear MODER
  GPIOA->MODER |= GPIO_MODER_MODER11_1;         //set mode AF
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT_11;          //clear OTYPER, PP
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR11;          //clear PUPDR
  GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR11;   //clear OSPEEDR
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR11;    //set very high speed 100MHz
  
  // PA12 - DP
  GPIOA->MODER &= ~GPIO_MODER_MODER12;          //clear MODER
  GPIOA->MODER |= GPIO_MODER_MODER12_1;         //set mode AF
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT_12;          //clear OTYPER, set type PP
  GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR12;   //clear OSPEEDR
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR12;          //clear PUPDR
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR12;    //set very high speed 100MHz

  //GPIOA->AFR[1] |= (10 << (4*0));
  //GPIOA->AFR[1] |= (10 << (4*2));
  GPIOA->AFR[1] |= (10 << (4*3));
  GPIOA->AFR[1] |= (10 << (4*4));
  
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
  
  /* enable the PWR clock */
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;

  /* Configure the Tamper button in EXTI mode */
  //STM_EVAL_PBInit(BUTTON_KEY, Mode_EXTI);

  /* Configure Tamper EXTI line to generate an interrupt on rising & falling
   * edges */
  //EXTI_InitStructure.EXTI_Line = KEY_BUTTON_EXTI_LINE;
  //EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  //EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  //EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  //EXTI_Init(&EXTI_InitStructure);
  //
  ///* Clear the Tamper EXTI line pending bit */
  //EXTI_ClearITPendingBit(KEY_BUTTON_EXTI_LINE);
  //Delay_Init();
}

/**
* @brief  USB_OTG_BSP_EnableInterrupt
*         Enable USB Global interrupt
* @param  None
* @retval None
*/
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE * pdev)
{
  //NVIC_SetPriorityGrouping(1);

  NVIC_SetPriority(OTG_FS_IRQn, 1);
  NVIC_EnableIRQ(OTG_FS_IRQn);
  
  
  /* Enable the Key EXTI line Interrupt */
  //NVIC_InitStructure.NVIC_IRQChannel = KEY_BUTTON_EXTI_IRQn;
  //NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  //NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  //NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  //NVIC_Init(&NVIC_InitStructure);

  /* NVIC configuration for DMA transfer complete interrupt */
  
  //NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  //NVIC_SetPriority(DMA2_Stream0_IRQn, 5);
}

void USB_OTG_BSP_uDelay(const uint32_t usec)
{
  Delay_us(usec);
}

void USB_OTG_BSP_mDelay(const uint32_t msec)
{
  Delay_ms(msec);
}

#ifdef USE_STM3210C_EVAL
void EXTI9_5_IRQHandler(void)
#else
void EXTI15_10_IRQHandler(void)
#endif
{
  //if (EXTI_GetITStatus(KEY_BUTTON_EXTI_LINE) != RESET)
  //{
  //  /* Clear the EXTI line pending bit */
  //  EXTI_ClearITPendingBit(KEY_BUTTON_EXTI_LINE);
  //
  //  if ((PrevXferDone) && (USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED))
  //  {
  //    Send_Buffer[0] = KEY_REPORT_ID;
  //
  //    if (STM_EVAL_PBGetState(BUTTON_KEY) == Bit_RESET)
  //    {
  //      Send_Buffer[1] = 0x01;
  //    }
  //    else
  //    {
  //      Send_Buffer[1] = 0x00;
  //    }
  //
  //    USBD_CUSTOM_HID_SendReport(&USB_OTG_dev, Send_Buffer, 2);
  //    PrevXferDone = 0;
  //  }
  //}
}

/**
  * @brief  This function handles ADCx DMA IRQHandler Handler.
  * @param  None
  * @retval None
  */
void ADCx_DMA_IRQHandler(void)
{
  //Send_Buffer[0] = 0x07;
  //
  //if ((ADC_ConvertedValueX >> 4) - (ADC_ConvertedValueX_1 >> 4) > 4)
  //{
  //  if ((PrevXferDone) && (USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED))
  //  {
  //    Send_Buffer[1] = (uint8_t) (ADC_ConvertedValueX >> 4);
  //
  //    USBD_CUSTOM_HID_SendReport(&USB_OTG_dev, Send_Buffer, 2);
  //
  //    ADC_ConvertedValueX_1 = ADC_ConvertedValueX;
  //    PrevXferDone = 0;
  //  }
  //}
}

void OTG_FS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
void OTG_HS_EP1_IN_IRQHandler(void)
{
  USBD_OTG_EP1IN_ISR_Handler(&USB_OTG_dev);
}

void OTG_HS_EP1_OUT_IRQHandler(void)
{
  USBD_OTG_EP1OUT_ISR_Handler(&USB_OTG_dev);
}
#endif
