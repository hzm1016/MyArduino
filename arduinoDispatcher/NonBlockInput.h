#ifndef NONBLOCKINPUT_H
#define NONBLOCKINPUT_H

class NonBlockInput
{
public:
  NonBlockInput(SerialPort* port, byte* buffer, int buflen);
  virtual ~NonBlockInput();

  virtual int poll(void) = 0;   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual byte* data(void); // Returns pointer to buffer
  virtual void reset(void); // Resets buffer state
  virtual bool available(void);
  virtual int read(void);
  SerialPort* getPort(void);
  // virtual int recvData(byte input);  // Adds input to buffer

protected:

// private:
  int m_index;
  int m_buflen;
  byte* m_buf;

private:
  SerialPort* m_port;
};


class UsbInput : public NonBlockInput
{
public:
  UsbInput(SerialPort* port);
  virtual ~UsbInput(void);

  virtual int poll(void);   // Returns !=0 when complete response ready.  <0 => error, >0 is length

private:
  static const int USB_BUF_LEN = 128;
  byte m_buffer[USB_BUF_LEN];
};


class RfidInput : public NonBlockInput
{
public:
  RfidInput(SerialPort* port);
  virtual ~RfidInput(void);

  virtual int poll(void);   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual void reset(void); // Resets buffer state

private:
  int m_count;
  static const int RFID_BUF_LEN = 32;
  byte m_buffer[RFID_BUF_LEN];
};

#endif // NONBLOCKINPUT_H
