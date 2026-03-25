
#include "park_dsp_lite_4x4.h"
#include "flash.h"
#include "i2c_eeprom.h"
#include "ParkDSPLite_4x4_ADAU1466_PARAM.h"

//DEVICE INFO-------------------------------------------------------------------
DeviceInfoType DEVICE_INFO = {10, "PARK AUDIO", "DF", "DF1000", "DF1000", {0, 0}, {0, 0}, {0}};
//------------------------------------------------------------------------------

//PRESET INIT-------------------------------------------------------------------
PresetParamType         PRESET_PARAM = {FLASH_BASE_ADDR_PRESET, EEPROM_ADDR_PRESET, 0, 1, false};
PresetType              PRESET;
//------------------------------------------------------------------------------

//USER EQ INIT------------------------------------------------------------------
UserEQType              USER_EQ[OUT_CH_QTY] = {{FLASH_BASE_ADDR_USER_EQ_A, EEPROM_ADDR_USER_EQ_A, 0, 1, false}, 
                                      {FLASH_BASE_ADDR_USER_EQ_B, EEPROM_ADDR_USER_EQ_B, 0, 1, false}, 
                                      {FLASH_BASE_ADDR_USER_EQ_C, EEPROM_ADDR_USER_EQ_C, 0, 1, false}, 
                                      {FLASH_BASE_ADDR_USER_EQ_D, EEPROM_ADDR_USER_EQ_D, 0, 1, false}};

//SPEAKER INIT--------------------------------------------------------------------
#pragma location = ".sram" 
FilterFirParamType      SPEAKER_EQ_FIR[OUT_CH_QTY];              //Speaker EQ Fir define
#pragma location = ".sram"
FilterFirParamType      SPEAKER_OUT_FIR[OUT_CH_QTY];             //Speaker Out Fir define         
SpeakerEQType           SPEAKER_EQ[OUT_CH_QTY] = {{FLASH_BASE_ADDR_SPK_EQ_CH_A, EEPROM_ADDR_SPK_EQ_CH_A, 0, 1, false}, 
                                         {FLASH_BASE_ADDR_SPK_EQ_CH_B, EEPROM_ADDR_SPK_EQ_CH_B, 0, 1, false}, 
                                         {FLASH_BASE_ADDR_SPK_EQ_CH_C, EEPROM_ADDR_SPK_EQ_CH_C, 0, 1, false}, 
                                         {FLASH_BASE_ADDR_SPK_EQ_CH_D, EEPROM_ADDR_SPK_EQ_CH_D, 0, 1, false}};  

SpeakerConfigType       SPEAKER_CONFIG[OUT_CH_QTY] = {{FLASH_BASE_ADDR_SPK_CONFIG_MEQ_CH_A, EEPROM_ADDR_SPK_CONFIG_MEQ_CH_A, 0, 1, false}, 
                                             {FLASH_BASE_ADDR_SPK_CONFIG_MEQ_CH_B, EEPROM_ADDR_SPK_CONFIG_MEQ_CH_B, 0, 1, false}, 
                                             {FLASH_BASE_ADDR_SPK_CONFIG_MEQ_CH_C, EEPROM_ADDR_SPK_CONFIG_MEQ_CH_C, 0, 1, false},
                                             {FLASH_BASE_ADDR_SPK_CONFIG_MEQ_CH_D, EEPROM_ADDR_SPK_CONFIG_MEQ_CH_D, 0, 1, false}};              

SpeakerOutType          SPEAKER_OUT[OUT_CH_QTY] = {{FLASH_BASE_ADDR_SPK_OUT_CH_A, EEPROM_ADDR_SPK_OUT_CH_A, 0, 1, false}, 
                                          {FLASH_BASE_ADDR_SPK_OUT_CH_B, EEPROM_ADDR_SPK_OUT_CH_B, 0, 1, false}, 
                                          {FLASH_BASE_ADDR_SPK_OUT_CH_C, EEPROM_ADDR_SPK_OUT_CH_C, 0, 1, false}, 
                                          {FLASH_BASE_ADDR_SPK_OUT_CH_D, EEPROM_ADDR_SPK_OUT_CH_D, 0, 1, false}};                //Speaker Out define
SpeakerOutConfigType    SPEAKER_OUT_CONFIG[OUT_CH_QTY];          //Speaker Out Config define
SpeakerType             SPEAKER[OUT_CH_QTY];                     //Speaker define
SpeakerLibType          SPEAKER_LIB = {FLASH_BASE_ADDR_SPK_LIB};
PresetLibType           PRESET_LIB = {FLASH_BASE_ADDR_PRESET_LIB};
LinkMatrixParamType     LINK_MATRIX[OUT_CH_QTY] = {{{MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT0_ADDR, MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT0_MEMORYPAGE}, 0,}, 
                                                  {{MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT1_ADDR, MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT1_MEMORYPAGE}, 1},
                                                  {{MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT2_ADDR, MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT2_MEMORYPAGE}, 2},
                                                  {{MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT3_ADDR, MOD_LINKOUT_ALG0_SIMPLEROUTERNOSLEW32S300ALG3INPUTSELECT3_MEMORYPAGE}, 3}};

SpeakerResetType        SpeakerReset = SpeakerResetNo;
SysResetType            SystemReset = SysResetNo;


SpeakerEQType* GetSpeakerEQAddr(uint8_t index)
{
  //uint32_t addr = (uint32_t)&SPEAKER_EQ; 
  return &SPEAKER_EQ[index];
}

void UserEQ(UserEQType* user_eq)
{
  
}