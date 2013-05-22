#include "Arduino.h"
#include "rfid.h"
#include "nonblockinput.h"

#define DEBUG

int NonBlockInput::sm_portcount = 0;

NonBlockInput::NonBlockInput(SerialPort* port, byte* buffer, int buflen) :
   m_index(0),
   m_buf(buffer),
   m_buflen(buflen),
   m_port(port)
{
  m_portNum = sm_portcount++;
  reset();
}

NonBlockInput::~NonBlockInput()
{
}

byte* NonBlockInput::data()
{
  return m_buf;
}

void NonBlockInput::reset()
{
  m_index = 0;
}

bool NonBlockInput::available()
{
  return m_port->available();
}

int NonBlockInput::read()
{
  return m_port->read();
}

SerialPort* NonBlockInput::getPort()
{
  return m_port;
}

int NonBlockInput::getPortNum()
{
  return m_portNum;
}


void NonBlockInput::showReply(int len)
{
#ifdef DEBUG /* [ */
  int i;

  if (0 == m_portNum) {
    Serial.print("CMD is ");
  } else {
    Serial.print("RR#");
    Serial.print(m_portNum);
    Serial.print(" replies with ");
  }
  if ((len > -10) && (len < 10)) Serial.print(' ');
  Serial.print(len);
  Serial.print(" bytes: ");
  if (len < 0) len = -len;
  for (i=0; i<len; i++) {
    if ((len-1) == i) Serial.print(" * "); // Checksum
      else Serial.print(' ');
    if (16 > m_buf[i]) Serial.print(0, HEX);
    Serial.print(m_buf[i], HEX);
  }
  Serial.println();
#endif DEBUG /* ] */
}


UsbInput::UsbInput(SerialPort* port) :
   NonBlockInput(port, m_buffer, USB_BUF_LEN)
{
}

UsbInput::~UsbInput()
{
}

int UsbInput::poll()
{
  int ret = 0;
  int i;
  byte data;

  if (available()) {
    data = read();
    m_buf[m_index++] = data;
    if (USB_BUF_LEN == m_index) ret = -m_index;
    if (('\n' == data) || ('\r' == data) || ('\000' == data)) {
      ret = m_index;
    }
    if (ret) {
      m_buf[m_index-1] = 0;
      reset();
    }
  }
  return ret;
}


RfidInput::RfidInput(SerialPort* port) :
   NonBlockInput(port, m_buffer, RFID_BUF_LEN),
   m_count(0)
{
  reset();
}

RfidInput::~RfidInput()
{
}

// Custom poll() for SM130 RFID readers
int RfidInput::poll()
{
  int ret = 0;
  int i;
  byte recSum = 0;
  byte data;

  if (available()) {
    data = read();
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

void RfidInput::reset(void)
{
  m_count = 0;
  NonBlockInput::reset();
}

void RfidInput::init(void)
{
  static const byte nonzero = 1;
  getPort()->begin(RFID_BAUD_RATE);
  delay(20);
  while (-1 != read()); // flush out the receive FIFO
// The following are silly placeholders for initialization commands:
  writeCmd(0, RFID_CMD_Firmware, 0);
  writeCmd(1, RFID_CMD_Antenna_Power, &nonzero);
  writeCmd(0, RFID_CMD_Seek_for_Tag, 0);
}

//  writeCmd -- write a command, including the header prefix and the checksum
//       count: # of bytes in data (may be 0) -- note: this will be incremented
//                to make the count field of the RFID command.
//       cmd:   command
//       data:  bytes to send after command, if count > 0
void RfidInput::writeCmd(int count, byte cmd, const byte* data)
{
  SerialPort* rfid = getPort();
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

