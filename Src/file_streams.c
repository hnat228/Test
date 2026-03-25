
#include "file_streams.h"
#include "i2c_eeprom.h"
#include "amp_application.h"

#include "array.h"
#include "flash.h"
#include "parkfileformatmanager.h"
#include "dataconverter.h"
#include "crc.h"

//#pragma location = ".sram"
uint8_t FILE_BUFF[FILE_BUFF_SIZE];

//#pragma location = ".sram"
uint8_t FLASH_INFO_BUFF[SIZE_256B];

//------------------------------------------------------------------------------
void Search_Valid_Data_Addr(BlockStatusType* BlockStatus, uint32_t size)
{
  uint8_t i;
  uint8_t buff[2];
  uint32_t addr;
  uint16_t filetype;
  
  //Find last valid data
  for(i = 0; i < FlashBlockQty; i++)
  {
    addr = BlockStatus->FlashAddress + (size * i);
    Flash_Read(addr, buff, 2);                                          //Read Header
    
    filetype = (uint16_t)buff[0] << 8;
    filetype |= (uint16_t)buff[1];
    
    if(filetype == 0xFFFF)
    {
      if(i > 0) 
      {
        BlockStatus->CycleCounter = i -1; break;
      }
      else if(i == 0) 
      {
        BlockStatus->CycleCounter = FlashBlockQty -1; break;
      }
    }
  }
  
  BlockStatus->CurrIndex = BlockStatus->CycleCounter % FlashBlockQty;                                   //Define block in flash  
  
  if(BlockStatus->CurrIndex < FlashBlockQty -1) BlockStatus->NextIndex = BlockStatus->CurrIndex +1;     //
  else BlockStatus->NextIndex = 0;
}

uint32_t File_Header_Flash_Read(BlockStatusType* BlockStatus, uint32_t SizeBlock)
{
  uint32_t size;
  uint32_t addr = BlockStatus->FlashAddress + (SizeBlock * BlockStatus->CurrIndex);
  
  Flash_Read(addr, FILE_BUFF, FileHeaderSize);                                          //Read Header
  size = FileFormatGetFileSize(FILE_BUFF);                                              //Get file size
  
  return size;
}

//------------------------------------------------------------------------------

//PRESET------------------------------------------------------------------------
//Preset Read time -> 30ms
void Preset_Flash_Read(PresetParamType* Preset, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  bool error = false;
  
  Eeprom_Param_Config(&Preset->MemoryConfig, READ_EEPROM);                                              //Get number of block in flash
  
  size = File_Header_Flash_Read(&Preset->MemoryConfig, SIZE_4kB);                                       //Read header file from flash and get size of file  
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.CurrIndex);               //Define file addr in flash; 
    
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);     //Read file
    error = FileFormatReadPresetParam(Preset, Buff);                                               //Deserialize file to data struct
    
    //If loaded data was written incorect
    if(error != false) error = Preset_Error_Handling(Preset, Buff);
  }
  else error = Preset_Error_Handling(Preset, Buff);

  //Clear next block if it need
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.NextIndex);

  Flash_Read(addr, FILE_BUFF, SIZE_4kB);
  if(CRC_Calc_4kB_Free(FILE_BUFF)) Flash_Erase_4kB(addr); 
}

//Preset Read Backup
void Preset_Flash_Read_Backup(PresetParamType* Preset, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr = 0;
  bool error = false;

  Search_Valid_Data_Addr(&Preset->MemoryConfig, SIZE_4kB);                              //Search addr valid data in flash 
  
  size = File_Header_Flash_Read(&Preset->MemoryConfig, SIZE_4kB);                                       //Read header file from flash and get size of file  
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.CurrIndex);               //Define file addr in flash; 
  
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);
    error = FileFormatReadPresetParam(Preset, Buff);
    
    //If loaded data was written incorect
    if(error != false) error = Preset_Error_Handling(Preset, Buff);
  }
  else error = Preset_Error_Handling(Preset, Buff);
 
  //Clear next block if it need
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.NextIndex);
  Flash_Read(addr, FILE_BUFF, SIZE_4kB);
  if(CRC_Calc_4kB_Free(FILE_BUFF)) Flash_Erase_4kB(addr);
}

