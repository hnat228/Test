
#include "amp_application.h"
#include "front_board.h"
#include "rcc.h"
#include "delay.h"
#include "menu.h"
#include "flash.h"
#include "dsp_board.h"
#include "string.h"
#include "stdint.h"
#include "math.h"
#include "array.h"
#include "ParkDSPLite_4x4_ADAU1466_PARAM.h"
#include "dsp_functions.h"
#include "parkfileformatmanager.h"
#include "file_streams.h"
#include "adc_mcu.h"
#include "i2c_eeprom.h"
#include "crc.h"
#include "park_dsp_lite_4x4.h"
#include "connection.h"

DeviceStatusType          DeviceStatus;

ControlStateType        EncoderControl = {NO,0};
ControlStateType        ButtonControl = {NO,0};
//uint8_t                 ButtonMask = 0;
uint8_t                 LinkButtonMask;
uint8_t                 BridgeButtonMask;
uint8_t                 LoadSpkFreeWays = 0;
uint8_t                 AmpFreeWays = 0;
uint8_t                 SelectedCh = 0;
uint8_t                 MuteCh = 0;
uint8_t                 MenuParamEntering = false;
int32_t                 VeluePrev = 0;    
#define INDIRECTPARAM_SIZE      96
uint8_t                 DSP_READ_DATA[INDIRECTPARAM_SIZE];   //Test 
DspModuleType           ReadDspStatus = {MOD_INDIRECTPARAMACCESSMODULE_READ_ANALOG_LEVEL_1_RMS_ADDR, MOD_INDIRECTPARAMACCESSMODULE_READ_ANALOG_LEVEL_1_RMS_MEMORYPAGE};
MuteValueType           AmpHardwareMute = MUTED;
//uint16_t                FlashMemory = 0;
//uint16_t                EepromMemory = 0;

//SpeakerInfoType         SPEAKER_INFO[OUT_CH_QTY];

//Status data--------------------------------------------------------------------
//InputLevelType          INPUT_LEVEL;
//ChannelOutInfoType      CH_OUT_INFO;

                                           
                                           //LED Signal                      //LED Clip                        //LED Mute        
LEDsIndicatorsType      LED_INDICATORS = {{LED_OFF, LED_OFF, LED_OFF, LED_OFF}, {LED_OFF, LED_OFF, LED_OFF, LED_OFF}, {LED_OFF, LED_OFF, LED_OFF, LED_OFF}}; 
uint8_t LED_COUNTERS = 0;       //For animations

void App_Init()
{ 
  DeviceStatus.DeviceMode = INIT_MODE;
  CRC_Init();
  ADC_WatchDog_Init();
  
  Main_Board_Init();
  Front_Board_Init();           //Time execution = 125ms                           
  Menu_Init();
  I2C2_Init();
  
  DeviceStatus.DeviceFlash = (DeviceFlashType)SPI_Flash_Init();                                                       
  DeviceStatus.DeviceEeprom = (DeviceEepromType)Eeprom_Init();

  DSP_Board_Init();             //Time execution = 6.5s 
  
  if(DeviceStatus.DeviceFlash == FLASH_ERROR)
  {
    DeviceStatus.DeviceMode = ERROR_MODE;
  }
  else if(DeviceStatus.DeviceEeprom == EEPROM_ERROR){}
  else if(DeviceStatus.DeviceDsp == DSP_ERROR){}
  else
  {
    DSP_Address_Init();
    Preset_Init(&PRESET_PARAM);
    UserEQ_Init(USER_EQ);
    Speaker_Init(SPEAKER);
  
    Memory_Init();
    
    PRESET.PresetParam = &PRESET_PARAM;
    PRESET.UserEQ[0] = &USER_EQ[0];
    PRESET.UserEQ[1] = &USER_EQ[1];
    PRESET.UserEQ[2] = &USER_EQ[2];
    PRESET.UserEQ[3] = &USER_EQ[3];
    PRESET.Speaker[0] = &SPEAKER[0];
    PRESET.Speaker[1] = &SPEAKER[1];
    PRESET.Speaker[2] = &SPEAKER[2];
    PRESET.Speaker[3] = &SPEAKER[3];
    
    Speaker_Lib_Init(&SPEAKER_LIB);
    Preset_Lib_Init(&PRESET_LIB);
    
    Link_OUT_Matrix(SPEAKER, LINK_MATRIX);
    Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh);
      
    Screen_Menu(&EncoderControl);
  }
  
  //Timer_2_Init();               //Timer for exit to main screen
}

void Menu_Manager()
{
  if(DeviceStatus.DeviceMode != USB_MODE)
  {
    if(EncoderControl.Comand != NO)
    {
      switch(EncoderControl.Comand)
      {
        case ENTER : 
        {
          if(selectedMenuItem->MenuType == MENU)
          {
            Menu_Child();
            
            if(selectedMenuItem->MenuType == PARAM)
            {
              switch(selectedMenuItem->Param->ParamType)
              {
                case MENU_LOAD_SPEAKER : 
                {       
                  if(!(~ButtonControl.Counter & LoadSpkFreeWays)) SelectedCh = 0;
                  else SelectedCh = Select_Channel(&ButtonControl, LoadSpkFreeWays);
                  Screen_Param(SelectedCh); 
                  break;
                }
                case MENU_OUT_INFO : 
                case MENU_SPEAKER_RST : 
                {
                  if(!(~ButtonControl.Counter & LinkButtonMask)) SelectedCh = 0;
                  else SelectedCh = Select_Channel(&ButtonControl, LinkButtonMask);
                  Screen_Param(SelectedCh); 
                  break;
                }
                default : 
                {
                  if(!(~ButtonControl.Counter & LinkButtonMask)) SelectedCh = 0;
                  else SelectedCh = Select_Channel(&ButtonControl, LinkButtonMask);
                  Param_Change(&EncoderControl, SelectedCh); 
                  Screen_Param(SelectedCh); 
                  break;
                }
              } 
            }
            else if(selectedMenuItem->MenuType == MENU) 
            {
              Screen_Menu(&EncoderControl);
            }
          }
          else if(selectedMenuItem->MenuType == PARAM)
          {
            //MenuParamEntering = true;
            Param_Change(&EncoderControl, SelectedCh);
            Menu_Child();
            
            if(selectedMenuItem->MenuType == MENU) 
            {
              Screen_Menu(&EncoderControl);
              //MenuParamEntering = false;
            }
          }
  
          break;
        }
        case ESC : 
        {
          if(selectedMenuItem->MenuType == PARAM) Param_Change(&EncoderControl, SelectedCh);
          
          Menu_Parent();
          
          if(selectedMenuItem->MenuType == MENU)
          {
            Screen_Menu(&EncoderControl);
          }
          
          //MenuParamEntering = false;
  
          break;
        }
        case INC : 
        {
          if(selectedMenuItem->MenuType == MENU)
          {
            Menu_Next();
            Screen_Menu(&EncoderControl); 
          }
          else if(selectedMenuItem->MenuType == PARAM)
          {
            //MenuParamEntering = false;
            Param_Change(&EncoderControl, SelectedCh);
            
            if(selectedMenuItem->Param->ParamType == MENU_LOAD_SPEAKER)
            {
              if(!(~ButtonControl.Counter & LoadSpkFreeWays)) SelectedCh = 0;
              else SelectedCh = Select_Channel(&ButtonControl, LoadSpkFreeWays);
            }
            else SelectedCh = Select_Channel(&ButtonControl, LinkButtonMask);
            
            Screen_Param(SelectedCh);
          }
          
          break;
        }
        case DEC : 
        {
          if(selectedMenuItem->MenuType == MENU)
          {
            Menu_Prev();
            Screen_Menu(&EncoderControl);
          }
          else if(selectedMenuItem->MenuType == PARAM)
          {
            Param_Change(&EncoderControl, SelectedCh);
            
            if(selectedMenuItem->Param->ParamType == MENU_LOAD_SPEAKER)
            {
              if(!(~ButtonControl.Counter & LoadSpkFreeWays)) SelectedCh = 0;
              else SelectedCh = Select_Channel(&ButtonControl, LoadSpkFreeWays);
            }
            else SelectedCh = Select_Channel(&ButtonControl, LoadSpkFreeWays);
            
            Screen_Param(SelectedCh);
          }
          
          break;
        }
      }
  
      Menu_Reset_Timer_Start();
    }
  }
  
  if(ButtonControl.Comand != NO)
  {
    if(selectedMenuItem->MenuType == PARAM)
    {
      switch(selectedMenuItem->Param->ParamType)
      {
        case MENU_LOAD_SPEAKER : SelectedCh = Select_Channel(&ButtonControl, LoadSpkFreeWays); break;
        case MENU_OUT_INFO : SelectedCh = Select_Channel(&ButtonControl, BridgeButtonMask); break;
        case MENU_SPEAKER_RST : SelectedCh = Select_Channel(&ButtonControl, LinkButtonMask); break;
        default: 
        {
          EncoderControl.Comand = ENTER;
          Param_Change(&EncoderControl, SelectedCh);
          SelectedCh = Select_Channel(&ButtonControl, LinkButtonMask);
          //MenuParamEntering = false;
          Param_Change(&EncoderControl, SelectedCh);
        }
      }
      
      Screen_Param(SelectedCh);
      Menu_Reset_Timer_Start();
    }
    else if((selectedMenuItem->Param->ParamType == MAIN_MENU) || (DeviceStatus.DeviceMode == USB_MODE))
    {
      MuteCh = Mute_Channel(&ButtonControl, &PRESET_PARAM);
      Menu_Mute(&SPEAKER_OUT[MuteCh].Mute);
      DSP_Mute(&SPEAKER_OUT[MuteCh].Mute);
      SPEAKER_OUT[MuteCh].MemoryConfig.IsChanged = true;
    }
  }
}

void Task_Manager()
{
  Get_Device_Mode();
  DSP_Read_Data(&ReadDspStatus, DSP_READ_DATA, INDIRECTPARAM_SIZE);          //1.6ms
  DSP_Parsing_Data(DSP_READ_DATA);   
}

void LED_Manager()
{
  if((DeviceStatus.DeviceMode == MAIN_MENU_MODE) || (DeviceStatus.DeviceMode == USB_MODE))
  {
    LED_Mute_Mode(&LED_INDICATORS, &PRESET_PARAM);
  }
  else if(DeviceStatus.DeviceMode == MENU_MODE)
  {
    if(selectedMenuItem->Param->ParamType == MENU_LOAD_SPEAKER) 
    {
      LED_Select_Mode_Test(&LED_INDICATORS, LoadSpkFreeWays);
      //SelectedCh = Select_Channel(&ControlButton, ButtonMask);
    }
    else if(selectedMenuItem->Param->ParamType == MENU_OUT_INFO) 
    {
      LED_Select_Mode_Test(&LED_INDICATORS, BridgeButtonMask);
    }
    else LED_Select_Mode_Test(&LED_INDICATORS, LinkButtonMask);
  }
  //else if(DeviceInfo.DeviceMode == MENU_MODE) 
  //{
  //  LED_Select_Mode_Test(&LED_INDICATORS, ButtonMask);
  //  LED_COUNTERS = 0;
  //}
  
  Led_Signal(&LED_INDICATORS, SPEAKER_OUT);
  Led_Limiters(&LED_INDICATORS, SPEAKER_OUT);
  
  
  Update_LED(&LED_INDICATORS);
}

//Main Board Function-----------------------------------------------------------

void Main_Board_Init()
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;		                // enable clock for port C 
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;		                // enable clock for port C  
  
  //DC Enable PC13
  SET(DC_ENABLE_OUT, DC_ENABLE);
  GPIOC->MODER |= GPIO_MODER_MODE13_0;		                //Set MODE (mode register) -> Output
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR13_0;		        //Set OSPEEDR (output speed register) -> Medium speed

  //REL1
  CLR(REL1_PIN_OUT, REL1_PIN);
  GPIOC->MODER |= GPIO_MODER_MODE8_0;		                //Set MODE (mode register) -> Output
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8_0;		        //Set OSPEEDR (output speed register) -> Medium speed
 
  //REL2
  CLR(REL2_PIN_OUT, REL2_PIN);
  GPIOC->MODER |= GPIO_MODER_MODE7_0;		                //Set MODE (mode register) -> Output
  GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR7_0;		        //Set OSPEEDR (output speed register) -> Medium speed
  
  //Hardware Mute Amp
  SET(HARD_MUTE_OUT, HARD_MUTE);
  GPIOA->MODER &= ~GPIO_MODER_MODE3;		                //Reset MODE (mode register)
  GPIOA->MODER |= GPIO_MODER_MODE3_0;		                //Set MODE (mode register) -> Output
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT_3;		                //Set OTYPER -> Push Pull
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR3;		                //Set PUPDR -> Push Pull
  GPIOA->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR3;		        //Reset OSPEEDR (output speed register)
  GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3_0;		        //Set OSPEEDR (output speed register) -> Medium speed
}

//DSP Read and Parsing different data-------------------------------------------

void DSP_Parsing_Data(uint8_t* data)
{
  uint8_t addr = 0;
  //asm("NOP");
  
  DSP_Parsing_Input_Level(data, &addr, PRESET_PARAM.InputSource);
  DSP_Parsing_Out_Level(data, &addr, SPEAKER_OUT);
  DSP_Parsing_RMS_Limiter_Reduction(data, &addr, SPEAKER_OUT);
  DSP_Parsing_Peak_Limiter_Reduction(data, &addr, SPEAKER_OUT);
  DSP_Parsing_Clip_Limiter_Reduction(data, &addr, SPEAKER_OUT);
}

void DSP_Parsing_Input_Level(uint8_t* data, uint8_t* addr, InputSourceType* input_level)
{
  uint32_t level;
  uint8_t i;
  
  for(i = 0; i < 4; i++)
  {
    level = data[*addr] << 24;
    *addr +=1;
    level |= data[*addr] << 16;
    *addr +=1;
    level |= data[*addr] << 8;
    *addr +=1;
    level |= data[*addr];
    *addr +=1;
    
    if(level == 0) input_level->AnalogLevel[i] = MIN_LEVEL_METER;
    else input_level->AnalogLevel[i] = (int16_t)(((log10((double)level / 0x00800000) *10) +LEVELING_INPUT_dB) *100);
  }
  
  for(i = 0; i < 4; i++)
  {
    level = data[*addr] << 24;
    *addr +=1;
    level |= data[*addr] << 16;
    *addr +=1;
    level |= data[*addr] << 8;
    *addr +=1;
    level |= data[*addr];
    *addr +=1;
    
    if(level == 0) input_level->AesLevel[i] = MIN_LEVEL_METER;
    else input_level->AesLevel[i] = (int16_t)(log10((double)level / 0x00800000));
  }
}

void DSP_Parsing_Out_Level(uint8_t* data, uint8_t* addr, SpeakerOutType* out)
{
  uint32_t level;
  uint8_t i;
  
  for(i = 0; i < 4; i++)
  {
    level = data[*addr] << 24;
    *addr +=1;
    level |= data[*addr] << 16;
    *addr +=1;
    level |= data[*addr] << 8;
    *addr +=1;
    level |= data[*addr];
    *addr +=1;
    
    if(level == 0) out[i].OutLevel = MIN_LEVEL_METER;
    else out[i].OutLevel = (int16_t)(((log10((double)level / 0x00800000) *10) +LEVELING_INPUT_dB) *100);
  }
}

void DSP_Parsing_RMS_Limiter_Reduction(uint8_t* data, uint8_t* addr, SpeakerOutType* out)
{
  uint32_t level;
  uint8_t i;
  
  for(i = 0; i < 4; i++)
  {
    level = data[*addr] << 24;
    *addr +=1;
    level |= data[*addr] << 16;
    *addr +=1;
    level |= data[*addr] << 8;
    *addr +=1;
    level |= data[*addr];
    *addr +=1;
    
    if(level == 0) out[i].RMSLimiter.Reduction = 0;
    else out[i].RMSLimiter.Reduction = (int16_t)((log10((double)level / 0x01000000) *20) *100);
  }
}

void DSP_Parsing_Peak_Limiter_Reduction(uint8_t* data, uint8_t* addr, SpeakerOutType* out)
{
  uint32_t level;
  uint8_t i;
  
  for(i = 0; i < 4; i++)
  {
    level = data[*addr] << 24;
    *addr +=1;
    level |= data[*addr] << 16;
    *addr +=1;
    level |= data[*addr] << 8;
    *addr +=1;
    level |= data[*addr];
    *addr +=1;
    
    if(level == 0) out[i].PeakLimiter.Reduction = 0;
    else out[i].PeakLimiter.Reduction = (int16_t)((log10((double)level / 0x01000000) *20) *100);
  }
}

void DSP_Parsing_Clip_Limiter_Reduction(uint8_t* data, uint8_t* addr, SpeakerOutType* out)
{
  uint32_t level;
  uint8_t i;
  
  for(i = 0; i < 4; i++)
  {
    level = data[*addr] << 24;
    *addr +=1;
    level |= data[*addr] << 16;
    *addr +=1;
    level |= data[*addr] << 8;
    *addr +=1;
    level |= data[*addr];
    *addr +=1;
    
    if(level == 0) out[i].ClipLimiter.Reduction = 0;
    else out[i].ClipLimiter.Reduction = (int16_t)((log10((double)level / 0x01000000) *20) * 100);
  }
}

