
#include "stm32f4xx.h"
#include "rcc.h"

void RCC_Init()
{
  RCC->CR |= RCC_CR_HSEON;                                              //enable HSE
  while((RCC->CR & RCC_CR_HSERDY) == 0) {};                             //wait till HSE is ready
  
  //Inside flash 
  FLASH->ACR |= FLASH_ACR_PRFTEN;                                       //enable Prefetch
  FLASH->ACR &= ~FLASH_ACR_LATENCY;
  FLASH->ACR |= FLASH_ACR_LATENCY_3WS;                                  //Set flash latency to 2 ws  
  
  RCC->CFGR &= ~RCC_CFGR_SW;                                            //Clear SW bits
  RCC->CFGR &= ~RCC_CFGR_HPRE;                                          //Clear HPRE AHB prescaler
  RCC->CFGR &= ~RCC_CFGR_PPRE1;                                         //Clear APB1 prescaler
  RCC->CFGR &= ~RCC_CFGR_PPRE2;                                         //Clear APB2 prescaler
  
  //Divider for System Clock = 96MHz
  RCC->CFGR |= RCC_CFGR_HPRE_DIV1;                                      //Set AHB prescaler System Clock division /1 (SYSCLK not divided) 96/1 = 96MHz
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;                                     //Set APB1 (APB Low speed) prescaler /4 System Clock(144MHz) /4  = 36MHz
  RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;                                     //RCC_CFGR_PPRE2_DIV4;                                     //Set APB2 (APB High speed) prescaler /4 System Clock(144MHz) /4  = 36MHz
  
  //PLL Master Init (clock source->HSE = 8MHz, M =8, N =192, P =4, Q = 4) 
  RCC->CR &= ~RCC_CR_PLLON;                                             //disable Main PLL
  while((RCC->CR & RCC_CR_PLLRDY) == 1) {};                             //wait till PLL is ready
  
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLM;                                    //Clear Main PLL Division /M
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLN;                                    //Clear Main PLL multiplication *N
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLP;                                    //Clear Main PLL Division /P        
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLQ;                                    //Clear Main PLL Division /Q
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;                                  //Clear Main PLL entry clock source
    
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE;                               //Set Main PLL entry clock source HSE         
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLM_2;                                   //Set Main PLL division M = 4 (HSE/M = 8/4 = 2MHz)   
  
  //System Clock = 96MHz
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLN_5 | RCC_PLLCFGR_PLLN_6;              //Set Main PLL multiplication N = 96 ((HSE/M)*N = 2*96 = 192MHz)   
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLP;                                    //Set Main PLL division P = 2 (((HSE/M)*N)/P = 192/2 = 96MHz) System Clock Input = 96MHz
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLQ_2;                                   //Set Main PLL division Q = 4 (((HSE/M)*N)/Q = 192/4 = 48MHz) USB Clock Input = 48MHz
  
  RCC->CR |= RCC_CR_PLLON;                                              //enable PLL
  while((RCC->CR & RCC_CR_PLLRDY) == 0) {};                             //wait till PLL is ready
  
  RCC->CFGR &= ~RCC_CFGR_SW;                                            //clear SW bits
  RCC->CFGR |= RCC_CFGR_SW_PLL;                                         //Select PLL as system clock
  
  while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {};             //wait till PLL is used
 
  //RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;		                // enable clock for port B 
  // 
  // //SPI2_SCL PB13 
  //GPIOC->MODER |= GPIO_MODER_MODE9_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  //GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9;		        //Set OSPEEDR (output speed register) -> Medium speed
  //GPIOC->AFR[1] &= ~GPIO_AFRH_AFRH1;       //Set AFR (alternate function) SPI3_SCK 

  SystemCoreClockUpdate();
}
