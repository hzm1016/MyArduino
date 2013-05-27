#ifndef NONBLOCKINPUT_H
#define NONBLOCKINPUT_H


class NonBlockInput
{
public:
  NonBlockInput(SerialPort* port, byte* buffer, int buflen, int dev = -1, bool sendHex = 0);
  virtual ~NonBlockInput();

  virtual int poll(void) = 0;   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual byte* data(void); // Returns pointer to buffer
  virtual void reset(void); // Resets buffer state
  virtual bool available(void);
  virtual int read(void);
  SerialPort* getPort(void);
  int getPortNum(void);
  void showReply(int len);
  int encode(const char* dev, int sub, const byte* data, int len, char* dest);
  int doDecode(char * src, char* dev, int* sub, byte* dest);
  int decode(char * src, char* dev, int* sub, byte* dest);

protected:

// private:
  int m_index;
  int m_buflen;
  byte* m_buf;

private:
  SerialPort* m_port;
  int m_portNum;
  static int sm_portcount;
  bool m_sendHex;
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
  RfidInput(SerialPort* port, int dev = -1);
  virtual ~RfidInput(void);

  virtual int poll(void);   // Returns !=0 when complete response ready.  <0 => error, >0 is length
  virtual void reset(void); // Resets buffer state
  virtual void writeCmd(int count, byte cmd, const byte* data = 0);
  virtual void init(void);

private:
  int m_count;
  static const int RFID_BUF_LEN = 32;
  static const int RFID_BAUD_RATE = 19200;
  byte m_buffer[RFID_BUF_LEN];
};

#endif // NONBLOCKINPUT_H
