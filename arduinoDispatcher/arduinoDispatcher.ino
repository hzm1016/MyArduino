/*
  The loop() function polls each of the serial UARTs (#0 is the USB uplink,
  #1-3 are connected to the RFID readers) looking for a complete message.

  The USB uplink is the command/response port on the VMC (Janus 400ap).
  The VMC sends commands to be dispatched to the various I/O devices, and
  this Arduino will also relay responses (currently, from the RFID readers)
  back up to the VMC over the USB port.

  The command messages are in a fairly tight format, and contain a device type,
  a device selector, a length, data bytes, and a checksum.
  The device type is two ASCII characters followed by a space.
    The device selector is a single hex digit (0 if only a single such device).
    The length is an ASCII number, either (a) a positive decimal count of the
      number of ASCII characters in the data, or (b) a negative decimal number
      of hex encoded bytes that follow, used for binary data.  The data (ASCII
      or hex) will be followed by a space.
    The checksum is a pair of hex digits that when added to the sum of the
      message text (rendered bytes) makes the sum equal to 0.

  For example:
   1.  ASCII data ("Hello, World!"):
       XX 0 -13 Hello, World! <cs>
   2.  Binary data:
       RF 0 5 FF 00 01 81 82 <cs>

  Each line is terminated by a newline.  [Perhaps at some point multiple
  command messages may be separated by semicolons within a line...]

  The dispatcher will poll the various I/O devices each time loop() is called.
  The input objects are designed so that they are non-blocking and will only
  read a single character each loop; this is done in the poll() method.
  Poll() returns 0 if it is still accumulating data, and non-zero when a line
  of input is complete.

  When the loop() function gets a non-zero response from one if the I/O
  devices (e.g. a RFID reader), that message will be packaged up into a
  message much like the command strings: device type, device number, length,
  data, and checksum.  In this case, the data will be the entire message
  read from the I/O device (and, in the case of the RFID readers, at least,
  will be in hex because those messages contain arbitrary binary data).


looks for a message on the USB console, and then uses the
  low bits of the first byte to switch between 4 different messages that it
  writes to one of the UARTs.  It also looks for complete replies (complete in
  the symantics of an RFID reader) to come back from each of the UARTs and
  reports them when it sees them.

  Note: RFID Reset attached to D4
*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "rfid.h"

const int FALSE = 0;
const int  TRUE = 1;

const int led = 13;

const byte zero = 0;
const byte nonzero = 1;

#include "NonBlockInput.h"

UsbInput cmdPort(&Serial);

RfidInput s1Buff(&rfid1);
RfidInput s2Buff(&rfid2);
RfidInput s3Buff(&rfid3);

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
    switch(7 & (*data)) {
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
    case 4:
      writeCmd(&rfid1, 1, RFID_CMD_Antenna_Power, &nonzero);
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
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
