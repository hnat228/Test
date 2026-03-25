
#include "front_board.h"
//#include "stm32f1xx.h"
#include "amp_application.h"
#include "lcd.h"
#include "spi.h"
#include "menu.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"

uint16_t EncSW_counter = 0;
uint16_t BUT_counter = 0;
uint8_t Encoder_Timeout;
uint8_t cursor = 0;


uint8_t LinePosition = 1;
uint8_t MenuLevel = 0;
uint8_t SpeakerPosition = 0;

static void Menu_Hints(menuItem* menu);

void Front_Board_Init()
{  
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;		                // enable clock for port C  
    
  //OE_Front_Board PB3
  CLR(OE_FB_OUT, OE_FB);                                        //Output Enable OFF                                     
  GPIOB->MODER &= ~GPIO_MODER_MODE3;		                //Clear MODE (mode register) 
  GPIOB->MODER |= GPIO_MODER_MODE3_0;		                //Set MODE (mode register) -> Alternate
  GPIOB->OTYPER &= ~GPIO_OTYPER_OT_3;		                //Set OTYPER -> Push Pull
  GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR3;		                //Set PUPDR -> Push Pull
  GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR3;		        //Clear OSPEEDR (output speed register) 
  GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3_0;		        //Set OSPEEDR (output speed register) -> Medium speed
   
  SPI3_Init();
  
  //Led Pre_Init
  LED_Clear();
  LCD_Pre_Init();                                               //Clear data on display for first load
    
  SET(OE_FB_OUT, OE_FB);                                        //Output Enable ON
  LCD_Init();
  LED_Write(0x0000);                                            //Led Init
  
  //uint8_t i;
  //
  //for(i = 0; i < 200; i++)
  //{
  //  if(i % 2)
  //  {
  //    Menu_USB();
  //    Screen_Menu_USB();
  //  }
  //  else
  //  {
  //    Menu_Init();
  //    Screen_Menu(0);
  //  }
  //  
  //  Delay_ms(500);
  //}
  
  Encoder_Init();
}

void Encoder_Init()
{
  //Timer_1, ENC_A->TIM1_CH1->PA8, ENC_B->TIM1_CH2->PA9, ENC_SW->PC9  
  
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;		                // enable clock for port A
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;		                // enable clock for port C
  
  //ENC_A->TIM1_CH1->PA8
  GPIOA->MODER |= GPIO_MODER_MODE8_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOA->AFR[1] |= GPIO_AFRH_AFRH0_0;                           //Set AFR (alternate function)
  //ENC_B->TIM1_CH2->PA9
  GPIOA->MODER |= GPIO_MODER_MODE9_1;		                //Set MODE (mode register) -> Alternate function mode push-pull
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  GPIOA->AFR[1] |= GPIO_AFRH_AFRH1_0;                           //Set AFR (alternate function)
  //ENC_SW->PC9
  GPIOC->MODER &= ~GPIO_MODER_MODE9_1;		                //Set MODE (mode register) -> input floating
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  
  TIM1->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0; 
  TIM1->CCER = TIM_CCER_CC1P | TIM_CCER_CC2P;
  TIM1->SMCR = TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;
  TIM1->CNT = EncoderTIM_CNT;
  TIM1->ARR = 2000;
  TIM1->CR1 = TIM_CR1_CEN;
}

uint8_t Encoder_Scan(ControlStateType* encoder)
{
  encoder->Comand = NO;
  
  if(TIM1->CNT >= (EncoderTIM_CNT + 4))
  {
    encoder->Counter = ((TIM1->CNT - EncoderTIM_CNT) / 4);
    
    if(((TIM1->CNT - EncoderTIM_CNT) / 4) > 0) encoder->Comand = DEC;
    TIM1->CNT = EncoderTIM_CNT;
  }
  else if(TIM1->CNT <= (EncoderTIM_CNT - 4))
  {
    encoder->Counter = ((EncoderTIM_CNT - TIM1->CNT) / 4);
    
    if(((EncoderTIM_CNT - TIM1->CNT) / 4) > 0) encoder->Comand = INC;
    TIM1->CNT = EncoderTIM_CNT;
  }
  else if(!GET(ENC_SW_IN, ENC_SW) && (EncSW_counter < EncSW_Timeout))
  {
    EncSW_counter++;
    if(EncSW_counter >= EncSW_Timeout)
    {
      encoder->Comand = ESC;
      encoder->Counter = EncSW_counter;
    }
  }
  else encoder->Counter = ((EncoderTIM_CNT - TIM1->CNT) / 4);
    
  if(GET(ENC_SW_IN, ENC_SW) && (EncSW_counter > 0))
  {
    if((EncSW_counter < EncSW_Timeout))
    {
      encoder->Comand = ENTER;
      encoder->Counter = EncSW_counter;
    }
    EncSW_counter = 0;
  }
  
  return encoder->Comand;
}

