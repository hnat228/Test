
#include "dsp_board.h"

#include "stm32f412rx.h"
#include "ParkDSPLite_4x4_ADAU1466.h"
#include "ParkDSPLite_4x4_ADAU1466_PARAM.h"
#include "ParkDSPLite_4x4_ADAU1466_REG.h"
#include "delay.h"
#include "dsp_functions.h"
#include "math.h"
#include "park_dsp_lite_4x4.h"
#include "stdlib.h"

//uint8_t DSP_ERROR = 0;

void DSP_Board_Init()
{
   RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		                // enable clock for port B  
    
  //RST DSP Board GPIO->PB9
  SET(RST_DSP_OUT, RST_DSP);
  GPIOB->MODER &= ~GPIO_MODER_MODE8;		                //Set MODE (mode register) -> Output
  GPIOB->MODER |= GPIO_MODER_MODE8_0;		                //Set MODE (mode register) -> Output
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_8;		                //Set OTYPER -> Push Pull
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR8;		                //Set PUPDR -> Push Pull
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR8;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8_0;		        //Set OSPEEDR (output speed register) -> Medium speed

  CLR(RST_DSP_OUT, RST_DSP);
  Delay_ms(20);
  SET(RST_DSP_OUT, RST_DSP);
  Delay_ms(20);
  
  I2C1_Init();
  
  DeviceStatus.DeviceDsp = (DeviceDSPType)DSP_test();
  Delay_ms(20);
  
  if(DeviceStatus.DeviceDsp != DSP_ERROR)
  {
    default_download_ADAU1466();    
    ADC_Init();  
    DAC_Init();
  }
}

void I2C1_Init()
{
  //Devices -> DSP(ADAU1466 addr = 0x70), ADC(AK5554), DAC(AK4458)
  //I2C1 SCL->PB6, SDA->PB7
  
  //RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
  //RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
  //
  ////I2C1_SCL PB6
  //GPIOB->CRL &= ~GPIO_CRL_CNF6;		                // clear CNF 
  //GPIOB->CRL &= ~GPIO_CRL_MODE6;                        // clear MODE   
  //GPIOB->CRL |= GPIO_CRL_CNF6;                          // set as alternative output open-drain
  //GPIOB->CRL |= GPIO_CRL_MODE6_1;                       // 2 MHz
  //
  ////I2C1_SDA PB7
  //GPIOB->CRL &= ~GPIO_CRL_CNF7;		                // clear CNF 
  //GPIOB->CRL &= ~GPIO_CRL_MODE7;                        // clear MODE   
  //GPIOB->CRL |= GPIO_CRL_CNF7;                          // set as alternative output open-drain
  //GPIOB->CRL |= GPIO_CRL_MODE7_1;                       // 2 MHz

    
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		                // enable clock for port B 
    
  //I2C SCL PB6
  GPIOB->MODER |= GPIO_MODER_MODE6_1;		                //Set MODE (mode register) -> Alternative
  GPIOB->OTYPER |= GPIO_OTYPER_OT_6;		                //Set OTYPER -> open-drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR6;		                //Clear PUPDR -> open-drain
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR6_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[0] |= GPIO_AFRL_AFRL6_2;                           //Set AFR (alternate function) I2C1_SCL
    
  //I2C SDA PB7
  GPIOB->MODER |= GPIO_MODER_MODE7_1;		                //Set MODE (mode register) -> Alternative
  GPIOB->OTYPER |= GPIO_OTYPER_OT_7;		                //Set OTYPER -> open-drain
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR7;		                //Clear PUPDR -> open-drain
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR7_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOB->AFR[0] |= GPIO_AFRL_AFRL7_2;                           //Set AFR (alternate function) I2C1_SDA
  
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;		                // enable clock for I2C 
  I2C1->CR1 &= ~I2C_CR1_PE;                     // diseble i2c
  I2C1->CR2 &= ~I2C_CR2_FREQ;                   // clear CR2 
  I2C1->CCR &= ~I2C_CCR_CCR;                    // clear CCR 
  I2C1->CR2 |= 48;                              // set PCLK1 frequency 
  I2C1->TRISE = 24;                              // set rising time  
  I2C1->CCR |= I2C_CCR_FS;                      // set fast mode
  I2C1->CCR |= 40;                              // set i2c clock frequency 400kHz 
  I2C1->CR1 |= I2C_CR1_PE;                      // enable i2c 
}

void i2c1_start()
{
  I2C1->CR1 |= I2C_CR1_START; 
}

void i2c1_stop()
{
  I2C1->CR1 |= I2C_CR1_STOP;
}