void DSP_Address_Init()
{
  //PRESET
  //DSP Addr input trim
  DSP_Addr_Analog_Input_Trim(&PRESET_PARAM.AnalogTrim);
  DSP_Addr_AES_Input_Trim(&PRESET_PARAM.AESTrim); 
  //DSP Addr Out Bridge
  DSP_Addr_Bridge(PRESET_PARAM.Bridge);
  //DSP Addr Input Source
  DSP_Addr_Input_Source(PRESET_PARAM.InputSource);
  //DSP Addr Matrix
  DSP_Addr_Matrix_Init();
  //DSP Addr Out Mute
  //PRESET.OutMute[0].Address = MOD_SPEAKEROUT1_MUTE_ALG0_MUTENOSLEWADAU145XALG9MUTE_ADDR;
  //PRESET.OutMute[1].Address = MOD_SPEAKEROUT1_MUTE_ALG0_MUTENOSLEWADAU145XALG9MUTE_ADDR;
  //PRESET.OutMute[2].Address = MOD_SPEAKEROUT1_MUTE_ALG0_MUTENOSLEWADAU145XALG9MUTE_ADDR;
  //PRESET.OutMute[3].Address = MOD_SPEAKEROUT1_MUTE_ALG0_MUTENOSLEWADAU145XALG9MUTE_ADDR;
  
  //USER EQ
  UserEQ_DSP_Addr_Init(USER_EQ);
  //SPEAKER EQ
  SpeakerEQ_DSP_Addr_Init(SPEAKER_EQ);
  //SPEAKER OUT
  Speaker_OUT_DSP_Addr_Init(SPEAKER_OUT);
}
//------------------------------------------------------------------------------

//PRESET------------------------------------------------------------------------
//Preset Init
void Preset_Init(PresetParamType* Preset)
{
  Preset_Default_Param(Preset);
}

void Preset_Default_Param(PresetParamType* Preset)
{
  Preset->InputMode = InputModeType_Def;
  Input_Trim_Init(Preset);
  Default_XLR_Mode_Param(Preset->ModeXLR);   
  Default_Bridge_Param(Preset->Bridge);
  Default_Input_Source_Param(Preset->InputSource);
  Default_Matrix_Param(Preset->Matrix[0]);
  Default_Matrix_Param(Preset->Matrix[1]);
  Default_Matrix_Param(Preset->Matrix[2]);
  Default_Matrix_Param(Preset->Matrix[3]);
  Preset->Matrix[0][0].Gain = MatrixGain_Def;
  Preset->Matrix[1][1].Gain = MatrixGain_Def;
  Preset->Matrix[2][2].Gain = MatrixGain_Def;
  Preset->Matrix[3][3].Gain = MatrixGain_Def;
}

void DSP_Addr_Input_Source(InputSourceType* InputSource)
{
  //Selector Address in DSP
  //PRESET.InputSource[0].SelectorAddr = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT0_ADDR;
  InputSource[0].DspModuleSelector.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT0_ADDR;
  InputSource[0].DspModuleSelector.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT0_MEMORYPAGE;
  InputSource[1].DspModuleSelector.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT1_ADDR;
  InputSource[1].DspModuleSelector.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT1_MEMORYPAGE;
  InputSource[2].DspModuleSelector.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT2_ADDR;
  InputSource[2].DspModuleSelector.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT2_MEMORYPAGE;
  InputSource[3].DspModuleSelector.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT3_ADDR;
  InputSource[3].DspModuleSelector.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1INPUTSELECT3_MEMORYPAGE;
  
  //Input Mute Address in DSP
  InputSource[0].DspModuleMute.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN0_ADDR;
  InputSource[0].DspModuleMute.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN0_MEMORYPAGE;
  InputSource[1].DspModuleMute.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN1_ADDR;
  InputSource[1].DspModuleMute.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN1_MEMORYPAGE;
  InputSource[2].DspModuleMute.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN2_ADDR;
  InputSource[2].DspModuleMute.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN2_MEMORYPAGE;
  InputSource[3].DspModuleMute.Address = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN3_ADDR;
  InputSource[3].DspModuleMute.MemoryPage = MOD_INPUT_INPUT_SELECTOR_ALG0_SIMPLEROUTERNOSLEW32S300ALG1OUTPUTGAIN3_MEMORYPAGE;
}

void Default_Input_Source_Param(InputSourceType* InputSource)
{
  uint8_t i;
  
  Default_Analog_Input_Param(InputSource);
  
  for(i = 0; i < IN_CH_QTY; i++)
  {
    if(i <= AnalogInputValueType_Max) InputSource[i].BackupSource[0] = (AmpInputValueType)(AnalogInputValueType_Min +i);
    else InputSource[i].BackupSource[0] = AmpInputValueType_Def;
    
    InputSource[i].BackupSource[1] = AmpInputValueType_Def;
    InputSource[i].BackupSource[2] = AmpInputValueType_Def;
    
    InputSource[i].BackupSelectedSource = BackupSourceItemType_Def;
    
    InputSource[i].BackupForse = StateType_Def;
    InputSource[i].BackupReturn = StateType_Def;
  }
}

void Default_Analog_Input_Param(InputSourceType* InputSource)
{
  uint8_t i;
  
  for(i = 0; i < IN_CH_QTY; i++)
  {
    if(i <= AnalogInputValueType_Max) InputSource[i].ManualSource = (AmpInputValueType)(AnalogInputValueType_Min +i);
    else InputSource[i].ManualSource = AmpInputValueType_Def;
  }
}

//INPUT INIT--------------------------------------------------------------------
//Input Trim--------------------------------------------------------------------
void Input_Trim_Init(PresetParamType* Preset)
{
  Default_Analog_Input_Trim_Param(&Preset->AnalogTrim);
  Default_AES_Input_Trim_Param(&Preset->AESTrim);
}

void DSP_Addr_Analog_Input_Trim(InputTrimType* AnalogInputTrim)
{
  AnalogInputTrim->Gain.DspModule.Address = MOD_INPUT_INPUT_AMP_SETTING_GAIN_ANALOG_ALG0_TARGET_ADDR;
  AnalogInputTrim->Gain.DspModule.MemoryPage = MOD_INPUT_INPUT_AMP_SETTING_GAIN_ANALOG_ALG0_TARGET_MEMORYPAGE;
  
  AnalogInputTrim->Delay.DspModule.Address = MOD_INPUT_INPUT_AMP_SETTING_DELAY_ANALOG_DELAYAMT_ADDR;
  AnalogInputTrim->Delay.DspModule.MemoryPage = MOD_INPUT_INPUT_AMP_SETTING_DELAY_ANALOG_DELAYAMT_MEMORYPAGE;
}

void Default_Analog_Input_Trim_Param(InputTrimType* AnalogInputTrim)
{
  AnalogInputTrim->Gain.Value = InputTrimGain_Def;
  AnalogInputTrim->Delay.Value = InputTrimDelay_Def;
}

void DSP_Addr_AES_Input_Trim(InputTrimType* AESInputTrim)
{
  AESInputTrim->Gain.DspModule.Address = MOD_INPUT_INPUT_AMP_SETTING_GAIN_AES_ALG0_TARGET_ADDR;
  AESInputTrim->Gain.DspModule.MemoryPage = MOD_INPUT_INPUT_AMP_SETTING_GAIN_AES_ALG0_TARGET_MEMORYPAGE;
  
  AESInputTrim->Delay.DspModule.Address = MOD_INPUT_INPUT_AMP_SETTING_DELAY_AES_DELAYAMT_ADDR;
  AESInputTrim->Delay.DspModule.MemoryPage = MOD_INPUT_INPUT_AMP_SETTING_DELAY_AES_DELAYAMT_MEMORYPAGE;
}

void Default_AES_Input_Trim_Param(InputTrimType* AESInputTrim)
{
  AESInputTrim->Gain.Value = InputTrimGain_Def;
  AESInputTrim->Delay.Value = InputTrimDelay_Def;
}

//Mode XLR----------------------------------------------------------------------
void Default_XLR_Mode_Param(ModeXLRParamType* ModeXLR)
{
  uint8_t i;
  
  for(i = 0; i < IN_CH_QTY/2; i++)
  {
    ModeXLR[i].State = ModeXLRVlueType_Def;
    ModeXLR[i].StatePrev = ModeXLRVlueType_Def;
  } 
}

//Bridge------------------------------------------------------------------------
void Default_Bridge_Param(BridgeParamType* Bridge)
{
  uint8_t i;
  
  for(i = 0; i < OUT_CH_QTY/2; i++)
  {
    Bridge[i].State = IsBridgeType_Def;
    //Bridge[i].StatePrev = IsBridgeType_Def;
  } 
}

void DSP_Addr_Bridge(BridgeParamType* Bridge)
{
  Bridge[0].BridgeSwitch.DspModule.Address = MOD_CLIPLIMITERAB_BRIDGE_SWITH_ALG0_MONOMUXSIGMA300NS4INDEX_ADDR;
  Bridge[0].BridgeSwitch.DspModule.MemoryPage = MOD_CLIPLIMITERAB_BRIDGE_SWITH_ALG0_MONOMUXSIGMA300NS4INDEX_MEMORYPAGE;
  Bridge[0].ClipSwitch.DspModule.Address = MOD_CLIPLIMITERAB_BRIDGECLIP_ALG0_MONOMUXSIGMA300NS3INDEX_ADDR;
  Bridge[0].ClipSwitch.DspModule.MemoryPage = MOD_CLIPLIMITERAB_BRIDGECLIP_ALG0_MONOMUXSIGMA300NS3INDEX_MEMORYPAGE;
  
  Bridge[1].BridgeSwitch.DspModule.Address = MOD_CLIPLIMITERCD_BRIDGE_SWITH_ALG0_MONOMUXSIGMA300NS1INDEX_ADDR;
  Bridge[1].BridgeSwitch.DspModule.MemoryPage = MOD_CLIPLIMITERCD_BRIDGE_SWITH_ALG0_MONOMUXSIGMA300NS1INDEX_MEMORYPAGE;
  Bridge[1].ClipSwitch.DspModule.Address = MOD_CLIPLIMITERCD_BRIDGECLIP_ALG0_MONOMUXSIGMA300NS5INDEX_ADDR;
  Bridge[1].ClipSwitch.DspModule.MemoryPage = MOD_CLIPLIMITERCD_BRIDGECLIP_ALG0_MONOMUXSIGMA300NS5INDEX_MEMORYPAGE;
}

//MATRIX INIT-------------------------------------------------------------------
void DSP_Addr_Matrix_Init()
{
  //Matrix DSP Addr Init Input 1
  PRESET_PARAM.Matrix[0][0].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_00_ADDR;
  PRESET_PARAM.Matrix[0][0].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_00_MEMORYPAGE;
  PRESET_PARAM.Matrix[0][1].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_00_ADDR;
  PRESET_PARAM.Matrix[0][1].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_00_MEMORYPAGE;
  PRESET_PARAM.Matrix[0][2].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_00_ADDR;
  PRESET_PARAM.Matrix[0][2].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_00_MEMORYPAGE;
  PRESET_PARAM.Matrix[0][3].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_00_ADDR;
  PRESET_PARAM.Matrix[0][3].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_00_MEMORYPAGE;
  
  //Matrix DSP Addr Init Input 2
  PRESET_PARAM.Matrix[1][0].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_01_ADDR;
  PRESET_PARAM.Matrix[1][0].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_01_MEMORYPAGE;
  PRESET_PARAM.Matrix[1][1].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_01_ADDR;
  PRESET_PARAM.Matrix[1][1].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_01_MEMORYPAGE;  
  PRESET_PARAM.Matrix[1][2].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_01_ADDR;
  PRESET_PARAM.Matrix[1][2].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_01_MEMORYPAGE;  
  PRESET_PARAM.Matrix[1][3].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_01_ADDR;  
  PRESET_PARAM.Matrix[1][3].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_01_MEMORYPAGE;
  
  //Matrix DSP Addr Init Inpu 3
  PRESET_PARAM.Matrix[2][0].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_02_ADDR;
  PRESET_PARAM.Matrix[2][0].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_02_MEMORYPAGE;
  PRESET_PARAM.Matrix[2][1].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_02_ADDR; 
  PRESET_PARAM.Matrix[2][1].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_02_MEMORYPAGE;
  PRESET_PARAM.Matrix[2][2].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_02_ADDR;  
  PRESET_PARAM.Matrix[2][2].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_02_MEMORYPAGE;
  PRESET_PARAM.Matrix[2][3].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_02_ADDR; 
  PRESET_PARAM.Matrix[2][3].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_02_MEMORYPAGE;
  
  //Matrix DSP Addr Init Input 4
  PRESET_PARAM.Matrix[3][0].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_03_ADDR;
  PRESET_PARAM.Matrix[3][0].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_00_03_MEMORYPAGE;
  PRESET_PARAM.Matrix[3][1].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_03_ADDR;
  PRESET_PARAM.Matrix[3][1].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_01_03_MEMORYPAGE;
  PRESET_PARAM.Matrix[3][2].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_03_ADDR;
  PRESET_PARAM.Matrix[3][2].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_02_03_MEMORYPAGE;
  PRESET_PARAM.Matrix[3][3].DspModule.Address = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_03_ADDR;
  PRESET_PARAM.Matrix[3][3].DspModule.MemoryPage = MOD_MATRIX_MIX_ALG0_STAGE0_TARGET_03_03_MEMORYPAGE;
}

void Default_Matrix_Param(MatrixType* Matrix)
{
  uint8_t out;
  
  //Matrix Default Param Init
  for(out = 0; out < OUT_CH_QTY; out++)
  {
    Matrix[out].Gain = MatrixGain_Min;
    Matrix[out].Mute = MuteValueType_Def;
    //Matrix[out].MutePrev = MuteValueType_Def;
  }
}


//USER EQ-----------------------------------------------------------------------
void UserEQ_Default_Param(UserEQType* UserEQ)
{
  Default_Gain_Param(&UserEQ->Gain, Gain_Def);
  Default_Delay_Param(&UserEQ->Delay, UserEQDelay_Def);
  Default_Mute_Param(&UserEQ->Mute, MuteValueType_Def);
  Default_Polarity_Param(&UserEQ->Polarity, Polarity_Def);
  Default_HP_Filter_Param(&UserEQ->FilterHighPass);
  Default_LP_Filter_Param(&UserEQ->FilterLowPass); 
  for(uint8_t eq = 0; eq < USREQ_EQ_QTY; eq++) Default_EQ_Filter_Param(&UserEQ->FilterEQ[eq]);  
}

void UserEQ_Init(UserEQType* UserEQ)
{
  uint8_t i;
  
  for(i = 0; i < IN_CH_QTY; i++) UserEQ_Default_Param(&UserEQ[i]);
}

