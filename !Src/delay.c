
#include "delay.h"

#define    DWT_CYCCNT    *(volatile unsigned long *)0xE0001004
#define    DWT_CONTROL   *(volatile unsigned long *)0xE0001000
#define    SCB_DEMCR     *(volatile unsigned long *)0xE000EDFC

void Delay_Init()
{ 
  SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;      //разрешаем использовать счётчик
  DWT_CYCCNT  = 0;                              //обнуляем значение счётного регистра  
  DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;        //запускаем счётчик 
}

__STATIC_INLINE uint32_t delta(uint32_t t0, uint32_t t1)
{
  return (t1 - t0); 
}

void Delay_us(uint32_t us)
{
  uint32_t t0 =  DWT->CYCCNT;
  uint32_t us_count_tic =  us * (SystemCoreClock/1000000);
  while (delta(t0, DWT->CYCCNT) < us_count_tic) ;
}

void Delay_ms(uint32_t ms)
{
  uint32_t t0 =  DWT->CYCCNT;
  uint32_t us_count_tic =  ms * (SystemCoreClock/1000);
  while (delta(t0, DWT->CYCCNT) < us_count_tic) ;
}

