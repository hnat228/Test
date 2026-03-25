
#include "parkdataserializer.h"
#include "park_dsp_lite_4x4.h"
#include "parkfileformatmanager.h"

#ifdef ParkDSPLiteBoard
#include "dsp_board.h"
#include "flash.h"
#endif

SpeakerType SpeakerRead[4];

uint32_t payloadSize = 0;
uint32_t counter = 0;

static bool CheckUint8Param(uint8_t* param, uint8_t value, uint8_t min, uint8_t max, uint8_t def);
static bool CheckUint16Param(uint16_t* param, uint16_t value, uint16_t min, uint16_t max, uint16_t def);
static bool CheckInt16Param(int16_t* param, int16_t value, int16_t min, int16_t max, int16_t def);
static bool CheckUint32Param(uint32_t* param, uint32_t value, uint32_t min, uint32_t max, uint32_t def);

static void SourceStatusWrite(uint8_t* buff, uint32_t* counter, InputSourceType* source);
static void ChannelStatusWrite(uint8_t* buff, uint32_t* counter, SpeakerOutType* spkOut);

//Serialize EntityPayload

//Status
uint16_t SerializeStatusEntityPayloadHome(uint8_t* buff, PresetType* preset)
{
  counter = 0;
  uint8_t InCh = sizeof(&preset->PresetParam->InputSource);
  uint8_t OutCh = sizeof(&preset->Speaker);
  uint8_t index = 0;
  
  buff[counter++] = (InCh << 4) | (OutCh & 0x0F);       // In/Out Config
  for(index = 0; index < InCh; index++) 
  {
    //SourceStatusWrite(buff, &counter, &preset->PresetParam->InputSource[index]);    // Sourse Status 
    
    int16_t level = 0;
    AmpInputValueType source = IN_NONE;

    uint8_t levelindex = preset->PresetParam->InputSource[index].ManualSource % 32;
    
    //if((preset->PresetParam->InputSource[index].ManualSource / 32) == 0) level = preset->PresetParam->InputSource[levelindex].InputLevel;
    
    if(index == 0)level = preset->PresetParam->InputSource[index].InputLevel;
    else level = preset->PresetParam->InputSource[levelindex].AnalogLevel;
  
    buff[counter++] = (uint8_t)source;         // Sourse
    buff[counter++] = (int8_t)(level / 100);  // In Level
  }
  for(index = 0; index < OutCh; index++) ChannelStatusWrite(buff, &counter, preset->Speaker[index]->OUT[0]);        // Channel  Status                    
  
  return (uint16_t)counter;
}

uint16_t SerializeStatusEntityPayloadSource(uint8_t* buff, PresetType* preset)
{
  counter = 0;
  uint8_t InCh = sizeof(&preset->PresetParam->InputSource);
  uint8_t OutCh = sizeof(&preset->Speaker);
  uint8_t index = 0;
  
  buff[counter++] = (InCh << 4) | (OutCh & 0x0F);       // In/Out Config
  for(index = 0; index < InCh; index++) SourceStatusWrite(buff, &counter, &preset->PresetParam->InputSource[index]);    // Sourse Status                                     
  
  return (uint16_t)counter;
}

uint16_t SerializeStatusEntityPayloadMatrix(uint8_t* buff, PresetType* preset)
{
  counter = 0;
  uint8_t InCh = sizeof(&preset->PresetParam->InputSource);
  uint8_t OutCh = sizeof(&preset->Speaker);
  uint8_t index = 0;
  
  buff[counter++] = (InCh << 4) | (OutCh & 0x0F);       // In/Out Config                   
  for(index = 0; index < OutCh; index++) ChannelStatusWrite(buff, &counter, preset->Speaker[index]->OUT[index]);        // Channel  Status                    
  
  return (uint16_t)counter;
}

uint16_t SerializeStatusEntityPayloadSpeakerOut(uint8_t* buff, PresetType* preset)
{
  counter = 0;
  uint8_t InCh = sizeof(&preset->PresetParam->InputSource);
  uint8_t OutCh = sizeof(&preset->Speaker);
  uint8_t index = 0;
  
  buff[counter++] = (InCh << 4) | (OutCh & 0x0F);       // In/Out Config                  
  for(index = 0; index < OutCh; index++) ChannelStatusWrite(buff, &counter, preset->Speaker[index]->OUT[index]);        // Channel  Status                    
  
  return (uint16_t)counter;
}

// General
uint16_t SerializeParamEntityPayloadByte(uint8_t* buff, ParamType type, uint8_t args, uint8_t byte)
{
  counter = 0;
  
  buff[counter++] = (uint8_t)(type >> 8);               // ParamType
  buff[counter++] = (uint8_t)type;
  buff[counter++] = args;                               // ParamArgs
  Uint16ToByteArray(sizeof(uint8_t), buff, &counter);   // ParamSize

  buff[counter++] = byte;

  return (uint16_t)counter;
}

uint16_t SerializeParamEntityPayloadUshort(uint8_t* buff, ParamType type, uint8_t args, uint16_t data)
{
  counter = 0;
  
  buff[counter++] = (uint8_t)(type >> 8);               // ParamType
  buff[counter++] = (uint8_t)type;
  buff[counter++] = args;                               // ParamArgs
  Uint16ToByteArray(sizeof(uint16_t), buff, &counter);  // ParamSize

  buff[counter++] = (uint8_t)(data >> 8);
  buff[counter++] = (uint8_t)data;

  return (uint16_t)counter;
}

uint16_t SerializeParamEntityPayloadArray(uint8_t* buff, ParamType type, uint8_t args, uint8_t* array, uint16_t size)
{
  uint16_t index = 0;
  counter = 0;
  
  buff[counter++] = (uint8_t)(type >> 8);               // ParamType
  buff[counter++] = (uint8_t)type;
  buff[counter++] = args;                               // ParamArgs
  Uint16ToByteArray(size, buff, &counter);              // ParamSize
  
  for(index = 0; index < size; index++)
  {
    buff[counter++] = array[index];
  }
  
  return (uint16_t)counter;
}

