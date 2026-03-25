
#include "ParkNetTransferProtocol.h"

#ifdef ParkDSPLiteBoard
extern USB_OTG_CORE_HANDLE USB_OTG_dev;
#else
#include "usbd_customhid_core.h"
extern USB_OTG_CORE_HANDLE USB_OTG_dev;
#endif

#define TransmitBuffSize 64
#define DataBuffSizeMin 256         
#define DataBuffSizeMax 70000

uint8_t ReceivedID = 0;

uint8_t DataBuff[DataBuffSizeMin + DataBuffSizeMax];
uint8_t TransmitBuff[TransmitBuffSize];
EntityHelperType ReceiveHelper = {DataBuff, 0, 0};
EntityHelperType TransmitHelper = {DataBuff, 0, 0};

//uint16_t TransferProtocolSerialize(Connection* connection, uint8_t* entity, uint16_t entitySize, uint16_t pktcounter, bool crc)
//{
//  uint16_t DataCounter = 0;
//  uint16_t HeaderSize = 0;
//  uint16_t crcData = 0;
//  uint16_t counter = 0;
//  uint16_t i = 0;
//  
//  DataCounter = connection->DataSizeMax * pktcounter;
//  HeaderSize = entitySize - DataCounter;
//
//  if(HeaderSize > connection->DataSizeMax) HeaderSize = connection->DataSizeMax;
//  else pktcounter = 0xFFFF;
//
//  EntityBuff[counter++] = 0x01;
//  EntityBuff[counter++] = 0x00;
//  EntityBuff[counter++] = (uint8_t)(pktcounter >> 8);
//  EntityBuff[counter++] = (uint8_t)(pktcounter);
//  EntityBuff[counter++] = (uint8_t)(HeaderSize >> 8);
//  EntityBuff[counter++] = (uint8_t)(HeaderSize);
//  
//  for(i = 0; i < HeaderSize; i++)
//  {
//    EntityBuff[counter++] = entity[DataCounter++];
//  }
//
//  if(crc) crcData = 0x00; // calculate crc
//  else crcData = 0xFFFF;
//    
//  EntityBuff[counter++] = (uint8_t)(crcData >> 8);
//  EntityBuff[counter++] = (uint8_t)(crcData);
//
//  return counter;
//}

//bool TransferProtocolDeserialize(Connection* connection, uint8_t* data)
//{
//  bool error = false;
//  uint16_t counter = 0;
//  uint8_t transactionID = 0;
//  uint16_t transactionCounter = 0;
//  uint16_t transactionSize = 0;
//  uint8_t transactionEntityIndex = 0;
//  uint16_t transactionCRC = 0;
//
//  transactionID = data[counter++];
//  transactionCounter = data[counter++] << 8;
//  transactionCounter |= data[counter++];
//  transactionSize = data[counter++] << 8;
//  transactionSize |= data[counter++];
//  transactionEntityIndex = counter;
//  
//  counter += transactionSize;
//  transactionCRC = data[counter++] << 8;
//  transactionCRC |= data[counter++];
//
//  // check CRC
//  
//  if(ReceivedID != transactionID)
//  {
//    ReceivedID = transactionID;
//    ReceivedLenght = 0;
//  }
//  
//  // deserialize entity
//  if(transactionCounter == 0xFFFF)
//  {
//    //DeserializeEntity(&data[transactionEntityIndex], DataBuffUSB);
//  }
//  else
//  {
//    for(counter = 0; counter < transactionSize; counter++)
//    {
//      EntityBuff[ReceivedLenght++] = data[transactionEntityIndex + counter++];
//    }
//  }
//  
//  uint8_t size = TransferProtocolSerialize(connection, EntityBuff, 10, 0, false);
//  if(size > 0) USBD_CUSTOM_HID_SendReport(&USB_OTG_dev, EntityBuff, 64);
//  
//  return error;
//}