void UserEQ_DSP_Addr_Init(UserEQType* UserEQ)
{
  //User EQ MUTE Addr in DSP
  UserEQ[0].Mute.DspModule.Address = MOD_USEREQ1_MUTE_ALG0_MUTE_ADDR;
  UserEQ[0].Mute.DspModule.MemoryPage = MOD_USEREQ1_MUTE_ALG0_MUTE_MEMORYPAGE;
  UserEQ[1].Mute.DspModule.Address = MOD_USEREQ2_MUTE_ALG0_MUTE_ADDR;
  UserEQ[1].Mute.DspModule.MemoryPage = MOD_USEREQ2_MUTE_ALG0_MUTE_MEMORYPAGE;
  UserEQ[2].Mute.DspModule.Address = MOD_USEREQ3_MUTE_ALG0_MUTE_ADDR;
  UserEQ[1].Mute.DspModule.MemoryPage = MOD_USEREQ3_MUTE_ALG0_MUTE_MEMORYPAGE;
  UserEQ[3].Mute.DspModule.Address = MOD_USEREQ4_MUTE_ALG0_MUTE_ADDR;
  UserEQ[1].Mute.DspModule.MemoryPage = MOD_USEREQ4_MUTE_ALG0_MUTE_MEMORYPAGE;
  
  //User EQ Polarity Addr in DSP
  UserEQ[0].Polarity.DspModule.Address = MOD_USEREQ1_PHASE_ALG0_EQS300INVERT9INVERT_ADDR;
  UserEQ[0].Polarity.DspModule.MemoryPage = MOD_USEREQ1_PHASE_ALG0_EQS300INVERT9INVERT_MEMORYPAGE;
  UserEQ[1].Polarity.DspModule.Address = MOD_USEREQ2_PHASE_ALG0_EQS300INVERT1INVERT_ADDR;
  UserEQ[1].Polarity.DspModule.MemoryPage = MOD_USEREQ2_PHASE_ALG0_EQS300INVERT1INVERT_MEMORYPAGE;
  UserEQ[2].Polarity.DspModule.Address = MOD_USEREQ3_PHASE_ALG0_EQS300INVERT2INVERT_ADDR;
  UserEQ[2].Polarity.DspModule.MemoryPage = MOD_USEREQ3_PHASE_ALG0_EQS300INVERT2INVERT_MEMORYPAGE;
  UserEQ[3].Polarity.DspModule.Address = MOD_USEREQ4_PHASE_ALG0_EQS300INVERT3INVERT_ADDR;
  UserEQ[3].Polarity.DspModule.MemoryPage = MOD_USEREQ4_PHASE_ALG0_EQS300INVERT3INVERT_MEMORYPAGE;
  
  
  //User EQ GAIN Addr in DSP
  UserEQ[0].Gain.DspModule.Address = MOD_USEREQ1_GAIN_ALG0_TARGET_ADDR;
  UserEQ[0].Gain.DspModule.MemoryPage = MOD_USEREQ1_GAIN_ALG0_TARGET_MEMORYPAGE;
  
  UserEQ[1].Gain.DspModule.Address = MOD_USEREQ2_GAIN_ALG0_TARGET_ADDR;
  UserEQ[1].Gain.DspModule.MemoryPage = MOD_USEREQ2_GAIN_ALG0_TARGET_MEMORYPAGE;
  
  UserEQ[2].Gain.DspModule.Address = MOD_USEREQ3_GAIN_ALG0_TARGET_ADDR;
  UserEQ[2].Gain.DspModule.MemoryPage = MOD_USEREQ3_GAIN_ALG0_TARGET_MEMORYPAGE;
  
  UserEQ[3].Gain.DspModule.Address = MOD_USEREQ4_GAIN_ALG0_TARGET_ADDR;
  UserEQ[3].Gain.DspModule.MemoryPage = MOD_USEREQ4_GAIN_ALG0_TARGET_MEMORYPAGE;
  
  //User EQ DELAY Addr in DSP
  UserEQ[0].Delay.DspModule.Address = MOD_USEREQ1_DELAY_ALG0_DELAYAMT_ADDR;
  UserEQ[0].Delay.DspModule.MemoryPage = MOD_USEREQ1_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  UserEQ[1].Delay.DspModule.Address = MOD_USEREQ2_DELAY_ALG0_DELAYAMT_ADDR;
  UserEQ[1].Delay.DspModule.MemoryPage = MOD_USEREQ2_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  UserEQ[2].Delay.DspModule.Address = MOD_USEREQ3_DELAY_ALG0_DELAYAMT_ADDR;
  UserEQ[2].Delay.DspModule.MemoryPage = MOD_USEREQ3_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  UserEQ[3].Delay.DspModule.Address = MOD_USEREQ4_DELAY_ALG0_DELAYAMT_ADDR;
  UserEQ[3].Delay.DspModule.MemoryPage = MOD_USEREQ4_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  //User EQ Channel 1 FILTER EQ Addr in DSP 
  UserEQ[0].FilterEQ[0].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE0_B2_ADDR;                //FilterEQ1
  UserEQ[0].FilterEQ[0].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[1].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE1_B2_ADDR;                //FilterEQ2
  UserEQ[0].FilterEQ[1].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[2].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE2_B2_ADDR;                //FilterEQ3
  UserEQ[0].FilterEQ[2].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[3].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE3_B2_ADDR;                //FilterEQ4
  UserEQ[0].FilterEQ[3].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[4].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE4_B2_ADDR;                //FilterEQ5
  UserEQ[0].FilterEQ[4].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[5].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE5_B2_ADDR;                //FilterEQ6
  UserEQ[0].FilterEQ[5].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[6].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE6_B2_ADDR;                //FilterEQ7
  UserEQ[0].FilterEQ[6].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[7].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE7_B2_ADDR;                //FilterEQ8
  UserEQ[0].FilterEQ[7].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[8].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE8_B2_ADDR;                //FilterEQ9
  UserEQ[0].FilterEQ[8].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  UserEQ[0].FilterEQ[9].DspModule.Address =     MOD_USEREQ1_EQ_ALG0_STAGE9_B2_ADDR;                //FilterEQ10
  UserEQ[0].FilterEQ[9].DspModule.MemoryPage =  MOD_USEREQ1_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  
  //User EQ Channel 2 FILTER EQ Addr in DSP 
  UserEQ[1].FilterEQ[0].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE0_B2_ADDR;                //FilterEQ1
  UserEQ[1].FilterEQ[0].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[1].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE1_B2_ADDR;                //FilterEQ2
  UserEQ[1].FilterEQ[1].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[2].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE2_B2_ADDR;                //FilterEQ3
  UserEQ[1].FilterEQ[2].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[3].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE3_B2_ADDR;                //FilterEQ4
  UserEQ[1].FilterEQ[3].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[4].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE4_B2_ADDR;                //FilterEQ5
  UserEQ[1].FilterEQ[4].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[5].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE5_B2_ADDR;                //FilterEQ6
  UserEQ[1].FilterEQ[5].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[6].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE6_B2_ADDR;                //FilterEQ7
  UserEQ[1].FilterEQ[6].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[7].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE7_B2_ADDR;                //FilterEQ8
  UserEQ[1].FilterEQ[7].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[8].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE8_B2_ADDR;                //FilterEQ9
  UserEQ[1].FilterEQ[8].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  UserEQ[1].FilterEQ[9].DspModule.Address =     MOD_USEREQ2_EQ_ALG0_STAGE9_B2_ADDR;                //FilterEQ10
  UserEQ[1].FilterEQ[9].DspModule.MemoryPage =  MOD_USEREQ2_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  
  //User EQ Channel 3 FILTER EQ Addr in DSP 
  UserEQ[2].FilterEQ[0].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE0_B2_ADDR;                //FilterEQ1
  UserEQ[2].FilterEQ[0].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[1].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE1_B2_ADDR;                //FilterEQ2
  UserEQ[2].FilterEQ[1].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[2].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE2_B2_ADDR;                //FilterEQ3
  UserEQ[2].FilterEQ[2].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[3].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE3_B2_ADDR;                //FilterEQ4
  UserEQ[2].FilterEQ[3].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[4].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE4_B2_ADDR;                //FilterEQ5
  UserEQ[2].FilterEQ[4].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[5].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE5_B2_ADDR;                //FilterEQ6
  UserEQ[2].FilterEQ[5].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[6].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE6_B2_ADDR;                //FilterEQ7
  UserEQ[2].FilterEQ[6].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[7].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE7_B2_ADDR;                //FilterEQ8
  UserEQ[2].FilterEQ[7].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[8].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE8_B2_ADDR;                //FilterEQ9
  UserEQ[2].FilterEQ[8].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  UserEQ[2].FilterEQ[9].DspModule.Address =     MOD_USEREQ3_EQ_ALG0_STAGE9_B2_ADDR;                //FilterEQ10
  UserEQ[2].FilterEQ[9].DspModule.MemoryPage =  MOD_USEREQ3_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  
  //User EQ Channel 4 FILTER EQ Addr in DSP 
  UserEQ[3].FilterEQ[0].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE0_B2_ADDR;                //FilterEQ1
  UserEQ[3].FilterEQ[0].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[1].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE1_B2_ADDR;                //FilterEQ2
  UserEQ[3].FilterEQ[1].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[2].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE2_B2_ADDR;                //FilterEQ3
  UserEQ[3].FilterEQ[2].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[3].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE3_B2_ADDR;                //FilterEQ4
  UserEQ[3].FilterEQ[3].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[4].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE4_B2_ADDR;                //FilterEQ5
  UserEQ[3].FilterEQ[4].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[5].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE5_B2_ADDR;                //FilterEQ6
  UserEQ[3].FilterEQ[5].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[6].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE6_B2_ADDR;                //FilterEQ7
  UserEQ[3].FilterEQ[6].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[7].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE7_B2_ADDR;                //FilterEQ8
  UserEQ[3].FilterEQ[7].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[8].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE8_B2_ADDR;                //FilterEQ9
  UserEQ[3].FilterEQ[8].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  UserEQ[3].FilterEQ[9].DspModule.Address =     MOD_USEREQ4_EQ_ALG0_STAGE9_B2_ADDR;                //FilterEQ10
  UserEQ[3].FilterEQ[9].DspModule.MemoryPage =  MOD_USEREQ4_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  
  //User EQ HP Filter
  UserEQ[0].FilterHighPass.DspModule1.Address =     MOD_USEREQ1_HP_ALG0_STAGE0_B2_ADDR;                //Filter HP 1
  UserEQ[0].FilterHighPass.DspModule1.MemoryPage =  MOD_USEREQ1_HP_ALG0_STAGE0_B2_MEMORYPAGE;
  UserEQ[0].FilterHighPass.DspModule2.Address =     MOD_USEREQ1_HP_ALG0_STAGE1_B2_ADDR;                //Filter HP 2
  UserEQ[0].FilterHighPass.DspModule2.MemoryPage =  MOD_USEREQ1_HP_ALG0_STAGE1_B2_MEMORYPAGE;
  UserEQ[0].FilterHighPass.DspModule3.Address =     MOD_USEREQ1_HP_ALG0_STAGE2_B2_ADDR;                //Filter HP 3
  UserEQ[0].FilterHighPass.DspModule3.MemoryPage =  MOD_USEREQ1_HP_ALG0_STAGE2_B2_MEMORYPAGE;
  UserEQ[0].FilterHighPass.DspModule4.Address =     MOD_USEREQ1_HP_ALG0_STAGE3_B2_ADDR;                //Filter HP 4
  UserEQ[0].FilterHighPass.DspModule4.MemoryPage =  MOD_USEREQ1_HP_ALG0_STAGE4_B2_MEMORYPAGE;
}

//SPEAKER-----------------------------------------------------------------------
//Init Data Speaker and Addr in DSP
void SpeakerEQ_Init(SpeakerEQType* SpeakerEq)
{
  SpeakerEq[0].Fir = &SPEAKER_EQ_FIR[0];
  SpeakerEq[1].Fir = &SPEAKER_EQ_FIR[1];
  SpeakerEq[2].Fir = &SPEAKER_EQ_FIR[2];
  SpeakerEq[3].Fir = &SPEAKER_EQ_FIR[3];
}

void SpeakerEQ_DSP_Addr_Init(SpeakerEQType* SpeakerEQ)
{
  //Fir addr in dsp
  SpeakerEQ[0].Fir[0].DspModule.Address = MOD_FIR_EQ1_FIR_ALG0_FIRSIGMA300ALG5FIRCOEFF9_ADDR;
  SpeakerEQ[1].Fir[1].DspModule.Address = MOD_FIR_EQ2_FIR_ALG0_FIRSIGMA300ALG6FIRCOEFF9_ADDR;
  SpeakerEQ[2].Fir[2].DspModule.Address = MOD_FIR_EQ3_FIR_ALG0_FIRSIGMA300ALG7FIRCOEFF9_ADDR;
  SpeakerEQ[3].Fir[3].DspModule.Address = MOD_FIR_EQ4_FIR_ALG0_FIRSIGMA300ALG8FIRCOEFF9_ADDR;
  //Fir state Addr in dsp
  SpeakerEQ[0].Fir[0].State.DspModule.Address = MOD_FIR_EQ1_STATE_ALG0_SLEW_MODE_ADDR;
  SpeakerEQ[1].Fir[1].State.DspModule.Address = MOD_FIR_EQ2_STATE_ALG0_SLEW_MODE_ADDR;
  SpeakerEQ[2].Fir[2].State.DspModule.Address = MOD_FIR_EQ3_STATE_ALG0_SLEW_MODE_ADDR;
  SpeakerEQ[3].Fir[3].State.DspModule.Address = MOD_FIR_EQ4_STATE_ALG0_SLEW_MODE_ADDR;
  //HP Filter Addr in dsp
  SpeakerEQ[0].FilterHighPass.DspModule1.Address = MOD_SPEAKEREQ1_HP_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[1].FilterHighPass.DspModule1.Address = MOD_SPEAKEREQ2_HP_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[2].FilterHighPass.DspModule1.Address = MOD_SPEAKEREQ3_HP_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[3].FilterHighPass.DspModule1.Address = MOD_SPEAKEREQ4_HP_ALG0_STAGE0_B2_ADDR;
  //Speaker EQ1 Filter EQ Addr in dsp
  SpeakerEQ[0].FilterEQ[0].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[0].FilterEQ[1].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerEQ[0].FilterEQ[2].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerEQ[0].FilterEQ[3].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerEQ[0].FilterEQ[4].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerEQ[0].FilterEQ[5].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerEQ[0].FilterEQ[6].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerEQ[0].FilterEQ[7].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerEQ[0].FilterEQ[8].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerEQ[0].FilterEQ[9].DspModule.Address = MOD_SPEAKEREQ1_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerEQ[0].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerEQ[0].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEREQ1_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  
  //Speaker EQ2 Filter EQ Addr in dsp
  SpeakerEQ[1].FilterEQ[0].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[1].FilterEQ[1].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerEQ[1].FilterEQ[2].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerEQ[1].FilterEQ[3].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerEQ[1].FilterEQ[4].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerEQ[1].FilterEQ[5].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerEQ[1].FilterEQ[6].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerEQ[1].FilterEQ[7].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerEQ[1].FilterEQ[8].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerEQ[1].FilterEQ[9].DspModule.Address = MOD_SPEAKEREQ2_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerEQ[1].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerEQ[1].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEREQ2_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  //Speaker EQ3 Filter EQ Addr in dsp
  SpeakerEQ[2].FilterEQ[0].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[2].FilterEQ[1].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerEQ[2].FilterEQ[2].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerEQ[2].FilterEQ[3].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerEQ[2].FilterEQ[4].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerEQ[2].FilterEQ[5].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerEQ[2].FilterEQ[6].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerEQ[2].FilterEQ[7].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerEQ[2].FilterEQ[8].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerEQ[2].FilterEQ[9].DspModule.Address = MOD_SPEAKEREQ3_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerEQ[2].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerEQ[2].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEREQ3_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  //Speaker EQ4 Filter EQ Addr in dsp
  SpeakerEQ[3].FilterEQ[0].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[3].FilterEQ[1].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerEQ[3].FilterEQ[2].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerEQ[3].FilterEQ[3].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerEQ[3].FilterEQ[4].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerEQ[3].FilterEQ[5].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerEQ[3].FilterEQ[6].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerEQ[3].FilterEQ[7].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerEQ[3].FilterEQ[8].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerEQ[3].FilterEQ[9].DspModule.Address = MOD_SPEAKEREQ4_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerEQ[3].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerEQ[3].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEREQ4_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  //LP Filter Addr in dsp
  SpeakerEQ[0].FilterLowPass.DspModule.Address = MOD_SPEAKEREQ1_LP_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[1].FilterLowPass.DspModule.Address = MOD_SPEAKEREQ2_LP_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[2].FilterLowPass.DspModule.Address = MOD_SPEAKEREQ3_LP_ALG0_STAGE0_B2_ADDR;
  SpeakerEQ[3].FilterLowPass.DspModule.Address = MOD_SPEAKEREQ4_LP_ALG0_STAGE0_B2_ADDR;
}

void Speaker_Init(SpeakerType* Speaker)
{   
  SpeakerEQ_Init(SPEAKER_EQ);

  //Set point to SPEAKER CONFIG for SPEAKER
 Speaker[0].Config = &SPEAKER_CONFIG[0];
 Speaker[1].Config = &SPEAKER_CONFIG[1];
 Speaker[2].Config = &SPEAKER_CONFIG[2];
 Speaker[3].Config = &SPEAKER_CONFIG[3];
  
  //Set point to SPEKAER EQ for SPEAKER  
  Speaker[0].EQ = &SPEAKER_EQ[0];
  Speaker[1].EQ = &SPEAKER_EQ[1];
  Speaker[2].EQ = &SPEAKER_EQ[2];
  Speaker[3].EQ = &SPEAKER_EQ[3];
  
  //Set point to SPEKAER OUT for SPEAKER
  //SPEAKER 0
  Speaker[0].OUT[0] = &SPEAKER_OUT[0];
  Speaker[0].OUT[1] = &SPEAKER_OUT[1];
  Speaker[0].OUT[2] = &SPEAKER_OUT[2];
  Speaker[0].OUT[3] = &SPEAKER_OUT[3];
  //SPEAKER 1
  Speaker[1].OUT[0] = &SPEAKER_OUT[1];
  Speaker[1].OUT[1] = &SPEAKER_OUT[2];
  Speaker[1].OUT[2] = &SPEAKER_OUT[3];
  //speaker[1].OUT[3] = &SPEAKER_OUT[3];
  //SPEAKER 2
  Speaker[2].OUT[0] = &SPEAKER_OUT[2];
  Speaker[2].OUT[1] = &SPEAKER_OUT[3];

  //SPEAKER 3
  Speaker[3].OUT[0] = &SPEAKER_OUT[3];
  
  //----------------------------------------------------------------------------
  
  //Set point to SPEKAER OUT CONFIG for SPEAKER
  Speaker[0].OUT[0]->SpeakerOutConfig = &SPEAKER_OUT_CONFIG[0];
  Speaker[1].OUT[0]->SpeakerOutConfig = &SPEAKER_OUT_CONFIG[1];
  Speaker[2].OUT[0]->SpeakerOutConfig = &SPEAKER_OUT_CONFIG[2];
  Speaker[3].OUT[0]->SpeakerOutConfig = &SPEAKER_OUT_CONFIG[3];
  
  Speaker[0].OUT[0]->Fir = &SPEAKER_OUT_FIR[0];
  Speaker[1].OUT[0]->Fir = &SPEAKER_OUT_FIR[1];
  Speaker[2].OUT[0]->Fir = &SPEAKER_OUT_FIR[2];
  Speaker[3].OUT[0]->Fir = &SPEAKER_OUT_FIR[3];
  
  //Set Speaker Config Default Param
  Default_Speaker_Config_Param(Speaker[0].Config); Speaker[0].Config->Ways = 1;
  Default_Speaker_Config_Param(Speaker[1].Config); Speaker[1].Config->Ways = 1;
  Default_Speaker_Config_Param(Speaker[2].Config); Speaker[2].Config->Ways = 1;
  Default_Speaker_Config_Param(Speaker[3].Config); Speaker[3].Config->Ways = 1;
  
  //Set Speaker EQ Defaul Param
  Default_SpeakerEQ_Param(Speaker[0].EQ);
  Default_SpeakerEQ_Param(Speaker[1].EQ);
  Default_SpeakerEQ_Param(Speaker[2].EQ);
  Default_SpeakerEQ_Param(Speaker[3].EQ);
  
  //Set Speaker Out Default Param
  Default_Speaker_Out_Param(Speaker[0].OUT[0]);
  Default_Speaker_Out_Param(Speaker[1].OUT[0]);
  Default_Speaker_Out_Param(Speaker[2].OUT[0]);
  Default_Speaker_Out_Param(Speaker[3].OUT[0]);
}  