uint16_t SerializeParamEntityPayloadString(uint8_t* buff, ParamType type, uint8_t args, uint8_t* str, uint16_t sizemax)
{
  uint16_t index = 0;
  payloadSize = 0;
  counter = 0;
  
  while((str[payloadSize++] != 0x00) && (payloadSize < sizemax));
  
  buff[counter++] = (uint8_t)(type >> 8);               // ParamType
  buff[counter++] = (uint8_t)type;
  buff[counter++] = args;                               // ParamArgs
  Uint16ToByteArray(payloadSize, buff, &counter);       // ParamSize
  
  for(index = 0; index < payloadSize; index++)
  {
    buff[counter++] = str[index];
  }
  if(index == sizemax) buff[counter - 1] = 0x00;

  return (uint16_t)counter;
}

// Audio
uint16_t SerializeParamEntityPayloadMute(uint8_t* buff, uint8_t args, MuteParamType* mute)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)mute->Value;

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeMute >> 8);              // ParamType
  buff[counter++] = (uint8_t)ParamTypeMute;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadPolarity(uint8_t* buff, uint8_t args, PolarityParamType* polarity)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)polarity->Value;

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypePolarity >> 8);          // ParamType
  buff[counter++] = (uint8_t)ParamTypePolarity;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadGain(uint8_t* buff, uint8_t args, GainParamType* gain)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  Int16ToByteArray(gain->Value, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeGain >> 8);              // ParamType
  buff[counter++] = (uint8_t)ParamTypeGain;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadDelay(uint8_t* buff, uint8_t args, DelayParamType* delay)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  Uint32ToByteArray(delay->Value, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeDelay >> 8);              // ParamType
  buff[counter++] = (uint8_t)ParamTypeDelay;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadHP(uint8_t* buff, uint8_t args, FilterHPParamType* hp)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)hp->State;
  buff[counter++] = (uint8_t)hp->Filter;
  Uint16ToByteArray(hp->Freq, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeFilterHP >> 8);          // ParamType
  buff[counter++] = (uint8_t)ParamTypeFilterHP;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadLP(uint8_t* buff, uint8_t args, FilterLPParamType* lp)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)lp->State;
  buff[counter++] = (uint8_t)lp->Filter;
  Uint16ToByteArray(lp->Freq, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeFilterLP >> 8);          // ParamType
  buff[counter++] = (uint8_t)ParamTypeFilterLP;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadEQ(uint8_t* buff, uint8_t args, FilterEQParamType* eq)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)eq->State;
  buff[counter++] = (uint8_t)eq->Filter;
  Uint16ToByteArray(eq->Freq, buff, &counter);
  Int16ToByteArray(eq->Boost, buff, &counter);
  Uint16ToByteArray(eq->Q, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeFilterEQ >> 8);          // ParamType
  buff[counter++] = (uint8_t)ParamTypeFilterEQ;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadFIR(uint8_t* buff, uint8_t args, FilterFirParamType* fir)
{
  uint8_t i = 0;
  uint16_t index = 0;
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)fir->State.State;
  buff[counter++] = (uint8_t)fir->Smpl;
  Uint16ToByteArray(fir->Taps, buff, &counter);
  
  for(index = 0; index < fir->Taps; index++)
  {
    for(i = 0; i < 8; i++)
    {
      buff[counter++] = ((uint8_t*)&fir->Coef[index])[i];
    }
  }
  
  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeFilterFIR >> 8);         // ParamType
  buff[counter++] = (uint8_t)ParamTypeFilterFIR;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadLimiterRMS(uint8_t* buff, uint8_t args, LimiterRMSParamType* limiter)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  // Param Payload
  buff[counter++] = (uint8_t)limiter->Switch.State;
  Uint16ToByteArray(limiter->Threshold.Value, buff, &counter);
  Uint32ToByteArray(limiter->Attack.Value, buff, &counter);
  Uint32ToByteArray(limiter->Release.Value, buff, &counter);
  Uint32ToByteArray(limiter->Hold.Value, buff, &counter);
  
  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeLimiterRMS >> 8);        // ParamType
  buff[counter++] = (uint8_t)ParamTypeLimiterRMS;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadLimiterPeak(uint8_t* buff, uint8_t args, LimiterPeakParamType* limiter)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;

  // Param Payload
  buff[counter++] = (uint8_t)limiter->Switch.State;
  Uint16ToByteArray(limiter->Threshold.Value, buff, &counter);
  Uint32ToByteArray(limiter->Attack.Value, buff, &counter);
  Uint32ToByteArray(limiter->Release.Value, buff, &counter);
  Uint32ToByteArray(limiter->Hold.Value, buff, &counter);
  
  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeLimiterPeak >> 8);       // ParamType
  buff[counter++] = (uint8_t)ParamTypeLimiterPeak;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadHizHP(uint8_t* buff, uint8_t args, FilterHizHPFParamType* hp)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)hp->State;
  Uint16ToByteArray(hp->Freq, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeSpeakerHizHPF >> 8);     // ParamType
  buff[counter++] = (uint8_t)ParamTypeSpeakerHizHPF;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadWayHizPower(uint8_t* buff, uint8_t args, WayHizPowerParamType* power)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = power->Voltage;
  Uint16ToByteArray(power->Power, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeOutHizPower >> 8);       // ParamType
  buff[counter++] = (uint8_t)ParamTypeOutHizPower;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadInputTrim(uint8_t* buff, ParamType type, uint8_t args, InputTrimType* inputTrim)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  Int16ToByteArray(inputTrim->Gain.Value, buff, &counter);
  Int32ToByteArray(inputTrim->Delay.Value, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(type >> 8);                       // ParamType
  buff[counter++] = (uint8_t)type;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadInputSource(uint8_t* buff, uint8_t args, InputSourceType* InputSource)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)InputSource->ManualSource;
  //buff[counter++] = (uint8_t)InputSource->BackupSource[0];
  //buff[counter++] = (uint8_t)InputSource->BackupSource[1];
  //buff[counter++] = (uint8_t)InputSource->BackupSource[2];
  //buff[counter++] = (uint8_t)InputSource->BackupSelectedSource;
  //buff[counter++] = (uint8_t)InputSource->BackupForse;
  //buff[counter++] = (uint8_t)InputSource->BackupReturn;

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeInputSource >> 8);       // ParamType
  buff[counter++] = (uint8_t)ParamTypeInputSource;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}