uint8_t Button_Scan(ControlStateType* button)
{
  uint8_t data = 0;

  CS_Enable(IC_BUTTON);
  CS_Disable(IC_BUTTON);
  
  data = (SPI3_Transmit(0xB5));
  
  if(data != 0x0F)
  {
    BUT_counter++;
    button->Counter = data;
  }
  else
  {
    if(BUT_counter > But_Timeout) button->Comand = BUTTON; 
    else button->Comand = NO; 

    BUT_counter = 0;
  }
  
  return button->Comand;
}

void Screen_Menu(ControlStateType* control)
{
  uint8_t size = 0;

  if(selectedMenuItem->Param->ParamType == MAIN_MENU)
  {
    lcd_clrscr();
    lcd_goto(LCD_1st_LINE, 2u);
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}
  }
  else
  {
    switch(control->Comand)
    {
      case ENTER : Screen_Menu_Enter(); return;
      case INC : Screen_Menu_DEC(); return;
      case DEC : Screen_Menu_INC(); return;
      case ESC : Screen_Menu_Enter(); return;
    }
  }
}

void Screen_Menu_Enter()
{
  uint8_t size = 0;
  
  if((previousMenuItem == &NULL_ENTRY) | (LinePosition == 1))
  {
    lcd_goto(LCD_1st_LINE, 0u);
    lcd_putc('>'); 
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    Menu_Hints(selectedMenuItem);
    //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}
  
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc(' '); 
    lcd_puts((void*)&nextMenuItem->Param->ParamName);
    Menu_Hints(nextMenuItem);
    //size = (16 - sizeof(&nextMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //} 
  }
  else if((nextMenuItem == &NULL_ENTRY))
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('>'); 
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    Menu_Hints(selectedMenuItem);
    //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}
    
    lcd_goto(LCD_1st_LINE, 0u);
    lcd_putc(' '); 
    lcd_puts((void*)&previousMenuItem->Param->ParamName);
    Menu_Hints(previousMenuItem);
    //size = (16 - sizeof(&previousMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}
  }
  else if(LinePosition == 1)
  {
    lcd_goto(LCD_1st_LINE, 0u);
    lcd_putc('>'); 
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    Menu_Hints(selectedMenuItem);
    //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}
  
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc(' '); 
    lcd_puts((void*)&nextMenuItem->Param->ParamName);
    Menu_Hints(nextMenuItem);
    
    //size = (16 - sizeof(&nextMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}  
  }
  
  else if(LinePosition == 2)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('>'); 
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    Menu_Hints(selectedMenuItem);
    //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //}
    
    lcd_goto(LCD_1st_LINE, 0u);
    lcd_putc(' '); 
    lcd_puts((void*)&previousMenuItem->Param->ParamName);
    Menu_Hints(previousMenuItem);
    
    //size = (16 - sizeof(&previousMenuItem->Param->ParamName));
    //while(size > 0) 
    //{
    //  lcd_putc(' ');
    //  size--;
    //} 
  }
  
  
  /*
  if(LinePosition == 1)
  {
    lcd_goto(LCD_1st_LINE, 0u);
    lcd_putc('>'); 
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    while(size > 0) 
    {
      lcd_putc(' ');
      size--;
    }
  
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc(' '); 
    lcd_puts((void*)&nextMenuItem->Param->ParamName);
    size = (16 - sizeof(&nextMenuItem->Param->ParamName));
    while(size > 0) 
    {
      lcd_putc(' ');
      size--;
    } 
  }
  else if(LinePosition == 2)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('>'); 
    lcd_puts((void*)&selectedMenuItem->Param->ParamName);
    size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
    while(size > 0) 
    {
      lcd_putc(' ');
      size--;
    }
    
    lcd_goto(LCD_1st_LINE, 0u);
    lcd_putc(' '); 
    lcd_puts((void*)&previousMenuItem->Param->ParamName);
    size = (16 - sizeof(&previousMenuItem->Param->ParamName));
    while(size > 0) 
    {
      lcd_putc(' ');
      size--;
    } 
  }
  */
  
}


