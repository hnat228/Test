
#include "dataconverter.h"
#include "stm32f4xx.h"

void Uint16ToByteArray(uint16_t data, uint8_t* buff, uint32_t* counter)
{
  buff[(*counter)++] = (uint8_t)((data & 0xFF00) >> 8);
  buff[(*counter)++] = (uint8_t)(data & 0x00FF);
}

void Int16ToByteArray(int16_t data, uint8_t* buff, uint32_t* counter)
{
  buff[(*counter)++] = (uint8_t)((data & 0xFF00) >> 8);
  buff[(*counter)++] = (uint8_t)(data & 0x00FF);
}

void Uint32ToByteArray(uint32_t data, uint8_t* buff, uint32_t* counter)
{
  buff[(*counter)++] = (uint8_t)((data & 0xFF000000) >> 24);
  buff[(*counter)++] = (uint8_t)((data & 0x00FF0000) >> 16);
  buff[(*counter)++] = (uint8_t)((data & 0x0000FF00) >> 8);
  buff[(*counter)++] = (uint8_t)(data & 0x000000FF);
}

void Int32ToByteArray(int32_t data, uint8_t* buff, uint32_t* counter)
{
  buff[(*counter)++] = (uint8_t)((data & 0xFF000000) >> 24);
  buff[(*counter)++] = (uint8_t)((data & 0x00FF0000) >> 16);
  buff[(*counter)++] = (uint8_t)((data & 0x0000FF00) >> 8);
  buff[(*counter)++] = (uint8_t)(data & 0x000000FF);
}

void CRC32ToByteArray(uint32_t data, uint8_t* buff, uint32_t* counter)
{
  buff[(*counter)++] = (uint8_t)(data & 0x000000FF);
  buff[(*counter)++] = (uint8_t)((data & 0x0000FF00) >> 8);
  buff[(*counter)++] = (uint8_t)((data & 0x00FF0000) >> 16);
  buff[(*counter)++] = (uint8_t)((data & 0xFF000000) >> 24);
}

void DoubleToByteArray(double data, uint8_t* buff, uint32_t* counter)
{
  uint8_t i = 0;
  
  for(i = 0; i < 8; i++)
  {
    buff[(*counter)++] = ((uint8_t*)&data)[i];
  }
}

void DMAmem2mem(uint8_t* src, uint8_t* dest, uint16_t size)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
  //DMA
  DMA2_Stream0->PAR = (uint32_t) src;
  DMA2_Stream0->M0AR = (uint32_t) dest;
  DMA2_Stream0->NDTR = (uint32_t) size;
  
  DMA2_Stream0->CR &= ~DMA_SxCR_DIR;
  DMA2_Stream0->CR |= DMA_SxCR_DIR_1;
  DMA2_Stream0->CR |= DMA_SxCR_MINC;
  DMA2_Stream0->CR |= DMA_SxCR_PINC;
  
  DMA2_Stream0->CR &= ~DMA_SxCR_MSIZE;
  DMA2_Stream0->CR |= DMA_SxCR_MSIZE_1;
  
  DMA2_Stream0->CR &= ~DMA_SxCR_PSIZE;
  DMA2_Stream0->CR |= DMA_SxCR_PSIZE_1;
  
  DMA2_Stream0->CR |= DMA_SxCR_EN;
  
  while(!(DMA2->LISR & DMA_LISR_TCIF0));
}

uint16_t ByteArrayToUint16(uint8_t* buff, uint32_t* counter)
{
  uint16_t data = 0;
  
  data = (uint16_t)buff[(*counter)++] << 8;
  data |= buff[(*counter)++];
  
  return data;
}

int16_t ByteArrayToInt16(uint8_t* buff, uint32_t* counter)
{
  int16_t data = 0;
  
  data = (int16_t)buff[(*counter)++] << 8;
  data |= buff[(*counter)++];
  
  return data;
}

uint32_t ByteArrayToUint32(uint8_t* buff, uint32_t* counter)
{
  uint32_t data = 0;
  
  data = (uint32_t)buff[(*counter)++] <<24;
  data |= (uint32_t)buff[(*counter)++] << 16;
  data |= (uint32_t)buff[(*counter)++] << 8;
  data |= buff[(*counter)++];
  
  return data;
}

int32_t ByteArrayToInt32(uint8_t* buff, uint32_t* counter)
{
  int32_t data = 0;
  
  data = (int32_t)buff[(*counter)++] <<24;
  data |= (int32_t)buff[(*counter)++] << 16;
  data |= (int32_t)buff[(*counter)++] << 8;
  data |= buff[(*counter)++];
  
  return data;
}

double ByteArrayToDouble(uint8_t* buff, uint32_t* counter)
{
  uint8_t i = 0;
  double data = 0;

  for(i = 0; i < 8; i++)
  {
    ((uint8_t*)&data)[i] = buff[(*counter)++];
  }
  
  return data;
}

uint32_t DoubleToDspFixpoint(double value)
{
  return (uint32_t)(0x01000000 * value);
}

double DspFixpointToDouble(uint32_t value)
{
  double a = (double)value;
  return (a / 0x01000000);
}

