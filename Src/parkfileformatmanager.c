
#include "parkfileformatmanager.h"
#include "dataconverter.h"
#include "crc.h"

static bool DataBlockWrite(uint32_t datasize, uint32_t *pointer, uint8_t* buff, uint16_t* blockcounter);
static uint32_t DataBlockRead(uint8_t* data);
static bool CheckEndFlag(uint8_t* data);
static bool FileFinalise(uint16_t filetype, uint16_t fileversion, uint8_t* buff, uint32_t* pointer, uint16_t blockcounter);
//FileHeader Header;
uint32_t crc = 0;

//File Write

uint16_t FileFormatWriteDeviceInfo(DeviceInfoType* DeviceInfo, uint8_t* buff)
{         
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Type
  if(DataBlockWrite(SerializeParamEntityPayloadUshort(&buff[pointer + FileBlockSize], ParamTypeDeviceType, 0, DeviceInfo->Type), &pointer, buff, &blockcounter)) return 0; // error
  
  //Brand
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeDeviceBrand, 0, DeviceInfo->Brand, sizeof(DeviceInfo->Brand)), &pointer, buff, &blockcounter)) return 0; // error
  
  //Series
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeDeviceSeries, 0, DeviceInfo->Series, sizeof(DeviceInfo->Series)), &pointer, buff, &blockcounter)) return 0; // error
  
  //Model
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeDeviceModel, 0, DeviceInfo->Model, sizeof(DeviceInfo->Model)), &pointer, buff, &blockcounter)) return 0; // error
  
  //Name
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeDeviceName, 0, DeviceInfo->Name, sizeof(DeviceInfo->Name)), &pointer, buff, &blockcounter)) return 0; // error
  
  //FW
  if(DataBlockWrite(SerializeParamEntityPayloadArray(&buff[pointer + FileBlockSize], ParamTypeDeviceFW, 0, DeviceInfo->FW, sizeof(DeviceInfo->FW)), &pointer, buff, &blockcounter)) return 0; // error
  
  //HW
  if(DataBlockWrite(SerializeParamEntityPayloadArray(&buff[pointer + FileBlockSize], ParamTypeDeviceHW, 0, DeviceInfo->HW, sizeof(DeviceInfo->HW)), &pointer, buff, &blockcounter)) return 0; // error
  
  //SN
  if(DataBlockWrite(SerializeParamEntityPayloadArray(&buff[pointer + FileBlockSize], ParamTypeDeviceSN, 0, DeviceInfo->SN, sizeof(DeviceInfo->SN)), &pointer, buff, &blockcounter)) return 0; // error
  
  //Finalisation
  if(FileFinalise(DeviceInfoFileType, DeviceInfoFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint16_t FileFormatWriteUserEQ(UserEQType* UserEQ, uint8_t* buff)
{     
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;

  //Mute
  if(DataBlockWrite(SerializeParamEntityPayloadMute(&buff[pointer + FileBlockSize], 0, &UserEQ->Mute), &pointer, buff, &blockcounter)) return 0; // error
  
  //Polarity
  if(DataBlockWrite(SerializeParamEntityPayloadPolarity(&buff[pointer + FileBlockSize], 0, &UserEQ->Polarity), &pointer, buff, &blockcounter)) return 0; // error
  
  //Gain
  if(DataBlockWrite(SerializeParamEntityPayloadGain(&buff[pointer + FileBlockSize], 0, &UserEQ->Gain), &pointer, buff, &blockcounter)) return 0; // error
  
  //Delay
  if(DataBlockWrite(SerializeParamEntityPayloadDelay(&buff[pointer + FileBlockSize], 0, &UserEQ->Delay), &pointer, buff, &blockcounter)) return 0; // error
  
  //EQ[]
  for(counter = 0; counter < USREQ_EQ_QTY; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadEQ(&buff[pointer + FileBlockSize], counter, &UserEQ->FilterEQ[counter]), &pointer, buff, &blockcounter)) return 0; // error
  }

  //Finalisation
  if(FileFinalise(UserEQFileType, UserEQFileVer, buff, &pointer, blockcounter)) return 0; // error
  
  return pointer;
}