void Screen_Menu_DEC()
{
  uint8_t size = 0;

  lcd_goto(LCD_1st_LINE, 0u);
  lcd_putc(' '); 
  lcd_puts((void*)&previousMenuItem->Param->ParamName);
  Menu_Hints(previousMenuItem);
  //size = (16 - sizeof(&previousMenuItem->Param->ParamName));
  //while(size > 0) 
  //{
  //  lcd_putc(' ');
  //  size--;
  //} 
  
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_putc('>'); 
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  Menu_Hints(selectedMenuItem);
  //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
  //while(size > 0) 
  //{
  //  lcd_putc(' ');
  //  size--;
  //} 
  
  LinePosition = 2;
}

void Screen_Menu_INC()
{
  uint8_t size = 0;

  lcd_goto(LCD_1st_LINE, 0u);
  lcd_putc('>'); 
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  Menu_Hints(selectedMenuItem);
  
  //size = (16 - sizeof(&selectedMenuItem->Param->ParamName));
  //while(size > 0) 
  //{
  //  lcd_putc(' ');
  //  size--;
  //} 
  
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_putc(' '); 
  lcd_puts((void*)&nextMenuItem->Param->ParamName);
  Menu_Hints(nextMenuItem);
  
  //size = (16 - sizeof(&nextMenuItem->Param->ParamName));
  //while(size > 0) 
  //{
  //  lcd_putc(' ');
  //  size--;
  //} 
  
   LinePosition = 1;
}

void Screen_Param(uint8_t channel)
{
  switch(selectedMenuItem->Param->ParamType)
  {
    case OUT_VOLUME : Screen_Volume(&USER_EQ[channel].Gain, channel, Gain_Min, Gain_Max); break;
    case OUT_DELAY : Screen_Delay(&USER_EQ[channel].Delay, channel, SpkOutDelay_Min, SpkOutDelay_Max); break;
    case MATRIX_IN1 : Screen_Matrix(&PRESET_PARAM.Matrix[0][channel], channel, 1); break;
    case MATRIX_IN2 : Screen_Matrix(&PRESET_PARAM.Matrix[1][channel], channel, 2); break;
    case MATRIX_IN3 : Screen_Matrix(&PRESET_PARAM.Matrix[2][channel], channel, 3); break;
    case MATRIX_IN4 : Screen_Matrix(&PRESET_PARAM.Matrix[3][channel], channel, 4); break;
    case MODE_XLR1 : Screen_Mode_XLR(&PRESET_PARAM.ModeXLR[0], 1); break;
    case MODE_XLR3 : Screen_Mode_XLR(&PRESET_PARAM.ModeXLR[1], 3); break;
    case AES_DELAY : Screen_AES_Delay(&PRESET_PARAM.AESTrim.Delay); break;
    case AES_GAIN : Screen_AES_Gain(&PRESET_PARAM.AESTrim.Gain); break;
    case ANALOG_DELAY : Screen_Analog_Delay(&PRESET_PARAM.AnalogTrim.Delay); break;
    case ANALOG_GAIN : Screen_Analog_Gain(&PRESET_PARAM.AnalogTrim.Gain); break;
    case SOURCE_INPUT_1 : Screen_Input_Source(&PRESET_PARAM.InputSource[0].ManualSource, 1); break;
    case SOURCE_INPUT_2 : Screen_Input_Source(&PRESET_PARAM.InputSource[1].ManualSource, 2); break;
    case SOURCE_INPUT_3 : Screen_Input_Source(&PRESET_PARAM.InputSource[2].ManualSource, 3); break;
    case SOURCE_INPUT_4 : Screen_Input_Source(&PRESET_PARAM.InputSource[3].ManualSource, 4); break;
    case MENU_LOAD_SPEAKER : Screen_Load_Speaker(&SPEAKER_LIB, channel); break;
    case MENU_OUT_CONFIG_BRIDGE_AB : Screen_Out_Config_Bridge(&PRESET_PARAM.Bridge[0], 0); break;
    case MENU_OUT_CONFIG_BRIDGE_CD : Screen_Out_Config_Bridge(&PRESET_PARAM.Bridge[1], 2); break;
    case MENU_OUT_CONFIG_WARNING_BRIDGE : Screen_menu_Warning_Bridge(); break;
    case MENU_OUT_INFO : Screen_Speaker_Info(SPEAKER, SPEAKER_OUT, channel); break; 
    case MENU_SPEAKER_RST : Screen_Speaker_Reset(&SpeakerReset, channel); break; 
    case MENU_LOAD_PRESET : Screen_Load_Preset(&PRESET_LIB); break;
    //case USB_MENU : Screen_Menu_USB(); break;
    case MENU_SYS_RESET : Screen_Sys_Reset(&SystemReset); break;
  }   
}

