#include "Arduino.h"
#include "rfid.h"
#include "nonblockinput.h"

NonBlockInput::NonBlockInput(SerialPort* port, byte* buffer, int buflen) :
   m_index(0),
   m_buf(buffer),
   m_buflen(buflen),
   m_port(port)
{
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

void RfidInput::reset()
{
  m_count = 0;
  NonBlockInput::reset();
}