void Speaker_OUT_DSP_Addr_Init(SpeakerOutType* SpeakerOut)
{
  //MUTE Addr in DSP
  SpeakerOut[0].Mute.DspModule.Address = MOD_SPEAKEROUT1_MUTE_ALG0_MUTE_ADDR;
  SpeakerOut[0].Mute.DspModule.MemoryPage = MOD_SPEAKEROUT1_MUTE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[1].Mute.DspModule.Address = MOD_SPEAKEROUT2_MUTE_ALG0_MUTE_ADDR;
  SpeakerOut[1].Mute.DspModule.MemoryPage = MOD_SPEAKEROUT2_MUTE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[2].Mute.DspModule.Address = MOD_SPEAKEROUT3_MUTE_ALG0_MUTE_ADDR;
  SpeakerOut[2].Mute.DspModule.MemoryPage = MOD_SPEAKEROUT3_MUTE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[3].Mute.DspModule.Address = MOD_SPEAKEROUT4_MUTE_ALG0_MUTE_ADDR;
  SpeakerOut[2].Mute.DspModule.MemoryPage = MOD_SPEAKEROUT4_MUTE_ALG0_MUTE_MEMORYPAGE;
  
  //PHASE Addr in DSP
  SpeakerOut[0].Polarity.DspModule.Address = MOD_SPEAKEROUT1_PHASE_ALG0_EQS300INVERT17INVERT_ADDR;
  SpeakerOut[1].Polarity.DspModule.Address = MOD_SPEAKEROUT2_PHASE_ALG0_EQS300INVERT18INVERT_ADDR;
  SpeakerOut[2].Polarity.DspModule.Address = MOD_SPEAKEROUT3_PHASE_ALG0_EQS300INVERT19INVERT_ADDR;
  SpeakerOut[3].Polarity.DspModule.Address = MOD_SPEAKEROUT4_PHASE_ALG0_EQS300INVERT20INVERT_ADDR;
  
  SpeakerOut[0].Polarity.DspModule.MemoryPage = MOD_SPEAKEROUT1_PHASE_ALG0_EQS300INVERT17INVERT_MEMORYPAGE;
  SpeakerOut[1].Polarity.DspModule.MemoryPage = MOD_SPEAKEROUT2_PHASE_ALG0_EQS300INVERT18INVERT_MEMORYPAGE;
  SpeakerOut[2].Polarity.DspModule.MemoryPage = MOD_SPEAKEROUT3_PHASE_ALG0_EQS300INVERT19INVERT_MEMORYPAGE;
  SpeakerOut[3].Polarity.DspModule.MemoryPage = MOD_SPEAKEROUT4_PHASE_ALG0_EQS300INVERT20INVERT_MEMORYPAGE;
  
  //VOLUME Addr in DSP
  SpeakerOut[0].Gain.DspModule.Address = MOD_SPEAKEROUT1_VOLUME_ALG0_TARGET_ADDR;
  SpeakerOut[0].Gain.DspModule.MemoryPage = MOD_SPEAKEROUT1_VOLUME_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[1].Gain.DspModule.Address = MOD_SPEAKEROUT2_VOLUME_ALG0_TARGET_ADDR;
  SpeakerOut[1].Gain.DspModule.MemoryPage = MOD_SPEAKEROUT2_VOLUME_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[2].Gain.DspModule.Address = MOD_SPEAKEROUT3_VOLUME_ALG0_TARGET_ADDR;
  SpeakerOut[2].Gain.DspModule.MemoryPage = MOD_SPEAKEROUT3_VOLUME_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[3].Gain.DspModule.Address = MOD_SPEAKEROUT4_VOLUME_ALG0_TARGET_ADDR;
  SpeakerOut[3].Gain.DspModule.MemoryPage = MOD_SPEAKEROUT4_VOLUME_ALG0_TARGET_MEMORYPAGE;
  
  //DELAY Addr in DSP
  SpeakerOut[0].Delay.DspModule.Address = MOD_SPEAKEROUT1_DELAY_ALG0_DELAYAMT_ADDR;
  SpeakerOut[0].Delay.DspModule.MemoryPage = MOD_SPEAKEROUT1_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  SpeakerOut[1].Delay.DspModule.Address = MOD_SPEAKEROUT2_DELAY_ALG0_DELAYAMT_ADDR;
  SpeakerOut[1].Delay.DspModule.MemoryPage = MOD_SPEAKEROUT2_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  SpeakerOut[2].Delay.DspModule.Address = MOD_SPEAKEROUT3_DELAY_ALG0_DELAYAMT_ADDR;
  SpeakerOut[2].Delay.DspModule.MemoryPage = MOD_SPEAKEROUT3_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  SpeakerOut[3].Delay.DspModule.Address = MOD_SPEAKEROUT4_DELAY_ALG0_DELAYAMT_ADDR;
  SpeakerOut[3].Delay.DspModule.MemoryPage = MOD_SPEAKEROUT4_DELAY_ALG0_DELAYAMT_MEMORYPAGE;
  
  //FIR FILTER Addr in DSP
  SpeakerOut[0].Fir->DspModule.Address = MOD_FIR_OUT1_FIR_ALG0_FIRSIGMA300ALG1FIRCOEFF9_ADDR;
  SpeakerOut[1].Fir->DspModule.Address = MOD_FIR_OUT2_FIR_ALG0_FIRSIGMA300ALG2FIRCOEFF9_ADDR;
  SpeakerOut[2].Fir->DspModule.Address = MOD_FIR_OUT3_FIR_ALG0_FIRSIGMA300ALG3FIRCOEFF9_ADDR;
  SpeakerOut[3].Fir->DspModule.Address = MOD_FIR_OUT4_FIR_ALG0_FIRSIGMA300ALG4FIRCOEFF9_ADDR;
  //FILTER_EQ Addr in DSP
  //CH1
  SpeakerOut[0].FilterEQ[0].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerOut[0].FilterEQ[1].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerOut[0].FilterEQ[2].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerOut[0].FilterEQ[3].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerOut[0].FilterEQ[4].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerOut[0].FilterEQ[5].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerOut[0].FilterEQ[6].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerOut[0].FilterEQ[7].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerOut[0].FilterEQ[8].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerOut[0].FilterEQ[9].DspModule.Address = MOD_SPEAKEROUT1_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerOut[0].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerOut[0].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEROUT1_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  //CH2
  SpeakerOut[1].FilterEQ[0].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerOut[1].FilterEQ[1].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerOut[1].FilterEQ[2].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerOut[1].FilterEQ[3].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerOut[1].FilterEQ[4].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerOut[1].FilterEQ[5].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerOut[1].FilterEQ[6].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerOut[1].FilterEQ[7].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerOut[1].FilterEQ[8].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerOut[1].FilterEQ[9].DspModule.Address = MOD_SPEAKEROUT2_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerOut[1].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerOut[1].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEROUT2_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  //CH3
  SpeakerOut[2].FilterEQ[0].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerOut[2].FilterEQ[1].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerOut[2].FilterEQ[2].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerOut[2].FilterEQ[3].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerOut[2].FilterEQ[4].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerOut[2].FilterEQ[5].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerOut[2].FilterEQ[6].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerOut[2].FilterEQ[7].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerOut[2].FilterEQ[8].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerOut[2].FilterEQ[9].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerOut[2].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerOut[2].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEROUT3_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  //CH4
  SpeakerOut[3].FilterEQ[0].DspModule.Address = MOD_SPEAKEROUT3_EQ_ALG0_STAGE0_B2_ADDR;
  SpeakerOut[3].FilterEQ[1].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE1_B2_ADDR;
  SpeakerOut[3].FilterEQ[2].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE2_B2_ADDR;
  SpeakerOut[3].FilterEQ[3].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE3_B2_ADDR;
  SpeakerOut[3].FilterEQ[4].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE4_B2_ADDR;
  SpeakerOut[3].FilterEQ[5].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE5_B2_ADDR;
  SpeakerOut[3].FilterEQ[6].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE6_B2_ADDR;
  SpeakerOut[3].FilterEQ[7].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE7_B2_ADDR;
  SpeakerOut[3].FilterEQ[8].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE8_B2_ADDR;
  SpeakerOut[3].FilterEQ[9].DspModule.Address = MOD_SPEAKEROUT4_EQ_ALG0_STAGE9_B2_ADDR;
  
  SpeakerOut[3].FilterEQ[0].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE0_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[1].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE1_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[2].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE2_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[3].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE3_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[4].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE4_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[5].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE5_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[6].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE6_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[7].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE7_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[8].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE8_B2_MEMORYPAGE;
  SpeakerOut[3].FilterEQ[9].DspModule.MemoryPage = MOD_SPEAKEROUT4_EQ_ALG0_STAGE9_B2_MEMORYPAGE;
  
  //RMS_LIMITER Addr in DSP
  //CH1
  SpeakerOut[0].RMSLimiter.Switch.DspModule.Address = MOD_RMSLIMITER1_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[0].RMSLimiter.Switch.DspModule.MemoryPage = MOD_RMSLIMITER1_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[0].RMSLimiter.Threshold.DspModule.Address = MOD_RMSLIMITER1_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[0].RMSLimiter.Threshold.DspModule.MemoryPage = MOD_RMSLIMITER1_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[0].RMSLimiter.Attack.DspModule.Address = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_TC_ADDR;
  SpeakerOut[0].RMSLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_TC_MEMORYPAGE;
  
  SpeakerOut[0].RMSLimiter.Hold.DspModule.Address = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_HOLD_ADDR;
  SpeakerOut[0].RMSLimiter.Hold.DspModule.MemoryPage = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_HOLD_MEMORYPAGE;
  
  SpeakerOut[0].RMSLimiter.Release.DspModule.Address = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_DECAY_ADDR;  
  SpeakerOut[0].RMSLimiter.Release.DspModule.MemoryPage = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_DECAY_MEMORYPAGE;  
  //CH2
  SpeakerOut[1].RMSLimiter.Switch.DspModule.Address = MOD_RMSLIMITER2_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[1].RMSLimiter.Switch.DspModule.MemoryPage = MOD_RMSLIMITER2_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[1].RMSLimiter.Threshold.DspModule.Address = MOD_RMSLIMITER2_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[1].RMSLimiter.Threshold.DspModule.MemoryPage = MOD_RMSLIMITER2_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[1].RMSLimiter.Attack.DspModule.Address = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_TC_ADDR;
  SpeakerOut[1].RMSLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_TC_MEMORYPAGE;
  
  SpeakerOut[1].RMSLimiter.Hold.DspModule.Address = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_HOLD_ADDR;
  SpeakerOut[1].RMSLimiter.Hold.DspModule.MemoryPage = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_HOLD_MEMORYPAGE;
  
  SpeakerOut[1].RMSLimiter.Release.DspModule.Address = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_DECAY_ADDR;
  SpeakerOut[1].RMSLimiter.Release.DspModule.MemoryPage = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_DECAY_MEMORYPAGE;
  
  //CH3
  SpeakerOut[2].RMSLimiter.Switch.DspModule.Address = MOD_RMSLIMITER3_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[2].RMSLimiter.Switch.DspModule.MemoryPage = MOD_RMSLIMITER3_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[2].RMSLimiter.Threshold.DspModule.Address = MOD_RMSLIMITER3_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[2].RMSLimiter.Threshold.DspModule.MemoryPage = MOD_RMSLIMITER3_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[2].RMSLimiter.Attack.DspModule.Address = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_TC_ADDR;
  SpeakerOut[2].RMSLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_TC_MEMORYPAGE;
  
  SpeakerOut[2].RMSLimiter.Hold.DspModule.Address = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_HOLD_ADDR;
  SpeakerOut[2].RMSLimiter.Hold.DspModule.MemoryPage = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_HOLD_MEMORYPAGE;
  
  SpeakerOut[2].RMSLimiter.Release.DspModule.Address = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_DECAY_ADDR;
  SpeakerOut[2].RMSLimiter.Release.DspModule.MemoryPage = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_DECAY_MEMORYPAGE;
  //CH4
  SpeakerOut[3].RMSLimiter.Switch.DspModule.Address = MOD_RMSLIMITER4_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[3].RMSLimiter.Switch.DspModule.MemoryPage = MOD_RMSLIMITER4_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[3].RMSLimiter.Threshold.DspModule.Address = MOD_RMSLIMITER4_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[3].RMSLimiter.Threshold.DspModule.MemoryPage = MOD_RMSLIMITER4_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  SpeakerOut[3].RMSLimiter.Attack.DspModule.Address = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_TC_ADDR;
  SpeakerOut[3].RMSLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_TC_MEMORYPAGE;
  
  SpeakerOut[3].RMSLimiter.Hold.DspModule.Address = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_HOLD_ADDR;
  SpeakerOut[3].RMSLimiter.Hold.DspModule.MemoryPage = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_HOLD_MEMORYPAGE;
  
  SpeakerOut[3].RMSLimiter.Release.DspModule.Address = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_DECAY_ADDR;
  SpeakerOut[3].RMSLimiter.Release.DspModule.MemoryPage = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_DECAY_MEMORYPAGE;
  
  //PEAK_LIMITER Addr in DSP
  //CH1
  SpeakerOut[0].PeakLimiter.Switch.DspModule.Address = MOD_PEAKLIMITER1_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[0].PeakLimiter.Switch.DspModule.MemoryPage = MOD_PEAKLIMITER1_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[0].PeakLimiter.Threshold.DspModule.Address = MOD_PEAKLIMITER1_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[0].PeakLimiter.Threshold.DspModule.MemoryPage = MOD_PEAKLIMITER1_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  //SpeakerOut[0].PeakLimiter.Attack.DspModule.Address = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_TC_ADDR;
  //SpeakerOut[0].PeakLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER1_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG5_TC_MEMORYPAGE;
  
  SpeakerOut[0].PeakLimiter.Hold.DspModule.Address = MOD_PEAKLIMITER1_PEAK_ALG0_HOLD_ADDR;
  SpeakerOut[0].PeakLimiter.Hold.DspModule.MemoryPage = MOD_PEAKLIMITER1_PEAK_ALG0_HOLD_MEMORYPAGE;
  
  SpeakerOut[0].PeakLimiter.Release.DspModule.Address = MOD_PEAKLIMITER1_PEAK_ALG0_DECAY_ADDR;  
  SpeakerOut[0].PeakLimiter.Release.DspModule.MemoryPage = MOD_PEAKLIMITER1_PEAK_ALG0_DECAY_MEMORYPAGE;  
  //CH2
  SpeakerOut[1].PeakLimiter.Switch.DspModule.Address = MOD_PEAKLIMITER2_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[1].PeakLimiter.Switch.DspModule.MemoryPage = MOD_PEAKLIMITER2_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[1].PeakLimiter.Threshold.DspModule.Address = MOD_PEAKLIMITER2_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[1].PeakLimiter.Threshold.DspModule.MemoryPage = MOD_PEAKLIMITER2_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  //SpeakerOut[1].PeakLimiter.Attack.DspModule.Address = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_TC_ADDR;
  //SpeakerOut[1].PeakLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER2_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG1_TC_MEMORYPAGE;
  
  SpeakerOut[1].PeakLimiter.Hold.DspModule.Address = MOD_PEAKLIMITER2_PEAK_ALG0_HOLD_ADDR;
  SpeakerOut[1].PeakLimiter.Hold.DspModule.MemoryPage = MOD_PEAKLIMITER2_PEAK_ALG0_HOLD_MEMORYPAGE;
  
  SpeakerOut[1].PeakLimiter.Release.DspModule.Address = MOD_PEAKLIMITER2_PEAK_ALG0_DECAY_ADDR;
  SpeakerOut[1].PeakLimiter.Release.DspModule.MemoryPage = MOD_PEAKLIMITER2_PEAK_ALG0_DECAY_MEMORYPAGE;
  
  //CH3
  SpeakerOut[2].PeakLimiter.Switch.DspModule.Address = MOD_PEAKLIMITER3_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[2].PeakLimiter.Switch.DspModule.MemoryPage = MOD_PEAKLIMITER3_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[2].PeakLimiter.Threshold.DspModule.Address = MOD_PEAKLIMITER3_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[2].PeakLimiter.Threshold.DspModule.MemoryPage = MOD_PEAKLIMITER3_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  //SpeakerOut[2].PeakLimiter.Attack.DspModule.Address = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_TC_ADDR;
  //SpeakerOut[2].PeakLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER3_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG2_TC_MEMORYPAGE;
  
  SpeakerOut[2].PeakLimiter.Hold.DspModule.Address = MOD_PEAKLIMITER3_PEAK_ALG0_HOLD_ADDR;
  SpeakerOut[2].PeakLimiter.Hold.DspModule.MemoryPage = MOD_PEAKLIMITER3_PEAK_ALG0_HOLD_MEMORYPAGE;
  
  SpeakerOut[2].PeakLimiter.Release.DspModule.Address = MOD_PEAKLIMITER3_PEAK_ALG0_DECAY_ADDR;
  SpeakerOut[2].PeakLimiter.Release.DspModule.MemoryPage = MOD_PEAKLIMITER3_PEAK_ALG0_DECAY_MEMORYPAGE;
  //CH4
  SpeakerOut[3].PeakLimiter.Switch.DspModule.Address = MOD_PEAKLIMITER4_STATE_ALG0_MUTE_ADDR;
  SpeakerOut[3].PeakLimiter.Switch.DspModule.MemoryPage = MOD_PEAKLIMITER4_STATE_ALG0_MUTE_MEMORYPAGE;
  
  SpeakerOut[3].PeakLimiter.Threshold.DspModule.Address = MOD_PEAKLIMITER4_THRESHOLD_ALG0_TARGET_ADDR;
  SpeakerOut[3].PeakLimiter.Threshold.DspModule.MemoryPage = MOD_PEAKLIMITER4_THRESHOLD_ALG0_TARGET_MEMORYPAGE;
  
  //SpeakerOut[3].PeakLimiter.Attack.DspModule.Address = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_TC_ADDR;
  //SpeakerOut[3].PeakLimiter.Attack.DspModule.MemoryPage = MOD_RMSLIMITER4_RMS_ALG0_MULTICHANRMSNOPOSTGAINS300FULLRANGEEXPGAINALG6_TC_MEMORYPAGE;
  
  SpeakerOut[3].PeakLimiter.Hold.DspModule.Address = MOD_PEAKLIMITER4_PEAK_ALG0_HOLD_ADDR;
  SpeakerOut[3].PeakLimiter.Hold.DspModule.MemoryPage = MOD_PEAKLIMITER4_PEAK_ALG0_HOLD_MEMORYPAGE;
  
  SpeakerOut[3].PeakLimiter.Release.DspModule.Address = MOD_PEAKLIMITER4_PEAK_ALG0_DECAY_ADDR;
  SpeakerOut[3].PeakLimiter.Release.DspModule.MemoryPage = MOD_PEAKLIMITER4_PEAK_ALG0_DECAY_MEMORYPAGE;
  
  
}