// before 15.01.2026
//uint16_t TransferProtocolSerialize_V2(Connection* connection, uint8_t* buff, uint8_t id, EntityHelperType* transmitHelper, bool crc)
//{
//  uint16_t crcData = 0;
//  uint16_t counter = 0;
//  uint16_t dataSize = 0;
//  uint16_t i = 0;
//
//  buff[counter++] = 0x01;
//  buff[counter++] = id; 
//  buff[counter++] = (uint8_t)(transmitHelper->Size >> 24);
//  buff[counter++] = (uint8_t)(transmitHelper->Size >> 16);
//  buff[counter++] = (uint8_t)(transmitHelper->Size >> 8);
//  buff[counter++] = (uint8_t)(transmitHelper->Size);
//
//  if(transmitHelper->Size == 0xFFFFFFFF) return counter;
//  
//  dataSize = transmitHelper->Size;
//  if(dataSize > connection->DataSizeMax) dataSize = connection->DataSizeMax;
//  
//  for(i = 0; i < dataSize; i++)
//  {
//    buff[counter++] = transmitHelper->Entity[transmitHelper->Index++];
//    transmitHelper->Size--;
//  }
//
//  if(crc) crcData = 0x00; // calculate crc
//  else crcData = 0xFFFF;
//    
//  buff[counter++] = (uint8_t)(crcData >> 8);
//  buff[counter++] = (uint8_t)(crcData);
//
//  return counter;
//}
//bool TransferProtocolDeserialize_V2(Connection* connection, uint8_t* data)
//{
//  bool error = false;
//  uint16_t counter = 0;
//  uint8_t transactionID = 0;
//  uint8_t transactionEntityIndex = 0;
//  uint16_t transactionCRC = 0;
//  uint32_t transactionSize = 0;
//  
//  transactionID = data[counter++];
//  transactionSize = data[counter++] << 24;
//  transactionSize |= data[counter++] << 16;
//  transactionSize |= data[counter++] << 8;
//  transactionSize |= data[counter++];
//  transactionEntityIndex = counter;
//  
//  counter += transactionSize;
//  transactionCRC = data[counter++] << 8;
//  transactionCRC |= data[counter++];
//
//  // check CRC
//  
//  if(ReceivedID != transactionID) //check if new transaction start
//  {
//    ReceivedID = transactionID;
//    ReceiveHelper.Size = 0;
//  }
//
//  if(ReceiveHelper.Size == 0) ReceiveHelper.Size = transactionSize;
//  
//  ReceiveHelper.Index = ReceiveHelper.Size - transactionSize;
//  
//  if(transactionSize <= connection->DataSizeMax) // receive small or last packet
//  {
//    if(ReceiveHelper.Index == 0)
//    {
//      error = DeserializeEntity(&data[transactionEntityIndex], &TransmitHelper);
//    }
//    else
//    {
//      for(counter = 0; counter < transactionSize; counter++)
//      {
//        ReceiveHelper.Entity[ReceiveHelper.Index++] = data[transactionEntityIndex++];
//      }
//      error = DeserializeEntity(ReceiveHelper.Entity, &TransmitHelper);
//    }
//    
//    ReceiveHelper.Size = 0;
//  }
//  else if(transactionSize > EntityBuffSizeR) // receive too big packet
//  {
//    // send Nack - error
//    TransmitHelper.Index = 0;
//    TransmitHelper.Size = SerializeCommandEntity(TransmitHelper.Entity, CommandType_Nack, 0, 0, &TransmitHelper.Entity[EntityHeaderSize]);
//  }
//  else // receive addition packet
//  {
//    for(counter = 0; counter < connection->DataSizeMax; counter++)
//    {
//      ReceiveHelper.Entity[ReceiveHelper.Index++] = data[transactionEntityIndex++];
//    }
//    // send ack
//    TransmitHelper.Index = 0;
//    TransmitHelper.Size = SerializeCommandEntity(TransmitHelper.Entity, CommandType_Ack, 0, 0, &TransmitHelper.Entity[EntityHeaderSize]);
//  }
//
//  if(error)
//  {
//    // send Nack - error
//    TransmitHelper.Index = 0;
//    TransmitHelper.Size = SerializeCommandEntity(TransmitHelper.Entity, CommandType_Nack, 0, 0, &TransmitHelper.Entity[EntityHeaderSize]);
//  }
//  
//  BuffClear(TransmitBuff, sizeof(TransmitBuff));
//  
//  counter = TransferProtocolSerialize_V2(connection, TransmitBuff, ReceivedID, &TransmitHelper, false);
//  if(counter > 0) USBD_CUSTOM_HID_SendReport(&USB_OTG_dev, TransmitBuff, sizeof(TransmitBuff));
//  
//  return error;
//}