void Screen_Volume(GainParamType* volume, uint8_t channel, int16_t value_min, int16_t value_max)
{
  uint8_t Channel = 0x41 + channel;
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 2u);
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  lcd_goto(LCD_1st_LINE, 9u);
  lcd_puts("OUT");
  lcd_putc(Channel);

  //lcd_puts(&Channel);
  //lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  
  Volume_Screen_Data(volume, value_min, value_max);
}

void Screen_Matrix(MatrixType* matrix, uint8_t channel, uint8_t index)
{
  uint8_t Channel = 0x41 + channel;
  uint8_t Index = 0x30 + index; 
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("MATRIX ");
  lcd_puts("IN");
  lcd_putc(Index);
  lcd_puts(" OUT");
  lcd_putc(Channel);
  
  uint8_t data[16];
  int16_t tmp1 = 0;
  int16_t tmp2 = 0;
  
  tmp1 = matrix->Gain / 100;
  tmp2 = matrix->Gain % 100;
  
  
  
  if(matrix->Gain > MatrixGain_Min)
  {
    data[0] = ' ';
    data[1] = '<';
    data[2] = ' ';
    data[3] = ' ';
  }
  else
  {
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
  }
  
  if(matrix->Gain <= MatrixGain_Max)
  {
    tmp1 = abs(matrix->Gain) / 100;
    tmp2 = abs(matrix->Gain) % 100;
    
    if((tmp1 == 0) & (tmp2 == 0))
    {
      data[4] = ' ';
      data[5] = (tmp1 / 10) + 0x30;
      data[6] = (tmp1 % 10) + 0x30;
      data[7] = '.';
      data[8] = (tmp2 / 10) + 0x30;
      data[9] = (tmp2 % 10) + 0x30;
      data[10] = 'd';
      data[11] = 'B';
    }
    else if((tmp1 > 0) | (tmp2 > 0))
    {
      data[4] = '-';
      data[5] = (tmp1 / 10) + 0x30;
      data[6] = (tmp1 % 10) + 0x30;
      data[7] = '.';
      data[8] = (tmp2 / 10) + 0x30;
      data[9] = (tmp2 % 10) + 0x30;
      data[10] = 'd';
      data[11] = 'B';
    }
  }
  
  if(matrix->Gain < MatrixGain_Max)
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = '>';
    data[15] = ' ';
  }
  else 
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = ' ';
    data[15] = ' ';
  }

  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(data);

  //Volume_Screen_Data(volume, MatrixGain_Min, MatrixGain_Max);
}

