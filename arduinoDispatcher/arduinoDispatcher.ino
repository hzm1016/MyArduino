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

#define DEBUG

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

//INIT
void setup()  
{
  int len;
  byte data;

  delay(200);

  pinMode(led, OUTPUT);     
  pinMode(RFIDRESET, OUTPUT);     

  Serial.begin(9600);
  digitalWrite(RFIDRESET, HIGH);
  delay(50);
  digitalWrite(RFIDRESET, LOW);
  delay(50);
  Serial.println("Start");
  
  // initialize the RFID UARTs (set the data rate for the ports)
  s1Buff.init();
  s2Buff.init();
  s3Buff.init();

}

void loop()
{
  int len;
  byte* data;

  // Serial.print('.');
  if (len = cmdPort.poll()) {
    data = cmdPort.data();
    cmdPort.showReply(len);
    switch(7 & (*data)) {
    case 0:
      s3Buff.writeCmd(1,  RFID_CMD_Set_Baud_Rate, &nonzero);
      break;
    case 1:
      s1Buff.writeCmd(0, RFID_CMD_Seek_for_Tag);
      break;
    case 2:
      s2Buff.writeCmd(1, RFID_CMD_Antenna_Power, &nonzero);
      break;
    case 3:
      s3Buff.writeCmd(1, RFID_CMD_Antenna_Power, &zero);
      break;
    case 4:
      s1Buff.writeCmd(1, RFID_CMD_Antenna_Power, &nonzero);
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
    s1Buff.showReply(len);
  }
  if (len = s2Buff.poll()) {
    s2Buff.showReply(len);
  }
  if (len = s3Buff.poll()) {
    s3Buff.showReply(len);
  }
}
