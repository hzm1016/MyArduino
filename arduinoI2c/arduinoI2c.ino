/*
 * An attempt to use I2C to communicate, but it seems that it is ncessary to
 *  reload the "firmware" in the SM130 module to use I2C instead of RS-232.
 *
 * Which means this has not been tested...
 */

#include <Wire.h>

const int led = 13;
const int RFIDRESET = 4;

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
  Serial.println("Reset 0->1->0");

  Wire.begin();  // Send a RESET command to I2C addr 0x42
  Wire.beginTransmission(0x42);
  Wire.write(0xff);
  Wire.write(0x00);
  Wire.write(0x01);
  Wire.write(0x80);
  Wire.write(0x81);
  Wire.endTransmission();
  Serial.println("Wire.transmit");

  Wire.requestFrom(0x42, 8, 1); // Request an 8 byte reply
}

void loop()
{
  static int x = -1;
  int i;
  int n;
  byte c;

  n = Wire.available();
  if (n != x) {
    Serial.print("New count: ");
    Serial.println(n);
    x = n;
  }
  if (8 == x) {
    Serial.print("Data: ");
    for (i=0; i<8; i++) {
      c = Wire.read();
      Serial.print(c, HEX);
      Serial.print(" ");
    }
    Serial.println("");
    x = 0;
  }
}
