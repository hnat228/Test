
#include "array.h"

void arrcopy(uint8_t* dest, uint8_t* src, uint16_t size)
{
  uint16_t i = 0;
  
  for(i = 0; i < size; i++)
  {
    dest[i] = src[i];
  }
}

void arrinit(uint8_t* src, uint32_t size, uint8_t value)
{
  uint32_t i = 0;
  
  for(i = 0; i < size; i++)
  {
    src[i] = value;
  }
}

void strcopy(uint8_t* dest, uint8_t* src)
{
  uint8_t i = 0;
  
  while ('\0' != src[i])
  {
    dest[i] = src[i];		
    i++;
  }
}