// new from 15.06.2026
uint16_t TransferProtocolSerialize_V3(Connection* connection, uint8_t* buff, uint8_t id, EntityHelperType* transmitHelper, TransferFlagType flags, bool crc)
{
  uint16_t crcData = 0;
  uint16_t counter = 0;
  uint16_t dataSize = 0;
  uint16_t i = 0;

  buff[counter++] = 0x01;
  buff[counter++] = (flags << 4) | (id & 0x0F); 
  buff[counter++] = (uint8_t)(transmitHelper->Size >> 24);
  buff[counter++] = (uint8_t)(transmitHelper->Size >> 16);
  buff[counter++] = (uint8_t)(transmitHelper->Size >> 8);
  buff[counter++] = (uint8_t)(transmitHelper->Size);

  if(transmitHelper->Size == 0xFFFFFFFF) return counter;
  
  dataSize = transmitHelper->Size;
  if(dataSize > connection->DataSizeMax) dataSize = connection->DataSizeMax;
  
  for(i = 0; i < dataSize; i++)
  {
    buff[counter++] = transmitHelper->Entity[transmitHelper->Index++];
    transmitHelper->Size--;
  }

  if(crc) crcData = 0x00; // calculate crc
  else crcData = 0xFFFF;
    
  buff[counter++] = (uint8_t)(crcData >> 8);
  buff[counter++] = (uint8_t)(crcData);

  return counter;
}

bool TransferProtocolDeserialize_V3(Connection* connection, uint8_t* data)
{
  bool error = false;
  uint16_t counter = 0;
  uint8_t transactionID = 0;
  TransferFlagType transactionFlag = TransferFlag_No;
  uint8_t transactionEntityIndex = 0;
  uint16_t transactionCRC = 0;
  uint32_t transactionSize = 0;
  
  transactionID = data[counter++];
  transactionFlag = (TransferFlagType)(data[counter] & 0xF0);
  transactionSize = (data[counter++] & 0x0F) << 24;
  transactionSize |= data[counter++] << 16;
  transactionSize |= data[counter++] << 8;
  transactionSize |= data[counter++];
  transactionEntityIndex = counter;
  
  counter += transactionSize;
  transactionCRC = data[counter++] << 8;
  transactionCRC |= data[counter++];

  // Todo: check CRC

  if(transactionSize > DataBuffSizeMax) // check if received too big packet
  {
    // prepare Oversize command
    TransmitHelper.Index = 0;
    TransmitHelper.Size = SerializeCommandEntity(TransmitHelper.Entity, CommandType_OversizedPacket, 0, 0, &TransmitHelper.Entity[EntityHeaderSize]);
    // prepare Oversize flag
    transactionFlag = TransferFlag_Oversize;
    error = true;
  }
  else
  {
    if(ReceivedID != transactionID) //check if new transaction start
    {
      ReceivedID = transactionID;
      ReceiveHelper.Size = transactionSize;
      
      // divide general buff for Rx buff and Tx buff
      if(transactionSize < DataBuffSizeMin) TransmitHelper.Entity = DataBuff + DataBuffSizeMin;
      else TransmitHelper.Entity = DataBuff + transactionSize;
    }
  
    // copy new data
    for(counter = 0; (counter < transactionSize && counter < connection->DataSizeMax); counter++)
    {
      ReceiveHelper.Entity[ReceiveHelper.Index++] = data[transactionEntityIndex++];
    }
    
    if(transactionSize > connection->DataSizeMax) // checking for additional packets
    {
      // prepare Ack command
      TransmitHelper.Index = 0;
      TransmitHelper.Size = SerializeCommandEntity(TransmitHelper.Entity, CommandType_Ack, 0, 0, &TransmitHelper.Entity[EntityHeaderSize]);
      // prepare Ack flag
      transactionFlag = TransferFlag_Ack;
    }
    else // received small or last packet
    {
      // prepare respond 
      error = DeserializeEntity(ReceiveHelper.Entity, &TransmitHelper);
      if(error)
      {
        // prepare Nack command
        TransmitHelper.Index = 0;
        TransmitHelper.Size = SerializeCommandEntity(TransmitHelper.Entity, CommandType_Nack, 0, 0, &TransmitHelper.Entity[EntityHeaderSize]);
        // prepare Nack flag
        transactionFlag = TransferFlag_Nack;
      }
      else
      {
        // prepare Ack flag
        transactionFlag = TransferFlag_Ack;
      }
      //clear receive buffer index
      ReceiveHelper.Index = 0;
    }
  }
  
  // clear buff
  for(counter = 0; counter < TransmitBuffSize; counter++)
  {
    TransmitBuff[counter] = 0x00;
  }
  
  counter = TransferProtocolSerialize_V3(connection, TransmitBuff, ReceivedID, &TransmitHelper, transactionFlag, false);
  if(counter > 0) USBD_CUSTOM_HID_SendReport(&USB_OTG_dev, TransmitBuff, sizeof(TransmitBuff));
  
  return error;
}