void Default_Speaker_Config_Param(SpeakerConfigType* speaker_config)
{ 
  strcopy(speaker_config->BrandName, "SPK BRAND");
  strcopy(speaker_config->SeriesName, "SPK SERIES");
  strcopy(speaker_config->ModelName, "SPK MODEL");
  strcopy(speaker_config->VariationName, "SPK VARIATION");
  strcopy(speaker_config->Notes, "SPK NOTES");

  speaker_config->SpeakerType = PointSource;
  speaker_config->Ways = 1;
  speaker_config->HizHPF.Freq = HP_Freq_Def;
  speaker_config->HizHPF.State = HP_State_Def;
  
  //Speaker_Config_Flash_Write(speaker_config);
}

void Default_Delay_Param(DelayParamType* delay, uint32_t def)
{
  delay->Value = def;
}

void Default_Gain_Param(GainParamType* Gain, int16_t def)
{
  Gain->Value = def;
}

void Default_Mute_Param(MuteParamType* mute, MuteValueType def)
{
  mute->Value = def;
}

void Default_Polarity_Param(PolarityParamType* polarity, PolarityValueType def)
{
  polarity->Value = def;
}

void Default_HP_Filter_Param(FilterHPParamType* filter)
{
  filter->State = HP_State_Def;
  filter->Filter = HP_Filter_Def;
  filter->Freq = HP_Freq_Def;
}

void Default_LP_Filter_Param(FilterLPParamType* filter)
{
  filter->State = LP_State_Def;
  filter->Filter = LP_Filter_Def;
  filter->Freq = LP_Freq_Def;
}

void Default_EQ_Filter_Param(FilterEQParamType* filter)
{
  filter->State = EQ_State_Def;
  filter->Boost = EQ_Boost_Def;
  filter->Filter = EQ_Filter_Def;
  filter->Freq = EQ_Freq_Def;
  filter->Q = EQ_Q_Def;  
}

void Default_FIR_Filter_Param(FilterFirParamType* fir_filter)
{
  fir_filter->State.State = FIR_State_Def;
  fir_filter->Smpl = FIR_Smpl_Def;
  fir_filter->Taps = FIR_Taps_Max;
 
  uint16_t i;
  
  fir_filter->Coef[0] = 1.0;
  for(i = 1; i < fir_filter->Taps; i++) fir_filter->Coef[i] = 0.0;
}

void Default_RMS_Limiter_Param(LimiterRMSParamType* limiter)
{
  //Limiter SWITCH state
  limiter->Switch.State = LimRMS_State_Def;
  limiter->Switch.PrevState = limiter->Switch.State;
  
  //Limiter THRESHOLD param
  limiter->Threshold.Value = LimRMS_Threshold_Def;
  limiter->Threshold.ValuePrev = limiter->Threshold.Value;
  limiter->Threshold.ValueCoef = LimRMS_Threshold_Coef;
  
  //Limiter ATTACK param
  limiter->Attack.Value = LimRMS_Attack_Def;
  //limiter->Attack.ValuePrev = limiter->Attack.Value;
  limiter->Attack.ValueCoef = LimRMS_Attack_Coef;
  
  //Limiter RELEASE param
  limiter->Release.Value = LimRMS_Release_Def;
  limiter->Release.ValuePrev = limiter->Release.Value;
  limiter->Release.ValueCoef = LimRMS_Release_Coef;
  
  //Limiter HOLD param
  limiter->Hold.Value = LimRMS_Hold_Def;
  limiter->Hold.ValuePrev = limiter->Hold.Value;
  limiter->Hold.ValueCoef = LimRMS_Hold_Coef;
}

void Default_Peak_Limiter_Param(LimiterPeakParamType* limiter)
{
    //Limiter SWITCH state
  limiter->Switch.State = LimPeak_State_Def;
  limiter->Switch.PrevState = limiter->Switch.State;
  
  //Limiter THRESHOLD param
  limiter->Threshold.Value = LimPeak_Threshold_Def;
  limiter->Threshold.ValuePrev = limiter->Threshold.Value;
  limiter->Threshold.ValueCoef = LimPeak_Threshold_Coef;
  
  //Limiter ATTACK param
  //limiter->Attack.Value = LimPeak_Attack_Def;
  ////limiter->Attack.ValuePrev = limiter->Attack.Value;
  //limiter->Attack.ValueCoef = LimPeak_Attack_Coef;
  
  //Limiter RELEASE param
  limiter->Release.Value = LimPeak_Release_Def;
  limiter->Release.ValuePrev = limiter->Release.Value;
  limiter->Release.ValueCoef = LimPeak_Release_Coef;
  
  //Limiter HOLD param
  limiter->Hold.Value = LimPeak_Hold_Def;
  limiter->Hold.ValuePrev = limiter->Hold.Value;
  limiter->Hold.ValueCoef = LimPeak_Hold_Coef;
  
  //limiter->Switch.State = LimPeak_State_Def;
  //limiter->Threshold.Value = LimPeak_Threshold_Def; 
  //limiter->Attack.Value = LimPeak_Attack_Def;
  //limiter->Release.Value = LimPeak_Release_Def;
  //limiter->Hold.Value = LimPeak_Hold_Def;
}

void Default_Speaker_Out_Config_Param(SpeakerOutConfigType* speaker_out_config)
{
  uint8_t i;
  
  arrinit(speaker_out_config->Name, sizeof(speaker_out_config->Name), '-');
  speaker_out_config->Index = 0;
  for(i = 0; i < 10; i++) speaker_out_config->HizPower[i].Voltage = HizVoltageTyp_Def;
}

void Default_SpeakerEQ_Param(SpeakerEQType* speaker_eq)
{
  uint8_t i = 0;
    
  Default_HP_Filter_Param(&speaker_eq->FilterHighPass);
  for(i = 0; i < SPKEQ_EQ_QTY; i++) Default_EQ_Filter_Param(&speaker_eq->FilterEQ[i]);  
  Default_LP_Filter_Param(&speaker_eq->FilterLowPass);
  Default_FIR_Filter_Param(speaker_eq->Fir);
}

void Default_Speaker_Out_Param(SpeakerOutType* speaker_out)
{
  uint8_t i;
  
  Default_Speaker_Out_Config_Param(speaker_out->SpeakerOutConfig);                              //Set Default Out Config
  Default_Mute_Param(&speaker_out->Mute, MuteValueType_Def);                                    //Set Default Out Mute     
  
  Default_Polarity_Param(&speaker_out->Polarity, PolarityValueType_Def);                        //Set Default Out Polarity 
  Default_Gain_Param(&speaker_out->Gain, Gain_Def);                                       //Set Default Out Gain
  Default_Delay_Param(&speaker_out->Delay, SpkOutDelay_Def);                                    //Set Default Out Delay 
  Default_LP_Filter_Param(&speaker_out->FilterLowPass);                                         //Set Default Out Low pass filter
  for(i = 0; i < SPKOUT_EQ_QTY; i++) Default_EQ_Filter_Param(&speaker_out->FilterEQ[i]);        //Set Default Out Filter EQ
  Default_HP_Filter_Param(&speaker_out->FilterHighPass);                                        //Set Default Out High pass filter 
  Default_FIR_Filter_Param(speaker_out->Fir);                                                  //Set Default Out FIR            
  Default_RMS_Limiter_Param(&speaker_out->RMSLimiter);                                          //Set Default Out RMS limiter 
  Default_Peak_Limiter_Param(&speaker_out->PeakLimiter);                                        //Set Default Out Peak Limiter 
}

//void Default_Speaker(SpeakerType* speaker)
//{
//  uint8_t i = 0;
//  uint8_t ways = speaker->Config->Ways;
//  
//  if(ways == 0) 
//  {
//    Default_Speaker_Config_Param(speaker->Config);
//    Default_Speaker_Out_Param(speaker->OUT[0]);
//    Default_SpeakerEQ_Param(speaker->EQ);
//  }
//  else 
//  {
//    Default_Speaker_Config_Param(speaker->Config);
//    Default_SpeakerEQ_Param(speaker->EQ);
//    for(i = 0; i < ways; i++) 
//    {
//      Default_Speaker_Out_Param(speaker->OUT[i]);
//      DSP_Speaker_OUT_Write(speaker->OUT[i]);
//    }
//  }
//}

//Def
//void Default_Bridged_Speakers(PresetParamType* Preset, SpeakerType* Speaker, uint8_t channel)
//{
//  uint8_t i = 0;
//  uint8_t ways = Speaker[channel].Config->Ways;
//  uint8_t offset = 0;
//
//  for(i = 0; i < ways; i++) 
//  {
//    //??????????
//    UserEQ_Default_Param(&USER_EQ[channel + i]);
//    
//    Default_Speaker_Config_Param(Speaker[channel +i +offset].Config);
//    Speaker[channel +i +offset].Config->MemoryConfig.IsChanged = true;
//    Default_SpeakerEQ_Param(Speaker[channel +i +offset].EQ);
//    
//    Default_Speaker_Out_Param(Speaker[channel].OUT[i]);
//    Speaker[channel].OUT[i]->MemoryConfig.IsChanged = true;
//
//    //check if speaker is bridged
//    if(!((channel +i) %2))
//    {
//      if(Preset->Bridge[(channel +i) /2].State == BRIDGE) offset++;
//    }
//  }
//}

void Default_Speaker(SpeakerType* Speaker)
{
  uint8_t i;
  
  for(i = 0; i < Speaker->Config->Ways; i++) 
  {
    Default_Speaker_Out_Param(Speaker->OUT[i]);
    DSP_Speaker_OUT_Write(Speaker->OUT[i]);
  }
  
  Default_SpeakerEQ_Param(Speaker->EQ);
  DSP_SpeakerEQ_Write(Speaker->EQ);

  Default_Speaker_Config_Param(Speaker->Config);
}

//void Default_Linked_Speaker(PresetParamType* Preset, SpeakerType* Speaker, uint8_t channel)
//{
//  uint8_t i = 0;
//  uint8_t ways = Speaker[channel].Config->Ways;
//  uint8_t offset = 0;
//
//  for(i = 0; i < ways; i++) 
//  {
//    Speaker_Config_Default_Param(Speaker[channel +i +offset].Config);
//    SpeakerEQ_Default_Param(Speaker[channel +i +offset].EQ);
//    Default_Speaker_Out_Param(Speaker[channel].OUT[i]);
//    
//    if(!((channel +i) %2))
//    {
//      if(Preset->Bridge[(channel +i) /2].State == BRIDGE) offset++;
//    }
//  }
//}

void Speaker_Lib_Init(SpeakerLibType* SpeakerLib)
{
  uint8_t i;
  uint32_t addr;
  uint8_t buff[HEADER_SPK_LIB];
  //uint8_t HeaderSize = HEADER_SPK_LIB; //Name size + ways qty
  
  for(i = 0; i < LIB_SPEAKERS_QTY; i++)
  {
    addr = SpeakerLib->BaseFlashAddress + (i * SpeakerLib->BaseFlashAddress);
    Flash_Read(addr, buff, HEADER_SPK_LIB);
    
    if(buff[0] == 0xFF) 
    {
      strcopy(SpeakerLib->Speaker[i].Name, "EMPTY");
      SpeakerLib->Speaker[i].Way = 1;
    }
    else 
    {
      strcopy(SpeakerLib->Speaker[i].Name, buff);
      SpeakerLib->Speaker[i].Way = buff[HEADER_SPK_LIB-1];
    }
  }
}

//SPEAKER Load------------------------------------------------------------------

void Load_Speaker(uint8_t channel, uint8_t ways, SpeakerType* Speaker, uint8_t* FileBuff)
{
  uint8_t i = 0;
  uint8_t offset = 0;
  
  uint8_t tmp = (1 << channel);
  tmp = tmp & LoadSpkFreeWays;
  
  if(tmp) 
  {
    for(i = 0; i < ways; i++) 
    {
      if(Check_Bridged_Channel(channel +i)) offset++;
      Speaker_Reset(Speaker, channel +i +offset);
      UserEQ_Reset(&USER_EQ[channel +i +offset]);
    }
    
    if(ways > 1) Link_Speaker(channel, Speaker, ways);

    FileFormatReadSpeaker(&Speaker[channel], FileBuff +HEADER_SPK_LIB);
    
    Link_OUT_Matrix(Speaker, LINK_MATRIX);  
  }
}

uint8_t Found_Master_Channel(uint8_t channel, SpeakerType* Speaker)
{
  uint8_t master_channel = 0;
  
  for(master_channel = channel; Speaker[master_channel].Config->Ways == 0; master_channel--) 
  {
    if(master_channel == 0) break;
  }
  
  //if(channel > 0)
  //{
  //  if(Speaker[master_channel].Config->Ways == 0)
  //  {
  //    for(master_channel = channel; Speaker[master_channel].Config->Ways == 0; master_channel--); 
  //  }
  //}
  //
  //if(Speaker[channel].Config->Ways > 0)
  //{
  //  master_channel = channel;
  //}
  //else 
  //{
  //  for(master_channel = channel; Speaker[master_channel].Config->Ways == 0; master_channel--) if(master_channel == 0) break;
  //}

  return master_channel;
}

//void Unlink_Speaker(uint8_t* channel, SpeakerType* destination_speaker)
//{
//  uint8_t i;
//  uint8_t ways = destination_speaker[*channel].Config->Ways;
//
//  for(i = 0; i < ways; i++) destination_speaker[*channel +i].Config->Ways = UNLINKED_CHANNEL;       //set Ways = 1 param for all channel which was linked
//}

void Link_Speaker(uint8_t channel, SpeakerType* Speaker, uint8_t ways)
{
  uint8_t i;
  uint8_t offset = 0;

  for(i = 1; i < ways; i++) 
  {
    if(Check_Bridged_Channel(channel +i)) offset++;
    Speaker[channel +i +offset].Config->Ways = LINKED_CHANNEL;
    Speaker[channel +i +offset].Config->MemoryConfig.IsChanged = true;
  }
}

void UnLink_Speaker(uint8_t channel, SpeakerType* Speaker, uint8_t ways)
{
  uint8_t i;
  uint8_t offset = 0;

  for(i = 1; i < ways; i++) 
  {
    if(Check_Bridged_Channel(channel +i)) offset++;
    Speaker[channel +i +offset].Config->Ways = UNLINKED_CHANNEL;
    Speaker[channel +i +offset].Config->MemoryConfig.IsChanged = true;
  }
}

BridgeValueType Check_Bridged_Channel(uint8_t channel)
{
  BridgeValueType Status = UNBRIDGE;

  if(channel %2)
  {
    if(PRESET_PARAM.Bridge[channel /2].State == BRIDGE) Status = BRIDGE;
  }
    
  return Status;
}

//void Load_Speaker_Data(uint8_t* channel, LoadSpeakerType* load_speaker, SpeakerType* destination_speaker)
//{
//  uint8_t i;
//  
//  destination_speaker[*channel].Config = &load_speaker->Config;
//  destination_speaker[*channel].EQ = &load_speaker->EQ;
//  
//  for(i = 0; i < load_speaker->Config.Ways; i++) destination_speaker[*channel].OUT[i] = &load_speaker->OUT[i];          //load data to select speaker 
//}

void Load_Speaker_From_Lib(uint8_t index_skp_lib, uint8_t channel, uint8_t ways, SpeakerType* Speaker, uint8_t* FileBuff)
{
  Speaker_Flash_Read(index_skp_lib, FileBuff);
  Load_Speaker(channel, ways, Speaker, FileBuff);
}

///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void Speaker_Info_Update(SpeakerInfoType* SpeakerInfo, SpeakerType* Speaker)
{
  uint8_t i;
  uint8_t spk;
  
  for(spk = 0; spk < 4; spk++)
  {
    if(Speaker[spk].Config->Ways == 1) 
    {
      for(i = 0; i < LCD_SPK_NAME_SIZE; i++) SpeakerInfo[spk].SpeakerName[i] = Speaker[spk].Config->ModelName[i];
    }
    else if(Speaker[spk].Config->Ways < 1) 
    {
      for(i = 0; i < LCD_SPK_NAME_SIZE; i++) SpeakerInfo[spk].SpeakerName[i] = Speaker[spk-1].Config->ModelName[i];
    }
  }
}

//------------------------------------------------------------------------------
void Link_OUT_Matrix(SpeakerType* speaker, LinkMatrixParamType* link_matrix)
{
  uint8_t spk = 0;
  uint8_t input = 0;
  
  for(spk = 0; spk < OUT_CH_QTY; spk++)
  {
    if(speaker[spk].Config->Ways > 0) input = spk;
    link_matrix[spk].Out = input;
  }
  
  DSP_Link_Out_Matrix(link_matrix);
}

