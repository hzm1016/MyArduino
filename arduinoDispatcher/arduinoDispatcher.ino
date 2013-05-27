/*
 *  The loop() function polls each of the serial UARTs (#0 is the USB uplink,
 *  #1-3 are connected to the RFID readers) looking for a complete message.
 *
 *  The USB uplink is the command/response port on the VMC (Janus 400ap).
 *  The VMC sends commands to be dispatched to the various I/O devices, and
 *  this Arduino will also relay responses (currently, from the RFID readers)
 *  back up to the VMC over the USB port.
 *
 *  The command messages are in a fairly tight format, and contain a device type,
 *  a device selector, a length, data bytes, and a checksum.
 *  The device type is two ASCII characters followed by a space.
 *    The device selector is a single hex digit (0 if only a single such device).
 *    The length is an ASCII number, either (a) a positive decimal count of the
 *      number of ASCII characters in the data, or (b) a negative decimal number
 *      of hex encoded bytes that follow, used for binary data.  The data (ASCII
 *      or hex) will be followed by a space.
 *    The checksum is a pair of hex digits that when added to the sum of the
 *      message text (rendered bytes) makes the sum equal to 0.
 *
 *  For example:
 *   1.  ASCII data ("Hello, World!"):
 *       XX 0 13 Hello, World! <cs>
 *   2.  Binary data:
 *       RF 0 -5 FF 00 01 81 82 <cs>
 *
 *  Each line is terminated by a newline.  [Perhaps at some point multiple
 *  command messages may be separated by semicolons within a line...]
 *
 *  The dispatcher will poll the various I/O devices each time loop() is called.
 *  The input objects are designed so that they are non-blocking and will only
 *  read a single character each loop; this is done in the poll() method.
 *  Poll() returns 0 if it is still accumulating data, and non-zero when a line
 *  of input is complete.
 *
 *  When the loop() function gets a complete response from one if the I/O
 *  devices (e.g. a RFID reader), that message will be packaged up into a
 *  message much like the command strings: device type, device number, length,
 *  data, and checksum.  In this case, the data will be the entire message
 *  read from the I/O device (and, in the case of the RFID readers, at least,
 *  will be in hex because those messages contain arbitrary binary data).  Each
 *  device object must supply a virtual poll() function that collects replies
 *  (If the device never sends back data the poll() function should simply return
 *  0, or maybe we will just know better than to call it.).
 *
 *  Note: RFID Reset attached to D4
 */

#define DEBUG

const int FALSE = 0;
const int  TRUE = 1;

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "rfid.h"
#include "NonBlockInput.h"

const int led = 13;
int ledState = 0;

int ledGet() {return ledState;}

int ledSet(int val)
{
  if (val) {
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    ledState = 1;
  } else {
    digitalWrite(led, LOW);   // turn the LED offn (LOW is the voltage level)
    ledState = 0;
  }
  return ledState;
}

const byte zero = 0;
const byte nonzero = 1;


UsbInput cmdPort(&Serial);

RfidInput* snBufs[3];
RfidInput* s1Buff = NULL;
RfidInput* s2Buff = NULL;
RfidInput* s3Buff = NULL;

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
  s1Buff = new RfidInput(&rfid1, 1);
  s2Buff = new RfidInput(&rfid2, 2);
  // s3Buff = new RfidInput(&rfid3, 3);
  snBufs[0] = s1Buff;
  snBufs[1] = s2Buff;
  snBufs[2] = s3Buff;
  if (s1Buff) s1Buff->init();
  if (s2Buff) s2Buff->init();
  if (s3Buff) s3Buff->init();

}

char buf[128];

// Test code...
int dispatch(char* cmd)
{
  char dev[3];
  int sub;
  int len;

  len = cmdPort.decode(cmd, dev, &sub, (byte*) buf);
  if (len < 0) {
    Serial.print("decode returned ");
    Serial.println(len);
  } else
  switch (*dev) {
  case 'l':
  case 'L':
    switch (*buf) {
    case 'o':
      ledSet(0);
      break;
    case 'O':
      ledSet(1);
      break;
    case 't':
    case 'T':
      ledSet(ledGet()?0:1);
      break;
    }
    break;
  case 'r':
  case 'R':
    if (snBufs[sub]) {
      snBufs[sub]->writeCmd(buf[0]-1, buf[1], (byte*) (buf+2));
    }
    break;
  }
  return len;
}

int doPoll(class NonBlockInput* sp)
{
  int len;
  if (sp) {
    if (len = sp->poll()) {
      sp->showReply(len);
      sp->encode("RF", sp->getPortNum(), sp->data(), len, buf);
      Serial.println(buf);
    }
  }
}

void loop()
{
  int len;
  byte* data;

  // Serial.print('.');
  if (len = cmdPort.poll()) {
    data = cmdPort.data();
    cmdPort.showReply(len);
    cmdPort.encode("CM", 0, cmdPort.data(), len, buf);
    Serial.println(buf);
    dispatch((char*)data);
  }
  doPoll(s1Buff);
  doPoll(s2Buff);
  doPoll(s3Buff);
}
