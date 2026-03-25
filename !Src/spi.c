
#include "stm32f412rx.h"
#include "spi.h"
#include "amp_application.h"



void SPI2_Init()                                 
{
  //SPI2 init FLASH SPI2_CLK->PB13, SPI2_MOSI->PB15, SPI2_MISO->PB14, CS_Flash->PB12
  
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		                // enable clock for port B 

  //CS_Flash->PB12
  SET(CS_FLASH_OUT, CS_FLASH);                                  //Set OUT_DATA = 1
  GPIOB->MODER &= ~GPIO_MODER_MODE12;		                //Reset MODE (mode register)
  GPIOB->MODER |= GPIO_MODER_MODE12_0;		                //Set MODE (mode register) -> Output
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_12;		                //Set OTYPER -> Open drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR12;		                //Set PUPDR -> Pull Up 
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR12_0;		        //Reset OSPEEDR (output speed register)
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR12_0;		        //Set OSPEEDR (output speed register) -> Medium speed
 
  //SPI2_SCL PB13
  GPIOB->MODER &= ~GPIO_MODER_MODE13;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOB->MODER |= GPIO_MODER_MODE13_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_13;		                //Set OTYPER -> Open drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR13;		                //Set PUPDR -> Pull Up 
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR13;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR13_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[1] |= GPIO_AFRH_AFRH5_0 | GPIO_AFRH_AFRH5_2;       //Set AFR (alternate function) SPI3_SCK 

  //SPI2_MISO->PB14
  GPIOB->MODER &= ~GPIO_MODER_MODE14;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOB->MODER |= GPIO_MODER_MODE14_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_14;		                //Set OTYPER -> Push Pull
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR14;		                //Set PUPDR -> Pull Up 
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR14_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR14_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[1] |= GPIO_AFRH_AFRH6_0 | GPIO_AFRH_AFRH6_2;       //Set AFR (alternate function) SPI3_MISO 
 
  //SPI2_MOSI->PB15
  GPIOB->MODER &= ~GPIO_MODER_MODE15;		                //Reset MODE (mode register) 
  GPIOB->MODER |= GPIO_MODER_MODE15_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_15;		                //Set OTYPER -> Push Pull
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR14;		                //Set PUPDR -> Push Pull 
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR15;		        //Set OSPEEDR (output speed register)
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR15_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[1] |= GPIO_AFRH_AFRH7_0 | GPIO_AFRH_AFRH7_2;       //Set AFR (alternate function) SPI3_MOSI 
 
  RCC->APB1ENR |=  RCC_APB1ENR_SPI2EN;
  
  SPI2->CR1 &= ~SPI_CR1_BR;                             // clear baud rate register
  SPI2->CR1 |= SPI_CR1_BR_1;                            // set baud rate = 48/8 (6MHz)
  SPI2->CR1 &= ~SPI_CR1_CPOL;                            // set polarity
  SPI2->CR1 &= ~SPI_CR1_CPHA;                            // set phase
  SPI2->CR1 &= ~SPI_CR1_DFF;                            // 8-bit data frame format
  SPI2->CR1 &= ~SPI_CR1_LSBFIRST;                       // MSB first
  SPI2->CR1 |= SPI_CR1_SSM;                             // program NSS
  SPI2->CR1 |= SPI_CR1_SSI;                             // 
  SPI2->CR2 |= SPI_CR2_SSOE;                            // NSS is output
  SPI2->CR1 |= SPI_CR1_MSTR;                            // master
  SPI2->CR1 |= SPI_CR1_SPE;                             // enable SPI 
}