void Preset_Flash_Read_Previous_Block(PresetParamType* Preset, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr;
  
  if(Preset->MemoryConfig.CurrIndex > 0)
  {
    Preset->MemoryConfig.CurrIndex--;
    Preset->MemoryConfig.CycleCounter = Preset->MemoryConfig.CurrIndex;
    Preset->MemoryConfig.NextIndex = Preset->MemoryConfig.CurrIndex +1;
  }
  else 
  {
    Preset->MemoryConfig.CurrIndex = FlashBlockQty -1;
    Preset->MemoryConfig.CycleCounter = Preset->MemoryConfig.CurrIndex;
    Preset->MemoryConfig.NextIndex = 0;
  }
  
  size = File_Header_Flash_Read(&Preset->MemoryConfig, SIZE_4kB);                                              //Get file size
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.CurrIndex);
  
  if(size != 0) 
  {
    Flash_Read(addr, Buff, size);
  }
}

bool Preset_Error_Handling(PresetParamType* Preset, uint8_t* Buff)
{
  uint8_t error = false;
  uint32_t addr = 0;
  
  Preset_Flash_Read_Previous_Block(Preset, Buff);
  error = FileFormatReadPresetParam(Preset, Buff); 
  
  if(error != false) 
  {
    addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.NextIndex);
    Flash_Erase_4kB(addr);
    Preset_Default_Param(Preset);
    Preset_Flash_Write(Preset, Buff);
    FileFormatReadPresetParam(Preset, Buff);
  }
  
  return error;
}

//Preset Write time -> 13ms
void Preset_Flash_Write(PresetParamType* Preset, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  
  Preset->MemoryConfig.CycleCounter++;
  Preset->MemoryConfig.CurrIndex = Preset->MemoryConfig.CycleCounter % FlashBlockQty;
  
  if(Preset->MemoryConfig.CurrIndex < FlashBlockQty -1) Preset->MemoryConfig.NextIndex = Preset->MemoryConfig.CurrIndex +1;
  else Preset->MemoryConfig.NextIndex = 0;
  
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.CurrIndex);
  size = FileFormatWritePresetParam(Preset, Buff); 
  
  Flash_Data_Store(addr, Buff, size);      //1.7ms                                        //Store data to flash
  Eeprom_Param_Config(&Preset->MemoryConfig, WRITE_EEPROM);
  
  //Erase Next Preset Block
  addr = Preset->MemoryConfig.FlashAddress + (SIZE_4kB * Preset->MemoryConfig.NextIndex);
  Flash_Erase_4kB(addr);
}
//------------------------------------------------------------------------------

//Preset Lib Read
void Preset_Lib_Init(PresetLibType* PresetLib)
{
  uint32_t addr;
  uint8_t i;

  for(i = 0; i < LIB_PRESET_QTY; i++)
  {
    addr = FLASH_PRESET_LIB_ADDRESS(i); 
    arrinit(PresetLib->Name[i], LCD_PRESET_NAME_SIZE, '\0');
    
    Flash_Read(addr, PresetLib->Name[i], LCD_PRESET_NAME_SIZE);
    
    if(PresetLib->Name[i][0] == 0xFF)
    {
      arrinit(PresetLib->Name[i], LCD_PRESET_NAME_SIZE, '\0');
      strcopy(PresetLib->Name[i], "EMPTY");
    }
  } 
}
//------------------------------------------------------------------------------

//USER_EQ-----------------------------------------------------------------------
//User_EQ Read
void UserEQ_Flash_Read(UserEQType* UserEQ, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  bool error = false;
  
  Eeprom_Param_Config(&UserEQ->MemoryConfig, READ_EEPROM);                                              //Get number of block in flash
  
  size = File_Header_Flash_Read(&UserEQ->MemoryConfig, SIZE_4kB);                                       //Read header file from flash and get size of file  
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.CurrIndex);               //Define file addr in flash; 
    
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);                                                                  //Read file
    error = FileFormatReadUserEQ(UserEQ, Buff);                                                //Deserialize file to data struct
    
    //If loaded data was written incorect
    if(error != false) UserEQ_Error_Handling(UserEQ, Buff);
  }
  else error = UserEQ_Error_Handling(UserEQ, Buff);

  //Clear next block if it need
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.NextIndex);
  Flash_Read(addr, FILE_BUFF, SIZE_4kB);
  if(CRC_Calc_4kB_Free(FILE_BUFF)) Flash_Erase_4kB(addr); 
}