void Screen_Delay(DelayParamType* delay, uint8_t channel, uint32_t value_min, uint32_t value_max)
{
  uint8_t data[16];
  int16_t tmp1 = 0;
  int16_t tmp2 = 0;
  int16_t tmp3 = 0;
  
  uint8_t ChannelTmp = 0x41 + channel;
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 3u);
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  lcd_goto(LCD_1st_LINE,9u);
  lcd_puts("OUT");
  lcd_putc(ChannelTmp);
  
  lcd_goto(LCD_2nd_LINE, 0u);
  
  if(delay->Value > value_min)
  {
    data[0] = ' ';
    data[1] = '<';
    data[2] = ' ';
    data[3] = ' ';
    data[4] = ' ';
  }
  else
  {    
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    data[4] = ' ';
  }

  tmp1 = delay->Value / 100;
  tmp2 = delay->Value % 100;

  data[5] = (tmp1 / 100) + 0x30;
  data[6] = ((tmp1 % 100) / 10) + 0x30;
  data[7] = ((tmp1 % 100) % 10) + 0x30;
  data[8] = '.';
  data[9] = (tmp2 / 10) + 0x30;
  data[10] = 'm';
  data[11] = 's';

  if(delay->Value < value_max)
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = '>';
    data[15] = ' ';
  }
  else
  {    
    data[12] = ' ';
    data[13] = ' ';
    data[14] = ' ';
    data[15] = ' ';
  }
    
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(data);
}

void Screen_AES_Delay(DelayParamType* delay)
{
  uint8_t data[16];
  int16_t tmp1 = 0;
  int16_t tmp2 = 0;
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("   AES DELAY    ");

  if(delay->Value > InputTrimDelay_Min)
  {
    data[0] = ' ';
    data[1] = '<';
    data[2] = ' ';
    data[3] = ' ';
    data[4] = ' ';
  }
  else
  {    
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    data[4] = ' ';
  }

  tmp1 = delay->Value / 100;
  tmp2 = delay->Value % 100;

  data[5] = (tmp1 / 100) + 0x30;
  data[6] = ((tmp1 % 100) / 10) + 0x30;
  data[7] = ((tmp1 % 100) % 10) + 0x30;
  data[8] = '.';
  data[9] = (tmp2 / 10) + 0x30;
  data[10] = 'm';
  data[11] = 's';

  if(delay->Value < InputTrimDelay_Max)
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = '>';
    data[15] = ' ';
  }
  else
  {    
    data[12] = ' ';
    data[13] = ' ';
    data[14] = ' ';
    data[15] = ' ';
  }
    
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(data);
}

void Screen_AES_Gain(GainParamType* gain)
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("    AES GAIN     ");

  Volume_Screen_Data(gain, InputTrimGain_Min, InputTrimGain_Max);
}

void Screen_Analog_Delay(DelayParamType* delay)
{
  uint8_t data[16];
  int16_t tmp1 = 0;
  int16_t tmp2 = 0;
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("  ANALOG DELAY  ");

  if(delay->Value > InputTrimDelay_Min)
  {
    data[0] = ' ';
    data[1] = '<';
    data[2] = ' ';
    data[3] = ' ';
    data[4] = ' ';
  }
  else
  {    
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
    data[4] = ' ';
  }

  tmp1 = delay->Value / 100;
  tmp2 = delay->Value % 100;

  data[5] = (tmp1 / 100) + 0x30;
  data[6] = ((tmp1 % 100) / 10) + 0x30;
  data[7] = ((tmp1 % 100) % 10) + 0x30;
  data[8] = '.';
  data[9] = (tmp2 / 10) + 0x30;
  data[10] = 'm';
  data[11] = 's';

  if(delay->Value < InputTrimDelay_Max)
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = '>';
    data[15] = ' ';
  }
  else
  {    
    data[12] = ' ';
    data[13] = ' ';
    data[14] = ' ';
    data[15] = ' ';
  }
    
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(data);
}

void Screen_Analog_Gain(GainParamType* gain)
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("  ANALOG GAIN   ");
  
  Volume_Screen_Data(gain, InputTrimGain_Min, InputTrimGain_Max);
}