void SPI3_Init()                                 
{
  //SPI3 init LCD, LED, Button 
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;		                // enable clock for port A 
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		                // enable clock for port B 
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;		                // enable clock for port C  
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;		                // enable clock for port D  
  
  //CS_BUTTON PB4
  CLR(CS_BUT_OUT, CS_BUT);                                        //Output Enable OFF                                     
  GPIOB->MODER &= ~GPIO_MODER_MODE4;		                //Clear MODE (mode register) 
  GPIOB->MODER |= GPIO_MODER_MODE4_0;		                //Set MODE (mode register) -> Alternate
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_4;		                //Set OTYPER -> Open drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR4;		                //Set PUPDR -> Pull 
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR4;		        //Reset OSPEEDR (output speed register)
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR4_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  
  //CS_LED PD2
  SET(CS_LED_OUT, CS_LED);                                      //Set OUT_DATA = 1
  GPIOD->MODER |= GPIO_MODER_MODE2_0;		                //Set MODE (mode register) -> Output
  GPIOD->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR2_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  
  //CS_LCD PA15 
  SET(CS_LCD_OUT, CS_LCD); 
  GPIOA->MODER &= ~GPIO_MODER_MODE15;		                //Set MODE (mode register) -> Output
  GPIOA->MODER |= GPIO_MODER_MODE15_0;		                //Set MODE (mode register) -> Output
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT_15;		                //Set OTYPER -> Push Pull
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR15;		                //Set PUPDR -> Pull 
  GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR15;		        //Reset OSPEEDR (output speed register)
  GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR15_0;		        //Set OSPEEDR (output speed register) -> Medium speed

  //SPI3_SCL PC10 
  GPIOC->MODER |= GPIO_MODER_MODE10_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOC->AFR[1] |= GPIO_AFRH_AFRH2_1 | GPIO_AFRH_AFRH2_2;       //Set AFR (alternate function) SPI3_SCK 

  //SPI3_MISO PC11
  GPIOC->MODER |= GPIO_MODER_MODE11_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR11_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOC->AFR[1] |= GPIO_AFRH_AFRH3_1 | GPIO_AFRH_AFRH3_2;       //Set AFR (alternate function) SPI3_MISO 
 
  //SPI3_MOSI PC12
  GPIOC->MODER |= GPIO_MODER_MODE12_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR12_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOC->AFR[1] |= GPIO_AFRH_AFRH4_1 | GPIO_AFRH_AFRH4_2;       //Set AFR (alternate function) SPI3_MOSI 
 
  RCC->APB1ENR |=  RCC_APB1ENR_SPI3EN;
  SPI3->CR1 &= ~SPI_CR1_BR;                             // clear baud rate register
  SPI3->CR1 |= (SPI_CR1_BR_0 | 
                SPI_CR1_BR_1 |
                SPI_CR1_BR_2);                          // set baud rate = fplck(48)/128 (281kHz)
  SPI3->CR1 &= ~SPI_CR1_CPOL;                            // set polarity
  SPI3->CR1 &= ~SPI_CR1_CPHA;                            // set phase
  SPI3->CR1 &= ~SPI_CR1_DFF;                            // 8-bit data frame format
  SPI3->CR1 &= ~SPI_CR1_LSBFIRST;                       // MSB first
  SPI3->CR1 |= SPI_CR1_SSM;                             // program NSS
  SPI3->CR1 |= SPI_CR1_SSI;                             // 
  SPI3->CR2 |= SPI_CR2_SSOE;                            // NSS is output
  SPI3->CR1 |= SPI_CR1_MSTR;                            // master
  SPI3->CR1 |= SPI_CR1_SPE;                             // enable SPI 
}

uint8_t SPI2_Transmit(uint8_t data)
{ 
  while(!(SPI2->SR & SPI_SR_TXE));
  SPI2->DR = data;
   
  while(!(SPI2->SR & SPI_SR_RXNE));
  return SPI2->DR;
}

uint8_t SPI3_Transmit(uint8_t data)
{ 
  while(!(SPI3->SR & SPI_SR_TXE));
  SPI3->DR = data;
   
  while(!(SPI3->SR & SPI_SR_RXNE));
  return SPI3->DR;
}

void CS_Enable(IC_Type IC)
{
  switch(IC)
  {
    case IC_Flash : CLR(CS_FLASH_OUT, CS_FLASH); break;
    case IC_LCD : CLR(CS_LCD_OUT, CS_LCD); break;
    case IC_LED : CLR(CS_LED_OUT, CS_LED); break;
    case IC_BUTTON : SET(CS_BUT_OUT, CS_BUT); break;
  }
}

void CS_Disable(IC_Type IC)
{
  switch(IC)
  {
    case IC_Flash : SET(CS_FLASH_OUT, CS_FLASH); break;
    case IC_LCD : SET(CS_LCD_OUT, CS_LCD); break;
    case IC_LED : SET(CS_LED_OUT, CS_LED); break;
    case IC_BUTTON : CLR(CS_BUT_OUT, CS_BUT); break;
  }
}
