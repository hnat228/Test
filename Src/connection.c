#include "connection.h"
#include "ParkNetTransferProtocol.h"
#include "usbd_customhid_core.h"

#include "usbd_usr.h"
#include "usbd_desc.h"

Connection UsbConnection = {ConnTypeUSB, true, 56};

void ConnectionUSB_Init()
{
  USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CUSTOMHID_cb, &USR_cb); 
}

void CheckUSBDataReceived()
{
  //Report_buf;
  uint8_t* buff = USBD_CUSTOM_HID_IsReceived();
  
  if(buff[0] == 0x01) 
  {
    TransferProtocolDeserialize_V3(&UsbConnection, &buff[1]);
  }
}