void UserEQ_Flash_Read_Backup(UserEQType* UserEQ, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr = 0;
  bool error = false;

  Search_Valid_Data_Addr(&UserEQ->MemoryConfig, SIZE_4kB);                              //Search addr valid data in flash 
  
  size = File_Header_Flash_Read(&UserEQ->MemoryConfig, SIZE_4kB);                                       //Read header file from flash and get size of file  
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.CurrIndex);               //Define file addr in flash; 
  
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);
    error = FileFormatReadUserEQ(UserEQ, Buff);
    
    //If loaded data was written incorect
    if(error != false) error = UserEQ_Error_Handling(UserEQ, Buff);
  }
  else error = UserEQ_Error_Handling(UserEQ, Buff);
 
  //Clear next block if it need
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.NextIndex);
  Flash_Read(addr, FILE_BUFF, SIZE_4kB);
  if(CRC_Calc_4kB_Free(FILE_BUFF)) Flash_Erase_4kB(addr);
}

void UserEQ_Flash_Read_Previous_Block(UserEQType* UserEQ, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr;
  
  if(UserEQ->MemoryConfig.CurrIndex > 0)
  {
    UserEQ->MemoryConfig.CurrIndex--;
    UserEQ->MemoryConfig.CycleCounter = UserEQ->MemoryConfig.CurrIndex;
    UserEQ->MemoryConfig.NextIndex = UserEQ->MemoryConfig.CurrIndex +1;
  }
  else 
  {
    UserEQ->MemoryConfig.CurrIndex = FlashBlockQty -1;
    UserEQ->MemoryConfig.CycleCounter = UserEQ->MemoryConfig.CurrIndex;
    UserEQ->MemoryConfig.NextIndex = 0;
  }
  
  size = File_Header_Flash_Read(&UserEQ->MemoryConfig, SIZE_4kB);                                              //Get file size
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.CurrIndex);
  
  if(size != 0) 
  {
    Flash_Read(addr, Buff, size);
  }
}

bool UserEQ_Error_Handling(UserEQType* UserEQ, uint8_t* Buff)
{
  uint8_t error = false;
  uint32_t addr = 0;
  
  UserEQ_Flash_Read_Previous_Block(UserEQ, Buff);
  error = FileFormatReadUserEQ(UserEQ, Buff); 
  
  if(error != false) 
  {
    addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.NextIndex);
    Flash_Erase_4kB(addr);
    UserEQ_Default_Param(UserEQ);
    UserEQ_Flash_Write(UserEQ, Buff);
    FileFormatReadUserEQ(UserEQ, Buff);
  }
  
  return error;
}

//UserEQ Write
void UserEQ_Flash_Write(UserEQType* UserEQ, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  
  UserEQ->MemoryConfig.CycleCounter++;
  UserEQ->MemoryConfig.CurrIndex = UserEQ->MemoryConfig.CycleCounter % FlashBlockQty;
  
  if(UserEQ->MemoryConfig.CurrIndex < FlashBlockQty -1) UserEQ->MemoryConfig.NextIndex = UserEQ->MemoryConfig.CurrIndex +1;
  else UserEQ->MemoryConfig.NextIndex = 0;
  
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.CurrIndex);
  size = FileFormatWriteUserEQ(UserEQ, Buff); 

  Flash_Data_Store(addr, Buff, size);                                              //Store data to flash
  
  Eeprom_Param_Config(&UserEQ->MemoryConfig, WRITE_EEPROM);
  
  //Erase Next Preset Block
  addr = UserEQ->MemoryConfig.FlashAddress + (SIZE_4kB * UserEQ->MemoryConfig.NextIndex);
  Flash_Erase_4kB(addr);                                                             
}

//

//SPEAKER-----------------------------------------------------------------------

