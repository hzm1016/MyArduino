/*
  RFID Eval 13.56MHz Shield example sketch with Value Block Read/Writev10
  
  works with 13.56MHz MiFare 1k tags
  
  Based on hardware v13:
  D7 -> RFID RX
  D8 -> RFID TX
  
  Note: RFID Reset attached to D4
*/
#include <SoftwareSerial.h>

SoftwareSerial rfid(7, 8);

typedef unsigned char byte;
#define CMD_Header0             ((byte)0xff)
#define CMD_Header1             ((byte)0x00)
#define CMD_Reset               ((byte)0x80)
#define CMD_Firmware            ((byte)0x81)
#define CMD_Seek_for_Tag        ((byte)0x82)
#define CMD_Select_Tag          ((byte)0x83)
#define CMD_Authenticate        ((byte)0x85)
#define CMD_Read_Block          ((byte)0x86)
#define CMD_Read_Value          ((byte)0x87)
#define CMD_Write_Block         ((byte)0x89)
#define CMD_Write_Value         ((byte)0x8A)
#define CMD_Write_4_Byte_Block  ((byte)0x8B)
#define CMD_Write_Key           ((byte)0x8C)
#define CMD_Increment           ((byte)0x8D)
#define CMD_Decrement           ((byte)0x8E)
#define CMD_Antenna_Power       ((byte)0x90)
#define CMD_Read_port           ((byte)0x91)
#define CMD_Write_Port          ((byte)0x92)
#define CMD_Halt                ((byte)0x93)
#define CMD_Set_Baud_Rate       ((byte)0x94)
#define CMD_Sleep               ((byte)0x96)

const int FALSE = 0;
const int  TRUE = 1;

const int led = 13;
const int rfidReset = 4;

const byte zero = 0;
const byte nonzero = 1;

/* SM130 UART frames:
  Writing:
  1 byte - header - always 0xff
  1 byte - reserved - always 0x00
  1 byte - length - of command and data bytes
  1 byte - command
  N bytes - data
  1 byte - checksum - sum of everything but the header
  
  Reading:
  1 byte - header - always 0xff
  1 byte - reserved - always 0x00
  1 byte - length - of command and data bytes
  1 byte - command that SM130 is responding to
  N bytes - data - results, or error code
  1 byte - checksum - sum of everything but the header
*/

void showReply(int len, const byte* response)
{
  int i;

  Serial.print("Cmd response is ");
  if (len < 10) Serial.print(' ');
  Serial.print(len);
  Serial.print(" bytes: ");
  if (len < 0) len = -len;
  for (i=0; i<len; i++) {
    if ((len-1) == i) Serial.print(" * "); // Checksum
      else Serial.print(' ');
    if (16 > response[i]) Serial.print(0, HEX);
    Serial.print(response[i], HEX);
  }
  Serial.println();
}

//  writeCmd -- write a command, including the header prefix and the checksum
//       count: # of bytes in data (may be 0)
//       cmd:   command
//       data:  bytes to send after command, if count > 0
void writeCmd(int count, byte cmd, const byte* data = 0)
{
  byte csum = count + 1 + cmd;
  int i;
  
  rfid.write((uint8_t) CMD_Header0);
  rfid.write((uint8_t) CMD_Header1);
  rfid.write((uint8_t) count+1);
  rfid.write((uint8_t) cmd);
  for (i=0; i<count; i++) {
    csum += data[i];
    rfid.write((uint8_t) data[i]);
  }
  rfid.write((uint8_t) csum);
}

void blinky(int mask)
{
  static int count = 0;
  count++;
  digitalWrite(led, ((count & mask)?LOW:HIGH));
}

int readReply(byte* buf)
{
  int count;
  int i;
  int len;
  byte input;
  byte inSum;
  byte recSum = 0;
  
  for (i=0; i<32; i++) buf[i] = 0x42;
  
  while (!rfid.available()) blinky(7<<12);
  buf[0] = input = rfid.read();
  if (CMD_Header0 != input) return -1;
  while (!rfid.available()) blinky(3<<13);
  buf[1] = input = rfid.read();
  if (CMD_Header1 != input) return -2;
  while (!rfid.available()); blinky(1<<14);
  buf[2] = count = rfid.read();
  if (count > 20) return -3;
  while (!rfid.available()) blinky(7<<14);
  buf[3] = rfid.read(); // CMD
  for (i=1; i<count; i++) {
    while (!rfid.available()) blinky(3<<14);
    buf[i+3] = rfid.read(); // Data
  }
  while (!rfid.available()) blinky(1<<14);
  buf[3+count] = inSum = rfid.read(); // Checksum

  for (i=2; i<(count+3); i++) recSum+= buf[i];
  len = 4+count;
  if (recSum != inSum) len = -len;
  return len;
}


//INIT
void setup()  
{
  byte response[32];
  int len;

  delay(200);

  pinMode(led, OUTPUT);     
  pinMode(rfidReset, OUTPUT);     

  Serial.begin(9600);
  
  // set the data rate for the NewSoftSerial port
  rfid.begin(19200);

  digitalWrite(rfidReset, HIGH);
  delay(50);
  digitalWrite(rfidReset, LOW);
  delay(50);
  Serial.println("Start");
  writeCmd(0, CMD_Firmware);
  len = readReply(response);
  showReply(len, response);

  writeCmd(1, CMD_Antenna_Power, &nonzero);
  len = readReply(response);
  showReply(len, response);
  writeCmd(0, CMD_Seek_for_Tag);
  len = readReply(response);
  showReply(len, response);
}

void loop()
{
  int len;
  int i;
  byte response[32];

  writeCmd(0, CMD_Seek_for_Tag);
  len = readReply(response);
  showReply(len, response);
  len = readReply(response);
  showReply(len, response);
}