void Screen_Input_Source(AmpInputValueType* amp_input, uint8_t index)
{
  lcd_clrscr();
  lcd_goto(LCD_1st_LINE, 0u);
  
  if(index == 1) lcd_puts("  INPUT1 TYPE   ");
  else if(index == 2) lcd_puts("  INPUT2 TYPE   ");
  else if(index == 3) lcd_puts("  INPUT3 TYPE   ");
  else if(index == 4) lcd_puts("  INPUT4 TYPE   ");
  
  lcd_goto(LCD_2nd_LINE, 0u);
  if(*amp_input == IN_NONE) lcd_puts("      NONE    > ");
  else if(*amp_input == ANALOG_INPUT_1) lcd_puts(" <  ANALOG1   > ");
  else if(*amp_input == ANALOG_INPUT_2) lcd_puts(" <  ANALOG2   > ");
  else if(*amp_input == ANALOG_INPUT_3) lcd_puts(" <  ANALOG3   > ");
  else if(*amp_input == ANALOG_INPUT_4) lcd_puts(" <  ANALOG4   > ");
  else if(*amp_input == AES_INPUT_1) lcd_puts(" <    AES1    > ");
  else if(*amp_input == AES_INPUT_2) lcd_puts(" <    AES2    > ");
  else if(*amp_input == AES_INPUT_3) lcd_puts(" <    AES3    > ");
  else if(*amp_input == AES_INPUT_4) lcd_puts(" <    AES4      ");
}

void Volume_Screen_Data(GainParamType* volume, int16_t value_min, int16_t value_max)
{
  uint8_t data[16];
  int16_t tmp1 = 0;
  int16_t tmp2 = 0;
  
  tmp1 = volume->Value / 100;
  tmp2 = volume->Value % 100;
  
  if(volume->Value > value_min)
  {
    data[0] = ' ';
    data[1] = '<';
    data[2] = ' ';
    data[3] = ' ';
  }
  else
  {
    data[0] = ' ';
    data[1] = ' ';
    data[2] = ' ';
    data[3] = ' ';
  }
  
  if(volume->Value <= 0)
  {
    tmp1 = abs(volume->Value) / 100;
    tmp2 = abs(volume->Value) % 100;
    
    if((tmp1 == 0) & (tmp2 == 0))
    {
      data[4] = ' ';
      data[5] = (tmp1 / 10) + 0x30;
      data[6] = (tmp1 % 10) + 0x30;
      data[7] = '.';
      data[8] = (tmp2 / 10) + 0x30;
      data[9] = (tmp2 % 10) + 0x30;
      data[10] = 'd';
      data[11] = 'B';
    }
    else if((tmp1 > 0) | (tmp2 > 0))
    {
      data[4] = '-';
      data[5] = (tmp1 / 10) + 0x30;
      data[6] = (tmp1 % 10) + 0x30;
      data[7] = '.';
      data[8] = (tmp2 / 10) + 0x30;
      data[9] = (tmp2 % 10) + 0x30;
      data[10] = 'd';
      data[11] = 'B';
    }
  }
  else 
  {
    tmp1 = volume->Value / 100;
    tmp2 = volume->Value % 100;

    if((tmp1 > 0) | (tmp2 > 0))
    {
      data[4] = '+';
      data[5] = (tmp1 / 10) + 0x30;
      data[6] = (tmp1 % 10) + 0x30;
      data[7] = '.';
      data[8] = (tmp2 / 10) + 0x30;
      data[9] = (tmp2 % 10) + 0x30;
      data[10] = 'd';
      data[11] = 'B';
    }
  }
  
  if(volume->Value < value_max)
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = '>';
    data[15] = ' ';
  }
  else 
  {
    data[12] = ' ';
    data[13] = ' ';
    data[14] = ' ';
    data[15] = ' ';
  }

  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(data);
}

void Screen_Mode_XLR(ModeXLRParamType* XLR, uint8_t index)
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 1u);
  if(index == 1) lcd_puts("MODE XLR1 TYPE");
  else if(index == 3) lcd_puts("MODE XLR3 TYPE");
  
  lcd_goto(LCD_2nd_LINE, 0u);

  if(XLR->State == XLR_ANALOG)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_puts("    ANALAOG   > ");
  }
  else if(XLR->State == XLR_AES)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_puts(" <    AES     ");
  }
}