//Speaker Config-------------------------------------------------------------------
void Speaker_Config_Flash_Read(SpeakerConfigType* SpeakerConfig, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  bool error = false;
  
  Eeprom_Param_Config(&SpeakerConfig->MemoryConfig, READ_EEPROM);                                              //Get number of block in flash
  
  size = File_Header_Flash_Read(&SpeakerConfig->MemoryConfig, SIZE_4kB);                                       //Read header file from flash and get size of file  
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.CurrIndex);               //Define file addr in flash; 
    
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);                                                                  //Read file
    error = FileFormatReadSpeakerConfig(SpeakerConfig, Buff);                                               //Deserialize file to data struct
    
    //If loaded data was written incorect
    if(error != false) error = Speaker_Config_Error_Handling(SpeakerConfig, Buff);
  }
  else error = Speaker_Config_Error_Handling(SpeakerConfig, Buff);

  //Clear next block if it need
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.NextIndex);
  Flash_Read(addr, Buff, SIZE_4kB);
  if(CRC_Calc_4kB_Free(FILE_BUFF)) Flash_Erase_4kB(addr); 
}

void Speaker_Config_Read_Previous_Block(SpeakerConfigType* SpeakerConfig, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr;
  
  if(SpeakerConfig->MemoryConfig.CurrIndex > 0)
  {
    SpeakerConfig->MemoryConfig.CurrIndex--;
    SpeakerConfig->MemoryConfig.CycleCounter = SpeakerConfig->MemoryConfig.CurrIndex;
    SpeakerConfig->MemoryConfig.NextIndex = SpeakerConfig->MemoryConfig.CurrIndex +1;
  }
  else 
  {
    SpeakerConfig->MemoryConfig.CurrIndex = FlashBlockQty -1;
    SpeakerConfig->MemoryConfig.CycleCounter = SpeakerConfig->MemoryConfig.CurrIndex;
    SpeakerConfig->MemoryConfig.NextIndex = 0;
  }
  
  size = File_Header_Flash_Read(&SpeakerConfig->MemoryConfig, SIZE_4kB);                                              //Get file size
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.CurrIndex);
  
  if(size != 0) 
  {
    Flash_Read(addr, Buff, size);
  }
}

bool Speaker_Config_Error_Handling(SpeakerConfigType* SpeakerConfig, uint8_t* Buff)
{
  uint8_t error = false;
  uint32_t addr = 0;
  
  Speaker_Config_Read_Previous_Block(SpeakerConfig, Buff);
  error = FileFormatReadSpeakerConfig(SpeakerConfig, Buff);
  
  if(error != false) 
  {
    addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.NextIndex);
    Flash_Erase_4kB(addr);
    Default_Speaker_Config_Param(SpeakerConfig);
    Speaker_Config_Flash_Write(SpeakerConfig, Buff);
    FileFormatReadSpeakerConfig(SpeakerConfig, Buff);
  }
  
  return error;
}


void Speaker_Config_Flash_Read_Backup(SpeakerConfigType* SpeakerConfig, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr = 0;
  bool error = false;

  Search_Valid_Data_Addr(&SpeakerConfig->MemoryConfig, SIZE_4kB);                              //Search addr valid data in flash 
  
  size = File_Header_Flash_Read(&SpeakerConfig->MemoryConfig, SIZE_4kB);                                       //Read header file from flash and get size of file  
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.CurrIndex);               //Define file addr in flash; 
  
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);
    error = FileFormatReadSpeakerConfig(SpeakerConfig, Buff);
    
    //If loaded data was written incorect
    if(error != false) error = Speaker_Config_Error_Handling(SpeakerConfig, Buff);
  }
  else error = Speaker_Config_Error_Handling(SpeakerConfig, Buff);
 
  //Clear next block if it need
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.NextIndex);
  Flash_Read(addr, Buff, SIZE_4kB);
  if(CRC_Calc_4kB_Free(FILE_BUFF)) Flash_Erase_4kB(addr);
}
//Speaker Config Write
void Speaker_Config_Flash_Write(SpeakerConfigType* SpeakerConfig, uint8_t* buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  
  SpeakerConfig->MemoryConfig.CycleCounter++;
  SpeakerConfig->MemoryConfig.CurrIndex = SpeakerConfig->MemoryConfig.CycleCounter % FlashBlockQty;
  
  if(SpeakerConfig->MemoryConfig.CurrIndex < FlashBlockQty -1) SpeakerConfig->MemoryConfig.NextIndex = SpeakerConfig->MemoryConfig.CurrIndex +1;
  else SpeakerConfig->MemoryConfig.NextIndex = 0;
  
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.CurrIndex);
  size = FileFormatWriteSpeakerConfig(SpeakerConfig, buff); 

  Flash_Data_Store(addr, buff, size);                                              //Store data to flash
  
  Eeprom_Param_Config(&SpeakerConfig->MemoryConfig, WRITE_EEPROM);
  
  //Erase Next Preset Block
  addr = SpeakerConfig->MemoryConfig.FlashAddress + (SIZE_4kB * SpeakerConfig->MemoryConfig.NextIndex);
  Flash_Erase_4kB(addr);                                                             
}