/*
void Default_Matrix_Param(MatrixType* matrix)
{
  uint8_t i;
  
  for(i = 0; i < 8; i++)
  {
    matrix->Input_1[i].MaxLevel = 0;            //MAX matrix gain = 0dB
    matrix->Input_1[i].MinLevel = -6000;        //MIN matrix gain = -60dB -60*100=-6000
    matrix->Input_2[i].MaxLevel = 0;    
    matrix->Input_2[i].MinLevel = -6000;
    matrix->Input_3[i].MaxLevel = 0;    
    matrix->Input_3[i].MinLevel = -6000;
    matrix->Input_4[i].MaxLevel = 0;    
    matrix->Input_4[i].MinLevel = -6000;
    matrix->Input_5[i].MaxLevel = 0;    
    matrix->Input_5[i].MinLevel = -6000;
    matrix->Input_6[i].MaxLevel = 0;    
    matrix->Input_6[i].MinLevel = -6000;
    matrix->Input_7[i].MaxLevel = 0;    
    matrix->Input_7[i].MinLevel = -6000;
    matrix->Input_8[i].MaxLevel = 0;    
    matrix->Input_8[i].MinLevel = -6000;
  }  
}
*/

/*
void Input_Setting_Init(InputSettingType* InputSetting)
{
  InputSetting->AESSetting.AESDelay.MaxDelay = 1000;                    //MAX AES delay = 10ms 10*100=1000
  InputSetting->AESSetting.AESDelay.MinDelay = 0;                       //MIN AES delay = 0ms 
  InputSetting->AESSetting.AESGain.MaxLevel = 1800;                     //MAX AES gain = +18dB 18*100=1800  
  InputSetting->AESSetting.AESGain.MinLevel = -3000;                    //MIN AES gain = -30dB -30*100=-3000
  
  InputSetting->AnalogSetting.AnalogDelay.MaxDelay = 1000;              //MAX Analog delay = 10ms 10*100=1000
  InputSetting->AnalogSetting.AnalogDelay.MinDelay = 0;                 //MIN Analog delay = 0ms 
  InputSetting->AnalogSetting.AnalogGain.MaxLevel = 1800;               //MAX Analog gain = +18dB 18*100=1800 
  InputSetting->AnalogSetting.AnalogGain.MinLevel = -3000;              //MIN AES gain = -30dB -30*100=-3000
}
*/

//Get Data from ENCODER and BUTTONS---------------------------------------------
uint8_t Get_Control_Data()
{
  if(Encoder_Scan(&EncoderControl)) return 1;
  else if(Button_Scan(&ButtonControl)) return 1;
  
  return 0;
}

//MENU--------------------------------------------------------------------------
//Changed param from menu-------------------------------------------------------

void Param_Change(ControlStateType* control, uint8_t channel)
{
  switch(selectedMenuItem->Param->ParamType)
  {
    case OUT_VOLUME : Menu_Spk_Gain(USER_EQ, control, channel); break;
    case OUT_DELAY : Menu_Spk_Delay(USER_EQ, control, channel); break;
    case MATRIX_IN1 : Menu_Matrix_Gain(&PRESET_PARAM, 0, control, channel); break;
    case MATRIX_IN2 : Menu_Matrix_Gain(&PRESET_PARAM, 1, control, channel); break;
    case MATRIX_IN3 : Menu_Matrix_Gain(&PRESET_PARAM, 2, control, channel); break;
    case MATRIX_IN4 : Menu_Matrix_Gain(&PRESET_PARAM, 3, control, channel); break;
    case MODE_XLR1 : Menu_MODE_XLR(&PRESET_PARAM, 0, control); break;
    case MODE_XLR3 : Menu_MODE_XLR(&PRESET_PARAM, 1, control); break;
    case AES_DELAY : Menu_Trim_Delay(&PRESET_PARAM.AESTrim.Delay, &PRESET_PARAM.MemoryConfig, control); break;
    case AES_GAIN : Menu_Trim_Volume(&PRESET_PARAM.AESTrim.Gain, &PRESET_PARAM.MemoryConfig, control); break;
    case ANALOG_DELAY : Menu_Trim_Delay(&PRESET_PARAM.AnalogTrim.Delay, &PRESET_PARAM.MemoryConfig, control); break;
    case ANALOG_GAIN : Menu_Trim_Volume(&PRESET_PARAM.AnalogTrim.Gain, &PRESET_PARAM.MemoryConfig, control); break;
    case SOURCE_INPUT_1 : Menu_Input_Source(&PRESET_PARAM, 0, control); break;
    case SOURCE_INPUT_2 : Menu_Input_Source(&PRESET_PARAM, 1, control); break;
    case SOURCE_INPUT_3 : Menu_Input_Source(&PRESET_PARAM, 2, control); break;
    case SOURCE_INPUT_4 : Menu_Input_Source(&PRESET_PARAM, 3, control); break;
    case MENU_LOAD_SPEAKER : Menu_Load_Speaker(SPEAKER, &SPEAKER_LIB, channel, control); break; 
    case MENU_OUT_CONFIG_BRIDGE_AB : Menu_Out_Config_Bridge_Param(&PRESET_PARAM, SPEAKER, 0, control); /*DSP_Bridge(&PRESET_PARAM.Bridge[0]);*/ break;
    case MENU_OUT_CONFIG_BRIDGE_CD : Menu_Out_Config_Bridge_Param(&PRESET_PARAM, SPEAKER, 2, control); /*DSP_Bridge(&PRESET_PARAM.Bridge[1]);*/ break;
    case MENU_SPEAKER_RST : Menu_Speaker_Reset(SPEAKER, channel, control, &SpeakerReset); break;
    case MENU_LOAD_PRESET : Menu_Load_Preset(&PRESET, &PRESET_LIB, control); break;
    case MENU_SYS_RESET : Menu_Sys_Reset(control, &SystemReset); break;
  }
}

void Menu_Trim_Volume(GainParamType* volume, BlockStatusType* MemoryConfig, ControlStateType* control)
{  
  if(control->Comand == INC)
  {
    if(volume->Value < InputTrimGain_Max) volume->Value += 100;
  }
  else if(control->Comand == DEC) 
  {
    if(volume->Value > InputTrimGain_Min) volume->Value -= 100;
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering)
    {
      VeluePrev = volume->Value;
      MenuParamEntering = true;
    }
    else if(VeluePrev != volume->Value)
    {
      Preset_Flash_Write(&PRESET_PARAM, FILE_BUFF);
      VeluePrev = volume->Value;
      MenuParamEntering = false;
    }
    else MenuParamEntering = false;    
  }
  else if(control->Comand == ESC)
  {
    volume->Value = VeluePrev;
  }
  
  DSP_Gain(volume);
}

void Menu_Trim_Delay(DelayParamType* delay, BlockStatusType* MemoryConfig, ControlStateType* control)
{  
  if(control->Comand == INC)
  {
    if(delay->Value < InputTrimDelay_Max) delay->Value += 100;
  }
  else if(control->Comand == DEC) 
  {
    if(delay->Value > InputTrimDelay_Min) delay->Value -= 100;
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering)
    {
      VeluePrev = delay->Value;
      MenuParamEntering = true;
    }
    else if(VeluePrev != delay->Value)
    {
      Preset_Flash_Write(&PRESET_PARAM, FILE_BUFF);
      VeluePrev = delay->Value;
      MenuParamEntering = false;
    }
    else MenuParamEntering = false;
  }
  else if(control->Comand == ESC)
  {
    delay->Value = VeluePrev;
    MenuParamEntering = false;
  }
  
  DSP_Delay(delay);
}

//MENU SPEAKER PARAM--------------------------------------------------------------
void Menu_Spk_Gain(UserEQType* UserEQ, ControlStateType* control, uint8_t channel)
{
  if(control->Comand == INC)
  {
    if(UserEQ[channel].Gain.Value < Gain_Max) UserEQ[channel].Gain.Value += 100;
  }
  else if(control->Comand == DEC) 
  {
    if(UserEQ[channel].Gain.Value > Gain_Min) UserEQ[channel].Gain.Value -= 100;
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering) 
    {
      VeluePrev = UserEQ[channel].Gain.Value;
      MenuParamEntering = true;
    }
    else if(VeluePrev != UserEQ[channel].Gain.Value)
    {
      VeluePrev = UserEQ[channel].Gain.Value;
      MenuParamEntering = false;
      UserEQ_Flash_Write(&UserEQ[channel], FILE_BUFF);
    }
    else MenuParamEntering = false;
  }
  else if(control->Comand == ESC)
  {
    UserEQ[channel].Gain.Value = VeluePrev;
    MenuParamEntering = false;
  }
  
  DSP_Gain(&UserEQ[channel].Gain);
}

void Menu_Spk_Delay(UserEQType* UserEQ, ControlStateType* control, uint8_t channel)
{
  if(control->Comand == INC)
  {
    if(UserEQ[channel].Delay.Value < UserEQDelay_Max)UserEQ[channel].Delay.Value += 100;
  }
  else if(control->Comand == DEC) 
  {
    if(UserEQ[channel].Delay.Value > UserEQDelay_Min) UserEQ[channel].Delay.Value -= 100;
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering)
    {
      VeluePrev = UserEQ[channel].Delay.Value;
      MenuParamEntering = true;
    }
    else if(VeluePrev != UserEQ[channel].Delay.Value)
    {
      VeluePrev = UserEQ[channel].Delay.Value;
      UserEQ_Flash_Write(&UserEQ[channel], FILE_BUFF);
      MenuParamEntering = false;
    }
    else MenuParamEntering = false;
  }
  else if(control->Comand == ESC)
  {
    UserEQ[channel].Delay.Value = VeluePrev;
    MenuParamEntering = false;
  }
  
  DSP_Delay(&UserEQ[channel].Delay);
}

void Menu_Load_Speaker(SpeakerType* Speaker, SpeakerLibType* SpeakerLib, uint8_t channel, ControlStateType* EncoderControl)
{ 
  if(EncoderControl->Comand == ENTER)
  {
    Load_Speaker_From_Lib(SpeakerLib->Index, channel, SpeakerLib->Speaker[SpeakerLib->Index].Way, Speaker, FILE_BUFF);
    
    Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh);  
  }
  else if(EncoderControl->Comand == INC)
  {
    if(SpeakerLib->Index < (LIB_SPEAKERS_QTY -1)) SpeakerLib->Index++;
    Load_Speaker_Button_Mask(SpeakerLib->Speaker[SpeakerLib->Index].Way, BridgeButtonMask);
  }
  else if(EncoderControl->Comand == DEC)
  {
    channel = 0;
    if(SpeakerLib->Index > 0) SpeakerLib->Index--;
    Load_Speaker_Button_Mask(SpeakerLib->Speaker[SpeakerLib->Index].Way, BridgeButtonMask);
  }
  else if(EncoderControl->Comand == ESC)
  {
  }
}

void Menu_Speaker_Reset(SpeakerType* Speaker, uint8_t channel, ControlStateType* EncoderControl, SpeakerResetType* SpkReset)
{
  if(EncoderControl->Comand == INC)
  {
    if(*SpkReset == SpeakerResetNo) *SpkReset = SpeakerResetYes;
  }
  else if(EncoderControl->Comand == DEC)
  {
    if(*SpkReset == SpeakerResetYes) *SpkReset = SpeakerResetNo;
  }
  else if(EncoderControl->Comand == ENTER)
  {
    if(*SpkReset == SpeakerResetYes) 
    {
      Speaker_Reset(Speaker, channel);
      UserEQ_Reset(&USER_EQ[channel]);
      Link_OUT_Matrix(SPEAKER, LINK_MATRIX);
      Screen_Speaker_Reset_Complate();
      Flash_Write_Param();
      *SpkReset = SpeakerResetNo;
    }
  }
  else if(EncoderControl->Comand == ESC) SpkReset = SpeakerResetNo; 
}

void Menu_Sys_Reset(ControlStateType* EncoderControl, SysResetType* SysReset)
{
  if(EncoderControl->Comand == INC)
  {
    if(*SysReset == SysResetNo) *SysReset = SysResetYes;
  }
  else if(EncoderControl->Comand == DEC)
  {
    if(*SysReset == SysResetYes) *SysReset = SysResetNo;
  }
  else if(EncoderControl->Comand == ENTER)
  {
    if(*SysReset == SysResetYes) 
    {
      Screen_Sys_Reset_Complate();
      Device_Reset();
      Flash_Write_Param();
      *SysReset = SysResetNo;
    }
  }
  else if(EncoderControl->Comand == ESC) SysReset = SysResetNo;
}

void Speaker_Reset(SpeakerType* Speaker, uint8_t channel)
{
  uint8_t spk_ways = Speaker[channel].Config->Ways;
  uint8_t i;
  
  Default_Speaker(&Speaker[channel]);
  
  if(spk_ways > 1) UnLink_Speaker(channel, Speaker, spk_ways);
  
  Speaker[channel].Config->MemoryConfig.IsChanged = true;
  Speaker[channel].EQ->MemoryConfig.IsChanged = true;
  for(i = 0; i < spk_ways; i++) Speaker[channel].OUT[i]->MemoryConfig.IsChanged = true;

  Button_Mask(&PRESET_PARAM, Speaker, &channel);
}

//------------------------------------------------------------------------------

//MENU MATRIX-------------------------------------------------------------------
void Menu_Matrix_Gain(PresetParamType* Preset, uint8_t input_ch, ControlStateType* control, uint8_t channel)
{
  if(control->Comand == INC)
  {
    if(Preset->Matrix[input_ch][channel].Gain < MatrixGain_Max) Preset->Matrix[input_ch][channel].Gain +=100;
  }
  else if(control->Comand == DEC) 
  {
    if(Preset->Matrix[input_ch][channel].Gain > MatrixGain_Min) Preset->Matrix[input_ch][channel].Gain -=100;
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering)
    {
      VeluePrev = Preset->Matrix[input_ch][channel].Gain;
      MenuParamEntering = true;
    }
    else if(VeluePrev != Preset->Matrix[input_ch][channel].Gain)
    {
      VeluePrev = Preset->Matrix[input_ch][channel].Gain;
      Preset_Flash_Write(Preset, FILE_BUFF);
      MenuParamEntering = false;
    }
    else MenuParamEntering = false;
  }
  else if(control->Comand == ESC)
  {
    Preset->Matrix[input_ch][channel].Gain = VeluePrev;
    MenuParamEntering = false;
  }
  
  if(Preset->Matrix[input_ch][channel].Gain == MatrixGain_Min) Preset->Matrix[input_ch][channel].Mute = MUTED;
  else Preset->Matrix[input_ch][channel].Mute = UNMUTED;

  DSP_Matrix_Value(&Preset->Matrix[input_ch][channel]);
}

void Menu_Mute(MuteParamType* mute)
{
  if(mute->Value == UNMUTED) mute->Value = MUTED;
  else if(mute->Value == MUTED) mute->Value = UNMUTED; 
  //DSP Write
}
//MENU MODE XLR-----------------------------------------------------------------
void Menu_MODE_XLR(PresetParamType* Preset, uint8_t xlr_ch, ControlStateType* control)
{
  if(control->Comand == INC)
  {
    if(Preset->ModeXLR[xlr_ch].State < ModeXLRVlueType_Max) Preset->ModeXLR[xlr_ch].State++;
    XLR_Mode(&Preset->ModeXLR[xlr_ch], xlr_ch);
  }
  else if(control->Comand == DEC)
  {
    if(Preset->ModeXLR[xlr_ch].State > ModeXLRVlueType_Min) Preset->ModeXLR[xlr_ch].State--;
    XLR_Mode(&Preset->ModeXLR[xlr_ch], xlr_ch);
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering)
    {
      VeluePrev = Preset->ModeXLR[xlr_ch].State;
      MenuParamEntering = true;
    }
    else if(VeluePrev != Preset[xlr_ch].ModeXLR->State) 
    {
      VeluePrev = Preset->ModeXLR[xlr_ch].State;
      Preset->InputMode = SIMPLE;
      Preset_Flash_Write(Preset, FILE_BUFF);
      MenuParamEntering = false;
    }
    else MenuParamEntering = false;
  }
  else if(control->Comand == ESC)
  {
    MenuParamEntering = false;
    Preset->ModeXLR[xlr_ch].State = (ModeXLRVlueType)VeluePrev; 
    XLR_Mode(&Preset->ModeXLR[xlr_ch], xlr_ch);
  }
}

//PRESET MENU INPUT-------------------------------------------------------------
void Menu_Input_Source(PresetParamType* Preset, uint8_t input_ch, ControlStateType* control)
{
  if(control->Comand == INC)
  {
    if(Preset->InputSource[input_ch].ManualSource == AmpInputValueType_Def)
    {
      Preset->InputSource[input_ch].ManualSource = AnalogInputValueType_Min;
    }
    else if(Preset->InputSource[input_ch].ManualSource <= AnalogInputValueType_Max)
    {
      Preset->InputSource[input_ch].ManualSource++; 
      if(Preset->InputSource[input_ch].ManualSource > AnalogInputValueType_Max) Preset->InputSource[input_ch].ManualSource = AesInputValueType_Min;
    }
    else if((Preset->InputSource[input_ch].ManualSource >= AesInputValueType_Min) & (Preset->InputSource[input_ch].ManualSource < AesInputValueType_Max))
    {
      Preset->InputSource[input_ch].ManualSource++;
    }
  }
  else if(control->Comand == DEC)
  {
    if(Preset->InputSource[input_ch].ManualSource <= AnalogInputValueType_Max)
    {
      if(Preset->InputSource[input_ch].ManualSource == IN_NONE) Preset->InputSource[input_ch].ManualSource = IN_NONE;
      else if(Preset->InputSource[input_ch].ManualSource == AnalogInputValueType_Min) Preset->InputSource[input_ch].ManualSource = IN_NONE;
      else Preset->InputSource[input_ch].ManualSource--;
      //if(Preset->InputSource[input_ch].ManualSource == AnalogInputValueType_Min) Preset->InputSource[input_ch].ManualSource = IN_NONE;
    }
    else if((Preset->InputSource[input_ch].ManualSource >= AesInputValueType_Min) & (Preset->InputSource[input_ch].ManualSource <= AesInputValueType_Max))
    {
      if(Preset->InputSource[input_ch].ManualSource == AesInputValueType_Min) Preset->InputSource[input_ch].ManualSource = AnalogInputValueType_Max;
      else Preset->InputSource[input_ch].ManualSource--; 
    }
  }
  else if(control->Comand == ENTER)
  {
    if(!MenuParamEntering)
    {
      VeluePrev = Preset->InputSource[input_ch].ManualSource;
      MenuParamEntering = true;
    }
    else if(VeluePrev != Preset->InputSource[input_ch].ManualSource)
    {
      VeluePrev = Preset->InputSource[input_ch].ManualSource;
      Preset->InputMode = SIMPLE;
      Preset_Flash_Write(Preset, FILE_BUFF);
      MenuParamEntering = false;
    }
    else MenuParamEntering = false;
    
    //if(VeluePrev != Preset->InputSource[input_ch].ManualSource)
    //{
    //  VeluePrev = Preset->InputSource[input_ch].ManualSource;
    //  Preset->InputMode = SIMPLE;
    //  Preset->MemoryConfig.IsChanged = true;
    //}
  }
  else if(control->Comand == ESC) 
  {
    Preset->InputSource[input_ch].ManualSource = (AmpInputValueType)VeluePrev;
    MenuParamEntering = false;
  }
  
  DSP_Source_Select(&Preset->InputSource[input_ch], &Preset->InputSource[input_ch].ManualSource);
}

