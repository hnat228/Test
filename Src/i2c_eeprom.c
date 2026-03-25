
#include "i2c_eeprom.h"
#include "stm32f412rx.h"
#include "delay.h"
#include "dataconverter.h"
#include "file_streams.h"
//#include "stdint.h"

void I2C2_Init()
{
  //Devices -> EEPROM (AT24C02 addr = 0x50)
  //I2C2 SCL->PB10, SDA->PB11

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		                // enable clock for port B 
    
  //I2C SCL PB10
  GPIOB->MODER &= ~GPIO_MODER_MODE10;		                //Reset MODE (mode register) 
  GPIOB->MODER |= GPIO_MODER_MODE10_1;		                //Set MODE (mode register) -> Alternative
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_10;		                //Set OTYPER -> open-drain
  GPIOB->OTYPER |= GPIO_OTYPER_OT_10;		                //Set OTYPER -> open-drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR10;		                //Clear PUPDR -> open-drain
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR10_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR10_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[1] |= GPIO_AFRH_AFRH2_2;                           //Set AFR (alternate function) I2C1_SCL
    
  //I2C SDA PB9
  GPIOB->MODER &= ~GPIO_MODER_MODE9;		                //Set MODE (mode register) -> Alternative
  GPIOB->MODER |= GPIO_MODER_MODE9_1;		                //Set MODE (mode register) -> Alternative
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_9;		                //Set OTYPER -> open-drain
  GPIOB->OTYPER |= GPIO_OTYPER_OT_9;		                //Set OTYPER -> open-drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR9;		                //Clear PUPDR -> open-drain
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR9_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[1] |= GPIO_AFRH_AFRH1_0 | 
                   GPIO_AFRH_AFRH1_3;                           //Set AFR (alternate function) I2C1_SDA
  
  RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;		                // enable clock for I2C 
  I2C2->CR1 &= ~I2C_CR1_PE;                                     // diseble i2c
  I2C2->CR2 &= ~I2C_CR2_FREQ;                                   // clear CR2 
  I2C2->CCR &= ~I2C_CCR_CCR;                                    // clear CCR 
  I2C2->CR2 |= 48;                                              // set PCLK1 frequency 
  I2C2->TRISE = 16;                                            // set rising time  
  I2C2->CCR |= I2C_CCR_FS;                                      // set fast mode
  I2C2->CCR |= 40;                                              // set i2c clock frequency 400kHz 
  I2C2->CR1 |= I2C_CR1_PE;                                      // enable i2c  
}

uint16_t Eeprom_Init()
{
  uint8_t buff[EEPROM_HEADER_SIZE];
  uint16_t MemoryType = 0;
  
  //Eeprom_Erase();
  
  I2C2_Eeprom_Receive(DEVICE_ADDR_EEPROM, EEPROM_ADDR_HEADER, buff, EEPROM_HEADER_SIZE);
  
  MemoryType = (uint16_t)buff[0] << 8;
  MemoryType |= buff[1];  

  return MemoryType;
}

void I2C2_Start()
{
  I2C2->CR1 |= I2C_CR1_START; 
}

void I2C2_Stop()
{
  I2C2->CR1 |= I2C_CR1_STOP;
}

