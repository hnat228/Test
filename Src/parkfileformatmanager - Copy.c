
#include "parkfileformatmanager.h"
#include "dataconverter.h"
#include "crc.h"

static void NextPointerBlockWrite(uint32_t datasize, uint32_t *datapointer, uint32_t *pointer, uint8_t* buff, uint16_t* blockcounter);
static bool DataBlockWrite(uint32_t datasize, uint32_t *pointer, uint8_t* buff, uint16_t* blockcounter); //Valentyn 10.07.2025
static void EndPointerBlockWrite(uint32_t *pointer,  uint8_t* buff);
static uint32_t NextPointerBlockRead(uint32_t *pointer, uint8_t* buff);
static uint32_t DataBlockRead(uint8_t* data); //Valentyn 11.07.2025
static bool CheckEndFlag(uint8_t* data); //Valentyn 11.07.2025
//static uint32_t CRC_Calc(uint8_t* buff, uint32_t count);
static void SizeAlignment(uint8_t* buff, uint32_t* pointer);
static bool FileFinalise(uint16_t filetype, uint16_t fileversion, uint8_t* buff, uint32_t* pointer, uint16_t blockcounter); //Valentyn 10.07.2025
//FileHeader Header;
uint32_t crc = 0;

//File Write

uint16_t FileFormatWriteDeviceInfo(DeviceInfoType* DeviceInfo, uint8_t* buff)
{         
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Type
  datasize = SerializeParamEntityPayloadWord(&buff[datapointer], ParamTypeDeviceType, 0, DeviceInfo->Type);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Brand
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeDeviceBrand, 0, DeviceInfo->Brand, sizeof(DeviceInfo->Brand));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Series
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeDeviceSeries, 0, DeviceInfo->Series, sizeof(DeviceInfo->Series));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Model
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeDeviceModel, 0, DeviceInfo->Model, sizeof(DeviceInfo->Model));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Name
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeDeviceName, 0, DeviceInfo->Name, sizeof(DeviceInfo->Name));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //FW
  datasize = SerializeParamEntityPayloadArray(&buff[datapointer], ParamTypeDeviceFW, 0, DeviceInfo->FW, sizeof(DeviceInfo->FW));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //HW
  datasize = SerializeParamEntityPayloadArray(&buff[datapointer], ParamTypeDeviceHW, 0, DeviceInfo->HW, sizeof(DeviceInfo->HW));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //SN
  datasize = SerializeParamEntityPayloadArray(&buff[datapointer], ParamTypeDeviceSN, 0, DeviceInfo->SN, sizeof(DeviceInfo->SN));
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(DeviceInfoFileType, buff, &counter);
  Uint16ToByteArray(DeviceInfoFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint16_t FileFormatWriteDeviceInfo_V1(DeviceInfoType* DeviceInfo, uint8_t* buff)
{         
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Type
  if(DataBlockWrite(SerializeParamEntityPayloadWord(&buff[pointer + FileBlockSize], ParamTypeDeviceType, 0, DeviceInfo->Type), &pointer, buff, &blockcounter)) return 0; // error
  
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
  uint16_t filetype = 0;       
  uint16_t fileversion = 0;          
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  filetype = UserEQFileType;         //File type definition     
  fileversion = UserEQFileVer;       //File version definition
  
  //Mute
  datasize = SerializeParamEntityPayloadMute(&buff[datapointer], 0, &UserEQ->Mute);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Polarity
  datasize = SerializeParamEntityPayloadPolarity(&buff[datapointer], 0, &UserEQ->Polarity);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Gain
  datasize = SerializeParamEntityPayloadGain(&buff[datapointer], 0, &UserEQ->Gain);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Delay
  datasize = SerializeParamEntityPayloadDelay(&buff[datapointer], 0, &UserEQ->Delay);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //EQ[]
  for(counter = 0; counter < USREQ_EQ_QTY; counter++)
  {
    datasize = SerializeParamEntityPayloadEQ(&buff[datapointer], counter, &UserEQ->FilterEQ[counter]);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(filetype, buff, &counter);
  Uint16ToByteArray(fileversion, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

//Valentyn 10.07.2025
uint16_t FileFormatWriteUserEQ_V1(UserEQType* UserEQ, uint8_t* buff)
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
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //BrandName
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeSpeakerBrand, 0, SpeakerConfig->BrandName, SpkConfxNameSize);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //SeriesName
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeSpeakerSeries, 0, SpeakerConfig->SeriesName, SpkConfxNameSize);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //ModelName
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeSpeakerModel, 0, SpeakerConfig->ModelName, SpkConfxNameSize);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //VariationName
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeSpeakerVariation, 0, SpeakerConfig->VariationName, SpkConfxNameSize);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Notes
  datasize = SerializeParamEntityPayloadString(&buff[datapointer], ParamTypeSpeakerNotes, 0, SpeakerConfig->Notes, SpkConfNoteSize);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;

  //SpeakerType
  datasize = SerializeParamEntityPayloadByte(&buff[datapointer], ParamTypeSpeakerType, 0, SpeakerConfig->SpeakerType);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Ways
  datasize = SerializeParamEntityPayloadByte(&buff[datapointer], ParamTypeSpeakerWays, 0, SpeakerConfig->Ways);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //HizHPF
  datasize = SerializeParamEntityPayloadHizHP(&buff[datapointer], 0, &SpeakerConfig->HizHPF);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(SpeakerConfigFileType, buff, &counter);
  Uint16ToByteArray(SpeakerConfigFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint16_t FileFormatWriteSpeakerConfig_V1(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
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
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //OUTName
  datasize = SerializeParamEntityPayloadArray(&buff[datapointer], ParamTypeOUTName, 0, SpeakerOutConfig->OutName, SpkOutConfOutNameSize);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //OutIndex
  datasize = SerializeParamEntityPayloadByte(&buff[datapointer], ParamTypeIndex, 0, SpeakerOutConfig->OutIndex);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //WayPower
  datasize = SerializeParamEntityPayloadWayPower(&buff[datapointer], 0, &SpeakerOutConfig->WayPower);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //WayHizPower[]
  for(counter = 0; counter < SPKCONF_HIzPOW_QTY; counter++)
  {
    datasize = SerializeParamEntityPayloadWayHizPower(&buff[datapointer], counter, &SpeakerOutConfig->WayHizPower[counter]);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(SpeakerOutConfigFileType, buff, &counter);
  Uint16ToByteArray(SpeakerOutConfigFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint16_t FileFormatWriteSpeakerOutConfig_V1(SpeakerOutConfigType* SpeakerOutConfig, uint8_t* buff)
{         
  uint8_t counter = 0;
  uint16_t blockcounter = 0;
  uint32_t pointer = FileHeaderSize;

  //OUTName
  if(DataBlockWrite(SerializeParamEntityPayloadArray(&buff[pointer + FileBlockSize], ParamTypeOUTName, 0, SpeakerOutConfig->OutName, SpkOutConfOutNameSize), &pointer, buff, &blockcounter)) return 0; // error

  //OutIndex
  if(DataBlockWrite(SerializeParamEntityPayloadByte(&buff[pointer + FileBlockSize], ParamTypeIndex, 0, SpeakerOutConfig->OutIndex), &pointer, buff, &blockcounter)) return 0; // error

  //WayPower
  if(DataBlockWrite(SerializeParamEntityPayloadWayPower(&buff[pointer + FileBlockSize], 0, &SpeakerOutConfig->WayPower), &pointer, buff, &blockcounter)) return 0; // error

  //WayHizPower[]
  for(counter = 0; counter < SPKCONF_HIzPOW_QTY; counter++)
  {
    if(DataBlockWrite(SerializeParamEntityPayloadWayHizPower(&buff[pointer + FileBlockSize], counter, &SpeakerOutConfig->WayHizPower[counter]), &pointer, buff, &blockcounter)) return 0; // error
  }
  
  //Finalisation
  if(FileFinalise(SpeakerOutConfigFileType, SpeakerOutConfigFileVer, buff, &pointer, blockcounter)) return 0; // error

  return pointer;
}

uint16_t FileFormatWriteSpeakerEQ(SpeakerEQType* SpeakerEQ, uint8_t* buff)
{          
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////

  //HP
  datasize = SerializeParamEntityPayloadHP(&buff[datapointer], 0, &SpeakerEQ->FilterHighPass);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //EQ[]
  for(counter = 0; counter < SPKEQ_EQ_QTY; counter++)
  {
    datasize = SerializeParamEntityPayloadEQ(&buff[datapointer], counter, &SpeakerEQ->FilterEQ[counter]);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //LP
  datasize = SerializeParamEntityPayloadLP(&buff[datapointer], 0, &SpeakerEQ->FilterLowPass);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //FIR
  datasize = SerializeParamEntityPayloadFIR(&buff[datapointer], 0, SpeakerEQ->Fir);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(SpeakerEQFileType, buff, &counter);
  Uint16ToByteArray(SpeakerEQFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint16_t FileFormatWriteSpeakerEQ_V1(SpeakerEQType* SpeakerEQ, uint8_t* buff)
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
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////

  //SpeakerOutConfig
  datasize = FileFormatWriteSpeakerOutConfig(SpeakerOut->SpeakerOutConfig, &buff[datapointer]);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Mute
  datasize = SerializeParamEntityPayloadMute(&buff[datapointer], 0, &SpeakerOut->Mute);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Polarity
  datasize = SerializeParamEntityPayloadPolarity(&buff[datapointer], 0, &SpeakerOut->Polarity);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Gain
  datasize = SerializeParamEntityPayloadGain(&buff[datapointer], 0, &SpeakerOut->Gain);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Delay
  datasize = SerializeParamEntityPayloadDelay(&buff[datapointer], 0, &SpeakerOut->Delay);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //HP
  datasize = SerializeParamEntityPayloadHP(&buff[datapointer], 0, &SpeakerOut->FilterHighPass);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //EQ[]
  for(counter = 0; counter < SPKOUT_EQ_QTY; counter++)
  {
    datasize = SerializeParamEntityPayloadEQ(&buff[datapointer], counter, &SpeakerOut->FilterEQ[counter]);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //LP
  datasize = SerializeParamEntityPayloadLP(&buff[datapointer], 0, &SpeakerOut->FilterLowPass);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //FIR
  datasize = SerializeParamEntityPayloadFIR(&buff[datapointer], 0, SpeakerOut->Fir);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Limiter RMS
  datasize = SerializeParamEntityPayloadLimiterRMS(&buff[datapointer], 0, &SpeakerOut->RMSLimiter);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //Limiter Peak
  datasize = SerializeParamEntityPayloadLimiterPeak(&buff[datapointer], 0, &SpeakerOut->PeakLimiter);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(SpeakerOutFileType, buff, &counter);
  Uint16ToByteArray(SpeakerOutFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint16_t FileFormatWriteSpeakerOut_V1(SpeakerOutType* SpeakerOut, uint8_t* buff)
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
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////
  
  //SpeakerConfig
  datasize = FileFormatWriteSpeakerConfig(Speaker->Config, &buff[datapointer]);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;

  //SpeakerEQ
  datasize = FileFormatWriteSpeakerEQ(Speaker->EQ, &buff[datapointer]);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //SpeakerOUT
  for(counter = 0; counter < Speaker->Config->Ways; counter++)
  {
    datasize = FileFormatWriteSpeakerOut(Speaker->OUT[counter], &buff[datapointer]);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(SpeakerFileType, buff, &counter);
  Uint16ToByteArray(SpeakerFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint32_t FileFormatWriteSpeaker_V1(SpeakerType* Speaker, uint8_t* buff)
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
  uint16_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t datasize = 0;
  uint32_t pointer = FileHeaderSize;
  uint32_t datapointer = FileHeaderSize + FilePointerBlockSize;
  
  //////////////////////////////////////////////////////////////////////////////

  //InputMode
  datasize = SerializeParamEntityPayloadByte(&buff[datapointer], ParamTypeInputMode, 0, (uint8_t)PresetParam->InputMode);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //AnalogTrim
  datasize = SerializeParamEntityPayloadInputTrim(&buff[datapointer], ParamTypeAnalogInputTrim, 0, &PresetParam->AnalogTrim);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //AESTrim
  datasize = SerializeParamEntityPayloadInputTrim(&buff[datapointer], ParamTypeAESInputTrim, 0, &PresetParam->AESTrim);
  NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
  if(datasize == 0) return 0;
  
  //ModeXLR
  for(counter = 0; counter < IN_CH_QTY/2; counter++)
  {
    datasize = SerializeParamEntityPayloadByte(&buff[datapointer], ParamTypeModeXLR, counter, (uint8_t)PresetParam->ModeXLR[counter].State);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //Bridge
  for(counter = 0; counter < OUT_CH_QTY/2; counter++)
  {
    datasize = SerializeParamEntityPayloadByte(&buff[datapointer], ParamTypeBridge, counter, (uint8_t)PresetParam->Bridge[counter].State);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }

  //InputSource
  for(counter = 0; counter < IN_CH_QTY; counter++)
  {
    datasize = SerializeParamEntityPayloadInputSource(&buff[datapointer], counter, &PresetParam->InputSource[counter]);
    NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
    if(datasize == 0) return 0;
  }
  
  //Matrix
  uint8_t indexOut = 0;
  for(counter = 0; counter < IN_CH_QTY; counter++)
  {
    for(indexOut = 0; indexOut < OUT_CH_QTY; indexOut++)
    {
      datasize = SerializeParamEntityPayloadMatrix(&buff[datapointer], (counter * 10) + indexOut, &PresetParam->Matrix[counter][indexOut]);
      NextPointerBlockWrite(datasize, &datapointer, &pointer, buff, &blockcounter);
      if(datasize == 0) return 0;
    }
  }
  
  //////////////////////////////////////////////////////////////////////////////
  
  //Write End Block
  EndPointerBlockWrite(&pointer, buff);
    
  //Check Buff Aligment by 4
  SizeAlignment(buff, &pointer);
  
  //Write Header
  counter = 0;
  Uint16ToByteArray(PresetParamFileType, buff, &counter);
  Uint16ToByteArray(PresetParamFileVer, buff, &counter);
  Uint32ToByteArray(pointer + FileCRCSize, buff, &counter);
  Uint16ToByteArray(blockcounter, buff, &counter);
  Uint16ToByteArray(FileHeaderSize, buff, &counter);
  
  //Write CRC
  crc = CRC_Calc(buff, pointer);  
  CRC32ToByteArray(crc, buff, &pointer);
  if(crc == 0xFFFFFFFF) return 0;
  
  return pointer;
}

uint16_t FileFormatWritePresetParam_V1(PresetParamType* PresetParam, uint8_t* buff)
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
  uint8_t indexOut = 0;
  for(counter = 0; counter < IN_CH_QTY; counter++)
  {
    for(indexOut = 0; indexOut < OUT_CH_QTY; indexOut++)
    {
      if(DataBlockWrite(SerializeParamEntityPayloadMatrix(&buff[pointer + FileBlockSize], (counter * 10) + indexOut, &PresetParam->Matrix[counter][indexOut]), &pointer, buff, &blockcounter)) return 0; // error
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != UserEQFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != UserEQFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    error |= DeserializeUserEQParamEntityPayload(&buff[pointer + FilePointerBlockSize], UserEQ);
    
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadUserEQ_V1(UserEQType* UserEQ, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerConfigFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerConfigFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    error |= DeserializeSpeakerConfigParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerConfig);
    
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerConfig_V1(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutConfigFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutConfigFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    error |= DeserializeSpeakerOutConfigParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerOutConfig);
    
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerOutConfig_V1(SpeakerOutConfigType* SpeakerOutConfig, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerEQFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerEQFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    error |= DeserializeSpeakerEQParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerEQ);
    
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerEQ_V1(SpeakerEQType* SpeakerEQ, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerOutFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    if((uint16_t)((buff[pointer + FilePointerBlockSize] << 8) | buff[pointer + FilePointerBlockSize + 1]) == SpeakerOutConfigFileType) 
    {
      error |= FileFormatReadSpeakerOutConfig(SpeakerOut->SpeakerOutConfig, &buff[pointer + FilePointerBlockSize]);
    }
    else
    {
      error |= DeserializeSpeakerOutParamEntityPayload(&buff[pointer + FilePointerBlockSize], SpeakerOut);
    }
      
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadSpeakerOut_V1(SpeakerOutType* SpeakerOut, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  counter = 0;
  
  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    switch((uint16_t)((buff[pointer + FilePointerBlockSize] << 8) | buff[pointer + FilePointerBlockSize + 1]))
      {
        case SpeakerConfigFileType:
          error |= FileFormatReadSpeakerConfig(Speaker->Config, &buff[pointer + FilePointerBlockSize]);
          break;
        
        case SpeakerEQFileType:
          error |= FileFormatReadSpeakerEQ(Speaker->EQ, &buff[pointer + FilePointerBlockSize]);
          break;
          
        case SpeakerOutFileType:
          if(counter < OUT_CH_QTY) 
          {
            error |= FileFormatReadSpeakerOut(Speaker->OUT[counter++], &buff[pointer + FilePointerBlockSize]);
          }
          break;
      }

    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadSpeaker_V1(SpeakerType* Speaker, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != PresetParamFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != PresetParamFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    error |= DeserializePresetParamEntityPayload(&buff[pointer + FilePointerBlockSize], PresetParam);
    
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatReadPresetParam_V1(PresetParamType* PresetParam, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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
  uint8_t blockcounter = 0;
  uint32_t counter = 0;
  uint32_t pointer = 0;
  
  //Header Read
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileType) return true; // error
  if(ByteArrayToUint16(buff, &counter) != SpeakerFileVer) return true; // error
  
  //Check CRC
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
  blockcount = ByteArrayToUint16(buff, &counter);
  pointer = ByteArrayToUint16(buff, &counter);

  for(blockcounter = 0; blockcounter < blockcount; blockcounter++)
  {
    if((uint16_t)((buff[pointer + FilePointerBlockSize] << 8) | buff[pointer + FilePointerBlockSize + 1]) == SpeakerConfigFileType) 
    {
      error |= FileFormatReadSpeakerConfig(SpeakerConfig, &buff[pointer + FilePointerBlockSize]);
    }
    
    pointer = NextPointerBlockRead(&pointer, buff);
    if(pointer == 0) return true; // error
  }
  
  if(NextPointerBlockRead(&pointer, buff) != 0x00FFFFFF) return true; // error
  
  return error;
}

bool FileFormatGetSpeakerInfo_V1(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
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
  if(CRC_Calc(buff, ByteArrayToUint32(buff, &counter)) != 0) return true; // error
  
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

static void NextPointerBlockWrite(uint32_t datasize, uint32_t *datapointer, uint32_t *pointer, uint8_t* buff, uint16_t* blockcounter)
{
  uint32_t nextpointer = (*pointer) + FilePointerBlockSize + datasize;
    
  buff[(*pointer)++] = 'P';
  buff[(*pointer)++] = (nextpointer & 0x00FF0000) >> 16;
  buff[(*pointer)++] = (nextpointer & 0x0000FF00) >> 8;
  buff[(*pointer)++] = (nextpointer & 0x000000FF);

  (*pointer) = nextpointer;
  (*datapointer) = nextpointer + FilePointerBlockSize;
  (*blockcounter)++;
}

static void EndPointerBlockWrite(uint32_t *pointer, uint8_t* buff)
{
  buff[(*pointer)++] = 'E';
  buff[(*pointer)++] = 0xFF;
  buff[(*pointer)++] = 0xFF;
  buff[(*pointer)++] = 0xFF;
}

static uint32_t NextPointerBlockRead(uint32_t *pointer, uint8_t* buff)
{
  uint32_t nextpointer = 0;

  switch(buff[(*pointer)++])
  {
    case 'P':
    case 'E':
    {
      nextpointer |= (buff[(*pointer)++] << 16);
      nextpointer |= (buff[(*pointer)++] << 8);
      nextpointer |= buff[(*pointer)++];
    }
  }
  
  return nextpointer;
}

static void SizeAlignment(uint8_t* buff, uint32_t* pointer)
{
  while((*pointer % 4) > 0)
  {
    buff[*pointer] = 0xFF;
    (*pointer)++;
  }
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