uint16_t SerializeParamEntityPayloadMatrix(uint8_t* buff, uint8_t args, MatrixType* Matrix)
{
  payloadSize = 0;
  counter = EntityPayloadParamAddr;
  
  buff[counter++] = (uint8_t)Matrix->Mute;
  Int16ToByteArray(Matrix->Gain, buff, &counter);

  payloadSize = counter - EntityPayloadParamAddr;
  counter = 0;
  
  // Param Header
  buff[counter++] = (uint8_t)(ParamTypeMatrix >> 8);            // ParamType
  buff[counter++] = (uint8_t)ParamTypeMatrix;
  buff[counter++] = args;                                       // ParamArgs
  Uint16ToByteArray((uint16_t)payloadSize, buff, &counter);     // ParamSize
  
  return (uint16_t)payloadSize + EntityPayloadParamAddr;
}


// Deserialize EntityPayload

bool DeserializeUserEQParamEntityPayload(uint8_t* buff, UserEQType* userEQ)
{
  bool error = false;
  uint32_t counter = 0;

  ParamType type = (ParamType)ByteArrayToUint16(buff, &counter);        // ParamType
  uint8_t args = buff[counter++];                                       // ParamArgs
  uint16_t size = ByteArrayToUint16(buff, &counter);                    // ParamSize
  
  switch(type)
  {
    case ParamTypeMute : error |= DeserializePayloadMute(&buff[counter], &userEQ->Mute, size); break;
    case ParamTypePolarity : error |= DeserializePayloadPolarity(&buff[counter], &userEQ->Polarity, size); break;
    case ParamTypeGain : error |= DeserializePayloadGain(&buff[counter], &userEQ->Gain, size); break;
    case ParamTypeDelay : error |= DeserializePayloadUserEQDelay(&buff[counter], &userEQ->Delay, size); break;
    case ParamTypeFilterEQ : error |= DeserializePayloadEQ(&buff[counter], &userEQ->FilterEQ[args], size); break;
    default: error = true;
  }
 
  #ifdef ParkDSPLiteBoard
  if(!error)
  {
    if(DeviceStatus.DeviceMode == USB_MODE)
    {
      if(userEQ->MemoryConfig.IsChanged != true)
      {
        Flash_Write_Param();
        userEQ->MemoryConfig.IsChanged = true;
      }
    }
  }
  #endif

  return error;
}

bool DeserializeSpeakerConfigParamEntityPayload(uint8_t* buff, SpeakerConfigType* speakerConfig)
{
  bool error = false;
  uint32_t counter = 0;

  ParamType type = (ParamType)ByteArrayToUint16(buff, &counter);        // ParamType
  uint8_t args = buff[counter++];                                       // ParamArgs
  uint16_t size = ByteArrayToUint16(buff, &counter);                    // ParamSize
  
  switch(type)
  {
    case ParamTypeSpeakerBrand : error |= DeserializePayloadString(&buff[counter], speakerConfig->BrandName, size); break;
    case ParamTypeSpeakerSeries : error |= DeserializePayloadString(&buff[counter], speakerConfig->SeriesName, size); break;
    case ParamTypeSpeakerModel : error |= DeserializePayloadString(&buff[counter], speakerConfig->ModelName, size); break;
    case ParamTypeSpeakerVariation : error |= DeserializePayloadString(&buff[counter], speakerConfig->VariationName, size); break;
    case ParamTypeSpeakerNotes : error |= DeserializePayloadString(&buff[counter], speakerConfig->Notes, size); break;
    case ParamTypeSpeakerType : error |= DeserializePayloadByte(&buff[counter], (uint8_t*)&speakerConfig->SpeakerType); break;
    case ParamTypeSpeakerWays : error |= DeserializePayloadByte(&buff[counter], &speakerConfig->Ways); break;
    case ParamTypeSpeakerHizHPF : error |= DeserializePayloadHizHP(&buff[counter], &speakerConfig->HizHPF, size); break;
    default: error = true;
  }
  
  return error;
}

bool DeserializeSpeakerOutConfigParamEntityPayload(uint8_t* buff, SpeakerOutConfigType* speakerOutConfig)
{
  bool error = false;
  uint32_t counter = 0;

  ParamType type = (ParamType)ByteArrayToUint16(buff, &counter);        // ParamType
  uint8_t args = buff[counter++];                                       // ParamArgs
  uint16_t size = ByteArrayToUint16(buff, &counter);                    // ParamSize
  
  switch(type)
  {
    case ParamTypeOutName : error |= DeserializePayloadArray(&buff[counter], speakerOutConfig->Name, size); break;
    case ParamTypeOutIndex : error |= DeserializePayloadByte(&buff[counter], (uint8_t*)&speakerOutConfig->Index); break;
    case ParamTypeOutImpedance : error |= DeserializePayloadByte(&buff[counter], (uint8_t*)&speakerOutConfig->Impedance); break;
    case ParamTypeOutPower : error |= DeserializePayloadUshort(&buff[counter], (uint16_t*)&speakerOutConfig->Power); break;
    case ParamTypeOutHizPower : error |= DeserializePayloadWayHizPower(&buff[counter], &speakerOutConfig->HizPower[args], size); break;
    default: error = true;
  }
  
  return error;
}

bool DeserializeSpeakerEQParamEntityPayload(uint8_t* buff, SpeakerEQType* speakerEQ)
{
  bool error = false;
  uint32_t counter = 0;

  ParamType type = (ParamType)ByteArrayToUint16(buff, &counter);        // ParamType
  uint8_t args = buff[counter++];                                       // ParamArgs
  uint16_t size = ByteArrayToUint16(buff, &counter);                    // ParamSize
  
  switch(type)
  {
    case ParamTypeFilterHP : error |= DeserializePayloadHP(&buff[counter], &speakerEQ->FilterHighPass, size); break;
    case ParamTypeFilterLP : error |= DeserializePayloadLP(&buff[counter], &speakerEQ->FilterLowPass, size); break;
    case ParamTypeFilterEQ : error |= DeserializePayloadEQ(&buff[counter], &speakerEQ->FilterEQ[args], size); break;
    case ParamTypeFilterFIR : error |= DeserializePayloadFIR(&buff[counter], speakerEQ->Fir, size); break;
    default: error = true;
  }
  
  return error;
}