void Screen_Out_Config_Bridge(BridgeParamType* out_bridge, uint8_t channel)
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 5u);
  if(channel == 0) lcd_puts("OUT A+B");
  else if(channel == 2) lcd_puts("OUT C+D");
  
  lcd_goto(LCD_2nd_LINE, 0u);

  if(out_bridge->State == UNBRIDGE) lcd_puts("    UNBRIDGE   >");
  else if(out_bridge->State == BRIDGE) lcd_puts("<    BRIDGE     ");
}

void Screen_Out_Config_Bridge_Param(BridgeParamType* out_bridge, SpeakerType* Speaker, uint8_t channel)
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 1u);
  //lcd_puts("BRIDGE");
  
  if(channel == 0) lcd_puts("BRIDGE OUT:A+B");
  else if(channel == 2) lcd_puts("BRIDGE OUT:C+D");
  
  lcd_goto(LCD_2nd_LINE, 0u);
  
  if((Speaker[channel].Config->Ways == 1) & (Speaker[channel+1].Config->Ways == 1)) 
  {
    if(out_bridge->State == UNBRIDGE) lcd_puts("    UNBRIDGE   >");
    else if(out_bridge->State == BRIDGE) lcd_puts("<    BRIDGE     ");
  }
  else
  {
    if(out_bridge->State == UNBRIDGE) lcd_puts("UNBRIDGE  LINKED");
    else if(out_bridge->State == BRIDGE) lcd_puts("BRIDGE    LINKED");
  }
}

void Screen_menu_Warning_Bridge()
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("WARN: OUT LINKED");
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(" NEED SPK RESET");
}

void LED_Write(uint16_t data)
{
  uint8_t tmp1 = (uint8_t)((data & 0xFF00) >> 8);
  uint8_t tmp2 = (uint8_t)(data & 0xFF);
  
  CS_Enable(IC_LED);
  SPI3_Transmit(tmp1);
  SPI3_Transmit(tmp2);
  CS_Disable(IC_LED);
  
  //Time to write -> 44us
}

//Get data from led struct and write it to the front board
void Update_LED(LEDsIndicatorsType* LED)
{  
  uint16_t data = 0xFFFF;
  
  data &= ~LED->LED_MUTE_SELECT[0];
  data &= ~(LED->LED_MUTE_SELECT[1] << 1);
  data &= ~(LED->LED_MUTE_SELECT[2] << 2);
  data &= ~(LED->LED_MUTE_SELECT[3] << 3);
  data &= ~(LED->LED_SIGNAL[0] << 4);
  data &= ~(LED->LED_SIGNAL[1] << 5);
  data &= ~(LED->LED_SIGNAL[2] << 6);
  data &= ~(LED->LED_SIGNAL[3] << 7);
  data &= ~(LED->LED_CLIP[0] << 8);
  data &= ~(LED->LED_CLIP[1] << 9);
  data &= ~(LED->LED_CLIP[2] << 10);
  data &= ~(LED->LED_CLIP[3] << 11);

  LED_Write(data);
}

void LED_Clear()
{
  uint16_t data = 0xFFFF;
  LED_Write(data);
}

void Menu_Hints(menuItem* menu)
{
  uint8_t size = 0;   
  if(menu->Param->ParamType == MENU_OUT_CONFIG_BRIDGE_AB)
  {
    if(PRESET_PARAM.Bridge[0].State == BRIDGE) lcd_puts(("     BRDG"));
    else lcd_puts(("     UNBR"));
  }
  else if(menu->Param->ParamType == MENU_OUT_CONFIG_BRIDGE_CD)
  {
    if(PRESET_PARAM.Bridge[1].State == BRIDGE) lcd_puts(("     BRDG"));
    else lcd_puts(("     UNBR"));
  }
  else
  {
    size = (16 - sizeof(&menu->Param->ParamName));
    while(size > 0) 
    {
      lcd_putc(' ');
      size--;
    }
  }
  
}