//Speaker EQ Flash Read
void SpeakerEQ_Flash_Read(SpeakerEQType* SpeakerEQ, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  bool error = false;
  
  Eeprom_Param_Config(&SpeakerEQ->MemoryConfig, READ_EEPROM);                                                   //Get number of block in flash
  
  size = File_Header_Flash_Read(&SpeakerEQ->MemoryConfig, SIZE_32kB);                                            //Read header file from flash and get size of file  
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.CurrIndex);                //Define file addr in flash; 
    
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);                                                                  //Read file
    error = FileFormatReadSpeakerEQ(SpeakerEQ, Buff);                                               //Deserialize file to data struct
    
    //If loaded data was written incorect
    if(error != false) error = SpeakerEQ_Error_Handling(SpeakerEQ, Buff);
  }
  else error = SpeakerEQ_Error_Handling(SpeakerEQ, Buff);

  //Clear next block if it need
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.NextIndex);
  Flash_Read(addr, Buff, SIZE_32kB);
  if(CRC_Calc_32kB_Free(FILE_BUFF)) Flash_Erase_32kB(addr); 
}

void SpeakerEQ_Read_Previous_Block(SpeakerEQType* SpeakerEQ, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr;
  
  if(SpeakerEQ->MemoryConfig.CurrIndex > 0)
  {
    SpeakerEQ->MemoryConfig.CurrIndex--;
    SpeakerEQ->MemoryConfig.CycleCounter = SpeakerEQ->MemoryConfig.CurrIndex;
    SpeakerEQ->MemoryConfig.NextIndex = SpeakerEQ->MemoryConfig.CurrIndex +1;
  }
  else 
  {
    SpeakerEQ->MemoryConfig.CurrIndex = FlashBlockQty -1;
    SpeakerEQ->MemoryConfig.CycleCounter = SpeakerEQ->MemoryConfig.CurrIndex;
    SpeakerEQ->MemoryConfig.NextIndex = 0;
  }
  
  size = File_Header_Flash_Read(&SpeakerEQ->MemoryConfig, SIZE_32kB);                                              //Get file size
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.CurrIndex);
  
  if(size != 0) 
  {
    Flash_Read(addr, Buff, size);
  }
}

bool SpeakerEQ_Error_Handling(SpeakerEQType* SpeakerEQ, uint8_t* Buff)
{
  uint8_t error = false;
  uint32_t addr = 0;
  
  SpeakerEQ_Read_Previous_Block(SpeakerEQ, Buff);
  error = FileFormatReadSpeakerEQ(SpeakerEQ, Buff);
  
  if(error != false) 
  {
    addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.NextIndex);
    Flash_Erase_32kB(addr);
    Default_SpeakerEQ_Param(SpeakerEQ);
    SpeakerEQ_Flash_Write(SpeakerEQ, Buff);
    FileFormatReadSpeakerEQ(SpeakerEQ, Buff);
  }
  
  return error;
}

//SpeakerEQ Load Backup 
void SpeakerEQ_Flash_Read_Backup(SpeakerEQType* SpeakerEQ, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr = 0;
  bool error = false;

  Search_Valid_Data_Addr(&SpeakerEQ->MemoryConfig, SIZE_4kB);                              //Search addr valid data in flash 
  
  size = File_Header_Flash_Read(&SpeakerEQ->MemoryConfig, SIZE_32kB);                                       //Read header file from flash and get size of file  
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.CurrIndex);               //Define file addr in flash; 
  
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);
    error = FileFormatReadSpeakerEQ(SpeakerEQ, Buff);
    
    //If loaded data was written incorect
    if(error != false) error = SpeakerEQ_Error_Handling(SpeakerEQ, Buff);
  }
  else error = SpeakerEQ_Error_Handling(SpeakerEQ, Buff);
 
  //Clear next block if it need
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.NextIndex);
  Flash_Read(addr, Buff, SIZE_32kB);
  if(CRC_Calc_32kB_Free(FILE_BUFF)) Flash_Erase_32kB(addr);
}

