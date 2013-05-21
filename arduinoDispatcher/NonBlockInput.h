#ifndef NONBLOCKINPUT_H
#define NONBLOCKINPUT_H

typedef unsigned char byte;

class NonBlockInput
{
public:
  NonBlockInput(SerialPort* port);
  virtual ~NonBlockInput();

  virtual int poll(void) = 0;   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual byte* data(void); // Returns pointer to buffer
  virtual void reset(void); // Resets buffer state
  // virtual int recvData(byte input);  // Adds input to buffer

protected:
  static const int BUF_LEN = 128;
  SerialPort* m_port;

// private:
  int m_index;
  byte m_buf[BUF_LEN];
};


class UsbInput : public NonBlockInput
{
public:
  UsbInput(SerialPort* port);
  virtual ~UsbInput(void);

  virtual int poll(void);   // Returns !=0 when complete response ready.  <0 => error, >0 is length
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
};

#endif // NONBLOCKINPUT_H