//*****************************************************************************
bool DeserializePresetParamEntityPayload(uint8_t* buff, PresetParamType* PresetParamType)
{
  bool error = false;
  uint32_t counter = 0;

  ParamType type = (ParamType)ByteArrayToUint16(buff, &counter);        // ParamType
  uint8_t args = buff[counter++];                                       // ParamArgs
  uint16_t size = ByteArrayToUint16(buff, &counter);                    // ParamSize
  
  switch(type)
  {
    case ParamTypeInputMode: 
    {
      error |= DeserializePayloadByte(&buff[counter], (uint8_t*)&PresetParamType->InputMode); 
      break; 
    }
    case ParamTypeAnalogInputTrim: 
      {
        error |= DeserializeParamEntityPayloadInputTrim(&buff[counter], &PresetParamType->AnalogTrim, size); 
        break;
      }
    case ParamTypeAESInputTrim: error |= DeserializeParamEntityPayloadInputTrim(&buff[counter], &PresetParamType->AESTrim, size); break;
    case ParamTypeModeXLR: 
      {
        error |= DeserializePayloadByte(&buff[counter], (uint8_t*)&PresetParamType->ModeXLR[args].State); 
        
        #ifdef ParkDSPLiteBoard
        if(!error) XLR_Mode(&PresetParamType->ModeXLR[args], args);
        #endif
        
        break;
      }
    case ParamTypeBridge: 
      {
        error |= DeserializePayloadByte(&buff[counter], (uint8_t*)&PresetParamType->Bridge[args].State); 
        
        #ifdef ParkDSPLiteBoard
        if(!error) Bridge(args);
        #endif
        
        break;
      }
    case ParamTypeInputSource:
      {
        error |= DeserializePayloadInputSource(&buff[counter], &PresetParamType->InputSource[args], size); 
        
        break;                //????
      }
    case ParamTypeMatrix: 
      {
        uint8_t in = args % 16;
        uint8_t out = args / 16;
        
        if(in >= IN_CH_QTY) break;
        if(out >= OUT_CH_QTY) break;
        
        DeserializeParamEntityPayloadMatrix(&buff[counter], &PresetParamType->Matrix[in][out], size); 
        break;
      }  
    default: error = true;
  }
  
  #ifdef ParkDSPLiteBoard
  if(!error)
  {
    if(DeviceStatus.DeviceMode == USB_MODE)
    {
      if(PresetParamType->MemoryConfig.IsChanged != true)
      {
        Flash_Write_Param();
        PresetParamType->MemoryConfig.IsChanged = true;
      }
    }
  }
  #endif
  
  return error;
}

bool DeserializeSpeakerOutParamEntityPayload(uint8_t* buff, SpeakerOutType* speakerOut)
{
  bool error = false;
  uint32_t counter = 0;

  ParamType type = (ParamType)ByteArrayToUint16(buff, &counter);        // ParamType
  uint8_t args = buff[counter++];                                       // ParamArgs
  uint16_t size = ByteArrayToUint16(buff, &counter);                    // ParamSize
  
  switch(type)
  {
    case ParamTypeMute : error |= DeserializePayloadMute(&buff[counter], &speakerOut->Mute, size); break;
    case ParamTypePolarity : error |= DeserializePayloadPolarity(&buff[counter], &speakerOut->Polarity, size); break;
    case ParamTypeGain : error |= DeserializePayloadGain(&buff[counter], &speakerOut->Gain, size); break;
    case ParamTypeDelay : error |= DeserializePayloadSpkOutDelay(&buff[counter], &speakerOut->Delay, size); break;
    case ParamTypeFilterHP : error |= DeserializePayloadHP(&buff[counter], &speakerOut->FilterHighPass, size); break;
    case ParamTypeFilterLP : error |= DeserializePayloadLP(&buff[counter], &speakerOut->FilterLowPass, size); break;
    case ParamTypeFilterEQ : error |= DeserializePayloadEQ(&buff[counter], &speakerOut->FilterEQ[args], size); break;
    case ParamTypeFilterFIR : error |= DeserializePayloadFIR(&buff[counter], speakerOut->Fir, size); break;
    case ParamTypeLimiterRMS : error |= DeserializePayloadLimiterRMS(&buff[counter], &speakerOut->RMSLimiter, size); break;
    case ParamTypeLimiterPeak : error |= DeserializePayloadLimiterPeak(&buff[counter], &speakerOut->PeakLimiter, size); break;
    default: error = true;
  }
  
  #ifdef ParkDSPLiteBoard
  if(!error)
  {
    if(DeviceStatus.DeviceMode == USB_MODE)
    {
      if(speakerOut->MemoryConfig.IsChanged != true)
      {
        Flash_Write_Param();
        speakerOut->MemoryConfig.IsChanged = true;
      }
    }
  }
  #endif
  
  return error;
}

bool DeserializePayloadByte(uint8_t* buff, uint8_t* data)
{
  *data = buff[0];
  
  return 0;
}

bool DeserializePayloadUshort(uint8_t* buff, uint16_t* data)
{
  *data = (buff[0] << 8) | buff[1];

  return 0;
}

bool DeserializePayloadArray(uint8_t* buff, uint8_t* array, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  if((buff[0] == 0xFF) && (buff[1] == 0xFF)) return 1; // error
  
  for(counter = 0; counter < size; counter++)
  {
    array[counter] = buff[counter];
  }
  
  return error;
}

bool DeserializePayloadString(uint8_t* buff, uint8_t* str, uint16_t sizemax)
{
  bool error = false;
  uint32_t counter = 0;
  
  while((buff[counter] != 0x00) && (counter < sizemax))
  {
    error |= CheckUint8Param(&str[counter], buff[counter], 0x20, 0x7E, 0x00);
    counter++;
  }
  str[counter] = 0x00;
  
  return error;
}

bool DeserializePayloadMute(uint8_t* buff, MuteParamType* mute, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  MuteValueType value = (MuteValueType)buff[counter++];
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&mute->Value, value, Mute_Min, Mute_Max, Mute_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Mute(mute);
  #endif
    
  return error;
}

bool DeserializePayloadPolarity(uint8_t* buff, PolarityParamType* polarity, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  PolarityValueType value = (PolarityValueType)buff[counter++];
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&polarity->Value, value, Polarity_Min, Polarity_Max, Polarity_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Phase(polarity);
  #endif
  
  return error;
}