//SpeakerEQ Flash Write
void SpeakerEQ_Flash_Write(SpeakerEQType* SpeakerEQ, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  
  SpeakerEQ->MemoryConfig.CycleCounter++;
  SpeakerEQ->MemoryConfig.CurrIndex = SpeakerEQ->MemoryConfig.CycleCounter % FlashBlockQty;
  
  if(SpeakerEQ->MemoryConfig.CurrIndex < FlashBlockQty -1) SpeakerEQ->MemoryConfig.NextIndex = SpeakerEQ->MemoryConfig.CurrIndex +1;
  else SpeakerEQ->MemoryConfig.NextIndex = 0;
  
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.CurrIndex);
  size = FileFormatWriteSpeakerEQ(SpeakerEQ, Buff); 

  Flash_Data_Store(addr, Buff, size);                                              //Store data to flash
  
  Eeprom_Param_Config(&SpeakerEQ->MemoryConfig, WRITE_EEPROM);
  
  //Erase Next Preset Block
  addr = SpeakerEQ->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerEQ->MemoryConfig.NextIndex);
  Flash_Erase_32kB(addr);                                                             
}


//Speaker Out-------------------------------------------------------------------
void Speaker_OUT_Flash_Read(SpeakerOutType* SpeakerOut, uint8_t* Buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  bool error = false;
  
  Eeprom_Param_Config(&SpeakerOut->MemoryConfig, READ_EEPROM);                                              //Get number of block in flash
  
  size = File_Header_Flash_Read(&SpeakerOut->MemoryConfig, SIZE_32kB);                                       //Read header file from flash and get size of file  
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.CurrIndex);               //Define file addr in flash; 
    
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);                                                                  //Read file
    error = FileFormatReadSpeakerOut(SpeakerOut, Buff);                                               //Deserialize file to data struct
    
    //If loaded data was written incorect
    if(error != false) error = Speaker_OUT_Error_Handling(SpeakerOut, Buff);
  }
  else error = Speaker_OUT_Error_Handling(SpeakerOut, Buff);

  //Clear next block if it need
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.NextIndex);
  Flash_Read(addr, Buff, SIZE_32kB);
  if(CRC_Calc_32kB_Free(Buff)) Flash_Erase_32kB(addr); 
}

void Speaker_OUT_Read_Previous_Block(SpeakerOutType* SpeakerOut, uint8_t* Buff)
{
  uint32_t size;
  uint32_t addr;
  
  if(SpeakerOut->MemoryConfig.CurrIndex > 0)
  {
    SpeakerOut->MemoryConfig.CurrIndex--;
    SpeakerOut->MemoryConfig.CycleCounter = SpeakerOut->MemoryConfig.CurrIndex;
    SpeakerOut->MemoryConfig.NextIndex = SpeakerOut->MemoryConfig.CurrIndex +1;
  }
  else 
  {
    SpeakerOut->MemoryConfig.CurrIndex = FlashBlockQty -1;
    SpeakerOut->MemoryConfig.CycleCounter = SpeakerOut->MemoryConfig.CurrIndex;
    SpeakerOut->MemoryConfig.NextIndex = 0;
  }
  
  size = File_Header_Flash_Read(&SpeakerOut->MemoryConfig, SIZE_32kB);                                              //Get file size
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.CurrIndex);
  
  if(size != 0) 
  {
    Flash_Read(addr, Buff, size);
  }
}

bool Speaker_OUT_Error_Handling(SpeakerOutType* SpeakerOut, uint8_t* Buff)
{
  uint8_t error = false;
  uint32_t addr = 0;
  
  Speaker_OUT_Read_Previous_Block(SpeakerOut, Buff);
  error = FileFormatReadSpeakerOut(SpeakerOut, Buff);
  
  if(error != false) 
  {
    addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.NextIndex);
    Flash_Erase_32kB(addr);
    Default_Speaker_Out_Param(SpeakerOut);
    Speaker_OUT_Flash_Write(SpeakerOut, Buff);
    FileFormatReadSpeakerOut(SpeakerOut, Buff);
  }
  
  return error;
}