void Menu_Out_Config_Bridge(PresetParamType* Preset, SpeakerType* Speaker, uint8_t* channel, ControlStateType* control)
{
  //uint8_t IsLinked = ((0x1 << index) & 
  //if((Speaker[*channel].Config->Ways == 1) & (Speaker[*channel +1].Config->Ways == 1))
  
  //if another channel was used in another menu param (volume, delay...)
  if(*channel == 1) *channel = 0;
  else if(*channel == 3) *channel = 2;
    
  if(control->Comand == INC)
  {
    //RESET OUT
    if((Speaker[*channel].Config->Ways == 1) & (Speaker[*channel+1].Config->Ways == 1))
    {
      if(Preset->Bridge[*channel/2].State < IsBridgeType_Max) Preset->Bridge[*channel/2].State++;
      //if(Preset->Bridge[*channel/2].State == UNBRIDGE) Preset->Bridge[*channel/2].State = BRIDGE;
      
      //Default_Speaker(&Speaker[*channel]);
      //Default_Speaker(&Speaker[*channel +1]);
      Button_Mask(Preset, Speaker, channel);
    }
    //else MenuTmp = SelectMenu; MenuWarning(BridgeWarning);
    
      //Default_Speaker(Speaker[index]);
      //Button_Mask(Preset, SpeakerType* Speaker, uint8_t* channel);
  }
  else if(control->Comand == DEC) 
  {
    //RESET OUT
    if((Speaker[*channel].Config->Ways == 1) & (Speaker[*channel+1].Config->Ways == 1))
    {
      if(Preset->Bridge[*channel/2].State > IsBridgeType_Min) Preset->Bridge[*channel/2].State--;
      //if(Preset->Bridge[*channel/2].State == BRIDGE) Preset->Bridge[*channel/2].State = UNBRIDGE;
      
      //Default_Speaker(&Speaker[*channel]);
      //Default_Speaker(&Speaker[*channel +1]);
      
      Button_Mask(Preset, Speaker, channel);
    }
  }
  else if(control->Comand == ENTER)
  {
    if(VeluePrev != Preset->Bridge[*channel/2].State)
    {
      VeluePrev = Preset->Bridge[*channel/2].State;
      Preset->MemoryConfig.IsChanged = true;
    }
  }
  else if(control->Comand == ESC)
  {
    Preset->Bridge[*channel/2].State = (BridgeValueType)VeluePrev;
  }
}

void Menu_Out_Config_Bridge_Param(PresetParamType* Preset, SpeakerType* Speaker, uint8_t channel, ControlStateType* control)
{
  if(!(channel %2))
  {
    if((Speaker[channel].Config->Ways != 1) | (Speaker[channel+1].Config->Ways > 1))
    {
      Menu_Warning();
    }
    else if(control->Comand == INC)
    {
      if(Preset->Bridge[channel/2].State < IsBridgeType_Max) Preset->Bridge[channel/2].State++;
      //Menu_Bridge(Preset, channel, Speaker, SPEAKER_OUT);
      //Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh);
    }
    else if(control->Comand == DEC) 
    {
      if(Preset->Bridge[channel/2].State > IsBridgeType_Min) Preset->Bridge[channel/2].State--;
      //Menu_Bridge(Preset, channel, Speaker, SPEAKER_OUT);
      //Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh);
    }
    else if(control->Comand == ENTER)
    {
      if(!MenuParamEntering)
      {
        VeluePrev = Preset->Bridge[channel/2].State;
        MenuParamEntering = true;
      }
      else if(VeluePrev != Preset->Bridge[channel/2].State)
      {
        Menu_Bridge(Preset, channel, Speaker, SPEAKER_OUT);
        DSP_Bridge(&Preset->Bridge[channel/2]);
        Link_OUT_Matrix(SPEAKER, LINK_MATRIX);
        Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh);
        VeluePrev = Preset->Bridge[channel/2].State;
        Preset_Flash_Write(Preset, FILE_BUFF);
        //Flash_Write_Param();
        MenuParamEntering = false;
      }
    }
    else if(control->Comand == ESC)
    {
      Preset->Bridge[channel/2].State = (BridgeValueType)VeluePrev; 
      MenuParamEntering = false;
      //Menu_Bridge(Preset, channel, Speaker, SPEAKER_OUT);
      //Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh);
    }
  }
  
  //Link_OUT_Matrix(SPEAKER, LINK_MATRIX);
}

void Menu_Bridge(PresetParamType* Preset, uint8_t channel, SpeakerType* Speaker, SpeakerOutType* SpeakerOut)
{
  uint8_t out;
  uint8_t offset = 0;
  uint8_t speaker;
  uint8_t i = 0;
  
  //Default_Speaker(&Speaker[channel]);                                                                
  //Default_Speaker(&Speaker[channel +1]);
  Speaker_Reset(Speaker, channel);
  UserEQ_Reset(&USER_EQ[channel]);
  Speaker_Reset(Speaker, channel+1);
  UserEQ_Reset(&USER_EQ[channel+1]);
  
  if(Preset->Bridge[channel /2].State == BRIDGE) Speaker[channel +1].Config->Ways = 0;
  else if(Preset->Bridge[channel /2].State == UNBRIDGE) Speaker[channel +1].Config->Ways = 1;
  
  //Speaker[channel].Config->MemoryConfig.IsChanged = true;
  //Speaker[channel +1].Config->MemoryConfig.IsChanged = true;
  //Speaker[channel].EQ->MemoryConfig.IsChanged = true;
  //Speaker[channel +1].EQ->MemoryConfig.IsChanged = true;
  //Speaker[channel].OUT[0]->MemoryConfig.IsChanged = true;
  //Speaker[channel +1].OUT[0]->MemoryConfig.IsChanged = true;
 
  for(speaker = 0; speaker < OUT_CH_QTY; speaker++)
  {
    for(out = 0; out < OUT_CH_QTY; out++) 
    {
      if((speaker +out +offset) < OUT_CH_QTY) Speaker[speaker].OUT[out] = &SpeakerOut[speaker +out +offset];
      else if(out < OUT_CH_QTY) Speaker[speaker].OUT[out] = 0x000000;
      
      if(!((speaker +out +offset) % 2))
      {
        if(Preset->Bridge[(speaker +out +offset) /2].State == BRIDGE) offset++;
      } 
    }
    
    for(i = 0; i < OUT_CH_QTY; i++) if(Speaker[speaker].OUT[i] == 0x000000) Speaker[speaker].Config->FreeWays = i +1;
    
    offset = 0;
  }
  
  Flash_Write_Param();
  
  //UserEQ_Flash_Write(&USER_EQ[channel], FILE_BUFF);
  //UserEQ_Flash_Write(&USER_EQ[channel], FILE_BUFF);
}

void Bridge_Init(PresetParamType* Preset, uint8_t channel, SpeakerType* Speaker, SpeakerOutType* SpeakerOut)
{
  uint8_t out;
  uint8_t offset = 0;
  uint8_t speaker;
  uint8_t i = 0;

  if(Preset->Bridge[channel /2].State == BRIDGE) Speaker[channel +1].Config->Ways = 0;
  else if(Preset->Bridge[channel /2].State == UNBRIDGE) Speaker[channel +1].Config->Ways = 1;

  for(speaker = 0; speaker < OUT_CH_QTY; speaker++)
  {
    for(out = 0; out < OUT_CH_QTY; out++) 
    {
      if((speaker +out +offset) < OUT_CH_QTY) Speaker[speaker].OUT[out] = &SpeakerOut[speaker +out +offset];
      else if(out < OUT_CH_QTY) Speaker[speaker].OUT[out] = 0x000000;
      
      if(!((speaker +out +offset) % 2))
      {
        if(Preset->Bridge[(speaker +out +offset) /2].State == BRIDGE) offset++;
      } 
    }
    
    for(i = 0; i < OUT_CH_QTY; i++) if(Speaker[speaker].OUT[i] == 0x000000) Speaker[speaker].Config->FreeWays = i +1;
    
    offset = 0;
  }
}

void Bridge(uint8_t channel)
{
  if(DeviceStatus.DeviceMode == INIT_MODE) Bridge_Init(&PRESET_PARAM, channel+1, SPEAKER, SPEAKER_OUT);
  else 
  {
    Menu_Bridge(&PRESET_PARAM, channel+1, SPEAKER, SPEAKER_OUT);
    Preset_Flash_Write(&PRESET_PARAM, FILE_BUFF);
  }
  
  Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh); 
  DSP_Bridge(&PRESET_PARAM.Bridge[channel]);
  Link_OUT_Matrix(SPEAKER, LINK_MATRIX);
}

//MENU LOAD PRESET--------------------------------------------------------------
void Menu_Load_Preset(PresetType* Preset, PresetLibType* PresetLib, ControlStateType* EncoderControl)
{ 
  if(EncoderControl->Comand == ENTER)
  {
    //read new speaker from flash addr = spk_lib_number 
    
    //New_Speaker_Read_Flash(NEW_SPEAKER_ADDR, &NEW_SPEAKER);
    //Load_Speaker_Data(&channel, &NEW_SPEAKER, CURRENT_SPEAKER);
    //Link_OUT_Matrix(CURRENT_SPEAKER, LINK_MATRIX);
    
    //Load_Speaker_From_Lib(SpeakerLib->Index, channel, SpeakerLib->Speaker[SpeakerLib->Index].Way, Speaker, FILE_BUFF);
    //Button_Mask(&PRESET, SPEAKER, &SelectedCh);
    
  }
  else if(EncoderControl->Comand == INC)
  {
    if(PresetLib->Index < (LIB_PRESET_QTY -1)) PresetLib->Index++;
    //Load_Speaker_Button_Mask(SpeakerLib->Speaker[SpeakerLib->Index].Way, BridgeButtonMask);
  }
  else if(EncoderControl->Comand == DEC)
  {
    //channel = 0;
    if(PresetLib->Index > 0) PresetLib->Index--;
    //Load_Speaker_Button_Mask(SpeakerLib->Speaker[SpeakerLib->Index].Way, BridgeButtonMask);
  }
  else if(EncoderControl->Comand == ESC)
  {
  }
}

//void Menu_Bridge_Test(PresetParamType* Preset, uint8_t channel, SpeakerType* Speaker)
//{
//  uint8_t out;
//  uint8_t offset = 0;
//  //uint8_t speaker;
//  
//  //Default_Speaker(&Speaker[channel]);               //Defualt with mute                                                 
//  //Default_Speaker(&Speaker[channel +1]);            //Defualt with mute 
//  
//  if(Preset->Bridge[channel /2].State == BRIDGE)
//  {
//    Speaker[channel +1].Config->Ways = 0;
//    offset = 1;
//  }
//  else if(Preset->Bridge[channel /2].State == UNBRIDGE);
//
//  for(out = 1; out < OUT_CH_QTY; out++)
//  {
//    if((channel +out +offset) >= OUT_CH_QTY)
//    {
//      for(out = out; out < OUT_CH_QTY; out++) Speaker[channel].OUT[out] = 0x000000;
//      break;
//    }
//    else Speaker[channel].OUT[out] = Speaker[channel+1].OUT[out +offset];// &SpeakerOut[channel +out +offset];
//  }
//}

void Get_Device_Mode()
{
  //Check if is USB conect
  if(USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED) DeviceStatus.DeviceMode = USB_MODE;
  else if(selectedMenuItem->Param->ParamType == MAIN_MENU) DeviceStatus.DeviceMode = MAIN_MENU_MODE;
  else if(selectedMenuItem->Param->ParamType != MAIN_MENU) DeviceStatus.DeviceMode = MENU_MODE;
  //else if(selectedMenuItem->MenuType == MENU) DeviceInfo.DeviceMode = MENU_MODE;
}

void Button_Mask(PresetParamType* Preset, SpeakerType* Speaker, uint8_t* channel)
{
  uint8_t i;
  
  *channel = 0; //Reset Select Channel to first
  BridgeButtonMask = 0xF;
  LinkButtonMask = 0x00;
  
  for(i = 0; i < OUT_CH_QTY /2; i++)
  {
    if(Preset->Bridge[i].State == BRIDGE)
    {
      BridgeButtonMask &= ~(0x01 << (((OUT_CH_QTY /2) * i) +1));
    }
  }  

  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if((BridgeButtonMask >> i) & 0x01)
    {
      if(Speaker[i].Config->Ways >= 1) LinkButtonMask |= 1 << i;
      else LinkButtonMask = LinkButtonMask;
    }
    else 
    {
      //Speaker[i].Config->Ways = 0;
      if(Speaker[i-1].Config->Ways > 1) i++;
    }
  }
  
  Load_Speaker_Button_Mask(SPEAKER_LIB.Speaker[SPEAKER_LIB.Index].Way, BridgeButtonMask);
}

void Load_Speaker_Button_Mask(uint8_t ways, uint8_t bridge_mask)
{
  LoadSpkFreeWays = 0x00;
  uint8_t i;
  uint8_t tmp = 0;
  //uint8_t AmpWays = 0;
  AmpFreeWays = 0;
  
  for(i = 0; i < OUT_CH_QTY; i++)      //count free ways with bridge ch
  {
    if((bridge_mask >> i) & 0x01) AmpFreeWays++;
  }
  
  if((ways > 0) & (ways <= AmpFreeWays))
  {
    for(i = 0; i < (OUT_CH_QTY - ways +1); i++)
    {
      LoadSpkFreeWays |= (1 << i); 
    }
    
    tmp = BridgeButtonMask >> (OUT_CH_QTY -1);
    if(!(tmp)) LoadSpkFreeWays &= ~(1 << (OUT_CH_QTY - ways));
    
    LoadSpkFreeWays = (LoadSpkFreeWays & BridgeButtonMask);
  }
  else LoadSpkFreeWays = 0; 
}

uint8_t Select_Channel(ControlStateType* button, uint8_t Mask)
{
  uint8_t data = ((~button->Counter) & Mask);
  //uint8_t Channel = 0;
  
  if(data != 0)
  {
    switch(data)
    {
      case 1: SelectedCh = 0; break;
      case 2: SelectedCh = 1; break;
      case 4: SelectedCh = 2; break;
      case 8: SelectedCh = 3; break;
    }
  }
  
  return SelectedCh;
}

//uint8_t Load_Speaker_Select_Channel(ControlStateType* button, uint8_t ways)
//{
//  uint8_t data = ((~button->Counter) & Mask);
//  //uint8_t Channel = 0;
//  
//  if(data != 0)
//  {
//    switch(data)
//    {
//      case 1: SelectedCh = 0; break;
//      case 2: SelectedCh = 1; break;
//      case 4: SelectedCh = 2; break;
//      case 8: SelectedCh = 3; break;
//    }
//  }
//  
//  return SelectedCh;
//}

uint8_t Mute_Channel(ControlStateType* button, PresetParamType* Preset)
{
  uint8_t Channel = 0;
  uint8_t data = ((~button->Counter) & 0x0F);
  
  if(data != 0)
  {
    switch(data)
    {
      case 1: Channel = 0; break;
      case 2: Channel = 1; break;
      case 4: Channel = 2; break;
      case 8: Channel = 3; break;
    }
  }
  
  if(((Channel == 0)  | (Channel == 1)) & (Preset->Bridge[0].State == BRIDGE)) Channel = 0;
  else if(((Channel == 2)  | (Channel == 3)) & (Preset->Bridge[1].State == BRIDGE)) Channel = 2;

  return Channel;
}

//LED Control-------------------------------------------------------------------

void LED_Mute_Mode(LEDsIndicatorsType* LED, PresetParamType* Preset)
{
  uint8_t i = 0;
  
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if(SPEAKER_OUT[i].Mute.Value == MUTED) LED->LED_MUTE_SELECT[i] = LED_ON;
    else LED->LED_MUTE_SELECT[i] = LED_OFF;
    //LED->LED_MUTE_SELECT[i] &= ~((LEDType)SPEAKER_OUT[i].Mute.Value);
    //if(Preset->Bridge[i].State == BRIDGE) LED->LED_MUTE_SELECT[i+1] = LED->LED_MUTE_SELECT[i];
    
  }

  for(i = 0; i < OUT_CH_QTY /2; i++)
  {
    if(Preset->Bridge[i].State == BRIDGE) LED->LED_MUTE_SELECT[((OUT_CH_QTY /2)*i)+1] = LED->LED_MUTE_SELECT[(OUT_CH_QTY /2)*i];   
  }
}