bool DeserializePayloadGain(uint8_t* buff, GainParamType* gain, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  int16_t value = ByteArrayToInt16(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckInt16Param(&gain->Value, value, Gain_Min, Gain_Max, Gain_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Gain(gain);
  #endif
  
  return error;
}

bool DeserializePayloadUserEQDelay(uint8_t* buff, DelayParamType* delay, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  uint32_t value = ByteArrayToUint32(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint32Param(&delay->Value, value, UserEQDelay_Min, UserEQDelay_Max, UserEQDelay_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Delay(delay);
  #endif

  return error;
}

bool DeserializePayloadSpkOutDelay(uint8_t* buff, DelayParamType* delay, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  uint32_t value = ByteArrayToUint32(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint32Param(&delay->Value, value, SpkOutDelay_Min, SpkOutDelay_Max, SpkOutDelay_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Delay(delay);
  #endif
  
  return error;
}

bool DeserializePayloadHP(uint8_t* buff, FilterHPParamType* hp, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  StateType state = (StateType)buff[counter++];
  FilterTypeHPLP filter = (FilterTypeHPLP)buff[counter++];
  uint16_t freq = ByteArrayToUint16(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&hp->State, state, HP_State_Min, HP_State_Max, HP_State_Def);
  error |= CheckUint8Param((uint8_t*)&hp->Filter, filter, HP_Filter_Min, HP_Filter_Max, HP_Filter_Def);
  error |= CheckUint16Param((uint16_t*)&hp->Freq, freq, HP_Freq_Min, HP_Freq_Max, HP_Freq_Def);
  
  return error;
}

bool DeserializePayloadLP(uint8_t* buff, FilterLPParamType* lp, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  StateType state = (StateType)buff[counter++];
  FilterTypeHPLP filter = (FilterTypeHPLP)buff[counter++];
  uint16_t freq = ByteArrayToUint16(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&lp->State, state, LP_State_Min, LP_State_Max, LP_State_Def);
  error |= CheckUint8Param((uint8_t*)&lp->Filter, filter, LP_Filter_Min, LP_Filter_Max, LP_Filter_Def);
  error |= CheckUint16Param((uint16_t*)&lp->Freq, freq, LP_Freq_Min, LP_Freq_Max, LP_Freq_Def);
  
  return error;
}

bool DeserializePayloadEQ(uint8_t* buff, FilterEQParamType* eq, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  StateType state = (StateType)buff[counter++];
  FilterTypeEQ filter = (FilterTypeEQ)buff[counter++];
  uint16_t freq = ByteArrayToUint16(buff, &counter);
  int16_t gain = ByteArrayToInt16(buff, &counter);
  uint16_t q = ByteArrayToInt16(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&eq->State, state, EQ_State_Min, EQ_State_Max, EQ_State_Def);
  error |= CheckUint8Param((uint8_t*)&eq->Filter, filter, EQ_Filter_Min, EQ_Filter_Max, EQ_Filter_Def);
  error |= CheckUint16Param((uint16_t*)&eq->Freq, freq, EQ_Freq_Min, EQ_Freq_Max, EQ_Freq_Def);
  error |= CheckInt16Param((int16_t*)&eq->Boost, gain, EQ_Boost_Min, EQ_Boost_Max, EQ_Boost_Def);
  error |= CheckUint16Param((uint16_t*)&eq->Q, q, EQ_Q_Min, EQ_Q_Max, EQ_Q_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Filter(eq);
  #endif
  
  return error;
}

bool DeserializePayloadFIR(uint8_t* buff, FilterFirParamType* fir, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  uint16_t index = 0;
  StateType state = (StateType)buff[counter++];
  uint8_t smpl = buff[counter++];
  int8_t i = 0;
  uint16_t taps = ByteArrayToUint16(buff, &counter);
  
  for(index = 0; index < taps; index++)
  {
    //fir->Coef[index] = DoubleToDspFixpoint(ByteArrayToDouble(buff, &counter));
    //fir->Coef[index] = ByteArrayToDouble(buff, &counter);
    for(i = 7; i >= 0; i--)
    {
      ((uint8_t*)&fir->Coef[index])[i] = buff[counter++];
    }
  }
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&fir->State, state, FIR_State_Min, FIR_State_Max, FIR_State_Def);
  error |= CheckUint8Param((uint8_t*)&fir->Smpl, smpl, FIR_Smpl_Min, FIR_Smpl_Max, FIR_Smpl_Def);
  error |= CheckUint16Param((uint16_t*)&fir->Taps, taps, FIR_Taps_Min, FIR_Taps_Max, FIR_Taps_Def);

  return error;
}

bool DeserializePayloadLimiterRMS(uint8_t* buff, LimiterRMSParamType* lim, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  StateType state = (StateType)buff[counter++];
  uint16_t Threshold = ByteArrayToUint16(buff, &counter);
  uint32_t Attack = ByteArrayToUint32(buff, &counter);
  uint32_t Release = ByteArrayToUint32(buff, &counter);
  uint32_t Hold = ByteArrayToUint32(buff, &counter);

  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&lim->Switch.State, state, LimRMS_State_Min, LimRMS_State_Max, LimRMS_State_Def);
  error |= CheckUint16Param((uint16_t*)&lim->Threshold.Value, Threshold, LimRMS_Threshold_Min, LimRMS_Threshold_Max, LimRMS_Threshold_Def);
  error |= CheckUint32Param((uint32_t*)&lim->Attack.Value, Attack, LimRMS_Attack_Min, LimRMS_Attack_Max, LimRMS_Attack_Def);
  error |= CheckUint32Param((uint32_t*)&lim->Release.Value, Release, LimRMS_Release_Min, LimRMS_Release_Max, LimRMS_Release_Def);
  error |= CheckUint32Param((uint32_t*)&lim->Hold.Value, Hold, LimRMS_Hold_Min, LimRMS_Hold_Max, LimRMS_Hold_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Limiter(lim);
  #endif

  return error;
}

bool DeserializePayloadLimiterPeak(uint8_t* buff, LimiterPeakParamType* lim, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  StateType state = (StateType)buff[counter++];
  uint16_t Threshold = ByteArrayToUint16(buff, &counter);
  uint32_t Attack = ByteArrayToUint32(buff, &counter);
  uint32_t Release = ByteArrayToUint32(buff, &counter);
  uint32_t Hold = ByteArrayToUint32(buff, &counter);

  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&lim->Switch.State, state, LimPeak_State_Min, LimPeak_State_Max, LimPeak_State_Def);
  error |= CheckUint16Param((uint16_t*)&lim->Threshold.Value, Threshold, LimPeak_Threshold_Min, LimPeak_Threshold_Max, LimPeak_Threshold_Def);
  error |= CheckUint32Param((uint32_t*)&lim->Attack.Value, Attack, LimPeak_Attack_Min, LimPeak_Attack_Max, LimPeak_Attack_Def);
  error |= CheckUint32Param((uint32_t*)&lim->Release.Value, Release, LimPeak_Release_Min, LimPeak_Release_Max, LimPeak_Release_Def);
  error |= CheckUint32Param((uint32_t*)&lim->Hold.Value, Hold, LimPeak_Hold_Min, LimPeak_Hold_Max, LimPeak_Hold_Def);
  
  #ifdef ParkDSPLiteBoard
  //if(!error) DSP_Peak_Limiter(lim);
  #endif

  return error;
}

bool DeserializePayloadHizHP(uint8_t* buff, FilterHizHPFParamType* hp, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  StateType state = (StateType)buff[counter++];
  uint16_t freq = ByteArrayToUint16(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&hp->State, state, HizHPF_State_Min, HizHPF_State_Max, HizHPF_State_Def);
  error |= CheckUint16Param((uint16_t*)&hp->Freq, freq, HizHPF_Freq_Min, HizHPF_Freq_Max, HizHPF_Freq_Def);
  
  return error;
}

bool DeserializePayloadWayHizPower(uint8_t* buff, WayHizPowerParamType* wayhizpower, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  uint8_t voltage = (StateType)buff[counter++];
  uint16_t power = ByteArrayToUint16(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckUint8Param((uint8_t*)&wayhizpower->Voltage, voltage, HizVoltageTyp_Min, HizVoltageTyp_Max, HizVoltageTyp_Def);
  error |= CheckUint16Param((uint16_t*)&wayhizpower->Power, power, WayHizPower_Power_Min, WayHizPower_Power_Max, WayHizPower_Power_Def);
  
  return error;
}

bool DeserializeParamEntityPayloadInputTrim(uint8_t* buff, InputTrimType* inputTrim, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  int16_t gainValue = ByteArrayToInt16(buff, &counter);
  int16_t delayValue = ByteArrayToUint32(buff, &counter);
  
  if(counter != size) return 1; // error
  
  error |= CheckInt16Param((int16_t*)&inputTrim->Gain.Value, gainValue, InputTrimGain_Min, InputTrimGain_Max, InputTrimGain_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Gain(&inputTrim->Gain);
  #endif
  
  error |= CheckUint32Param((uint32_t*)&inputTrim->Delay.Value, delayValue, InputTrimDelay_Min, InputTrimDelay_Max, InputTrimDelay_Def);

  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Delay(&inputTrim->Delay);
  #endif
  
  return error;
}

bool DeserializeParamEntityPayloadMatrix(uint8_t* buff, MatrixType* matrix, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;
  
  MuteValueType mute = (MuteValueType)buff[counter++];
  int16_t gainValue = ByteArrayToInt16(buff, &counter);

  if(counter != size) return 1; // error
  
  error |= CheckInt16Param((int16_t*)&matrix->Gain, gainValue, MatrixGain_Min, MatrixGain_Max, MatrixGain_Def);
  error |= CheckUint8Param((uint8_t*)&matrix->Mute, mute, MuteValueType_Min, MuteValueType_Max, MuteValueType_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Matrix_Value(matrix);
  #endif
  
  return error;
}

bool DeserializePayloadInputSource(uint8_t* buff, InputSourceType* InputSource, uint16_t size)
{
  bool error = false;
  uint32_t counter = 0;

  error |= CheckUint8Param((uint8_t*)&InputSource->ManualSource, buff[counter++], AmpInputValueType_Min, AmpInputValueType_Max, AmpInputValueType_Def);
  
  #ifdef ParkDSPLiteBoard
  if(!error) DSP_Source_Select(InputSource, &InputSource->ManualSource);
  #endif
  
  //error |= CheckUint8Param((uint8_t*)&InputSource->BackupSource[0], buff[counter++], AmpInputValueType_Min, AmpInputValueType_Max, AmpInputValueType_Def);
  //error |= CheckUint8Param((uint8_t*)&InputSource->BackupSource[1], buff[counter++], AmpInputValueType_Min, AmpInputValueType_Max, AmpInputValueType_Def);
  //error |= CheckUint8Param((uint8_t*)&InputSource->BackupSource[2], buff[counter++], AmpInputValueType_Min, AmpInputValueType_Max, AmpInputValueType_Def);
  //error |= CheckUint8Param((uint8_t*)&InputSource->BackupSelectedSource, buff[counter++], BackupSourceItemType_Min, BackupSourceItemType_Max, BackupSourceItemType_Def);
  //error |= CheckUint8Param((uint8_t*)&InputSource->BackupForse, buff[counter++], StateType_Min, StateType_Max, StateType_Def);
  //error |= CheckUint8Param((uint8_t*)&InputSource->BackupReturn, buff[counter++], StateType_Min, StateType_Max, StateType_Def);
  
  if(counter != size) return 1; // error
  
  return error;
}


// Serialize Entity

uint32_t SerializeEntity(uint8_t* entity, EntityType type, uint8_t payloadType, uint8_t payloadArgs, uint32_t payloadSize, uint8_t* payload)
{
  uint8_t i = 0;
  uint32_t counter = 0;
  
  entity[counter++] = (uint8_t)type;
  entity[counter++] = (uint8_t)payloadType;
  entity[counter++] = (uint8_t)payloadArgs;
  entity[counter++] = (uint8_t)(payloadSize >> 24);
  entity[counter++] = (uint8_t)(payloadSize >> 16);
  entity[counter++] = (uint8_t)(payloadSize >> 8);
  entity[counter++] = (uint8_t)(payloadSize);
  
  if(&entity[counter] == payload)
  {
    counter += payloadSize;
  }
  else
  {
    for(i = 0; i < payloadSize; i++)
    {
      entity[counter++] = payload[i];
    }
  }
  
  return counter;
}

uint32_t SerializeStatusEntity(uint8_t* buff, StatusPayloadType payloadType, uint8_t payloadArgs, uint16_t payloadSize, uint8_t* payload)
{
  return SerializeEntity(buff, EntityType_Status, payloadType, payloadArgs, payloadSize, payload);
}

uint32_t SerializeCommandEntity(uint8_t* buff, CommandPayloadType payloadType, uint8_t payloadArgs, uint16_t payloadSize, uint8_t* payload)
{
  return SerializeEntity(buff, EntityType_Command, payloadType, payloadArgs, payloadSize, payload);
}

uint32_t SerializeDataEntity(uint8_t* buff, DataPayloadType payloadType, uint8_t payloadArgs, uint16_t payloadSize, uint8_t* payload)
{
  return SerializeEntity(buff, EntityType_Data, payloadType, payloadArgs, payloadSize, payload);
}

uint32_t SerializeParamEntity(uint8_t* buff, ParamPayloadType payloadType, uint8_t payloadArgs, uint16_t payloadSize, uint8_t* payload)
{
  return SerializeEntity(buff, EntityType_Param, payloadType, payloadArgs, payloadSize, payload);
}

// Deserialize Entity

bool DeserializeEntity(uint8_t* entity, EntityHelperType* retEntity)
{
  bool error = false;
  uint8_t counter = 0;
  EntityType entityType = EntityType_None;
  uint8_t payloadType = 0;
  uint8_t payloadArgs = 0;
  uint32_t payloadSize = 0;
  
  entityType = (EntityType)entity[counter++];
  payloadType = entity[counter++];
  payloadArgs = entity[counter++];
  payloadSize = entity[counter++] << 24;
  payloadSize |= entity[counter++] << 16;
  payloadSize |= entity[counter++] << 8;
  payloadSize |= entity[counter++];
  
  switch(entityType)
  {
    case EntityType_Command: error |= DeserializeCommandEntity((CommandPayloadType)payloadType, payloadArgs, payloadSize, &entity[counter], retEntity); break;
    case EntityType_Status: error |= DeserializeStatusEntity((StatusPayloadType)payloadType, payloadArgs, payloadSize, &entity[counter], retEntity); break;
    case EntityType_Data: error |= DeserializeDataEntity((DataPayloadType)payloadType, payloadArgs, payloadSize, &entity[counter], retEntity); break;
    case EntityType_Param: error |= DeserializeParamEntity((ParamPayloadType)payloadType, payloadArgs, payloadSize, &entity[counter], retEntity); break;
    default: error = true;
  }
  
  return error;
}

bool DeserializeStatusEntity(StatusPayloadType payloadType, uint8_t payloadArgs, uint32_t payloadSize, uint8_t* payload, EntityHelperType* retEntity)
{
  bool error = false;
  //uint8_t counter = 0;
  
  switch(payloadType)
  {
    case StatusType_GetHomeStatus:
    {
      retEntity->Index = 0;
      retEntity->Size = 0;

      retEntity->Size = SerializeStatusEntityPayloadHome(&retEntity->Entity[EntityHeaderSize], &PRESET);
      if(retEntity->Size == 0) error = true;    //error
      
      retEntity->Size = SerializeStatusEntity(retEntity->Entity, StatusType_HomeStatus, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      break;
    }
    default: error = true;
  }
  
  if(retEntity->Size == 0) error = true;        //error

  return error;
}

bool DeserializeCommandEntity(CommandPayloadType payloadType, uint8_t payloadArgs, uint32_t payloadSize, uint8_t* payload, EntityHelperType* retEntity)
{
  bool error = false;
  uint8_t counter = 0;
  
  switch(payloadType)
  {
    case CommandType_GetDevInfo: 
    {
      retEntity->Index = 0;
      
      retEntity->Size = FileFormatWriteDeviceInfo(&DEVICE_INFO, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      retEntity->Size = SerializeDataEntity(retEntity->Entity, DataPayloadType_DevInfo, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      break;
    }
    case CommandType_GetDevPresetParam: 
    {
      retEntity->Index = 0;
      
      retEntity->Size = FileFormatWritePresetParam(&PRESET_PARAM, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      retEntity->Size = SerializeDataEntity(retEntity->Entity, DataPayloadType_DevPreset, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      break;
    }
    case CommandType_GetDeviceUserEQ: 
    {
      retEntity->Index = 0;
      
      retEntity->Size = FileFormatWriteUserEQ(&USER_EQ[payloadArgs], &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      retEntity->Size = SerializeDataEntity(retEntity->Entity, DataPayloadType_UserEQ, payloadArgs, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      break;
    }
    case CommandType_GetDeviceSpeaker: 
    {
      retEntity->Index = 0;
      
      retEntity->Size = FileFormatWriteSpeaker(&SPEAKER[payloadArgs], &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      retEntity->Size = SerializeDataEntity(retEntity->Entity, DataPayloadType_Speaker, payloadArgs, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
      if(retEntity->Size == 0) error = true;    //error
      
      break;
    }
    case CommandType_GetPacketOfIndex: 
    {
      counter = 0;
      retEntity->Index = payload[counter++] << 24;
      retEntity->Index |= payload[counter++] << 16;
      retEntity->Index |= payload[counter++] << 8;
      retEntity->Index |= payload[counter++];
      break;
    }
    //case CommandType_GetHomeStatus: 
    //case CommandType_GetInputSourceStatus:
    //case CommandType_GetMatrixStatus:
    //case CommandType_GetSpeakerOutStatus:
    //{
    //  retEntity->Index = 0;
    //  retEntity->Size = 0;
    //
    //  retEntity->Size = SerializeStatusEntityPayloadHome(&retEntity->Entity[EntityHeaderSize], &PRESET);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  retEntity->Size = SerializeStatusEntity(retEntity->Entity, StatusType_HomeStatus, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  break;
    //}
    //case CommandType_GetInputSourceStatus: 
    //{
    //  retEntity->Index = 0;
    //  retEntity->Size = 0;
    //
    //  retEntity->Size = SerializeStatusEntityPayloadSource(&retEntity->Entity[EntityHeaderSize], &PRESET);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  retEntity->Size = SerializeStatusEntity(retEntity->Entity, StatusType_InputSourceStatus, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  break;
    //}
    //case CommandType_GetMatrixStatus: 
    //{
    //  retEntity->Index = 0;
    //  retEntity->Size = 0;
    //
    //  retEntity->Size = SerializeStatusEntityPayloadMatrix(&retEntity->Entity[EntityHeaderSize], &PRESET);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  retEntity->Size = SerializeStatusEntity(retEntity->Entity, StatusType_MatrixStatus, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  break;
    //}
    //case CommandType_GetSpeakerOutStatus: 
    //{
    //  retEntity->Index = 0;
    //  retEntity->Size = 0;
    //
    //  retEntity->Size = SerializeStatusEntityPayloadSpeakerOut(&retEntity->Entity[EntityHeaderSize], &PRESET);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  retEntity->Size = SerializeStatusEntity(retEntity->Entity, StatusType_SpeakerOutStatus, 0, retEntity->Size, &retEntity->Entity[EntityHeaderSize]);
    //  if(retEntity->Size == 0) error = true;    //error
    //  
    //  break;
    //}
    
    
    default: error = true;
  }
  
  if(retEntity->Size == 0) error = true;        //error

  return error;
}

bool DeserializeParamEntity(ParamPayloadType payloadType, uint8_t payloadArgs, uint32_t payloadSize, uint8_t* payload, EntityHelperType* retEntity)
{
  bool error = false;
  
  switch(payloadType)
  {
    case ParamPayloadType_PresetParam: 
    {
      error = DeserializePresetParamEntityPayload(payload, &PRESET_PARAM);
      break;
    }
    case ParamPayloadType_UserEQ: 
    {
      error = DeserializeUserEQParamEntityPayload(payload, &USER_EQ[payloadArgs]); 
      break;
    }
    case ParamPayloadType_SpeakerEQ: 
    {
      error = DeserializeSpeakerEQParamEntityPayload(payload, &SPEAKER_EQ[payloadArgs]); 
      break;
    }
    case ParamPayloadType_SpeakerOut: 
    {
      error = DeserializeSpeakerOutParamEntityPayload(payload, &SPEAKER_OUT[payloadArgs]); 
      break;
    }
    
    default: error = true;
  }
      
  if(error)
  {
    retEntity->Index = 0;
    retEntity->Size = SerializeCommandEntity(retEntity->Entity, CommandType_Nack, 0, 0, &retEntity->Entity[EntityHeaderSize]);
    if(retEntity->Size == 0) error = true;    //error
  }
  else
  {
    retEntity->Index = 0;
    retEntity->Size = SerializeCommandEntity(retEntity->Entity, CommandType_Ack, 0, 0, &retEntity->Entity[EntityHeaderSize]);
    if(retEntity->Size == 0) error = true;    //error
  }
  
  return error;
}

bool DeserializeDataEntity(DataPayloadType payloadType, uint8_t payloadArgs, uint32_t payloadSize, uint8_t* payload, EntityHelperType* retEntity)
{
  bool error = false;
  
  switch(payloadType)
  {
    case DataPayloadType_Speaker: 
    {
      DWT->CYCCNT = 0;
      error = FileFormatReadSpeaker(&SpeakerRead[0], payload);
      break;
    }

    default: error = true;
  }
      
  if(error)
  {
    retEntity->Index = 0;
    retEntity->Size = SerializeCommandEntity(retEntity->Entity, CommandType_Nack, 0, 0, &retEntity->Entity[EntityHeaderSize]);
    if(retEntity->Size == 0) error = true;    //error
  }
  else
  {
    retEntity->Index = 0;
    retEntity->Size = SerializeCommandEntity(retEntity->Entity, CommandType_Ack, 0, 0, &retEntity->Entity[EntityHeaderSize]);
    if(retEntity->Size == 0) error = true;    //error
  }
  
  return error;
}

// Checking

bool CheckUint8Param(uint8_t* param, uint8_t value, uint8_t min, uint8_t max, uint8_t def)
{
  bool error = false;
  
  if((value >= min) && (value <= max)) *param = value;
  else 
  {
    *param = def;
    error = true;
  }
  
  return error;
}

bool CheckUint16Param(uint16_t* param, uint16_t value, uint16_t min, uint16_t max, uint16_t def)
{
  bool error = false;
  
  if((value >= min) && (value <= max)) *param = value;
  else 
  {
    *param = def;
    error = true;
  }
  
  return error;
}

bool CheckInt16Param(int16_t* param, int16_t value, int16_t min, int16_t max, int16_t def)
{
  bool error = false;
  
  if((value >= min) && (value <= max)) *param = value;
  else 
  {
    *param = def;
    error = true;
  }
  
  return error;
}

bool CheckUint32Param(uint32_t* param, uint32_t value, uint32_t min, uint32_t max, uint32_t def)
{
  bool error = false;
  
  if((value >= min) && (value <= max)) *param = value;
  else 
  {
    *param = def;
    error = true;
  }
  
  return error;
}

static void SourceStatusWrite(uint8_t* buff, uint32_t* counter, InputSourceType* inputsource)
{
  int16_t level = 0;
  AmpInputValueType source = IN_NONE;
  
  //if(inputsource->BackupForse == OFF)
  //{
  //  // input priority
  //}
  //else
  //{
  //  source = inputsource->ManualSource;
  if((inputsource->ManualSource / 32) == 0) level = inputsource->AnalogLevel;
  //}

  buff[(*counter)++] = (uint8_t)source;         // Sourse
  buff[(*counter)++] = (int8_t)(level / 100);  // In Level
  
}

static void ChannelStatusWrite(uint8_t* buff, uint32_t* counter, SpeakerOutType* spkOut)
{
  buff[(*counter)++] = (uint8_t)(spkOut->RMSLimiter.Reduction / 100);
  buff[(*counter)++] = (uint8_t)(spkOut->PeakLimiter.Reduction / 100);
  buff[(*counter)++] = (uint8_t)(spkOut->ClipLimiter.Reduction / 100);
  buff[(*counter)++] = (uint8_t)(spkOut->OutLevel / 100);
}
