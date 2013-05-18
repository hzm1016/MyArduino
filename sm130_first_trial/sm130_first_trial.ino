/*
  This is a test of my non-blocking Serial buffering scheme.

  It requires a Mega (or other board with 3 UARTs in addition to the USB port).
  The UARTs need to be jumpered together as follows:
    TX1->RX2  (pin 18 -> pin 17)
    TX2->RX3  (pin 16 -> pin 15)
    TX3->RX1  (pin 14 -> pin 19)

  The setup() function sends a few messages around this path to check it.

  The loop() function looks for a message on the USB console, and then uses the
  low bits of the first byte to switch between 4 different messages that it
  writes to one of the UARTs.  It also looks for complete replies (complete in
  the symantics of an RFID reader) to come back from each of the UARTs and
  reports them when it sees them.

  Note: RFID Reset attached to D4
*/
#include <SoftwareSerial.h>

const int FALSE = 0;
const int  TRUE = 1;

const int RFIDRESET = 4;

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

const int led = 13;

const byte zero = 0;
const byte nonzero = 1;

class UsbInput {
public:
  UsbInput(void);
  virtual ~UsbInput(void);

  virtual int poll(void);   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual byte* data(void); // Returns pointer to buffer
  virtual void reset(void); // Resets buffer state

private:
  // static const int USB_BUF_LEN = 128;
  static const int USB_BUF_LEN = 8;
  int m_index;
  byte m_buf[USB_BUF_LEN];
};

UsbInput::UsbInput()
{
  reset();
}

UsbInput::~UsbInput()
{
}

int UsbInput::poll()
{
  int ret = 0;
  int i;
  byte data;

  if (Serial.available()) {
    data = Serial.read();
    m_buf[m_index++] = data;
    if (USB_BUF_LEN == m_index) ret = -m_index;
    if (('\n' == data) || ('\r' == data)) {
      ret = m_index;
    }
    if (ret) {
      m_buf[m_index-1] = 0;
      reset();
    }
  }
  return ret;
}

byte* UsbInput::data()
{
  return m_buf;
}

void UsbInput::reset()
{
  m_index = 0;
}

UsbInput cmdPort;


class RfidResponse {
public:
  RfidResponse(SerialPort* port);
  virtual ~RfidResponse(void);

  virtual int poll(void);   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual byte* data(void); // Returns pointer to buffer
  virtual void reset(void); // Resets buffer state

private:
  static const int RFID_BUF_LEN = 32;
  SerialPort* m_port;
  int m_index;
  int m_count;
  byte m_buf[RFID_BUF_LEN];
};

RfidResponse::RfidResponse(SerialPort* port)
{
  m_port = port;
  reset();
}

RfidResponse::~RfidResponse()
{
}

int RfidResponse::poll()
{
  int ret = 0;
  int i;
  byte recSum = 0;
  byte data;

  if (m_port->available()) {
    data = m_port->read();
    if ((0 == m_index) && (RFID_CMD_Header0 != data)) ret = -1;
    if ((1 == m_index) && (RFID_CMD_Header1 != data)) ret = -2;
    if (2 == m_index) {
      if (data > (RFID_BUF_LEN-5)) ret = -3;
        else m_count = data;
    }
    if (ret < 0) {
      reset();
      return ret;
    }
    m_buf[m_index++] = data;
    if ((4+m_count) == m_index) {
      for (i=2; i<(m_count+3); i++) recSum+= m_buf[i];
      ret = 4 + m_count; // Assume checksum OK
      if (recSum != data) ret = -ret; // Bad checksum
      reset();
    }
  }
  return ret;
}

byte* RfidResponse::data()
{
  return m_buf;
}

void RfidResponse::reset()
{
  m_index = m_count = 0;
}

RfidResponse s1Buff(&rfid1);
RfidResponse s2Buff(&rfid2);
RfidResponse s3Buff(&rfid3);

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

void showReply(int port, int len, const byte* response)
{
  int i;

  if (0 == port) {
    Serial.print("CMD is ");
  } else {
    Serial.print("RR#");
    Serial.print(port);
    Serial.print(" replies with ");
  }
  if ((len > -10) && (len < 10)) Serial.print(' ');
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
//       rfid:  pointer to a serial port object
//       count: # of bytes in data (may be 0)
//       cmd:   command
//       data:  bytes to send after command, if count > 0
void writeCmd(SerialPort* rfid, int count, byte cmd, const byte* data = 0)
{
  byte csum = count + 1 + cmd;
  int i;
  
  rfid->write((uint8_t) RFID_CMD_Header0);
  rfid->write((uint8_t) RFID_CMD_Header1);
  rfid->write((uint8_t) count+1);
  rfid->write((uint8_t) cmd);
  for (i=0; i<count; i++) {
    csum += data[i];
    rfid->write((uint8_t) data[i]);
  }
  rfid->write((uint8_t) csum);
}

void roundRobin(int count, byte cmd, const byte* payload = 0)
{
  int len;
  byte* data = 0;

  writeCmd(&rfid1, count, cmd, payload);
  writeCmd(&rfid2, count, cmd+1, payload);
  writeCmd(&rfid3, count, cmd+2, payload);
  while (0 == (len = s1Buff.poll()));
  showReply(1, len, data = s1Buff.data());
  while (0 == (len = s2Buff.poll()));
  showReply(2, len, s2Buff.data());
  while (0 == (len = s3Buff.poll()));
  showReply(3, len, s3Buff.data());
}


//INIT
void setup()  
{
  int len;
  byte data;

  delay(200);

  pinMode(led, OUTPUT);     
  pinMode(RFIDRESET, OUTPUT);     

  Serial.begin(9600);
  
  // set the data rate for the NewSoftSerial port
  rfid1.begin(19200);
  rfid2.begin(19200);
  rfid3.begin(19200);

  digitalWrite(RFIDRESET, HIGH);
  delay(50);
  digitalWrite(RFIDRESET, LOW);
  delay(50);
  Serial.println("Start");
  roundRobin(0, RFID_CMD_Firmware);
  roundRobin(1, RFID_CMD_Antenna_Power, &nonzero);
  roundRobin(0, RFID_CMD_Seek_for_Tag);
}

void loop()
{
  int len;
  byte* data;

  if (len = cmdPort.poll()) {
    showReply(0, len, data = cmdPort.data());
    switch(3 & (*data)) {
    case 0:
      writeCmd(&rfid3, 1,  RFID_CMD_Set_Baud_Rate, &nonzero);
      break;
    case 1:
      writeCmd(&rfid1, 0, RFID_CMD_Seek_for_Tag);
      break;
    case 2:
      writeCmd(&rfid2, 1, RFID_CMD_Antenna_Power, &nonzero);
      break;
    case 3:
      writeCmd(&rfid3, 1, RFID_CMD_Antenna_Power, &zero);
      break;
    }
  }
  if (len = s1Buff.poll()) {
    showReply(1, len, data = s1Buff.data());
    // writeCmd(&rfid3, len-3, data[3], data+4);
  }
  if (len = s2Buff.poll()) {
    showReply(2, len, s2Buff.data());
  }
  if (len = s3Buff.poll()) {
    // writeCmd(&rfid2, len-3, data[3], data+4);
    showReply(3, len, s3Buff.data());
  }
}
