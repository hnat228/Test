
#include "stm32f4xx.h"
#include "stm32f412rx.h"
#include "adc_mcu.h"
#include "delay.h"
#include "dsp_board.h"
#include "front_board.h"
#include "file_streams.h"

void ADC_WatchDog_Init()
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;		                // enable clock for port A 
  //ADC_IN1 PA1
  GPIOA->MODER |= GPIO_MODER_MODE1;		                //Set MODE (mode register) -> Input Analog
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR1_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR1;		                //Clear PUPDR
  
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;		                // enable clock for adc 
   
  ADC1->CR2 |= ADC_CR2_ADON;                            // enable adc1
  Delay_ms(100);
  
  ADC1->SMPR2 |= ADC_SMPR2_SMP1;                        // set convertion time
     
  ADC1->CR2 |= ADC_CR2_CONT;                            //Continuous conversion mode
     
  ADC1->SQR1 &= ~ADC_SQR1_L;                            // set number of channel in group
  ADC1->SQR3 |= ADC_SQR3_SQ1_0;                         // select 1st ch in group
     
  ADC1->CR1 |= ADC_CR1_AWDIE;                           // enable analog watchdog interupt      
  ADC1->CR1 |= ADC_CR1_AWDSGL;                          // enable AWD single channel
  ADC1->CR1 |= ADC_CR1_AWDCH_0;                         // set ch number                
     
  ADC1->HTR = (uint16_t)ADC_H_TH;                       // set high level AWD
  ADC1->LTR = (uint16_t)ADC_L_TH;                       // set low level AWD
  ADC1->CR1 |= ADC_CR1_AWDEN;                           // enable AWD
     
  ADC1->CR2 |= ADC_CR2_SWSTART;
  
  while(ADC1->DR < ADC_L_TH);
  //Delay_ms(500);
  
  //NVIC_SetPriority(ADC1_IRQn, 4);                       // set irq priority
  NVIC_EnableIRQ(ADC_IRQn);  
  
  //ADC1->CR2 |= ADC_CR2_CAL;                             // calibration
  //while (ADC1->CR2 & ADC_CR2_CAL)       
  //Delay_ms(100);
}

void ADC_IRQHandler(void)
{
  CLR(RST_DSP_OUT, RST_DSP);            //DSP Disable
  CLR(OE_FB_OUT, OE_FB);                //Front Disable
  CLR(DC_ENABLE_OUT, DC_ENABLE);        //DC-DC Disable
  
  Flash_Write_Param();

  ADC1->SR &= ~ADC_SR_AWD;
  while(ADC1->DR < ADC_L_TH);
  
  NVIC_SystemReset();
}