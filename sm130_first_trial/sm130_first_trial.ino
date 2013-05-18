/*
  RFID Eval 13.56MHz Shield example sketch with Value Block Read/Writev10
  
  works with 13.56MHz MiFare 1k tags
  
  Based on hardware v13:
  D7 -> RFID RX
  D8 -> RFID TX
  
  Note: RFID Reset attached to D4
*/
#include <SoftwareSerial.h>

const int FALSE = 0;
const int  TRUE = 1;

const int RFIDRESET = 4;

#if 0
const int RFID0_RX = 7;
const int RFID0_TX = 8;
SoftwareSerial rfid0(RFID0_RX, RFID0_TX);
typedef SoftwareSerial SerialPort;
#else
#define rfid0 Serial1
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

RfidResponse response(&rfid0);

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
//       count: # of bytes in data (may be 0)
//       cmd:   command
//       data:  bytes to send after command, if count > 0
void writeCmd(int count, byte cmd, const byte* data = 0)
{
  byte csum = count + 1 + cmd;
  int i;
  
  rfid0.write((uint8_t) RFID_CMD_Header0);
  rfid0.write((uint8_t) RFID_CMD_Header1);
  rfid0.write((uint8_t) count+1);
  rfid0.write((uint8_t) cmd);
  for (i=0; i<count; i++) {
    csum += data[i];
    rfid0.write((uint8_t) data[i]);
  }
  rfid0.write((uint8_t) csum);
}


//INIT
void setup()  
{
  int len;

  delay(200);

  pinMode(led, OUTPUT);     
  pinMode(RFIDRESET, OUTPUT);     

  Serial.begin(9600);
  
  // set the data rate for the NewSoftSerial port
  rfid0.begin(19200);

  digitalWrite(RFIDRESET, HIGH);
  delay(50);
  digitalWrite(RFIDRESET, LOW);
  delay(50);
  Serial.println("Start");
  writeCmd(0, RFID_CMD_Firmware);
  while (0 == (len = response.poll()));
  showReply(len, response.data());

  writeCmd(1, RFID_CMD_Antenna_Power, &nonzero);
  while (0 == (len = response.poll()));
  showReply(len, response.data());
  writeCmd(0, RFID_CMD_Seek_for_Tag);
  while (0 == (len = response.poll()));
  showReply(len, response.data());
}

void loop()
{
  int len;
  int i;

  if (len = cmdPort.poll()) {
    showReply(len, cmdPort.data());
    writeCmd(0, RFID_CMD_Seek_for_Tag);
  }
  if (len = response.poll()) {
    showReply(len, response.data());
  }
}