void Screen_Load_Speaker(SpeakerLibType* SpeakerLib, uint8_t channel)
{
  uint8_t Channel = 0x41 + channel;
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  lcd_goto(LCD_1st_LINE, 13u);
  lcd_puts("CH");
  lcd_putc(Channel);
  
  if(SpeakerLib->Index > 0)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('<');
  }
  
  if(SpeakerLib->Index < (LIB_SPEAKERS_QTY -1))
  {
    lcd_goto(LCD_2nd_LINE, 15u);
    lcd_putc('>');
  }
  
  lcd_goto(LCD_2nd_LINE, 1u);
  lcd_putc(SpeakerLib->Index +0x30);
  lcd_putc(':');
  lcd_puts(SpeakerLib->Speaker[SpeakerLib->Index].Name);
}

void Screen_Load_Preset(PresetLibType* PresetLib)
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 2u);
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);

  if(PresetLib->Index > 0)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('<');
  }
  
  if(PresetLib->Index  < (LIB_PRESET_QTY -1))
  {
    lcd_goto(LCD_2nd_LINE, 15u);
    lcd_putc('>');
  }
  
  lcd_goto(LCD_2nd_LINE, 1u);
  lcd_putc(PresetLib->Index +0x30);
  lcd_putc(':');
  lcd_puts(PresetLib->Name[PresetLib->Index]);
}

void Screen_Speaker_Info(SpeakerType* speaker, SpeakerOutType* out, uint8_t channel)
{
  uint8_t i;
  uint8_t Channel = 0x41 + channel;
  uint8_t SPK_Name[LCD_SPK_NAME_SIZE];
  uint8_t  master_channel = Found_Master_Channel(channel, speaker);

  for(i = 0; i < LCD_SPK_NAME_SIZE; i++) SPK_Name[i] = speaker[master_channel].Config->ModelName[i];
 
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts((void*)&selectedMenuItem->Param->ParamName);
  lcd_goto(LCD_1st_LINE, 13u);
  lcd_puts("CH");
  lcd_putc(Channel);
  
  lcd_goto(LCD_2nd_LINE, 0u);
  lcd_puts(SPK_Name);
  lcd_goto(LCD_2nd_LINE, 14u);
  lcd_puts(out[channel].SpeakerOutConfig->Name); 
}

void Screen_Speaker_Reset(SpeakerResetType* SpkReset, uint8_t channel)
{
  uint8_t Channel = 0x41 + channel;
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 0u);
  lcd_puts("SPEAKER");
  lcd_goto(LCD_1st_LINE, 13u);
  lcd_puts("CH");
  lcd_putc(Channel);

  if(*SpkReset == SpeakerResetNo)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('>');
    lcd_goto(LCD_2nd_LINE, 3u);
    lcd_puts("RESET:NO?");
  }
  else if(*SpkReset == SpeakerResetYes)
  {
    lcd_goto(LCD_2nd_LINE, 3u);
    lcd_puts("RESET:YES?");
    lcd_goto(LCD_2nd_LINE, 15u);
    lcd_putc('<');
  }
}

void Screen_Speaker_Reset_Complate()
{
  lcd_clrscr();
  lcd_goto(LCD_1st_LINE, 1u);
  lcd_puts("SPEAKER RESET");
  lcd_goto(LCD_2nd_LINE, 4u);
  lcd_puts("COMPLATE");
}

void Screen_Sys_Reset(SysResetType* SysReset)
{
  //uint8_t Channel = 0x41 + channel;
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 2u);
  lcd_puts("SYSTEM RESET");

  if(*SysReset == SysResetNo)
  {
    lcd_goto(LCD_2nd_LINE, 0u);
    lcd_putc('>');
    lcd_goto(LCD_2nd_LINE, 3u);
    lcd_puts("RESET:NO?");
  }
  else if(*SysReset == SysResetYes)
  {
    lcd_goto(LCD_2nd_LINE, 3u);
    lcd_puts("RESET:YES?");
    lcd_goto(LCD_2nd_LINE, 15u);
    lcd_putc('<');
  }
}

void Screen_Sys_Reset_Complate()
{
  lcd_clrscr();
  lcd_goto(LCD_1st_LINE, 2u);
  lcd_puts("SYSTEM RESET");
  lcd_goto(LCD_2nd_LINE, 4u);
  lcd_puts("COMPLATE");
}

void Screen_Menu_USB()
{
  lcd_clrscr();
  
  lcd_goto(LCD_1st_LINE, 6u);
  lcd_puts("USB");
}