uint8_t I2C2_Eeprom_Write_Byte(uint8_t DevAddr, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
  uint32_t count;
  
  count = 0;
  I2C2_Start();                          //generate start
  while (!(I2C2->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }
  
  (void) I2C2->SR1;
  
  count = 0;
  I2C2->DR = DevAddr;                   //send devise adress
  Delay_us(1);
  while (!(I2C2->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }
  (void) I2C2->SR1;
  (void) I2C2->SR2;
  
  count = 0;
  I2C2->DR = Addr;      //send data Type byte
  Delay_us(1);
  while (!(I2C2->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }

  while(Size > 0u)                      //send data buffer
  {
    count = 0;
    I2C2->DR = (*pData++);
    Delay_us(1);
    while (!(I2C2->SR1 & I2C_SR1_BTF))
    {
      if(count < I2C2_Timeout) count++;
      else return 1;
    }
    Size--;
  }
  
  I2C2_Stop();                           //generate stop
  Delay_us(5);
  
  return 0;
} 

uint8_t I2C2_Eeprom_Write(uint8_t DevAddr, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
  #define EEPROM_PAGE 8
  
  uint16_t count = Size;
  uint8_t offset_addr = Addr;

  //uint8_t i;
  
  for(count = Size; count > EEPROM_PAGE; count = (count - EEPROM_PAGE))
  { 
    I2C2_Eeprom_Write_Byte(DevAddr, offset_addr, pData, EEPROM_PAGE);
    offset_addr = (offset_addr + EEPROM_PAGE);
    Delay_ms(2);
  }
  
  if(count > 0)
  {
    I2C2_Eeprom_Write_Byte(DevAddr, offset_addr, pData, count);
    Delay_ms(2);
  }
  
  return 0;
}

uint8_t I2C2_Eeprom_Receive(uint8_t DevAddr, uint8_t Addr, uint8_t *pData, uint16_t Size)
{
  uint32_t count;
  
  I2C2->CR1 |= I2C_CR1_ACK;
  
  count = 0;
  I2C2_Start();                          //generate start
  while (!(I2C2->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }
  
  (void) I2C2->SR1;
  
  count = 0;
  I2C2->DR = DevAddr;                   //send devise adress
  Delay_us(1);
  while (!(I2C2->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }
  (void) I2C2->SR1;
  (void) I2C2->SR2;

  count = 0;
  I2C2->DR = Addr;     //send data Type byte
  Delay_us(1);
  while (!(I2C2->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }

  I2C2_Start();                          //generate start
  while (!(I2C2->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }
  
  (void) I2C2->SR1;
  
  count = 0;
  I2C2->DR = DevAddr | 0x01;            //send devise adress
  Delay_us(1);
  while (!(I2C2->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C2_Timeout) count++;
    else return 1;
  }
  (void) I2C2->SR1;
  (void) I2C2->SR2;

  while(Size > 0u)                    
  {
    count = 0;
    
    if(Size == 1) 
    {
      I2C2->CR1 &= ~I2C_CR1_ACK;
      I2C2_Stop();
    }

    while (!(I2C2->SR1 & I2C_SR1_RXNE))
    {
      if(count < I2C2_Timeout) count++;
      else return 1;
    }  
    (*pData++) = I2C2->DR;
    Size--;       
  }     
  
  Delay_us(50);
  return 0;
}


void Eeprom_Param_Config(BlockStatusType* BlockStatus, EepromComandType EepromComand)
{
  if(EepromComand == WRITE_EEPROM) Write_Memory_Config(BlockStatus);
  else if(EepromComand == READ_EEPROM) Read_Memory_Config(BlockStatus);
}

void Write_Memory_Config(BlockStatusType* BlockStatus)
{
  uint8_t BlockStatusBuff[BlockStatusSize];                                                     //Create Buff for write data to eeprom
  
  BlockStatusBuff[0] = (uint8_t)((BlockStatus->CycleCounter & 0xFF0000) >> 16);
  BlockStatusBuff[1] = (uint8_t)((BlockStatus->CycleCounter & 0x00FF00) >> 8);
  BlockStatusBuff[2] = (uint8_t)(BlockStatus->CycleCounter & 0x0000FF);

  I2C2_Eeprom_Write_Byte(DEVICE_ADDR_EEPROM, BlockStatus->EepromAddres, BlockStatusBuff, BlockStatusSize);
  Delay_ms(2);
}

void Read_Memory_Config(BlockStatusType* BlockStatus)
{
  uint32_t CycleCounter;
  uint8_t BlockStatusBuff[BlockStatusSize];                                                     //Create Buff for write data to eeprom
  
  I2C2_Eeprom_Receive(DEVICE_ADDR_EEPROM, BlockStatus->EepromAddres, BlockStatusBuff, BlockStatusSize);
  
  CycleCounter = (uint32_t)BlockStatusBuff[0] << 16;
  CycleCounter |= (uint32_t)BlockStatusBuff[1] << 8;
  CycleCounter |= (uint32_t)BlockStatusBuff[2];
  
  if(CycleCounter != 0x00FFFFFF) BlockStatus->CycleCounter = CycleCounter;
  else BlockStatus->CycleCounter = 0;  
  
  BlockStatus->CurrIndex = BlockStatus->CycleCounter % FlashBlockQty;
  
  if(BlockStatus->CurrIndex < FlashBlockQty -1) BlockStatus->NextIndex = BlockStatus->CurrIndex +1;
  else BlockStatus->NextIndex = 0;
}

void Eeprom_Write_Header()
{
  uint8_t BlockStatusBuff[EEPROM_HEADER_SIZE];                                                     //Create Buff for write data to eeprom
  
  BlockStatusBuff[0] = (uint8_t)((EepromMemoryType & 0xFF00) >> 8);
  BlockStatusBuff[1] = (uint8_t)(EepromMemoryType & 0x00FF); 
  
  I2C2_Eeprom_Write_Byte(DEVICE_ADDR_EEPROM, EEPROM_ADDR_HEADER, BlockStatusBuff, BlockStatusSize);
}

void Eeprom_Erase()
{
  uint8_t buff[255];
  uint8_t i;
  
  for(i = 0; i < 0xFF; i++) buff[i] = 0xFF;
  
  I2C2_Eeprom_Write(DEVICE_ADDR_EEPROM, EEPROM_ADDR_HEADER, buff, 255);
  //I2C2_Eeprom_Write(uint8_t DevAddr, uint8_t Addr, const uint8_t *pData, uint16_t Size);
  Delay_ms(5);
  //I2C2_Eeprom_Receive(DEVICE_ADDR_EEPROM, 0, buff, 255);
}
