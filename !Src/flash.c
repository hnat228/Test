
#include "flash.h"
#include "file_streams.h"
#include "array.h"

static uint8_t Flash_Raed_Status(uint8_t reg);

uint8_t data[2];

uint16_t SPI_Flash_Init()
{
  uint16_t MemoryType = 0;
  
  SPI2_Init();
  Flash_Read_ID(data);

  if((data[0] != WB_MAN_ID) & (data[1] != WB_DEV_ID)) return FLASH_ERROR;
  
  MemoryType = Flash_Read_Header();
  if(MemoryType != FLASH_OK) MemoryType = FLASH_NOT_INIT;
    
  return MemoryType;
}


uint8_t MCU_Flash_Read(uint32_t address) 
{
  return (*(__IO uint32_t*) address);
}


uint8_t Flash_Raed_Status(uint8_t reg)
{
  uint8_t status;
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(reg);
  status = SPI2_Transmit(0x22);
  CS_Disable(IC_Flash);
  
  return status;
}

void Flash_Write_Protect(uint8_t status)
{
  CS_Enable(IC_Flash);
  SPI2_Transmit(status);
  CS_Disable(IC_Flash);
}

void Flash_Write(uint32_t address, uint8_t* buff, uint16_t size)
{
  uint16_t i;

  while (Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  Flash_Write_Protect(cmdWREN);
  
  CS_Enable(IC_Flash);
  
  SPI2_Transmit(cmdWrite);
  SPI2_Transmit(((address & 0x00FF0000) >> 16));
  SPI2_Transmit(((address & 0x0000FF00) >> 8));
  SPI2_Transmit(address & 0x000000FF);
  
  for (i = 0; i < size; i++)
  {
    //Delay_ms(1);
    SPI2_Transmit(buff[i]);
    //Delay_ms(1);
  }
  
  CS_Disable(IC_Flash);
}

void Flash_Read(uint32_t address, uint8_t* buff, uint32_t size)
{
  uint32_t i;
  
  while (Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(cmdRead_LS);
  SPI2_Transmit(((address & 0x00FF0000) >> 16));
  SPI2_Transmit(((address & 0x0000FF00) >> 8));
  SPI2_Transmit(address & 0x000000FF);
  

  for (i = 0; i < size; i++)
  {
    *(buff + i) = SPI2_Transmit(0xA5);
  }

  CS_Disable(IC_Flash);
}

void Flash_Erase_4kB(uint32_t address)
{
  //Time erase 4kB -> 50ms
  while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  Flash_Write_Protect(cmdWREN);
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(cmdErase_4);
  SPI2_Transmit(((address & 0x00FF0000) >> 16));
  SPI2_Transmit(((address & 0x0000FF00) >> 8));
  SPI2_Transmit(address & 0x000000FF);
  CS_Disable(IC_Flash); 
  
  //
  //while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  //Delay_ms(65);
}

void Flash_Erase_32kB(uint32_t address)
{
  while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  Flash_Write_Protect(cmdWREN);
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(cmdErase_32);
  SPI2_Transmit(((address & 0x00FF0000) >> 16));
  SPI2_Transmit(((address & 0x0000FF00) >> 8));
  SPI2_Transmit(address & 0x000000FF);
  CS_Disable(IC_Flash); 
  
  //Delay_ms(65);
}


void Flash_Erase_64kB(uint32_t address)
{
  while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  Flash_Write_Protect(cmdWREN);
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(cmdErase_64);
  SPI2_Transmit(((address & 0x00FF0000) >> 16));
  SPI2_Transmit(((address & 0x0000FF00) >> 8));
  SPI2_Transmit(address & 0x000000FF);
  CS_Disable(IC_Flash); 
  
  //Delay_ms(1000);
}

void Flash_Full_Chip_Erase()
{
  while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  Flash_Write_Protect(cmdWREN);
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(cmdErase_Full);
  CS_Disable(IC_Flash);
  
  while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
}


void Flash_Read_ID(uint8_t* data)
{
  while(Flash_Raed_Status(cmdRDSR1) & (1 << BSY));
  
  CS_Enable(IC_Flash);
  SPI2_Transmit(cmdRDID);
  SPI2_Transmit(0x22);
  SPI2_Transmit(0x22);
  SPI2_Transmit(0x22);
  data[0] = SPI2_Transmit(0x22);      
  data[1] = SPI2_Transmit(0x22);
  CS_Disable(IC_Flash); 
}

uint8_t Flash_Test()
{
  //uint8_t data[2];
  uint8_t error = 0;
  //
  //Flash_Read_ID(data);
  //
  //if((data[0] == AT_MAN_ID) && (data[1] >= AT_DEV_ID)) error = 0;
  //else if((data[0] == GD_MAN_ID) && (data[1] >= GD_DEV_ID)) error = 0;
  //else error = FlashError;
  //
  return error;
}

//Read Memory type flag, and another info 
uint16_t Flash_Read_Header()
{
  uint16_t MemoryType = 0;
  uint8_t buff[FlashHeaderSize];
  
  Flash_Read(FLASH_HEADER_ADDR, buff, FlashHeaderSize);
  
  MemoryType = (uint16_t)buff[0] << 8;
  MemoryType |= buff[1]; 
  
  return MemoryType;
}

void Flash_Write_Header()
{
  uint8_t buff[FlashHeaderSize];
  
  buff[0] = (uint8_t)((FlashMemoryType & 0xFF00) >> 8);
  buff[1] = (uint8_t)(FlashMemoryType & 0x00FF);
    
  Flash_Write(FLASH_HEADER_ADDR, buff, FlashHeaderSize);
}

void Flash_Data_Store(uint32_t addr, uint8_t* buff, uint32_t size)
{
  //Time erase 4kB -> 50ms (full data was write)
  //Time Write 256B -> 390us
  //Time Write 4kB -> 6.25ms
  
  uint8_t page256b_point = 0;
  uint8_t page_buff[SIZE_256B];
  uint32_t flash_addr = addr;
  uint16_t i;
  
  arrinit(page_buff, SIZE_256B, 0xFF);

  for(i = 0; i < size; i++)
  {
    page_buff[page256b_point] = buff[i];                                        //Fill buff page 256b valide data 
    
    if(page256b_point < (SIZE_256B - 1)) page256b_point++;
    else
    {
      Flash_Write(flash_addr, page_buff, SIZE_256B);
      flash_addr += SIZE_256B;                                                  //Update addr for write datat to flas
      //arrinit(page_buff, SIZE_256B, 0xFF);                                    //Clear buff
      page256b_point = 0;
    }
  }
    
  if(page256b_point != 0) Flash_Write(flash_addr, page_buff, SIZE_256B);
}
  