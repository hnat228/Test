
#include "stm32f4xx.h"
#include "crc.h"

void CRC_Init()
{
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
  CRC->CR |= CRC_CR_RESET; 
}

void CRC_Reset()
{
  CRC->CR |= CRC_CR_RESET; 
}

uint32_t CRC_Calc(uint8_t* buff, uint32_t count)
{
  uint32_t* pBuffer = (uint32_t*)buff;
  count = count / 4;
  
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
  CRC->CR |= CRC_CR_RESET; 

  while(count > 0)
  {
    CRC->DR = *(pBuffer++);
    count--;
  }
  
  return (CRC->DR);
}

bool CRC_Calc_4kB_Free(uint8_t* buff)
{
  bool error = false;
  uint32_t size = _BLOCK_4k / 4;
  uint32_t* pBuffer = (uint32_t*)buff;
  
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
  CRC->CR |= CRC_CR_RESET; 

  while(size > 0)
  {
    CRC->DR = *(pBuffer++);
    size--;
  }
  
  if(_CRC_4k != CRC->DR) error = true;
  
  return error;
}

bool CRC_Calc_32kB_Free(uint8_t* buff)
{
  bool error = false;
  uint32_t size = _BLOCK_32k / 4;
  uint32_t* pBuffer = (uint32_t*)buff;
  
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
  CRC->CR |= CRC_CR_RESET; 

  while(size > 0)
  {
    CRC->DR = *(pBuffer++);
    size--;
  }
  
  if(_CRC_32k != CRC->DR) error = true;
  
  return error;
}

bool CRC_Calc_64kB_Free(uint8_t* buff)
{
  bool error = false;
  uint32_t size = _BLOCK_64k / 4;
  uint32_t* pBuffer = (uint32_t*)buff;
  
  RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
  CRC->CR |= CRC_CR_RESET; 

  while(size > 0)
  {
    CRC->DR = *(pBuffer++);
    size--;
  }
  
  if(_CRC_64k != CRC->DR) error = true;
  
  return error;
}