void Speaker_OUT_Flash_Read_Backup(SpeakerOutType* SpeakerOut, uint8_t* Buff)
{
uint32_t size;
  uint32_t addr = 0;
  bool error = false;

  Search_Valid_Data_Addr(&SpeakerOut->MemoryConfig, SIZE_4kB);                              //Search addr valid data in flash 
  
  size = File_Header_Flash_Read(&SpeakerOut->MemoryConfig, SIZE_32kB);                                       //Read header file from flash and get size of file  
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.CurrIndex);               //Define file addr in flash; 
  
  if(size != 0)
  {  
    Flash_Read(addr, Buff, size);
    error = FileFormatReadSpeakerOut(SpeakerOut, Buff);
    
    //If loaded data was written incorect
    if(error != false) error = Speaker_OUT_Error_Handling(SpeakerOut, Buff);
  }
  else error = Speaker_OUT_Error_Handling(SpeakerOut, Buff);
 
  //Clear next block if it need
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.NextIndex);
  Flash_Read(addr, Buff, SIZE_32kB);
  if(CRC_Calc_32kB_Free(FILE_BUFF)) Flash_Erase_32kB(addr);
}

//Time -> 40ms
void Speaker_OUT_Flash_Write(SpeakerOutType* SpeakerOut, uint8_t* buff)
{
  uint32_t size = 0;
  uint32_t addr = 0;
  
  DWT->CYCCNT = 0;
  SpeakerOut->MemoryConfig.CycleCounter++;
  SpeakerOut->MemoryConfig.CurrIndex = SpeakerOut->MemoryConfig.CycleCounter % FlashBlockQty;
  
  if(SpeakerOut->MemoryConfig.CurrIndex < FlashBlockQty -1) SpeakerOut->MemoryConfig.NextIndex = SpeakerOut->MemoryConfig.CurrIndex +1;
  else SpeakerOut->MemoryConfig.NextIndex = 0;
  
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.CurrIndex);
  size = FileFormatWriteSpeakerOut(SpeakerOut, buff); 

  DWT->CYCCNT = 0;
  Flash_Data_Store(addr, buff, size);                                              //Store data to flash
  
  Eeprom_Param_Config(&SpeakerOut->MemoryConfig, WRITE_EEPROM);
  
  //Erase Next Preset Block
  addr = SpeakerOut->MemoryConfig.FlashAddress + (SIZE_32kB * SpeakerOut->MemoryConfig.NextIndex);
  Flash_Erase_32kB(addr);                                                             
}
//------------------------------------------------------------------------------

void Speaker_Flash_Write(SpeakerType* Speaker, uint8_t index)
{
  uint32_t addr = FLASH_BASE_ADDR_SPK_LIB + (index * FLASH_BASE_ADDR_SPK_LIB);
  uint32_t size = 0;
  //uint8_t buff[HEADER_SPK_LIB];
  
  Flash_Erase_64kB(addr);
  Flash_Erase_64kB(addr +SIZE_64kB);
  
  arrcopy(FILE_BUFF, Speaker->Config->ModelName, 12);
  FILE_BUFF[HEADER_SPK_LIB-1] = Speaker->Config->Ways; 
  
  size = FileFormatWriteSpeaker(Speaker, FILE_BUFF+HEADER_SPK_LIB) + HEADER_SPK_LIB;
  Flash_Data_Store(addr, FILE_BUFF, size);
  
  Flash_Read(addr+HEADER_SPK_LIB, FILE_BUFF, FileHeaderSize);
  size = FileFormatGetFileSize(FILE_BUFF);
  
  Flash_Read(addr, FILE_BUFF, size + HEADER_SPK_LIB);
  FileFormatReadSpeaker(Speaker, FILE_BUFF+HEADER_SPK_LIB);
  
  //Flash_Data_Store(addr, buff, HEADER_SPK_LIB);
}

void Speaker_Flash_Read(uint8_t index, uint8_t* FileBuff)
{  
  uint32_t addr = FLASH_BASE_ADDR_SPK_LIB + (index * FLASH_BASE_ADDR_SPK_LIB);
  uint32_t size = 0;
  
  Flash_Read(addr+HEADER_SPK_LIB, FileBuff, FileHeaderSize);
  size = FileFormatGetFileSize(FileBuff);
  
  Flash_Read(addr, FileBuff, size + HEADER_SPK_LIB);
}

//------------------------------------------------------------------------------