uint16_t FileFormatWriteSpeakerConfig(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
{         
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //BrandName
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeSpeakerBrand, 0, SpeakerConfig->BrandName, SpkConfxNameSize), &pointer, buff, &blockcounter)) return 0; // error;

  //SeriesName
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeSpeakerSeries, 0, SpeakerConfig->SeriesName, SpkConfxNameSize), &pointer, buff, &blockcounter)) return 0; // error;

  //ModelName
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeSpeakerModel, 0, SpeakerConfig->ModelName, SpkConfxNameSize), &pointer, buff, &blockcounter)) return 0; // error;
  
  //VariationName
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeSpeakerVariation, 0, SpeakerConfig->VariationName, SpkConfxNameSize), &pointer, buff, &blockcounter)) return 0; // error;

  //Notes
  if(DataBlockWrite(SerializeParamEntityPayloadString(&buff[pointer + FileBlockSize], ParamTypeSpeakerNotes, 0, SpeakerConfig->Notes, SpkConfNoteSize), &pointer, buff, &blockcounter)) return 0; // error;

  //SpeakerType
  if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeSpeakerType, 0, SpeakerConfig->SpeakerType), &pointer, buff, &blockcounter)) return 0; // error;

  //Ways
  if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeSpeakerWays, 0, SpeakerConfig->Ways), &pointer, buff, &blockcounter)) return 0; // error;

  //HizHPF
  if(DataBlockWrite(SerializeParamEntityPayloadHizHP(&buff[pointer + FileBlockSize], 0, &SpeakerConfig->HizHPF), &pointer, buff, &blockcounter)) return 0; // error;

  //Finalisation
  if(FileFinalise(SpeakerConfigFileType, SpeakerConfigFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint16_t FileFormatWriteSpeakerOutConfig(SpeakerOutConfigType* SpeakerOutConfig, uint8_t* buff)
{         
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;

  //OUTName
  if(DataBlockWrite(SerializeParamEntityPayloadArray(&buff[pointer + FileBlockSize], ParamTypeOutName, 0, SpeakerOutConfig->Name, SpkOutConfOutNameSize), &pointer, buff, &blockcounter)) return 0; // error

  //OutIndex
  if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeOutIndex, 0, SpeakerOutConfig->Index), &pointer, buff, &blockcounter)) return 0; // error

  //OutImpedance
  if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeOutImpedance, 0, SpeakerOutConfig->Impedance), &pointer, buff, &blockcounter)) return 0; // error
  
  //OutPower
  if(DataBlockWrite(SerializeParamEntityPayloadUshort(&buff[pointer + FileBlockSize], ParamTypeOutPower, 0, SpeakerOutConfig->Power), &pointer, buff, &blockcounter)) return 0; // error

  //WayHizPower[]
  for(counter = 0; counter < SPKCONF_HIzPOW_QTY; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadWayHizPower(&buff[pointer + FileBlockSize], counter, &SpeakerOutConfig->HizPower[counter]), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //Finalisation
  if(FileFinalise(SpeakerOutConfigFileType, SpeakerOutConfigFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint16_t FileFormatWriteSpeakerEQ(SpeakerEQType* SpeakerEQ, uint8_t* buff)
{          
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;
  
  //////////////////////////////////////////////////////////////////////////////

  //HP
  if(DataBlockWrite(SerializeParamEntityPayloadHP(&buff[pointer + FileBlockSize], 0, &SpeakerEQ->FilterHighPass), &pointer, buff, &blockcounter)) return 0; // error

  
  //EQ[]
  for(counter = 0; counter < SPKEQ_EQ_QTY; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadEQ(&buff[pointer + FileBlockSize], counter, &SpeakerEQ->FilterEQ[counter]), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //LP
  if(DataBlockWrite(SerializeParamEntityPayloadLP(&buff[pointer + FileBlockSize], 0, &SpeakerEQ->FilterLowPass), &pointer, buff, &blockcounter)) return 0; // error
  
  //FIR
  if(DataBlockWrite(SerializeParamEntityPayloadFIR(&buff[pointer + FileBlockSize], 0, SpeakerEQ->Fir), &pointer, buff, &blockcounter)) return 0; // error
  
  //Finalisation
  if(FileFinalise(SpeakerEQFileType, SpeakerEQFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint16_t FileFormatWriteSpeakerOut(SpeakerOutType* SpeakerOut, uint8_t* buff)
{        
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;

  //SpeakerOutConfig
  if(DataBlockWrite(FileFormatWriteSpeakerOutConfig(SpeakerOut->SpeakerOutConfig, &buff[pointer + FileBlockSize]), &pointer, buff, &blockcounter)) return 0; // error
  
  //Mute
  if(DataBlockWrite(SerializeParamEntityPayloadMute(&buff[pointer + FileBlockSize], 0, &SpeakerOut->Mute), &pointer, buff, &blockcounter)) return 0; // error
  
  //Polarity
  if(DataBlockWrite(SerializeParamEntityPayloadPolarity(&buff[pointer + FileBlockSize], 0, &SpeakerOut->Polarity), &pointer, buff, &blockcounter)) return 0; // error
  
  //Gain
  if(DataBlockWrite(SerializeParamEntityPayloadGain(&buff[pointer + FileBlockSize], 0, &SpeakerOut->Gain), &pointer, buff, &blockcounter)) return 0; // error
  
  //Delay
  if(DataBlockWrite(SerializeParamEntityPayloadDelay(&buff[pointer + FileBlockSize], 0, &SpeakerOut->Delay), &pointer, buff, &blockcounter)) return 0; // error
  
  //HP
  if(DataBlockWrite(SerializeParamEntityPayloadHP(&buff[pointer + FileBlockSize], 0, &SpeakerOut->FilterHighPass), &pointer, buff, &blockcounter)) return 0; // error
  
  //EQ[]
  for(counter = 0; counter < SPKOUT_EQ_QTY; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadEQ(&buff[pointer + FileBlockSize], counter, &SpeakerOut->FilterEQ[counter]), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //LP
  if(DataBlockWrite(SerializeParamEntityPayloadLP(&buff[pointer + FileBlockSize], 0, &SpeakerOut->FilterLowPass), &pointer, buff, &blockcounter)) return 0; // error
  
  //FIR
  if(DataBlockWrite(SerializeParamEntityPayloadFIR(&buff[pointer + FileBlockSize], 0, SpeakerOut->Fir), &pointer, buff, &blockcounter)) return 0; // error
  
  //Limiter RMS
  if(DataBlockWrite(SerializeParamEntityPayloadLimiterRMS(&buff[pointer + FileBlockSize], 0, &SpeakerOut->RMSLimiter), &pointer, buff, &blockcounter)) return 0; // error
  
  //Limiter Peak
  if(DataBlockWrite(SerializeParamEntityPayloadLimiterPeak(&buff[pointer + FileBlockSize], 0, &SpeakerOut->PeakLimiter), &pointer, buff, &blockcounter)) return 0; // error
  
  //Finalisation
  if(FileFinalise(SpeakerOutFileType, SpeakerOutFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint32_t FileFormatWriteSpeaker(SpeakerType* Speaker, uint8_t* buff)
{         
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //SpeakerConfig
  if(DataBlockWrite(FileFormatWriteSpeakerConfig(Speaker->Config, &buff[pointer + FileBlockSize]), &pointer, buff, &blockcounter)) return 0; // error

  //SpeakerEQ
  if(DataBlockWrite(FileFormatWriteSpeakerEQ(Speaker->EQ, &buff[pointer + FileBlockSize]), &pointer, buff, &blockcounter)) return 0; // error
  
  //SpeakerOUT
  for(counter = 0; counter < Speaker->Config->Ways; counter++)
  {
    if(DataBlockWrite(FileFormatWriteSpeakerOut(Speaker->OUT[counter], &buff[pointer + FileBlockSize]), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //Finalisation
  if(FileFinalise(SpeakerFileType, SpeakerFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint16_t FileFormatWritePresetParam(PresetParamType* PresetParam, uint8_t* buff)
{
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;
  
  //////////////////////////////////////////////////////////////////////////////

  //InputMode
  if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeInputMode, 0, (uint8_t)PresetParam->InputMode), &pointer, buff, &blockcounter)) return 0; // error
  
  //AnalogTrim
  if(DataBlockWrite(SerializeParamEntityPayloadInputTrim(&buff[pointer + FileBlockSize], ParamTypeAnalogInputTrim, 0, &PresetParam->AnalogTrim), &pointer, buff, &blockcounter)) return 0; // error
  
  //AESTrim
  if(DataBlockWrite(SerializeParamEntityPayloadInputTrim(&buff[pointer + FileBlockSize], ParamTypeAESInputTrim, 0, &PresetParam->AESTrim), &pointer, buff, &blockcounter)) return 0; // error
  
  //ModeXLR
  for(counter = 0; counter < IN_CH_QTY/2; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeModeXLR, counter, (uint8_t)PresetParam->ModeXLR[counter].State), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //Bridge
  for(counter = 0; counter < OUT_CH_QTY/2; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeBridge, counter, (uint8_t)PresetParam->Bridge[counter].State), &pointer, buff, &blockcounter)) return 0; // error
  }

  //InputSource
  for(counter = 0; counter < IN_CH_QTY; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadInputSource(&buff[pointer + FileBlockSize], counter, &PresetParam->InputSource[counter]), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //Matrix
  uint8_t index = 0;
  for(counter = 0; counter < OUT_CH_QTY; counter++)
  {
    for(index = 0; index < IN_CH_QTY; index++)
    {
      if(DataBlockWrite(SerializeParamEntityPayloadMatrix(&buff[pointer + FileBlockSize], (counter * 16) + index, &PresetParam->Matrix[index][counter]), &pointer, buff, &blockcounter)) return 0; // error
    }
  }
  
  //Finalisation
  if(FileFinalise(PresetParamFileType, PresetParamFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

//File Read

bool FileFormatReadUserEQ(UserEQType* UserEQ, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != UserEQFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != UserEQFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    error |= DeserializeUserEQParamEntityPayload(&buff[pointer + FileBlockSize], UserEQ);
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerConfig(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerConfigFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerConfigFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);
  
  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    error |= DeserializeSpeakerConfigParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerConfig);
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerOutConfig(SpeakerOutConfigType* SpeakerOutConfig, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutConfigFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutConfigFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    error |= DeserializeSpeakerOutConfigParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerOutConfig);
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerEQ(SpeakerEQType* SpeakerEQ, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerEQFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerEQFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    error |= DeserializeSpeakerEQParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerEQ);
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerOut(SpeakerOutType* SpeakerOut, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    if((uint16_t)((buff[pointer + FilePointerBlockSize] << 8) | buff[pointer + FilePointerBlockSize + 1]) == SpeakerOutConfigFileType) 
    {
      error |= FileFormatReadSpeakerOutConfig(SpeakerOut->SpeakerOutConfig, &buff[pointer + FilePointerBlockSize]);
    }
    else
    {
      error |= DeserializeSpeakerOutParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerOut);
    }
    
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error

  return error;
}

bool FileFormatReadSpeaker(SpeakerType* Speaker, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  uint8_t index = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileVer) return true; // error

  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    switch((uint16_t)((buff[pointer + FilePointerBlockSize] << 8) | buff[pointer + FilePointerBlockSize + 1]))
    {
      case SpeakerConfigFileType:
        error |= FileFormatReadSpeakerConfig(Speaker->Config, &buff[pointer + FilePointerBlockSize]);
        break;
      
      case SpeakerEQFileType:
        error |= FileFormatReadSpeakerEQ(Speaker->EQ, &buff[pointer + FilePointerBlockSize]);
        break;
        
      case SpeakerOutFileType:
        if(index < OUT_CH_QTY) 
        {
          error |= FileFormatReadSpeakerOut(Speaker->OUT[index++], &buff[pointer + FilePointerBlockSize]);
        }
        break;
    }
    
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error

  return error;
}

bool FileFormatReadPresetParam(PresetParamType* PresetParam, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != PresetParamFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != PresetParamFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    error |= DeserializePresetParamEntityPayload(&buff[pointer + FilePointerBlockSize], PresetParam);
    
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error

  return error;
}

bool FileFormatGetSpeakerInfo(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
{
  bool error = false;
  uint8_t blockcount = 0;
  uint32_t size = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileVer) return true; // error
  
  //Check CRC
  size = ByteArrayToUint32(buff, &counter);
  if(ByteArrayToUint32(&buff[size - 4], &pointer) != 0xFFFFFFFF)
  {
    if(CRC_Calc(buff, size) != 0) return true; // error
  }
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(counter = 0; counter < blockcount; counter++)
  {
    size = DataBlockRead(&buff[pointer]);
    if(size == 0) return true; // error
    
    if((uint16_t)((buff[pointer + FilePointerBlockSize] << 8) | buff[pointer + FilePointerBlockSize + 1]) == SpeakerConfigFileType) 
    {
      error |= FileFormatReadSpeakerConfig(SpeakerConfig, &buff[pointer + FilePointerBlockSize]);
    }
    
    pointer += FileBlockSize + size;
  }
  
  if(CheckEndFlag(&buff[pointer])) return true; // error

  return error;
}

uint32_t FileFormatGetFileSize(uint8_t* buff)
{
  uint32_t counter = 0;
  
  uint16_t filetype = ByteArrayToUint16(buff, &counter);
  uint16_t filever = ByteArrayToUint16(buff, &counter);
  uint32_t filesize = ByteArrayToUint32(buff, &counter);
  uint32_t fileCRC = ByteArrayToUint32(buff, &counter); // Check is correct
  
  switch(filetype)
  {
    case UserEQFileType:
    case SpeakerFileType:
    case SpeakerConfigFileType:
    case SpeakerEQFileType:
    case SpeakerOutConfigFileType:
    case SpeakerOutFileType:
    case SpeakerOutParamsFileType:
    case SpeakerOutFirFileType: 
    case PresetParamFileType: break;
    default: filesize = 0;
  }

  return filesize;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

//Valentyn 10.07.2025

static bool DataBlockWrite(uint32_t datasize, uint32_t *pointer, uint8_t* buff, uint16_t* blockcounter)
{
  if(datasize == 0) return 1; // error
  
  buff[(*pointer)++] = 'B';
  buff[(*pointer)++] = (datasize & 0x00FF0000) >> 16;
  buff[(*pointer)++] = (datasize & 0x0000FF00) >> 8;
  buff[(*pointer)++] = (datasize & 0x000000FF);

  (*pointer) += datasize;
  (*blockcounter)++;
  
  return 0; 
}

static uint32_t DataBlockRead(uint8_t* data)
{
  uint32_t size = 0;

  if(*data++ == 'B')
  {
    size = (*data++ << 16);
    size |= (*data++ << 8);
    size |= *data++;
  }
  
  return size;
}

static bool CheckEndFlag(uint8_t* data)
{
  if(*data == 'E') return 0;
  
  return 1;
}

static bool FileFinalise(uint16_t filetype, uint16_t fileversion, uint8_t* buff, uint32_t* pointer, uint16_t blockcounter)
{
  uint32_t counter = 0;
  uint32_t crc = 0;
  
  //Write "End" 
  buff[(*pointer)++] = 'E';
  
  //check Aligment by 16
  while(((*pointer + FileCRCSize) % 16) > 0)
  {
    buff[*pointer] = 0xFF;
    (*pointer)++;
  }
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(filetype, buff, &counter);
  Uint16ToByteArray(fileversion, buff, &counter);
  Uint32ToByteArray((*pointer) + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, *pointer);  
  CRC32ToByteArray(crc, buff, pointer);
  
  return 0; 
}