uint8_t i2c_dsp_transmit(uint8_t DevAddr, uint16_t Addr, const uint8_t *pData, uint32_t Size)
{
  uint32_t count;
  
  count = 0;
  i2c1_start();                          //generate start
  while (!(I2C1->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  (void) I2C1->SR1;
  
  count = 0;
  I2C1->DR = DevAddr;                   //send devise adress
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  (void) I2C1->SR1;
  (void) I2C1->SR2;
  
  count = 0;
  I2C1->DR = (Addr & 0xFF00) >> 8;      //send data Type byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  count = 0;
  I2C1->DR = (Addr & 0x00FF);           //send low adress byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }  
  
  while(Size > 0u)                      //send data buffer
  {
    count = 0;
    I2C1->DR = (*pData++);
    Delay_us(1);
    while (!(I2C1->SR1 & I2C_SR1_BTF))
    {
      if(count < I2C_Timeout) count++;
      else return 1;
    }
    Size--;
  }
  
  i2c1_stop();                           //generate stop
  Delay_us(5);
  
  return 0;
}

uint8_t i2c_dsp_transmit_init(uint8_t DevAddr, uint16_t Addr, uint32_t Size)
{
  uint32_t count;
  
  count = 0;
  i2c1_start();                          //generate start
  while (!(I2C1->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  (void) I2C1->SR1;
  
  count = 0;
  I2C1->DR = DevAddr;                   //send devise adress
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  (void) I2C1->SR1;
  (void) I2C1->SR2;
  
  count = 0;
  I2C1->DR = (Addr & 0xFF00) >> 8;      //send data Type byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  count = 0;
  I2C1->DR = (Addr & 0x00FF);           //send low adress byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }  
  
  while(Size > 0u)                      //send data buffer
  {
    count = 0;
    I2C1->DR = 0x00;
    Delay_us(1);
    while (!(I2C1->SR1 & I2C_SR1_BTF))
    {
      if(count < I2C_Timeout) count++;
      else return 1;
    }
    Size--;
  }
  
  i2c1_stop();                           //generate stop
  Delay_us(5);
  
  return 0;
}

uint8_t i2c_dsp_receive(uint8_t DevAddr, uint16_t Addr, uint8_t *pData, uint32_t Size)
{
  uint32_t count;
  
  I2C1->CR1 |= I2C_CR1_ACK;
  
  count = 0;
  i2c1_start();                          //generate start
  while (!(I2C1->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  (void) I2C1->SR1;
  
  count = 0;
  I2C1->DR = DevAddr;                   //send devise adress
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  (void) I2C1->SR1;
  (void) I2C1->SR2;

  count = 0;
  I2C1->DR = (Addr & 0xFF00) >> 8;     //send data Type byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  count = 0;
  I2C1->DR = (Addr & 0x00FF);           //send low adress byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }  

  i2c1_start();                          //generate start
  while (!(I2C1->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  (void) I2C1->SR1;
  
  count = 0;
  I2C1->DR = DevAddr | 0x01;            //send devise adress
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  (void) I2C1->SR1;
  (void) I2C1->SR2;

  while(Size > 0u)                    
  {
    count = 0;
    
    if(Size == 1) 
    {
      I2C1->CR1 &= ~I2C_CR1_ACK;
      i2c1_stop();
    }

    while (!(I2C1->SR1 & I2C_SR1_RXNE))
    {
      if(count < I2C_Timeout) count++;
      else return 1;
    }  
    (*pData++) = I2C1->DR;
    Size--;       
  }     
  
  Delay_us(50);
  return 0;
}

void adau1466_write_block(uint16_t address, const uint8_t *pData, uint32_t Size)
{
  i2c_dsp_transmit(DEVICE_ADDR_ADAU1466, address, pData, Size);
}


/*
void adau1466_write_register(uint16_t address, uint16_t Data)
{
  uint8_t data[2];
  
  data[0] = (uint8_t)((Data & 0xFF00) >> 8);
  data[1] = (uint8_t)(Data & 0x00FF);
  
  i2c_dsp_transmit(DEVICE_ADDR_ADAU1466, address, data, 2);
}
*/

uint8_t DSP_test()
{
  uint8_t data[2];
  
  return i2c_dsp_receive(DEVICE_ADDR_ADAU1466, REG_SOFT_RESET_ADAU1466_ADDR, data, REG_SOFT_RESET_ADAU1466_BYTE);
}

uint8_t i2c_akm_transmit(uint8_t DevAddr, uint8_t Addr, uint8_t Data)
{
  uint32_t count;
  
  count = 0;
  i2c1_start();                         //generate start
  while (!(I2C1->SR1 & I2C_SR1_SB))     //wait until "start" is end
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  
  (void) I2C1->SR1;
  
  count = 0;
  I2C1->DR = DevAddr;                   //send devise adress
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_ADDR))   //wait until adress will be send
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }
  (void) I2C1->SR1;
  (void) I2C1->SR2;
  
  count = 0;
  I2C1->DR = Addr;           //send adress byte
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }  

  count = 0;
  I2C1->DR = Data;
  Delay_us(1);
  while (!(I2C1->SR1 & I2C_SR1_BTF))
  {
    if(count < I2C_Timeout) count++;
    else return 1;
  }

  i2c1_stop();                           //generate stop
  Delay_us(5);
  
  return 0;

}

void DAC_Init()
{
  uint8_t data = 0;
  i2c_akm_transmit(DAC_ADDR, DAC_CR1, data);

  data = 0x0D;
  SET(data, DAC_CR7_TDM_0);
  i2c_akm_transmit(DAC_ADDR, DAC_CR7, data);
  
  data = 0x00;
  SET(data, DAC_CR1_DIF_0);
  SET(data, DAC_CR1_DIF_1);
  i2c_akm_transmit(DAC_ADDR, DAC_CR1, data);

  SET(data, DAC_CR1_RSTN);
  i2c_akm_transmit(DAC_ADDR, DAC_CR1, data);
}

void ADC_Init()
{
  uint8_t data = 0;
  i2c_akm_transmit(ADC_ADDR, ADC_PM2, data);
  
  data = 0;
  SET(data, ADC_CR1_HPFE);
  SET(data, ADC_CR1_DIF0);
  SET(data, ADC_CR1_CKS1);
  i2c_akm_transmit(ADC_ADDR, ADC_CR1, data);
  
  data = 0;
  SET(data, ADC_CR2_TDM0);
  i2c_akm_transmit(ADC_ADDR, ADC_CR2, data);
  
  data = 0;
  SET(data, ADC_PM2_RSTN);
  i2c_akm_transmit(ADC_ADDR, ADC_PM2, data);
}

void Safeload_DSP_Write(DspModuleType* DspModule, double* data, uint8_t size)
{
  #define BuffSize 4
  
  uint8_t Buff[BuffSize];
  uint8_t i = 0;
  
  Set_Memory_Page(DspModule->MemoryPage);
  
  for(i = 0; i < size; i++)
  {            
    Buff[0] = (fixpointConvert(data[i]) & 0xFF000000) >> 24;
    Buff[1] = (fixpointConvert(data[i]) & 0x00FF0000) >> 16;
    Buff[2] = (fixpointConvert(data[i]) & 0x0000FF00) >> 8;
    Buff[3] = (fixpointConvert(data[i]) & 0x000000FF);
    adau1466_write_block(MOD_SAFELOADMODULE_DATA_SAFELOAD0_ADDR + i, Buff, BuffSize);        
  }
  
  Buff[0] = 0x00;
  Buff[1] = 0x00;
  Buff[2] = (DspModule->Address & 0xFF00) >> 8;
  Buff[3] = (DspModule->Address & 0x00FF);
  adau1466_write_block(MOD_SAFELOADMODULE_ADDRESS_SAFELOAD_ADDR, Buff, BuffSize);
  
  //Buff[0] = 0x00;
  //Buff[1] = 0x00;
  //Buff[2] = 0x00;
  //Buff[3] = 0x00;

  Buff[0] = 0x00;
  Buff[1] = 0x00;
  Buff[2] = 0x00;
  Buff[3] = size;
  //adau1466_write_block(MOD_SAFELOADMODULE_NUM_SAFELOAD_LOWER_ADDR, Buff, BuffSize);
  
  //Set_Memory_Page(0);
  if(DspModule->MemoryPage == 0)  adau1466_write_block(MOD_SAFELOADMODULE_NUM_SAFELOAD_LOWER_ADDR, Buff, BuffSize);
  else if(DspModule->MemoryPage == 1)  adau1466_write_block(MOD_SAFELOADMODULE_NUM_SAFELOAD_UPPER_ADDR, Buff, BuffSize);

  Delay_us(100);
}

void Block_Write(DspModuleType DspModule, uint32_t data, uint8_t size)
{
  uint8_t Buff[4];
  
  Buff[0] = (data & 0x0F000000) >> 24;
  Buff[1] = (data & 0x00FF0000) >> 16;
  Buff[2] = (data & 0x0000FF00) >> 8;
  Buff[3] = (data & 0x000000FF);
  
  Set_Memory_Page(DspModule.MemoryPage);
  adau1466_write_block(DspModule.Address, Buff, size);
}

void Block_Write_Coef(DspModuleType DspModule, double coef)
{
  uint8_t Buff[4];
  
  Buff[0] = (fixpointConvert(coef) & 0xFF000000) >> 24;
  Buff[1] = (fixpointConvert(coef) & 0x00FF0000) >> 16;
  Buff[2] = (fixpointConvert(coef) & 0x0000FF00) >> 8;
  Buff[3] = (fixpointConvert(coef) & 0x000000FF);
  
  Set_Memory_Page(DspModule.MemoryPage);
  adau1466_write_block(DspModule.Address, Buff, 4);
}

void Set_Memory_Page(uint8_t Page)
{
  uint8_t Buff[2];
  
  Buff[0] = 0x00;
  Buff[1] = Page;
  adau1466_write_block(REG_SECOND_PAGE_ENABLE_ADAU1466_ADDR, Buff, 2);
}

int32_t fixpointConvert( double value)
{
  return (int32_t)(0x01000000 * value);
}

void DSP_Mute(MuteParamType* Mute)
{
  double tmp;
  
  if(Mute->Value == UNMUTED) 
  {
    tmp = 1.0;
    Safeload_DSP_Write(&Mute->DspModule, &tmp, 1);
    //dsp write block
  }
  else if(Mute->Value == MUTED)
  {
    tmp = 0.0; 
    Safeload_DSP_Write(&Mute->DspModule, &tmp, 1);
  }
  
  ///!!!!!!!!!!!!!
  Set_Memory_Page(0);
}

void DSP_Phase(PolarityParamType* Polarity)
{
  uint8_t polarity = 0;

  if(Polarity->Value == NORMAL) polarity = 0;
  else if(Polarity->Value == INVERSE) polarity = 1;
  
  Block_Write(Polarity->DspModule, polarity, 4);
}

void DSP_Gain(GainParamType* Gain)
{
  double tmp = gainliner((double)(Gain->Value/100));
  Safeload_DSP_Write(&Gain->DspModule, &tmp, 1);
}

//void DSP_Gain_Test(void* Gain)
//{
//  GainDspType *gain = (GainDspType*)Gain;
//  double tmp = gainliner((double)(gain->Value/100));
//  Safeload_DSP_Write(&gain->DspModule, &tmp, 1);
//  
//  //return Gain;
//}

void DSP_Matrix_Value(MatrixType* matrix)
{
 double tmp = 0;
 
 if(matrix->Mute != MUTED) tmp = gainliner((double)(matrix->Gain/MatrixGain_Coef)); 
 else tmp = 0.0;
 
 Safeload_DSP_Write(&matrix->DspModule, &tmp, 1);
}

void DSP_Delay(DelayParamType* Delay)
{
  uint32_t samples = (Delay->Value/100) *(uint32_t)DSP_FS;
  
  if(samples == 0) samples = 1;
  
  Block_Write(Delay->DspModule, samples, 4);
}

void DSP_EQ_Filter(FilterEQParamType* Filter)
{
  double* coef;
  
  if(Filter->State == ON)
  {
    coef = parametric((double)(Filter->Boost), Filter->Freq, 0, Filter->Q);
    Safeload_DSP_Write(&Filter->DspModule, coef, 5);
  }
  else if(Filter->State == OFF)
  {
    double coef[5] = {0, 0, 1.0, 0, 0};
    Safeload_DSP_Write(&Filter->DspModule, coef, 5);
  }
}

void DSP_Filter(FilterEQParamType* Filter)
{
  double* coef;
  
  if(Filter->DspModule.Address == 0) return;

  if(Filter->State == ON)
  {
    switch(Filter->Filter)
    {
      case Peaking : coef = parametric((double)(Filter->Boost /EQ_Boost_Coef), Filter->Freq, 0.0, ((double)Filter->Q) /EQ_Q_Coef); break;
      case LowPass : coef =  general_lowpass(Filter->Freq, 0, ((double)Filter->Q) /EQ_Q_Coef); break; // butt_2nd_lowpass(Filter->Freq, 0.0); break; 
      case HighPass : coef = general_highpass(Filter->Freq, 0, ((double)Filter->Q) /EQ_Q_Coef); break; // butt_2nd_highpass(Filter->Freq, 0.0); break;
      case LowShelf : coef = shelving_lowpass((double)Filter->Boost /EQ_Boost_Coef, Filter->Freq, 0.0, (double)Filter->Q /EQ_Q_Coef); break;
      case HighShelf : coef = shelving_highpass((double)Filter->Boost /EQ_Boost_Coef, Filter->Freq, 0.0, (double)Filter->Q /EQ_Q_Coef); break; break;
      case AllPass : break;  
    }
    
  }
  else if(Filter->State == OFF) coef = parametric(0.0, 100, 0.0, 0.7);
  
  Safeload_DSP_Write(&Filter->DspModule, coef, 5);
}

void DSP_HP_Filter(FilterHPParamType* Filter)
{
  double* coef;
  uint8_t i;
  
  if(Filter->DspModule1.Address == 0) return;
  if(Filter->DspModule2.Address == 0) return;
  if(Filter->DspModule3.Address == 0) return;
  if(Filter->DspModule4.Address == 0) return;
  
  if(Filter->State == ON)
  {
    switch(Filter->Filter)
    {    
      case Butterworth_12:
      {
        coef = butt_2nd_highpass(Filter->Freq, 0.0); 
        Safeload_DSP_Write(&Filter->DspModule1, coef, 5);
        coef = parametric(0.0, 100, 0.0, 0.7);
        for(i = 1; i < USREQ_HP_QTY; i++) Safeload_DSP_Write(&Filter->DspModule1 +i, coef, 5);
        break; 
      }
      case Butterworth_18: 
      {
        coef = butt_higherorder_highpass(Filter->Freq, 0, 3, 0); 
        Safeload_DSP_Write(&Filter->DspModule1, coef, 5);
        coef = butt_1st_highpass_5coef(Filter->Freq, 0.0); 
        Safeload_DSP_Write(&Filter->DspModule2, coef, 5);
        coef = parametric(0.0, 100, 0.0, 0.7);
        for(i = 2; i < USREQ_HP_QTY; i++) Safeload_DSP_Write(&Filter->DspModule1 +i, coef, 5);
        break;
      }
      case Butterworth_24: 
      {
        coef = butt_higherorder_highpass(Filter->Freq, 0, 4, 0);
        Safeload_DSP_Write(&Filter->DspModule1, coef, 5);
        coef = butt_higherorder_highpass(Filter->Freq, 0, 4, 1);  
        Safeload_DSP_Write(&Filter->DspModule2, coef, 5);
        coef = parametric(0.0, 100, 0.0, 0.7);
        for(i = 2; i < USREQ_HP_QTY; i++) Safeload_DSP_Write(&Filter->DspModule1 +i, coef, 5);
        break;
      }
      default :
      {
        coef = parametric(0.0, 100, 0.0, 0.7);
        for(i = 0; i < USREQ_HP_QTY; i++) Safeload_DSP_Write(&Filter->DspModule1 +i, coef, 5);
      }
    }
  }
  else if(Filter->State == OFF) 
  {
    coef = parametric(0.0, 100, 0.0, 0.7);
    for(i = 0; i < USREQ_HP_QTY; i++) Safeload_DSP_Write(&Filter->DspModule1 +i, coef, 5);
  }
}

//void DSP_Switch(SwithType* Switch)
//{
//  double tmp[2];
//  
//  if(Switch->State == ON)
//  {
//    tmp[0] = 1.0;
//    tmp[1] = 0.0;
//    
//    Safeload_DSP_Write(&Switch->DspModule, tmp, 2);
//  }
//  else if(Switch->State == OFF)
//  {
//    tmp[0] = 0.0;
//    tmp[1] = 1.0;
//    
//    Safeload_DSP_Write(&Switch->DspModule, tmp, 2);
//  }
//}

/*
void DSP_XLR_Switch(ModeXLRParamType* XLR_switch)
{
  double tmp[2];
  
  if(XLR_switch->State == XLR_ANALOG)
  {
    tmp[0] = 1.0;
    tmp[1] = 0.0;
    
    Safeload_DSP_Write(XLR_switch->Address, tmp, 2);
  }
  else if(XLR_switch->State == XLR_AES)
  {
    tmp[0] = 0.0;
    tmp[1] = 1.0;
    
    Safeload_DSP_Write(XLR_switch->Address, tmp, 2);
  }
}
*/

void DSP_Link_Out_Matrix(LinkMatrixParamType* link_matrix)
{
  double tmp;
  uint8_t out;
  
  for(out = 0; out < OUT_CH_QTY; out++)
  {
    tmp = link_matrix[out].Out;
    tmp = tmp / 0x01000000;
    Safeload_DSP_Write(&link_matrix[out].DspModule, &tmp, 1);
  }
}

void DSP_Read_Data(DspModuleType* DspModule, uint8_t* Data, uint8_t size)
{  
  Set_Memory_Page(DspModule->MemoryPage);
  i2c_dsp_receive(DEVICE_ADDR_ADAU1466, DspModule->Address, Data, size);
}

double Calculate_Limiter(LimiterRMSParamType* Limiter)
{
  double Threshold_dB = 0;
  
  Threshold_dB = (20 * (log10((((double)(Limiter->Threshold.Value / (double)Limiter->Threshold.ValueCoef)) *KU_IN_DSP) / KU_AMP)));
  
  return Threshold_dB;
}

double Calculate_Peak_Limiter(LimiterPeakParamType* Limiter)
{
  double Threshold_dB = 0;
  
  Threshold_dB = (20 * (log10((((double)(Limiter->Threshold.Value / (double)Limiter->Threshold.ValueCoef)) *KU_IN_DSP) / KU_AMP)));
  
  return Threshold_dB;
}

void DSP_Limiter(LimiterRMSParamType* Limiter)
{
  //uint8_t i;
  //double Threshold_X[54];                                                               //Limiter X points 
  //double Threshold_Y[54];                                                               //Limiter Y points
  //double Threshold_Coef[54];                                                            //Limiter DSP coefficient 
  //double Threshold_Tmp1 = 0;                                                            //Temp buff for calculate limiter dsp coef
  double Threshold_DSP_dB = 0;                                                          //Threshold in dB for calculate limiter, conversion from volts to dB
  double coef;                                                                          //DSP coef for attack, release, hold
  
  /*
  //DSP Limiter Threshold write
  if(Limiter->Threshold.Value != Limiter->Threshold.ValuePrev)                          //if changed limiter threshold param -> changed coef  
  {
    Threshold_X[0] = 21;
    for(i = 0; i < 54; i++) Threshold_Coef[i] = 1.0;                                    //Init coef buff which will write to dsp
    for(i = 1; i < 54; i++) Threshold_X[i] = Threshold_X[i - 1] -3;                     //Init Limiter X point from 21dB to -135dB
      
    Threshold_DSP_dB = Calculate_Limiter(Limiter);                                      //Conversion from threshold volts to threshold dB 
    
    for(i = 0; i < 54; i++)
    {
      if(Threshold_DSP_dB < Threshold_X[i]) Threshold_Y[i] = Threshold_DSP_dB;
      else Threshold_Y[i] = Threshold_X[i];
      
      Threshold_Tmp1 = Threshold_Y[i] - Threshold_X[i];
      
      Threshold_Coef[53-i] = gainliner(Threshold_Tmp1);
    }
    
    for(i = 0; i <= 50; i+=5)
    {
      if(i != 50) Safeload_DSP_Write(Limiter->Threshold.Address +i, Threshold_Coef +i, 5);
      else Safeload_DSP_Write(Limiter->Threshold.Address +i, Threshold_Coef +i, 4);
    }
    
    Limiter->Threshold.ValuePrev = Limiter->Threshold.Value;
  }
  */
  
  //DSP Limiter Threshold write
  if(Limiter->Threshold.Value != Limiter->Threshold.ValuePrev)                          //if changed limiter threshold param -> changed coef  
  {
  Threshold_DSP_dB = REDUCE_LIMITER + (Calculate_Limiter(Limiter) *-1);               //Conversion from threshold volts to threshold dB 
  coef = gainliner(Threshold_DSP_dB);
  Safeload_DSP_Write(&Limiter->Threshold.DspModule, &coef, 1);
  Limiter->Threshold.ValuePrev = Limiter->Threshold.Value;
  }
  
  //DSP Limiter Hold write
  if(Limiter->Hold.Value != Limiter->Hold.ValuePrev)
  {
    coef = limiter_hold((double)Limiter->Hold.Value / Limiter->Hold.ValueCoef);
    Block_Write_Coef(Limiter->Hold.DspModule, coef);
    Limiter->Hold.ValuePrev = Limiter->Hold.Value;
  }
  
  //DSP Limiter Attack time
  if(Limiter->Attack.Value != Limiter->Attack.ValuePrev)
  {
    coef = limiter_rms_tc(((double)Limiter->Attack.Value / Limiter->Attack.ValueCoef));
    Block_Write_Coef(Limiter->Attack.DspModule, coef);
    Limiter->Attack.ValuePrev = Limiter->Attack.Value;
  }
  
  //DSP Limiter Release time
  if(Limiter->Release.Value != Limiter->Release.ValuePrev)
  {
    coef = limiter_decay((double)Limiter->Release.Value / Limiter->Release.ValueCoef);
    Block_Write_Coef(Limiter->Release.DspModule, coef);
    Limiter->Release.ValuePrev = Limiter->Release.Value;  
  }
  
  //DSP Limiter Swich write
  if(Limiter->Switch.State != Limiter->Switch.PrevState)                                //if limiter ON/OFF -> changed switch state
  {
    Limiter->Switch.PrevState = Limiter->Switch.State;
    
    if(Limiter->Switch.State == ON) 
    {
      coef = 1.0;
      Safeload_DSP_Write(&Limiter->Switch.DspModule, &coef, 1);
      //dsp write block
    }
    else if(Limiter->Switch.State == OFF)
    {
      coef = 0.0;
      Safeload_DSP_Write(&Limiter->Switch.DspModule, &coef, 1);
    }        
  }
}


void DSP_Peak_Limiter(LimiterPeakParamType* Limiter)
{
  //uint8_t i;
  //double Threshold_X[54];                                                               //Limiter X points 
  //double Threshold_Y[54];                                                               //Limiter Y points
  //double Threshold_Coef[54];                                                            //Limiter DSP coefficient 
  //double Threshold_Tmp1 = 0;                                                            //Temp buff for calculate limiter dsp coef
  double Threshold_DSP_dB = 0;                                                          //Threshold in dB for calculate limiter, conversion from volts to dB
  double coef;                                                                          //DSP coef for attack, release, hold
  
  /*
  //DSP Limiter Threshold write
  if(Limiter->Threshold.Value != Limiter->Threshold.ValuePrev)                          //if changed limiter threshold param -> changed coef  
  {
    Threshold_X[0] = 21;
    for(i = 0; i < 54; i++) Threshold_Coef[i] = 1.0;                                    //Init coef buff which will write to dsp
    for(i = 1; i < 54; i++) Threshold_X[i] = Threshold_X[i - 1] -3;                     //Init Limiter X point from 21dB to -135dB
      
    Threshold_DSP_dB = Calculate_Limiter(Limiter);                                      //Conversion from threshold volts to threshold dB 
    
    for(i = 0; i < 54; i++)
    {
      if(Threshold_DSP_dB < Threshold_X[i]) Threshold_Y[i] = Threshold_DSP_dB;
      else Threshold_Y[i] = Threshold_X[i];
      
      Threshold_Tmp1 = Threshold_Y[i] - Threshold_X[i];
      
      Threshold_Coef[53-i] = gainliner(Threshold_Tmp1);
    }
    
    for(i = 0; i <= 50; i+=5)
    {
      if(i != 50) Safeload_DSP_Write(Limiter->Threshold.Address +i, Threshold_Coef +i, 5);
      else Safeload_DSP_Write(Limiter->Threshold.Address +i, Threshold_Coef +i, 4);
    }
    
    Limiter->Threshold.ValuePrev = Limiter->Threshold.Value;
  }
  */
  
  //DSP Limiter Threshold write
  if(Limiter->Threshold.Value != Limiter->Threshold.ValuePrev)                          //if changed limiter threshold param -> changed coef  
  {
  Threshold_DSP_dB = REDUCE_LIMITER + (Calculate_Peak_Limiter(Limiter) *-1);               //Conversion from threshold volts to threshold dB 
  coef = gainliner(Threshold_DSP_dB);
  Safeload_DSP_Write(&Limiter->Threshold.DspModule, &coef, 1);
  
  Limiter->Threshold.ValuePrev = Limiter->Threshold.Value;
  }
  
  //DSP Limiter Hold write
  if(Limiter->Hold.Value != Limiter->Hold.ValuePrev)
  {
    coef = limiter_hold((double)Limiter->Hold.Value / Limiter->Hold.ValueCoef);
    Block_Write_Coef(Limiter->Hold.DspModule, coef);
    Limiter->Hold.ValuePrev = Limiter->Hold.Value;
  }
  
  //DSP Limiter Attack time
  //if(Limiter->Attack.Value != Limiter->Attack.ValuePrev)
  //{
  //  coef = limiter_rms_tc(((double)Limiter->Attack.Value / Limiter->Attack.ValueCoef));
  //  Safeload_DSP_Write(&Limiter->Attack.DspModule, &coef, 1);
  //  Limiter->Attack.ValuePrev = Limiter->Attack.Value;
  //}
  
  //DSP Limiter Release time
  if(Limiter->Release.Value != Limiter->Release.ValuePrev)
  {
    coef = limiter_decay((double)Limiter->Release.Value / Limiter->Release.ValueCoef);
    Block_Write_Coef(Limiter->Release.DspModule, coef);
    Limiter->Release.ValuePrev = Limiter->Release.Value;
  }
  
  //DSP Limiter Swich write
  if(Limiter->Switch.State != Limiter->Switch.PrevState)                                //if limiter ON/OFF -> changed switch state
  {
    Limiter->Switch.PrevState = Limiter->Switch.State;
    
    if(Limiter->Switch.State == ON) 
    {
      coef = 1.0;
      Safeload_DSP_Write(&Limiter->Switch.DspModule, &coef, 1);
      //dsp write block
    }
    else if(Limiter->Switch.State == OFF)
    {
      coef = 0.0;
      Safeload_DSP_Write(&Limiter->Switch.DspModule, &coef, 1);
    }        
  }
}

void DSP_Bridge(BridgeParamType* out_bridge)
{
  uint8_t bridge = 1;
  uint8_t clip = 0;
  
  if(out_bridge->State == BRIDGE) 
  {
    bridge = 0;
    clip = 1;
  }
  else if(out_bridge->State == UNBRIDGE)
  {
    bridge = 1;
    clip = 0;
  }

  Block_Write(out_bridge->ClipSwitch.DspModule, clip, 4);
  Block_Write(out_bridge->BridgeSwitch.DspModule, bridge, 4);
}

void DSP_Source_Select(InputSourceType* input_source, AmpInputValueType* amp_input)
{
  double tmp;

  if(*amp_input != IN_NONE)
  {
    switch(*amp_input)
    {
      case ANALOG_INPUT_1 : tmp = 0.0; break;
      case ANALOG_INPUT_2 : tmp = 1.0; break;
      case ANALOG_INPUT_3 : tmp = 2.0; break;
      case ANALOG_INPUT_4 : tmp = 3.0; break;
      case AES_INPUT_1 : tmp = 4.0; break;
      case AES_INPUT_2 : tmp = 5.0; break;
      case AES_INPUT_3 : tmp = 6.0; break;
      case AES_INPUT_4 : tmp = 7.0; break;
    }

    tmp = tmp / 0x01000000;
    Safeload_DSP_Write(&input_source->DspModuleSelector, &tmp, 1);
    
    tmp = 1.0;
    Safeload_DSP_Write(&input_source->DspModuleMute, &tmp, 1);
  }
  else if(*amp_input == IN_NONE)
  {
    tmp = 0.0;
    Safeload_DSP_Write(&input_source->DspModuleMute, &tmp, 1);
  }
}

void DSP_Speaker_Write(SpeakerType* Speaker, uint8_t ways)
{
  uint8_t i;
  
  for(i = 0; i < ways; i++) DSP_Speaker_OUT_Write(Speaker->OUT[i]);
}

void DSP_SpeakerEQ_Write(SpeakerEQType* eq)
{
  uint8_t i;
  for(i = 0; i < SPKEQ_EQ_QTY; i++) DSP_Filter(&eq->FilterEQ[i]);
}

void DSP_Speaker_OUT_Write(SpeakerOutType* speaker_out)
{
  uint8_t i;
  DSP_Delay(&speaker_out->Delay);
  DSP_Gain(&speaker_out->Gain);
  DSP_Mute(&speaker_out->Mute);
  DSP_Phase(&speaker_out->Polarity);
  
  //HP filter
  for(i = 0; i < SPKOUT_EQ_QTY; i++) DSP_Filter(&speaker_out->FilterEQ[i]);
  //LP filter
  //FIR filter
  DSP_Limiter(&speaker_out->RMSLimiter);
  //Peak DSP_Limiter(LimiterRMSParamType* Limiter);
}

void DSP_UserEQ_Write(UserEQType* UserEQ)
{
  uint8_t i;
  
  DSP_Delay(&UserEQ->Delay);
  DSP_Gain(&UserEQ->Gain);
  DSP_Mute(&UserEQ->Mute);
  DSP_Phase(&UserEQ->Polarity);
  for(i = 0; i < SPKOUT_EQ_QTY; i++) DSP_Filter(&UserEQ->FilterEQ[i]);
  //FilterHighPass;
  //FilterLowPass;
}

void DSP_PresetParam_Write(PresetParamType* PresetParam)
{
  //DSP_Gain(&PresetParam->AnalogTrim.Gain);
  //DSP_Delay(&PresetParam->AnalogTrim.Delay);
  //DSP_Gain(&PresetParam->AESTrim.Gain);
  //DSP_Delay(&PresetParam->AESTrim.Delay);
  
  //XLR_Mode(&PRESET_PARAM.ModeXLR[0], 0);
  //XLR_Mode(&PRESET_PARAM.ModeXLR[1], 1);
    
 //DSP_Bridge(&PresetParam->Bridge[0]);
 //DSP_Bridge(&PresetParam->Bridge[1]);
  
  //DSP_Source_Select(&PresetParam->InputSource[0], &PresetParam->InputSource[0].ManualSource);
  //DSP_Source_Select(&PresetParam->InputSource[1], &PresetParam->InputSource[1].ManualSource);
  //DSP_Source_Select(&PresetParam->InputSource[2], &PresetParam->InputSource[2].ManualSource);
  //DSP_Source_Select(&PresetParam->InputSource[3], &PresetParam->InputSource[3].ManualSource);

  //DSP_Gain(GainParamType* Gain);
  //DSP_Matrix_Value(&PresetParam->Matrix[0][0]);
  //DSP_Matrix_Value(&PresetParam->Matrix[0][1]);
  //DSP_Matrix_Value(&PresetParam->Matrix[0][2]);
  //DSP_Matrix_Value(&PresetParam->Matrix[0][3]);
  //DSP_Matrix_Value(&PresetParam->Matrix[1][0]);
  //DSP_Matrix_Value(&PresetParam->Matrix[1][1]);
  //DSP_Matrix_Value(&PresetParam->Matrix[1][2]);
  //DSP_Matrix_Value(&PresetParam->Matrix[1][3]);
  //DSP_Matrix_Value(&PresetParam->Matrix[2][0]);
  //DSP_Matrix_Value(&PresetParam->Matrix[2][1]);
  //DSP_Matrix_Value(&PresetParam->Matrix[2][2]);
  //DSP_Matrix_Value(&PresetParam->Matrix[2][3]);
  //DSP_Matrix_Value(&PresetParam->Matrix[3][0]);
  //DSP_Matrix_Value(&PresetParam->Matrix[3][1]);
  //DSP_Matrix_Value(&PresetParam->Matrix[3][2]);
  //DSP_Matrix_Value(&PresetParam->Matrix[3][3]);
  
}
//void DSP_SpeakerEQ_Write()
//{
//  
//}
               