#ifndef RFID_H
#define RFID_H
/*
*/

const int RFIDRESET = 4;

#define RFID_CMD_Header0             ((byte)0xff)
#define RFID_CMD_Header1             ((byte)0x00)
#define RFID_CMD_Reset               ((byte)0x80)
#define RFID_CMD_Firmware            ((byte)0x81)
#define RFID_CMD_Seek_for_Tag        ((byte)0x82)
#define RFID_CMD_Select_Tag          ((byte)0x83)
#define RFID_CMD_Authenticate        ((byte)0x85)
#define RFID_CMD_Read_Block          ((byte)0x86)
#define RFID_CMD_Read_Value          ((byte)0x87)
#define RFID_CMD_Write_Block         ((byte)0x89)
#define RFID_CMD_Write_Value         ((byte)0x8A)
#define RFID_CMD_Write_4_Byte_Block  ((byte)0x8B)
#define RFID_CMD_Write_Key           ((byte)0x8C)
#define RFID_CMD_Increment           ((byte)0x8D)
#define RFID_CMD_Decrement           ((byte)0x8E)
#define RFID_CMD_Antenna_Power       ((byte)0x90)
#define RFID_CMD_Read_port           ((byte)0x91)
#define RFID_CMD_Write_Port          ((byte)0x92)
#define RFID_CMD_Halt                ((byte)0x93)
#define RFID_CMD_Set_Baud_Rate       ((byte)0x94)
#define RFID_CMD_Sleep               ((byte)0x96)
#endif // RFID_H

#if 0
const int RFID0_RX = 7;
const int RFID0_TX = 8;
SoftwareSerial rfid1(RFID0_RX, RFID0_TX);
typedef SoftwareSerial SerialPort;
#else
#define rfid1 Serial1
#define rfid2 Serial2
#define rfid3 Serial3
typedef HardwareSerial SerialPort;
#endif