void LED_Select_Mode(LEDsIndicatorsType* LED, uint8_t channel)
{
  for(uint8_t i = 0; i < OUT_CH_QTY; i++) LED->LED_MUTE_SELECT[i] = LED_OFF;

  if(selectedMenuItem->MenuType == PARAM)
  {
    switch(selectedMenuItem->Param->ParamType)
    {
      case MATRIX_IN1:
      case MATRIX_IN2:
      case MATRIX_IN3:
      case MATRIX_IN4:
      case OUT_VOLUME:
      case OUT_DELAY:
      case MENU_LOAD_SPEAKER: LED->LED_MUTE_SELECT[channel] = LED_ON; break;
    } 
  }
}

void LED_Select_Mode_Test(LEDsIndicatorsType* LED, uint8_t Mask)
{
  uint8_t i = 0;

  for(uint8_t i = 0; i < OUT_CH_QTY; i++) LED->LED_MUTE_SELECT[i] = LED_OFF;
  //for(i = 0; i < OUT_CH_QTY; i++) LED->LED_MUTE_SELECT[i] = (LEDType)((Mask >> i) & 0x01);
   
  if(selectedMenuItem->MenuType == PARAM)
  {
    switch(selectedMenuItem->Param->ParamType)
    {
      case MATRIX_IN1:
      case MATRIX_IN2:
      case MATRIX_IN3:
      case MATRIX_IN4:
      case OUT_VOLUME:
      case OUT_DELAY:
      case MENU_OUT_INFO:
      case MENU_SPEAKER_RST:
      case MENU_LOAD_SPEAKER: for(i = 0; i < OUT_CH_QTY; i++) LED->LED_MUTE_SELECT[i] = (LEDType)((Mask >> i) & 0x01); break;
      //case MENU_OUT_CONFIG_BRIDGE: for(i = 0; i < OUT_CH_QTY; i++) LED->LED_MUTE_SELECT[i] = (LEDType)((0x05 >> i) & 0x01); break;
    } 
  }
}

void Led_Signal(LEDsIndicatorsType* LED, SpeakerOutType* out)
{
  uint8_t i;
  
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if(out[i].OutLevel >= THRESHOLD_LEVEL_METER) LED->LED_SIGNAL[i] = LED_ON;
    else LED->LED_SIGNAL[i] = LED_OFF;
  }
}

void Led_Limiters(LEDsIndicatorsType* LED, SpeakerOutType* out)
{
  uint8_t i;
  
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if((out[i].RMSLimiter.Reduction < THRESHOLD_LIMITER_METER) | (out[i].PeakLimiter.Reduction < THRESHOLD_LIMITER_METER) | (out[i].ClipLimiter.Reduction < THRESHOLD_LIMITER_METER)) LED->LED_CLIP[i] = LED_ON;
    else LED->LED_CLIP[i] = LED_OFF;
  }
}

//------------------------------------------------------------------------------
//Timer2 for exit to main screen
void Timer_2_Init()
{
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;		                // enable clock for TIM2
  //RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;		                // enable clock for port B 
  
  //CS_Flash->PB12
  //CLR(PIN_TEST_OUT, PIN_TEST);                                  //Set OUT_DATA = 1
  //GPIOC->MODER |= GPIO_MODER_MODE0_0;		                //Set MODE (mode register) -> Output
  //GPIOC->OTYPER &= ~GPIO_OTYPER_OT_0;		                //Set OTYPER -> Open drain
  //GPIOC->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0_1;		        //Set OSPEEDR (output speed register) ->
  //GPIOC->PUPDR = GPIO_PUPDR_PUPDR0_0;		                //Set PUPDR -> Pull Up  
  //SET(PIN_TEST_OUT, PIN_TEST);                                  //Set OUT_DATA = 1
  
  //APB1 CLK = 24MHz *2 
  TIM2->PSC = 48000 -1;                 //48MHz/48000= 1KHz
  TIM2->ARR = 15000 -1;                 //30s = 30000 *(1/1KHz)
  TIM2->CNT = 0;                        //Counter register = 0
  TIM2->CR1 &= ~TIM_CR1_DIR;            //Direction -> Upcounter
  TIM2->CR1 |= TIM_CR1_URS;             //Set irq event filter
  TIM2->DIER |= TIM_DIER_UIE;           //Update interrupt enable
  TIM2->CR1 &= ~TIM_CR1_UDIS;           //UEV enabled
  TIM2->EGR |= TIM_EGR_UG;            //Generate event
  TIM2->SR &= ~TIM_SR_UIF;              //Reset Update interrupt flag
  NVIC_SetPriority(TIM2_IRQn, 4);
  NVIC_EnableIRQ(TIM2_IRQn);
}

void Menu_Reset_Timer_Start()
{
  TIM2->CNT = 0;
  TIM2->CR1 = TIM_CR1_CEN;
}

void Menu_Reset_Timer_Stop()
{
  TIM2->CR1 &= ~TIM_CR1_CEN;
  TIM2->CNT = 0;
}

void TIM2_IRQHandler(void)
{
  TIM2->SR &= ~TIM_SR_UIF;
  Menu_Reset_Timer_Stop();
  
  Menu_Init();
  Screen_Menu(&EncoderControl);
}

uint8_t* Clear_Buff(uint8_t* Buff, uint16_t size)
{
  uint16_t i;
  
  for(i = 0; i < size; i++)
  {
    Buff[i] = 0xFF;
  }
  
  return Buff;
}
  
//Memory Config-----------------------------------------------------------------

void Memory_Init()
{
  //Eeprom_Erase();
  //Flash_Full_Chip_Erase();
  
  if((DeviceStatus.DeviceEeprom == EEPROM_OK) & (DeviceStatus.DeviceFlash == FLASH_OK))
  {
    Preset_Flash_Read(&PRESET_PARAM, FILE_BUFF);
    
    UserEQ_Flash_Read(&USER_EQ[0], FILE_BUFF);
    UserEQ_Flash_Read(&USER_EQ[1], FILE_BUFF);
    UserEQ_Flash_Read(&USER_EQ[2], FILE_BUFF);
    UserEQ_Flash_Read(&USER_EQ[3], FILE_BUFF);  
    
    Speaker_Config_Flash_Read(SPEAKER[0].Config, FILE_BUFF);
    Speaker_Config_Flash_Read(SPEAKER[1].Config, FILE_BUFF);
    Speaker_Config_Flash_Read(SPEAKER[2].Config, FILE_BUFF);
    Speaker_Config_Flash_Read(SPEAKER[3].Config, FILE_BUFF);
    SpeakerEQ_Flash_Read(SPEAKER[0].EQ, FILE_BUFF);
    SpeakerEQ_Flash_Read(SPEAKER[1].EQ, FILE_BUFF);
    SpeakerEQ_Flash_Read(SPEAKER[2].EQ, FILE_BUFF);
    SpeakerEQ_Flash_Read(SPEAKER[3].EQ, FILE_BUFF);
    Speaker_OUT_Flash_Read(&SPEAKER_OUT[0], FILE_BUFF);
    Speaker_OUT_Flash_Read(&SPEAKER_OUT[1], FILE_BUFF);
    Speaker_OUT_Flash_Read(&SPEAKER_OUT[2], FILE_BUFF);
    Speaker_OUT_Flash_Read(&SPEAKER_OUT[3], FILE_BUFF);
    
  }
  else if((DeviceStatus.DeviceEeprom == EEPROM_OK) & (DeviceStatus.DeviceFlash != FLASH_OK))
  {
    Flash_Full_Chip_Erase();            //Full flash chip erase
    Flash_Write_Header();               //Init flash -> write header to flash
    Device_Reset();
  }
  else if((DeviceStatus.DeviceEeprom != EEPROM_OK) & (DeviceStatus.DeviceFlash == FLASH_OK))
  {
    Eeprom_Erase();                     //Full eeprom chip erase
    Eeprom_Write_Header();              //Init eeprom -> write header to eeprom
    
    Preset_Flash_Read_Backup(&PRESET_PARAM, FILE_BUFF);
    
    UserEQ_Flash_Read_Backup(&USER_EQ[0], FILE_BUFF);
    UserEQ_Flash_Read_Backup(&USER_EQ[1], FILE_BUFF);
    UserEQ_Flash_Read_Backup(&USER_EQ[2], FILE_BUFF);
    UserEQ_Flash_Read_Backup(&USER_EQ[3], FILE_BUFF); 
    
    Speaker_Config_Flash_Read_Backup(SPEAKER[0].Config, FILE_BUFF);
    Speaker_Config_Flash_Read_Backup(SPEAKER[1].Config, FILE_BUFF);
    Speaker_Config_Flash_Read_Backup(SPEAKER[2].Config, FILE_BUFF);
    Speaker_Config_Flash_Read_Backup(SPEAKER[3].Config, FILE_BUFF);
    
    SpeakerEQ_Flash_Read_Backup(SPEAKER[0].EQ, FILE_BUFF);
    SpeakerEQ_Flash_Read_Backup(SPEAKER[1].EQ, FILE_BUFF);
    SpeakerEQ_Flash_Read_Backup(SPEAKER[2].EQ, FILE_BUFF);
    SpeakerEQ_Flash_Read_Backup(SPEAKER[3].EQ, FILE_BUFF);
    
    Speaker_OUT_Flash_Read_Backup(&SPEAKER_OUT[0], FILE_BUFF);
    Speaker_OUT_Flash_Read_Backup(&SPEAKER_OUT[1], FILE_BUFF);
    Speaker_OUT_Flash_Read_Backup(&SPEAKER_OUT[2], FILE_BUFF);
    Speaker_OUT_Flash_Read_Backup(&SPEAKER_OUT[3], FILE_BUFF);
  }
  else if((DeviceStatus.DeviceEeprom != EEPROM_OK) & (DeviceStatus.DeviceFlash != FLASH_OK))
  {
    Flash_Full_Chip_Erase();            //Full flash chip erase
    Flash_Write_Header();               //Init flash -> write header to flash 
    Eeprom_Erase();                     //Full eeprom chip erase
    Eeprom_Write_Header();              //Init eeprom -> write header to eeprom
    
    //All params are default
    Device_Reset();
  }
}

void Flash_Write_Param()
{
  uint8_t i;
  //asm("nop");
  
  //PRESET FlashWrite
  if(PRESET_PARAM.MemoryConfig.IsChanged)
  {
    Preset_Flash_Write(&PRESET_PARAM, FILE_BUFF);
    PRESET_PARAM.MemoryConfig.IsChanged = false;
  }
  
  //SpeakerConfig Flash Write
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if(SPEAKER[i].Config->MemoryConfig.IsChanged)
    {
      Speaker_Config_Flash_Write(SPEAKER[i].Config, FILE_BUFF);
      SPEAKER[i].Config->MemoryConfig.IsChanged = false;
    }
  }
  
  //SpeakerEQ Flash Write
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if(SPEAKER[i].EQ->MemoryConfig.IsChanged)
    {
      SpeakerEQ_Flash_Write(SPEAKER[i].EQ, FILE_BUFF);
      SPEAKER[i].EQ->MemoryConfig.IsChanged = false;
    }
  }
  
  //SpeakerOUT Flash Write
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if(SPEAKER_OUT[i].MemoryConfig.IsChanged) 
    {
      Speaker_OUT_Flash_Write(&SPEAKER_OUT[i], FILE_BUFF);
      SPEAKER_OUT[i].MemoryConfig.IsChanged = false;
    }
  }
  
  //UserEQ Flash Write
  for(i = 0; i < OUT_CH_QTY; i++)
  {
    if(USER_EQ[i].MemoryConfig.IsChanged)
    {
      UserEQ_Flash_Write(&USER_EQ[i], FILE_BUFF);
      USER_EQ[i].MemoryConfig.IsChanged = false;
    }
  }
}

void Device_Reset()
{
  //Mute Amp
  AmpHardwareMute = MUTED;
  AMP_Hardware_Mute(AmpHardwareMute);
  //LED_Clear();

  //Preset_Param_Reset(&PRESET_PARAM, FILE_BUFF);
  
  Preset_Default_Param(&PRESET_PARAM);
  FileFormatWritePresetParam(&PRESET_PARAM, FILE_BUFF); 
  FileFormatReadPresetParam(&PRESET_PARAM, FILE_BUFF);
  PRESET_PARAM.MemoryConfig.IsChanged = true;
  
  //for(i = 0; i < IN_CH_QTY; i++) UserEQ_Reset(&USER_EQ[i], FILE_BUFF);
  //for(i = 0; i < OUT_CH_QTY; i++) Default_Speaker(&SPEAKER[i]);
  
  UserEQ_Default_Param(&USER_EQ[0]);
  FileFormatWriteUserEQ(&USER_EQ[0], FILE_BUFF);
  FileFormatReadUserEQ(&USER_EQ[0], FILE_BUFF);
  USER_EQ[0].MemoryConfig.IsChanged = true;
  
  UserEQ_Default_Param(&USER_EQ[1]);
  FileFormatWriteUserEQ(&USER_EQ[1], FILE_BUFF);
  FileFormatReadUserEQ(&USER_EQ[1], FILE_BUFF);
  USER_EQ[1].MemoryConfig.IsChanged = true;
  
  UserEQ_Default_Param(&USER_EQ[2]);
  FileFormatWriteUserEQ(&USER_EQ[2], FILE_BUFF);
  FileFormatReadUserEQ(&USER_EQ[2], FILE_BUFF);
  USER_EQ[2].MemoryConfig.IsChanged = true;
  
  UserEQ_Default_Param(&USER_EQ[3]);
  FileFormatWriteUserEQ(&USER_EQ[3], FILE_BUFF);
  FileFormatReadUserEQ(&USER_EQ[3], FILE_BUFF);
  USER_EQ[3].MemoryConfig.IsChanged = true;
  
  Speaker_Reset(SPEAKER, 0);
  Speaker_Reset(SPEAKER, 1);
  Speaker_Reset(SPEAKER, 2);
  Speaker_Reset(SPEAKER, 3);
  
  //Default_Speaker(&SPEAKER[0]);
  //FileFormatWriteSpeaker(&SPEAKER[0], FILE_BUFF);  
  //FileFormatReadSpeaker(&SPEAKER[0], FILE_BUFF);
  //SPEAKER[0].Config->MemoryConfig.IsChanged = true;
  //SPEAKER[0].EQ->MemoryConfig.IsChanged = true;
  //SPEAKER[0].OUT[0]->MemoryConfig.IsChanged = true;
  
  //Default_Speaker(&SPEAKER[1]);
  //FileFormatWriteSpeaker(&SPEAKER[1], FILE_BUFF);  
  //FileFormatReadSpeaker(&SPEAKER[1], FILE_BUFF);
  //SPEAKER[1].Config->MemoryConfig.IsChanged = true;
  //SPEAKER[1].EQ->MemoryConfig.IsChanged = true;
  //SPEAKER[1].OUT[0]->MemoryConfig.IsChanged = true;
  
  //Default_Speaker(&SPEAKER[2]);
  //FileFormatWriteSpeaker(&SPEAKER[2], FILE_BUFF);  
  //FileFormatReadSpeaker(&SPEAKER[2], FILE_BUFF);
  //SPEAKER[2].Config->MemoryConfig.IsChanged = true;
  //SPEAKER[2].EQ->MemoryConfig.IsChanged = true;
  //SPEAKER[2].OUT[0]->MemoryConfig.IsChanged = true;
  
  //Default_Speaker(&SPEAKER[3]);
  //FileFormatWriteSpeaker(&SPEAKER[3], FILE_BUFF);  
  //FileFormatReadSpeaker(&SPEAKER[3], FILE_BUFF);
  //SPEAKER[3].Config->MemoryConfig.IsChanged = true;
  //SPEAKER[3].EQ->MemoryConfig.IsChanged = true;
  //SPEAKER[3].OUT[0]->MemoryConfig.IsChanged = true;
  
  Button_Mask(&PRESET_PARAM, SPEAKER, &SelectedCh); 
  Link_OUT_Matrix(SPEAKER, LINK_MATRIX);
}

void Preset_Param_Reset(PresetParamType* Preset, uint8_t* Buff)
{
  Preset_Default_Param(Preset);
  Preset_Flash_Write(Preset, Buff);
  FileFormatReadPresetParam(Preset, Buff);
}

void UserEQ_Reset(UserEQType* UserEQ)
{
  UserEQ_Default_Param(UserEQ);
  //UserEQ_Flash_Write(UserEQType* UserEQ);
  DSP_UserEQ_Write(UserEQ);
  UserEQ->MemoryConfig.IsChanged = true;
}

//XLR Mode AES-Analog
void XLR_Mode(ModeXLRParamType *ModeXLR, uint8_t xlr_ch)
{
  if(ModeXLR->State == XLR_ANALOG) 
  {
    if(xlr_ch == 0) CLR(REL1_PIN_OUT, REL1_PIN);
    else if(xlr_ch == 1) CLR(REL2_PIN_OUT, REL2_PIN);
  }
  else if(ModeXLR->State == XLR_AES) 
  {
    if(xlr_ch == 0) SET(REL1_PIN_OUT, REL1_PIN);
    else if(xlr_ch == 1) SET(REL2_PIN_OUT, REL2_PIN);
  }
}

void AMP_Hardware_Mute(MuteValueType AmpHardMute)
{
  if(AmpHardMute == MUTED) SET(HARD_MUTE_OUT, HARD_MUTE);
  else if(AmpHardMute == UNMUTED) CLR(HARD_MUTE_OUT, HARD_MUTE);
}

//Backup MODE


//Error check

void Hardware_Error()
{
  //if()
}